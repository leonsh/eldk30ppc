/*
 * arch/ppc/platforms/pumaA_setup.c
 *
 * Board setup routines for the Momentum Puma-A (PPC 750) Board
 * Adapted from ev64260_setup.c (Marvell/Galileo EV-64260-BP Evaluation Board)
 *
 * Author: Gary Thomas <gary@mlbassoc.com>
 * Copyright (C) 2003 MLB Associates
 *
 * Orignal data
 * Author: Mark A. Greer <mgreer@mvista.com>
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
#include <linux/major.h>
#include <linux/blk.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/seq_file.h>
#include <linux/serial.h>

#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/time.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/machdep.h>
#include <asm/prom.h>
#include <asm/smp.h>
#include <asm/todc.h>
#include <asm/bootinfo.h>
#include <platforms/pumaA.h>
#include <asm/gt64260.h>


extern char cmd_line[];
unsigned long puma_find_end_of_memory(void);


/*
 * RTC/NVRAM support
 */

TODC_ALLOC();

static u_char
puma_todc_read_val(int addr)
{
    u_char val = 0;

    // For some reason, reading too fast causes the RTC to freeze
    udelay(1*1000*1000);
    val = *(u_char *)(todc_info->nvram_data + addr);
//    printk("TODC[%x] => %x\n", addr, val);
    return val;
}

static void
puma_todc_write_val(int addr, u_char val)
{  
//    printk("TODC[%x] <= %x\n", addr, val);
    *(volatile u_char *)(todc_info->nvram_data + addr) = val;
}

/*
 * Momentum PUMA-A PCI interrupt routing.
 */
static int __init
puma_map_irq(struct pci_dev *dev, unsigned char idsel, unsigned char pin)
{
	struct pci_controller	*hose = pci_bus_to_hose(dev->bus->number);
        printk(KERN_ERR "Puma-A PCI map IRQ\n");
        return -1;
}

static void __init
puma_setup_bridge(void)
{
	gt64260_bridge_info_t		info;

	GT64260_BRIDGE_INFO_DEFAULT(&info, puma_find_end_of_memory());

	/* Map in the bridge's registers */
	info.phys_base_addr = gt64260_base;
	gt64260_base = (u32)ioremap(gt64260_base, GT64260_INTERNAL_SPACE_SIZE);

	/* Lookup PCI host bridges */
	if (gt64260_find_bridges(&info, puma_map_irq)) {
		printk("Bridge initialization failed.\n");
	}

	/*
	 * Enabling of PCI internal-vs-external arbitration
	 * is a platform- and errata-dependent decision.
	 */
	if(gt64260_revision == GT64260)  {
		/* FEr#35 */
		gt_clr_bits(GT64260_PCI_0_ARBITER_CNTL, (1<<31));
		gt_clr_bits(GT64260_PCI_1_ARBITER_CNTL, (1<<31));
	} else if( gt64260_revision == GT64260A )  {
		gt_set_bits(GT64260_PCI_0_ARBITER_CNTL, (1<<31));
		gt_set_bits(GT64260_PCI_1_ARBITER_CNTL, (1<<31));
		/* Make external GPP interrupts level sensitive */
		gt_set_bits(GT64260_COMM_ARBITER_CNTL, (1<<10));
		/* Doc Change 9: > 100 MHz so must be set */
		gt_set_bits(GT64260_CPU_CONFIG, (1<<23));
	}

	gt_set_bits(GT64260_CPU_MASTER_CNTL, (1<<9)); /* Only 1 cpu */

	/*
	 * Set up windows for embedded FLASH (using boot CS window),
	 * and for the SRAM, TODC, UARTs, and FLASH on the device module.
	 *
	 * Assumes that the bootstrap (PMON) has set up the Device/Boot Bank Param regs.
	 */
	gt64260_cpu_boot_set_window(PUMA_EMB_FLASH_BASE, PUMA_EMB_FLASH_SIZE);
	gt64260_cpu_cs_set_window(0, PUMA_CPLD_BASE, PUMA_CPLD_SIZE);
	gt64260_cpu_cs_set_window(1, PUMA_TODC_BASE, PUMA_TODC_SIZE);
	gt64260_cpu_cs_set_window(3, PUMA_EXT_FLASH_BASE, PUMA_EXT_FLASH_SIZE);

	/* Set MPSC Multiplex RMII */
	/* NOTE: ethernet driver modifies bit 0 and 1 */
	gt_write(GT64260_MPP_SERIAL_PORTS_MULTIPLEX, 0x00001102);

	return;
}


