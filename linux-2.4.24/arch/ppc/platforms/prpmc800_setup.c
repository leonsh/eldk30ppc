/*
 *
 * Author: Dale Farnsworth <dale.farnsworth@mvista.com>
 *
 * Copyright 2001 MontaVista Software Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/config.h>
#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/reboot.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/types.h>
#include <linux/major.h>
#include <linux/blk.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/seq_file.h>
#include <linux/ide.h>

#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/machdep.h>
#include <asm/time.h>
#include <platforms/prpmc800.h>
#include <asm/open_pic.h>
#include <asm/bootinfo.h>
#include <asm/harrier.h>

#define HARRIER_REVI_REG	(PRPMC800_HARRIER_XCSR_BASE+HARRIER_REVI_OFF)
#define HARRIER_UCTL_REG	(PRPMC800_HARRIER_XCSR_BASE+HARRIER_UCTL_OFF)
#define HARRIER_MISC_CSR_REG  (PRPMC800_HARRIER_XCSR_BASE+HARRIER_MISC_CSR_OFF)
#define HARRIER_IFEVP_REG   (PRPMC800_HARRIER_MPIC_BASE+HARRIER_MPIC_IFEVP_OFF)
#define HARRIER_IFEDE_REG   (PRPMC800_HARRIER_MPIC_BASE+HARRIER_MPIC_IFEDE_OFF)
#define HARRIER_FEEN_REG	(PRPMC800_HARRIER_XCSR_BASE+HARRIER_FEEN_OFF)
#define HARRIER_FEMA_REG	(PRPMC800_HARRIER_XCSR_BASE+HARRIER_FEMA_OFF)

extern void prpmc800_find_bridges(void);
extern int mpic_init(void);
extern unsigned long loops_per_jiffy;
extern void gen550_progress(char *, unsigned short);

static u_char prpmc800_openpic_initsenses[] __initdata =
{
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_HOSTINT0 */
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_UNUSED */
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_DEBUGINT */
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_HARRIER_WDT */
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_UNUSED */
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_UNUSED */
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_HOSTINT1 */
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_HOSTINT2 */
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_HOSTINT3 */
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_PMC_INTA */
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_PMC_INTB */
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_PMC_INTC */
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_PMC_INTD */
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_UNUSED */
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_UNUSED */
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_UNUSED */
   (IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* PRPMC800_INT_HARRIER_INT (UARTS, ABORT, DMA) */
};

static int
prpmc800_show_cpuinfo(struct seq_file *m)
{
	seq_printf(m, "machine\t\t: PrPMC800\n");

	return 0;
}

static void __init
prpmc800_setup_arch(void)
{

	/* init to some ~sane value until calibrate_delay() runs */
	loops_per_jiffy = 50000000/HZ;

	/* Lookup PCI host bridges */
	prpmc800_find_bridges();

#ifdef CONFIG_BLK_DEV_INITRD
	if (initrd_start)
		ROOT_DEV = MKDEV(RAMDISK_MAJOR, 0); /* /dev/ram */
	else
#endif
#ifdef CONFIG_ROOT_NFS
		ROOT_DEV = to_kdev_t(0x00ff); /* /dev/nfs pseudo device */
#else
		ROOT_DEV = to_kdev_t(0x0802); /* /dev/sda2 */
#endif

#ifdef CONFIG_DUMMY_CONSOLE
	conswitchp = &dummy_con;
#endif

	printk("PrPMC800 port (C) 2001 MontaVista Software, Inc. (source@mvista.com)\n");
}

/*
 * Compute the PrPMC800's tbl frequency using the baud clock as a reference.
 */

static void __init
prpmc800_calibrate_decr(void)
{
	unsigned long tbl_start, tbl_end;
	unsigned long current_state, old_state, tb_ticks_per_second;
	unsigned int count;
	unsigned int harrier_revision;

	harrier_revision = readb(HARRIER_REVI_REG);
	if (harrier_revision < 2) {
		/* XTAL64 was broken in harrier revision 1 */
		printk("time_init: Harrier revision %d, assuming 100 Mhz bus\n",
			harrier_revision);
		tb_ticks_per_second = 100000000/4;
		tb_ticks_per_jiffy = tb_ticks_per_second / HZ;
		tb_to_us = mulhwu_scale_factor(tb_ticks_per_second, 1000000);
		return;
	}

	/*
	 * The XTAL64 bit oscillates at the 1/64 the base baud clock
	 * Set count to XTAL64 cycles per second.  Since we'll count
	 * half-cycles, we'll reach the count in half a second.
	 */
	count = PRPMC800_BASE_BAUD / 64;

	/* Find the first edge of the baud clock */
	old_state = readb(HARRIER_UCTL_REG) & HARRIER_XTAL64_MASK;
	do {
		current_state = readb(HARRIER_UCTL_REG) &
			HARRIER_XTAL64_MASK;
	} while(old_state == current_state);

	old_state = current_state;

	/* Get the starting time base value */
	tbl_start = get_tbl();

	/*
	 * Loop until we have found a number of edges (half-cycles)
	 * equal to the count (half a second)
	 */
	do {
		do {
			current_state = readb(HARRIER_UCTL_REG) &
				HARRIER_XTAL64_MASK;
		} while(old_state == current_state);
		old_state = current_state;
	} while (--count);

	/* Get the ending time base value */
	tbl_end = get_tbl();

	/* We only counted for half a second, so double to get ticks/second */
	tb_ticks_per_second = (tbl_end - tbl_start) * 2;
	tb_ticks_per_jiffy = tb_ticks_per_second / HZ;
	tb_to_us = mulhwu_scale_factor(tb_ticks_per_second, 1000000);
}

