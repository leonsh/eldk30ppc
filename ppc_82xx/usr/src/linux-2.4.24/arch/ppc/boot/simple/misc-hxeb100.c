/*
 * arch/ppc/boot/simple/hxeb100-misc.c
 *
 * Board-specific bootloader code for the Motorola HXEB100 board.
 *
 * Author: Dale Farnsworth <dfarnsworth@mvista.com>
 *
 * 2003 (c) MontaVista, Software, Inc.  This file is licensed under the terms
 * of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */

#include <linux/types.h>
#include <asm/bootinfo.h>
#include <asm/string.h>
#include <platforms/hxeb100.h>

extern void udelay(long);
extern struct bi_record *decompress_kernel(unsigned long load_addr,
						int num_words,
						unsigned long cksum);

struct motload_mailbox hxeb100_wait_for_kernel_mailbox;
static int found_cpu1;
extern void hxeb100_cpu1_wait_for_kernel(void);

extern int gt64260_iic_read(uint devaddr, u8 *buf, uint offset,
			    uint offset_size, uint count);

static bd_t board_info;

static void
hxeb100_get_board_info(void)
{
	long moto;
	int dev = HXEB100_VPD_IIC_DEV;
	bd_t *bi = &board_info;

	gt64260_iic_read(dev, (char *)&moto, 0, 2, sizeof(moto));
	if (moto == 0x4D4F544F) {	/* 'MOTO' */
	    	bi->bi_hxeb = HXEB100_BOARD_INFO_HXEB;		/* 'HXEB' */
		gt64260_iic_read(dev, (u8 *)&bi->bi_busfreq, 80, 2, 4);
		gt64260_iic_read(dev, bi->bi_enetaddr[0],    94, 2, 6);
		gt64260_iic_read(dev, bi->bi_enetaddr[1],   103, 2, 6);
	}
}

static int
start_cpu1(volatile struct motload_mailbox *mailbox, void (*func)(void)) {
	int i;

	if (mailbox->login != MOTLOAD_SMP_IDLE)
		return -1;
	mailbox->func = func;
	mailbox->cmd = MOTLOAD_SMP_START_CMD;
	asm("sync");
	asm("isync");

	for (i=0; i<1000; i++) {
		if (mailbox->login == MOTLOAD_SMP_EXEC)
			return 0;
		udelay(1000);
	}
	return -2;
}

/*
 * hxeb100_bootloader_start_cpu1
 * Checks the MOTLoad smp mailbox.
 * If cpu1 is found, it sets it running at hxeb100_cpu1_wait_for_kernel().
 *
 * Should be called after the bootloader is relocated, but before the
 * kernel is loaded into low memory.
 */
void
hxeb100_boot_start_cpu1(void)
{
	volatile struct motload_mailbox *mailbox;
	int rv;

	mailbox = MOTLOAD_SMP_MAILBOX;
	found_cpu1 = 0;

	rv = start_cpu1(mailbox, hxeb100_cpu1_wait_for_kernel);
	if (rv == 0)
		found_cpu1++;
}


/*
 * hxeb100_kernel_start_cpu1
 * Checks the hxeb100_wait_for_kernel smp mailbox.
 * If cpu1 is found, it sets it running at KERNEL_SECONDARY_HOLD.
 * KERNEL_SECONDARY_HOLD is at address 0xc0, by convention.
 *
 * Should be called after the kernel is loaded, but before the
 * kernel is started.
 */
void
hxeb100_kernel_start_cpu1(void)
{
	volatile struct motload_mailbox *mailbox;

	if (!found_cpu1)
		return;		/* no cpu to start */

	mailbox = &hxeb100_wait_for_kernel_mailbox; 
	start_cpu1(mailbox, KERNEL_SECONDARY_HOLD);
}

struct bi_record *
load_kernel(unsigned long load_addr, int num_words, unsigned long cksum)
{
    	struct bi_record *bi_recs;
	struct bi_record *rec;

	hxeb100_get_board_info();

	hxeb100_boot_start_cpu1();

	bi_recs = decompress_kernel(load_addr, num_words, cksum);

	rec = bi_recs;
	if (rec && rec->tag == BI_FIRST) {
		while (rec->tag != BI_LAST)
			rec = (struct bi_record *)((ulong)rec + rec->size);

		rec->tag = BI_BOARD_INFO;
		memcpy((char *)rec->data, &board_info, sizeof(board_info));
		rec->size = sizeof(struct bi_record) + sizeof(board_info);
		rec = (struct bi_record *)((unsigned long)rec + rec->size);

		rec->tag = BI_LAST;
	        rec->size = sizeof(struct bi_record);
	        rec = (struct bi_record *)((ulong)rec + rec->size);
	}

	hxeb100_kernel_start_cpu1();

	return bi_recs;
}