static void __init
puma_setup_arch(void)
{
#if	!defined(CONFIG_GT64260_CONSOLE)
	struct serial_struct	serial_req;
#endif

	if ( ppc_md.progress )
		ppc_md.progress("puma_setup_arch: enter", 0);

	loops_per_jiffy = 50000000 / HZ;

#ifdef CONFIG_BLK_DEV_INITRD
	if (initrd_start)
		ROOT_DEV = MKDEV(RAMDISK_MAJOR, 0);
	else
#endif
#ifdef	CONFIG_ROOT_NFS
		ROOT_DEV = to_kdev_t(0x00FF);	/* /dev/nfs pseudo device */
#else
		ROOT_DEV = to_kdev_t(0x0802);	/* /dev/sda2 SCSI disk */
#endif

	if ( ppc_md.progress )
		ppc_md.progress("puma_setup_arch: find_bridges", 0);

	/*
	 * Set up the L2CR register.
	 * L2 cache was invalidated by bootloader.
	 */
	switch (PVR_VER(mfspr(PVR))) {
		case PVR_VER(PVR_750):
			_set_L2CR(0xfd100000);
			break;
		case PVR_VER(PVR_7400):
		case PVR_VER(PVR_7410):
			_set_L2CR(0xcd100000);
			break;
		/* case PVR_VER(PVR_7450): */
			/* XXXX WHAT VALUE?? FIXME */
			break;
	}

	puma_setup_bridge();

	TODC_INIT(TODC_TYPE_DS17285, 0, 0,
		  ioremap(PUMA_TODC_BASE, PUMA_TODC_SIZE), 8);

#ifdef	CONFIG_DUMMY_CONSOLE
	conswitchp = &dummy_con;
#endif

#if	!defined(CONFIG_GT64260_CONSOLE)
	memset(&serial_req, 0, sizeof(serial_req));
	serial_req.line = 0;
	serial_req.baud_base = BASE_BAUD;
	serial_req.port = 0;
	serial_req.irq = PUMA_UART_0_IRQ;
	serial_req.flags = STD_COM_FLAGS;
	serial_req.io_type = SERIAL_IO_MEM;
	serial_req.iomem_base = ioremap(PUMA_SERIAL_0, PUMA_UART_SIZE);
	serial_req.iomem_reg_shift = 2;

	if (early_serial_setup(&serial_req) != 0) {
		printk("Early serial init of port 0 failed\n");
	}

	/* Assume early_serial_setup() doesn't modify serial_req */
	serial_req.line = 1;
	serial_req.port = 1;
	serial_req.irq = PUMA_UART_1_IRQ;
	serial_req.iomem_base = ioremap(PUMA_SERIAL_1, PUMA_UART_SIZE);

	if (early_serial_setup(&serial_req) != 0) {
		printk("Early serial init of port 1 failed\n");
	}
#endif

	printk("Momentum Puma-A (PPC 750FX) Board\n");
	printk("  (C) 2002 MLB Associates  (linux@mlbassoc.com)\n");

	if ( ppc_md.progress )
		ppc_md.progress("puma_setup_arch: exit", 0);

	return;
}

static void __init
puma_init_irq(void)
{
	gt64260_init_irq();

	if(gt64260_revision != GT64260)  {
		/* XXXX Kludge--need to fix gt64260_init_irq() interface */
		/* Mark PCI intrs level sensitive */
		irq_desc[91].status |= IRQ_LEVEL;
		irq_desc[93].status |= IRQ_LEVEL;
	}
}

