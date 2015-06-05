/*
 * Driver for monitoring MPC860 CPM workload.
 * Copyright (C) 2003
 * DENX Software Engineering, Wolfgang Denk, wd@denx.de
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA 02111-1307 USA
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <asm/8xx_immap.h>
#include <asm/mpc8xx.h>
#include <asm/commproc.h>


#define	DRIVER_VERSION		"$Revision: 1.0 $"

#if 0
#define DPRINTK(args...) printk(##args)
#else
#define DPRINTK(args...)
#endif

#define PROC_NAME		"cpm_load"

	/* The RISC controller supports up to 16 timers.
	 */
#define RISC_TIMER_MAX		16

	/* (From MPC862D Errata document)
	 * If the RCCR is written as a byte or halfword
	 * (to addresses IMMR+0x9C4, IMMR+0x9C5), then RCCR[ERAM4K]
	 * (located at address IMMR+0x9C7) will be cleared.
	 *
	 * Workaround:
	 * When one wants to change the RCCR bytes at address
	 * IMMR+(0x9C4, 0x9C5), then one should write the whole
	 * word (addresses IMMR+0x9C4 to IMMR+0x9C7).
	 */
#define RCCR_BUG

	/* Offset of the RISC timer parameter RAM.
	 */
#define PROFF_RTIMER		((uint)0x01B0)

	/* General purpose timer constants.
	 */
#define CPMTIMER_TGCR_GM1	0x0008	/* gate mode */
#define CPMTIMER_TGCR_FRZ1	0x0004	/* freeze timer */
#define CPMTIMER_TGCR_STP1	0x0002	/* stop timer */
#define CPMTIMER_TGCR_RST1	0x0001	/* restart timer */

#define CPMTIMER_TGCR_CAS2	0x0080	/* cascade timer */
#define CPMTIMER_TGCR_FRZ2	0x0040	/* freeze timer */
#define CPMTIMER_TGCR_STP2	0x0020	/* stop timer */
#define CPMTIMER_TGCR_RST2	0x0010	/* restart timer */


typedef struct risc_timer_s {
	ushort tm_base;			/* Pointer to DPRAM timers     */
	ushort tm_ptr;			/* RISC timer table pointer    */
	ushort r_tmr;			/* RISC timer mode register    */
	ushort r_tmv;			/* RISC timer valid register   */
	uint tm_cmd;			/* RISC timer command register */
	uint tm_cnt;			/* RISC timer internal count   */

} risc_timer_t;


	/* Pointer to the RISC timer table.
	 */
static uint risc_dpram_timers;


static int cpm_load_proc_read ( char *page, char **start,
				off_t off, int count, int *eof, void *data)
{
	char *out = page;
	int len;
	char *fmt = "RISC timer 15 value         = %u\n"
		    "General purpose timer value = %u (%u)\n"
		    "Difference is %d ticks\n";
	int val_gp;
	int val_risc;
	volatile immap_t *immap = (immap_t *) IMAP_ADDR;
	volatile cpm8xx_t *cp = &immap->im_cpm;
	uint *timer15_addr;
	ulong flags;

	timer15_addr = (uint *) (&cp->cp_dpmem[risc_dpram_timers +
						(sizeof (long)) * 15]);

	save_flags (flags);
	cli ();
	val_gp = immap->im_cpmtimer.cpmt_tcn1;
	val_risc = (unsigned short) *timer15_addr;
	restore_flags (flags);

	out += sprintf (out, fmt,
			val_risc,
			val_gp,
			0xffff - val_gp,
			val_risc - 0xffff + val_gp);

	len = out - page;
	len -= off;

	if (len < count) {
		*eof = 1;
		if (len < 0) {
			len = 0;
		}
	} else {
		len = count;
	}

	if (len) {
		*start = page + off;
	}

	return len;
}

static int cpm_load_proc_write (struct file *file, const char *buffer,
				unsigned long count, void *data)
{
	return count;
}

