/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * MA 02111-1307 USA
 */

#include <linux/config.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/user.h>
#include <linux/a.out.h>
#include <linux/tty.h>
#include <linux/major.h>
#include <linux/interrupt.h>
#include <linux/reboot.h>
#include <linux/init.h>
#include <linux/blk.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/seq_file.h>

#include <asm/mmu.h>
#include <asm/processor.h>
#include <asm/residual.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/ppcboot.h>
#include <asm/machdep.h>
#include <asm/bootinfo.h>
#include <asm/time.h>
#include <asm/open_pic.h>

unsigned char __res[sizeof(bd_t)];

extern void cu824_find_bridges(void);

static u_char cu824_openpic_initsenses[] = {
	0,
	1,
	1,
	1,
	1,
	1,
};

void __init
cu824_setup_arch(void)
{
	cu824_find_bridges();
}

/* The decrementer counts at the system (internal) clock frequency
 * divided by four.
 */
static void __init
cu824_calibrate_decr(void)
{
	bd_t	*binfo = (bd_t *)__res;
	int freq, divisor;

	freq = binfo->bi_busfreq;
        divisor = 4;
        tb_ticks_per_jiffy = freq / HZ / divisor;
	tb_to_us = mulhwu_scale_factor(freq / divisor, 1000000);
}

static void
cu824_restart(char *cmd)
{
	__cli();

		/* Enable watchdog.
		 */
	*(volatile char *)(0xfe800013) |= 0x20;

		/* Not reached.
		 */
	for (;;);
}

static void
cu824_power_off(void)
{
   cu824_restart(NULL);
}

static void
cu824_halt(void)
{
   cu824_restart(NULL);
}

static int
cu824_show_percpuinfo(struct seq_file *m, int i)
{
	bd_t	*bp;

	bp = (bd_t *)__res;
			
	seq_printf(m, "core clock\t: %lu MHz\n"
                   "bus  clock\t: %lu MHz\n",
                   bp->bi_intfreq / 1000000,
		   bp->bi_busfreq / 1000000);

	return 0;
}

/* Initialize the internal interrupt controller.  The number of
 * interrupts supported can vary with the processor type, and the
 * 8260 family can have up to 64.
 * External interrupts can be either edge or level triggered, and
 * need to be initialized by the appropriate driver.
 */
static void __init
cu824_init_IRQ(void)
{
	OpenPIC_InitSenses = cu824_openpic_initsenses;
	OpenPIC_NumInitSenses = sizeof( cu824_openpic_initsenses );

	openpic_set_sources(0, 3, OpenPIC_Addr + 0x10200);
	openpic_set_sources(16, 1, OpenPIC_Addr + 0x11020);

	openpic_init(0);

		/* Enable PCI interrupts within the on-board controller.
		 */
	*(volatile char *)(0xfe80000b) = 0x0f;
}

static unsigned long __init
cu824_find_end_of_memory(void)
{
	bd_t	*binfo;
	extern unsigned char __res[];
	
	binfo = (bd_t *)__res;

	return binfo->bi_memsize;
}

static void __init
cu824_map_io(void)
{
	io_block_mapping(0x80000000, 0x80000000, 0x10000000, _PAGE_IO);
	io_block_mapping(0xF8000000, 0xF8000000, 0x08000000, _PAGE_IO);
}

void __init
platform_init(unsigned long r3, unsigned long r4, unsigned long r5,
	      unsigned long r6, unsigned long r7)
{
	parse_bootinfo(find_bootinfo());
	
	if ( r3 )
		memcpy( (void *)__res,(void *)(r3+KERNELBASE), sizeof(bd_t) );
	
#ifdef CONFIG_BLK_DEV_INITRD
	/* take care of initrd if we have one */
	if ( r4 )
	{
		initrd_start = r4 + KERNELBASE;
		initrd_end = r5 + KERNELBASE;
	}
#endif /* CONFIG_BLK_DEV_INITRD */
	/* take care of cmd line */
	if ( r6 )
	{
		
		*(char *)(r7+KERNELBASE) = 0;
		strcpy(cmd_line, (char *)(r6+KERNELBASE));
	}

	ppc_md.setup_arch          = cu824_setup_arch;
	ppc_md.show_percpuinfo     = cu824_show_percpuinfo;
	ppc_md.init_IRQ            = cu824_init_IRQ;
	ppc_md.get_irq	           = openpic_get_irq;
	ppc_md.restart             = cu824_restart;
	ppc_md.power_off           = cu824_power_off;
	ppc_md.halt                = cu824_halt;
	ppc_md.calibrate_decr      = cu824_calibrate_decr;
	ppc_md.find_end_of_memory  = cu824_find_end_of_memory;
	ppc_md.setup_io_mappings   = cu824_map_io;
}