static void __init
puma_map_io(void)
{
    // This insures that the mapping persists after the MMU is initialized!
    io_block_mapping(0xf0000000, 0xf0000000, 0x10000000, _PAGE_IO);
}

static unsigned long __init
puma_find_end_of_memory(void)
{
	/* Next 2 lines are a kludge for gt64260_get_mem_size() */
	gt64260_base = PUMA_BRIDGE_REG_BASE;
	return gt64260_get_mem_size();
}

static void
puma_reset_board(void)
{
	__cli();

	/* Set exception prefix high - to the firmware */
	_nmask_and_or_msr(0, MSR_IP);

	/* XXX FIXME */
	printk("XXXX **** trying to reset board ****\n");
	return;
}

static void
puma_restart(char *cmd)
{
	volatile ulong	i = 10000000;

	puma_reset_board();

	while (i-- > 0);
	panic("restart failed\n");
}

static void
puma_halt(void)
{
	__cli();
	while (1);
	/* NOTREACHED */
}

static void
puma_power_off(void)
{
	puma_halt();
	/* NOTREACHED */
}

static int
puma_show_cpuinfo(struct seq_file *m)
{
	seq_printf(m, "vendor\t\t: Momentum\n");
	seq_printf(m, "machine\t\t: Puma-A\n");

	return 0;
}

static void __init
puma_calibrate_decr(void)
{
	ulong freq;

	freq = 133320000 / 4;

	printk("time_init: decrementer frequency = %lu.%.6lu MHz\n",
	       freq/1000000, freq%1000000);

	tb_ticks_per_jiffy = freq / HZ;
	tb_to_us = mulhwu_scale_factor(freq, 1000000);

	return;
}

#if !defined(CONFIG_USE_PPCBOOT) || defined(CONFIG_SERIAL_TEXT_DEBUG)
/*
 * Set BAT 3 to map 0xf0000000 to end of physical memory space.
 */
static __inline__ void
pumaA_set_bat(void)
{
	unsigned long   bat3u, bat3l;

	__asm__ __volatile__
		("lis %0,0xf000\n\t"
		 "ori %1,%0,0x002a\n\t"
		 "ori %0,%0,0x1ffe\n\t"
		 "mtspr 0x21e,%0\n\t"
		 "mtspr 0x21f,%1\n\t"
		 "isync\n\t"
		 "sync\n\t"
		 : "=r" (bat3u), "=r" (bat3l));
}
#endif

void __init
platform_init(unsigned long r3, unsigned long r4, unsigned long r5,
	      unsigned long r6, unsigned long r7)
{
        gt64260_base = CONFIG_GT64260_NEW_REG_BASE;

	parse_bootinfo(find_bootinfo());

	isa_mem_base = 0;

	ppc_md.setup_arch = puma_setup_arch;
	ppc_md.show_cpuinfo = puma_show_cpuinfo;
	ppc_md.irq_cannonicalize = NULL;
	ppc_md.init_IRQ = puma_init_irq;
	ppc_md.get_irq = gt64260_get_irq;
	ppc_md.init = NULL;

	ppc_md.restart = puma_restart;
	ppc_md.power_off = puma_power_off;
	ppc_md.halt = puma_halt;

	ppc_md.find_end_of_memory = puma_find_end_of_memory;

	ppc_md.calibrate_decr = puma_calibrate_decr;

	ppc_md.time_init = todc_time_init;
	ppc_md.set_rtc_time = todc_set_rtc_time;
	ppc_md.get_rtc_time = todc_get_rtc_time;

	ppc_md.nvram_read_val = puma_todc_read_val;
	ppc_md.nvram_write_val = puma_todc_write_val;


#ifdef	CONFIG_SERIAL_TEXT_DEBUG
	pumaA_set_bat();
	ppc_md.setup_io_mappings = puma_map_io;
	gt64260_base = PUMA_BRIDGE_REG_BASE;
	ppc_md.progress = gt64260_mpsc_progress; /* embedded UART */
#else	/* !CONFIG_SERIAL_TEXT_DEBUG */
	ppc_md.progress = NULL;
#endif	/* CONFIG_SERIAL_TEXT_DEBUG */
}