static void
prpmc800_restart(char *cmd)
{
	__cli();
	writeb(HARRIER_RSTOUT_MASK, HARRIER_MISC_CSR_REG);
	while(1);
}

static void
prpmc800_halt(void)
{
	__cli();
	while (1);
}

static void
prpmc800_power_off(void)
{
	prpmc800_halt();
}

static void __init
prpmc800_init_IRQ(void)
{
	OpenPIC_InitSenses = prpmc800_openpic_initsenses;
	OpenPIC_NumInitSenses = sizeof(prpmc800_openpic_initsenses);

	/* Setup external interrupt sources. */
	openpic_set_sources(0, 16, OpenPIC_Addr + 0x10000);
	/* Setup internal UART interrupt source. */
	openpic_set_sources(16, 1, OpenPIC_Addr + 0x10200);

	/* Do the MPIC initialization based on the above settings. */
	openpic_init(0);

	/* enable functional exceptions for uarts and abort */
	out_8((u8 *)HARRIER_FEEN_REG, (HARRIER_FE_UA0|HARRIER_FE_UA1));
	out_8((u8 *)HARRIER_FEMA_REG, ~(HARRIER_FE_UA0|HARRIER_FE_UA1));
}

/*
 * We need to read the Harrier memory controller
 * to properly determine this value
 */
static unsigned long __init
prpmc800_find_end_of_memory(void)
{
	/* Read the memory size from the Harrier XCSR */
	return harrier_get_mem_size(PRPMC800_HARRIER_XCSR_BASE);
}

/*
 * Set BAT 3 to map 0xf0000000 to end of physical memory space.
 */
static __inline__ void
prpmc800_set_bat(void)
{
	mb();
	mtspr(DBAT1U, 0xf0001ffe);
	mtspr(DBAT1L, 0xf000002a);
	mb();
}

static void __init
prpmc800_map_io(void)
{
        io_block_mapping(0x80000000, 0x80000000, 0x10000000, _PAGE_IO);
	io_block_mapping(0xf0000000, 0xf0000000, 0x10000000, _PAGE_IO);
}

void __init
platform_init(unsigned long r3, unsigned long r4, unsigned long r5,
	  unsigned long r6, unsigned long r7)
{
	parse_bootinfo(find_bootinfo());

	prpmc800_set_bat();

	isa_io_base = PRPMC800_ISA_IO_BASE;
	isa_mem_base = PRPMC800_ISA_MEM_BASE;
	pci_dram_offset = PRPMC800_PCI_DRAM_OFFSET;

	ppc_md.setup_arch	= prpmc800_setup_arch;
	ppc_md.show_cpuinfo	= prpmc800_show_cpuinfo;
	ppc_md.init_IRQ		= prpmc800_init_IRQ;
	ppc_md.get_irq		= openpic_get_irq;

	ppc_md.find_end_of_memory = prpmc800_find_end_of_memory;
	ppc_md.setup_io_mappings = prpmc800_map_io;

	ppc_md.restart		= prpmc800_restart;
	ppc_md.power_off	= prpmc800_power_off;
	ppc_md.halt		= prpmc800_halt;

	/* PrPMC800 has no timekeeper part */
	ppc_md.time_init	= NULL;
	ppc_md.get_rtc_time	= NULL;
	ppc_md.set_rtc_time	= NULL;
	ppc_md.calibrate_decr	= prpmc800_calibrate_decr;
#ifdef  CONFIG_SERIAL_TEXT_DEBUG
        ppc_md.progress = gen550_progress;
#else   /* !CONFIG_SERIAL_TEXT_DEBUG */
	ppc_md.progress = NULL;
#endif  /* CONFIG_SERIAL_TEXT_DEBUG */
}