static int __init cpm_load_init (void)
{
	int i;
	struct proc_dir_entry *entry;
	ulong timep, flags;
	volatile immap_t *immap = (immap_t *) IMAP_ADDR;
	volatile cpm8xx_t *cp = &immap->im_cpm;
	volatile risc_timer_t *rtram =
			(risc_timer_t *) & cp->cp_dparam[PROFF_RTIMER];


	entry = create_proc_entry (PROC_NAME,
				   S_IFREG | S_IRUGO | S_IWUGO,
				   NULL);
	if (entry == NULL) {
		printk ("Unable to create /proc/%s entry\n", PROC_NAME);
		return -EINVAL;
	}
	entry->read_proc = cpm_load_proc_read;
	entry->write_proc = cpm_load_proc_write;

	/* Make sure the RISC Timer tick clock is disabled.
	 */
#ifdef RCCR_BUG
	*(uint *) & (immap->im_cpm.cp_rccr) &= ~0x80000000;
#else
	immap->im_cpm.cp_rccr &= ~0x8000;
#endif

	timep = 15;

	/* Write the timep value to the rccr register.
	 */
#ifdef RCCR_BUG
	*(uint *) & (immap->im_cpm.cp_rccr) &= ~(0x3f << 24);
	*(uint *) & (immap->im_cpm.cp_rccr) |= timep << 24;
#else
	immap->im_cpm.cp_rccr &= ~(0x3f << 8);
	immap->im_cpm.cp_rccr |= timep << 8;
#endif

	/* Allocate DPRAM for the timers.
	 */
	risc_dpram_timers = m8xx_cpm_dpalloc (sizeof (ulong) * RISC_TIMER_MAX);
	rtram->tm_base = risc_dpram_timers;

	/* Clear number of ticks elapsed.
	 */
	rtram->tm_cnt = 0;

	/* Clear all RISC Timer events.
	 */
	immap->im_cpm.cp_rter = 0xffff;

	/* Disable all RISC Timer interrupts.
	 */
	immap->im_cpm.cp_rtmr = 0x0000;

	for (i = 0; i < RISC_TIMER_MAX; i++) {
		int timeout = 0;

		rtram->tm_cmd = (i << 16) | timeout;
		rtram->tm_cmd |= 0xC0000000;	/* periodic */

		/* Wait for a previous command to clear.
		 */
		while (cp->cp_cpcr & CPM_CR_FLG);

		/* Issue SET TIMER command.
		 */
		cp->cp_cpcr = mk_cr_cmd (CPM_CR_CH_TIMER, CPM_CR_SET_TIMER) |
				CPM_CR_FLG;
#if 0
		while (cp->cp_cpcr & CPM_CR_FLG);
#endif
	}


	/* Initialize a general purpose timer.
	 * We have to use both Timer 1 and Timer 2 to get the desired
	 * clock of 1/16384 system clock, which the MPC860 manual
	 * suggests.
	 */

	/* reset timer1 and timer2 */
	immap->im_cpmtimer.cpmt_tgcr &= ~(CPMTIMER_TGCR_GM1 |
					  CPMTIMER_TGCR_FRZ1 |
					  CPMTIMER_TGCR_STP1 |
					  CPMTIMER_TGCR_CAS2 |
					  CPMTIMER_TGCR_FRZ2 |
					  CPMTIMER_TGCR_STP2);

	immap->im_cpmtimer.cpmt_tmr1 = 0x0;	/* timer2 is the clock source */
	immap->im_cpmtimer.cpmt_trr1 = 0xffff;
	immap->im_cpmtimer.cpmt_tcr1 = 0;
	immap->im_cpmtimer.cpmt_tcn1 = 0;
	immap->im_cpmtimer.cpmt_ter1 = 0xffff;

	immap->im_cpmtimer.cpmt_tmr2 = 0xff04 | 0x8;
	immap->im_cpmtimer.cpmt_trr2 = 0x3;
	immap->im_cpmtimer.cpmt_tcr2 = 0;
	immap->im_cpmtimer.cpmt_tcn2 = 0;
	immap->im_cpmtimer.cpmt_ter2 = 0xffff;

	save_flags (flags);
	cli ();

	/* Start the RISC Timers.
	 */
#ifdef RCCR_BUG
	*(uint *) & (immap->im_cpm.cp_rccr) |= 0x80000000;
#else
	immap->im_cpm.cp_rccr |= 0x8000;
#endif

	/* Start the general purpose timers.
	 */
	immap->im_cpmtimer.cpmt_tgcr |=
			CPMTIMER_TGCR_RST1 | CPMTIMER_TGCR_RST2;

	restore_flags (flags);

	printk (KERN_INFO "CPM load tracking driver " DRIVER_VERSION "\n");
	return 0;
}

module_init (cpm_load_init);

#ifdef MODULE

static void __exit cpm_load_cleanup (void)
{
	int i;
	volatile immap_t *immap = (immap_t *) IMAP_ADDR;
	volatile cpm8xx_t *cp = &immap->im_cpm;
	volatile risc_timer_t *rtram =
			(risc_timer_t *) & cp->cp_dparam[PROFF_RTIMER];

	remove_proc_entry (PROC_NAME, NULL);

	/* Stop the general purpose timers.
	 */
	immap->im_cpmtimer.cpmt_tgcr |=
			CPMTIMER_TGCR_STP1 | CPMTIMER_TGCR_STP2;

	/* Stop the RISC timers.
	 */
#ifdef RCCR_BUG
	*(uint *) & (immap->im_cpm.cp_rccr) &= ~0x80000000;
#else
	immap->im_cpm.cp_rccr &= ~0x8000;
#endif

	/* Disable RISC timers.
	 */
	for (i = 0; i < RISC_TIMER_MAX; i++) {
		rtram->tm_cmd = (i << 16);

		/* Wait for a previous command to clear.
		 */
		while (cp->cp_cpcr & CPM_CR_FLG);

		/* Issue SET TIMER command.
		 */
		cp->cp_cpcr = mk_cr_cmd (CPM_CR_CH_TIMER, CPM_CR_SET_TIMER) |
					 CPM_CR_FLG;
#if 0
		while (cp->cp_cpcr & CPM_CR_FLG);
#endif
	}

	m8xx_cpm_dpfree (risc_dpram_timers);
}

module_exit (cpm_load_cleanup);

#endif							/* MODULE */
