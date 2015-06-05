/*
 *
 * Authors: Kent Borg, Dale Farnsworth <dale.farnsworth@mvista.com>,
 * 	and Wolfgang Denk <wd@denx.de>
 *
 * Copyright 2003 Motorola Inc.
 * Copyright 2003 MontaVista Software Inc.
 * Copyright 2003 DENX Software Engineering (wd@denx.de)
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
#include <asm/bootinfo.h>
#include <asm/machdep.h>
#include <asm/time.h>
#include <asm/pci-bridge.h>
#include <asm/mpc5xxx.h>
#ifdef CONFIG_UBOOT
#include <asm/ppcboot.h>
unsigned char __res[sizeof(bd_t)];
#endif

extern unsigned long mpc5xxx_find_end_of_memory(void);
extern void mpc5xxx_find_bridges(void);
extern void mpc5xxx_set_bat(void);
extern void mpc5xxx_map_io(void);
extern void mpc5xxx_restart(char *cmd);
extern void mpc5xxx_halt(void);
extern void mpc5xxx_power_off(void);
extern void mpc5xxx_ide_init_hwif_ports(hw_regs_t *hw,
		      ide_ioreg_t data_port, ide_ioreg_t ctrl_oort, int *irq);
extern ide_ioreg_t mpc5xxx_ide_get_base(int index);

static int
icecube_show_cpuinfo(struct seq_file *m)
{
	seq_printf(m, "machine\t\t: Motorola IceCube\n");

	return 0;
}

static void __init
icecube_setup_arch(void)
{
#ifdef CONFIG_BLK_DEV_INITRD
	if (initrd_start)
		ROOT_DEV = MKDEV(RAMDISK_MAJOR, 0); /* /dev/ram */
	else
#endif
#ifdef CONFIG_ROOT_NFS
		ROOT_DEV = to_kdev_t(0x00ff); /* /dev/nfs pseudo device */
#else
		ROOT_DEV = to_kdev_t(0x0301); /* /dev/hda1 */
#endif

#ifdef CONFIG_DUMMY_CONSOLE
	conswitchp = &dummy_con;
#endif

#if defined(CONFIG_PCI) && defined(CONFIG_MPC5200)
	mpc5xxx_find_bridges();
#endif
}

/* The decrementer counts at the system (internal) clock frequency
 * divided by four.
 */
static void __init
icecube_calibrate_decr(void)
{
	int freq, divisor;
#ifdef CONFIG_UBOOT
	bd_t *bd = (bd_t *)__res;
	freq = bd->bi_busfreq;
#else
	/* Decrementer driven from XLB clock; CPU clock/3.5 on IceCube */
	freq = CONFIG_PPC_5xxx_PROCFREQ/3.5;
#endif
	divisor = 4;
	tb_ticks_per_jiffy = freq / HZ / divisor;
	tb_to_us = mulhwu_scale_factor(freq / divisor, 1000000);
}

void __init
platform_init(unsigned long r3, unsigned long r4, unsigned long r5,
	  unsigned long r6, unsigned long r7)
{
	parse_bootinfo(find_bootinfo());

#ifdef CONFIG_UBOOT
	if ( r3 )
		memcpy( (void *)__res,(void *)(r3+KERNELBASE), sizeof(bd_t) );

#ifdef CONFIG_BLK_DEV_INITRD
	/* take care of initrd if we have one */
	if ( r4 ) {
		initrd_start = r4 + KERNELBASE;
		initrd_end = r5 + KERNELBASE;
	}
#endif /* CONFIG_BLK_DEV_INITRD */
	/* take care of cmd line */
	if ( r6 ) {
		*(char *)(r7+KERNELBASE) = 0;
		strcpy(cmd_line, (char *)(r6+KERNELBASE));
	}
#endif /* CONFIG_UBOOT */

	mpc5xxx_set_bat();

	isa_io_base		= MPC5xxx_ISA_IO_BASE;
	isa_mem_base		= MPC5xxx_ISA_MEM_BASE;
	pci_dram_offset		= MPC5xxx_PCI_DRAM_OFFSET;

	ppc_md.setup_arch	= icecube_setup_arch;
	ppc_md.show_cpuinfo	= icecube_show_cpuinfo;
	ppc_md.show_percpuinfo	= NULL;
	ppc_md.init_IRQ		= mpc5xxx_init_irq;
	ppc_md.get_irq		= mpc5xxx_get_irq;

	ppc_md.find_end_of_memory = mpc5xxx_find_end_of_memory;
	ppc_md.setup_io_mappings = mpc5xxx_map_io;

	ppc_md.restart		= mpc5xxx_restart;
	ppc_md.power_off	= mpc5xxx_power_off;
	ppc_md.halt		= mpc5xxx_halt;

	/* MPC5xxx have no timekeeper part */
	ppc_md.time_init	= NULL;
	ppc_md.get_rtc_time	= NULL;
	ppc_md.set_rtc_time	= NULL;
	ppc_md.calibrate_decr	= icecube_calibrate_decr;
#ifdef  CONFIG_SERIAL_TEXT_DEBUG
	ppc_md.progress = mpc5xxx_progress;
#endif  /* CONFIG_SERIAL_TEXT_DEBUG */

#ifdef CONFIG_VT
	ppc_md.kbd_setkeycode	   = mpc5xxx_setkeycode;
	ppc_md.kbd_getkeycode	   = mpc5xxx_getkeycode;
	ppc_md.kbd_translate	   = mpc5xxx_translate;
	ppc_md.kbd_unexpected_up   = mpc5xxx_unexpected_up;
	ppc_md.kbd_leds		   = NULL;
	ppc_md.kbd_init_hw	   = mpc5xxx_kbd_init_hw;
#ifdef CONFIG_MAGIC_SYSRQ
	ppc_md.ppc_kbd_sysrq_xlate = mpc5xxx_sysrq_xlate;
	SYSRQ_KEY = 0x54;
#endif
#endif
	
#ifdef CONFIG_BLK_DEV_IDE_MPC5xxx
	ppc_ide_md.ide_init_hwif	= mpc5xxx_ide_init_hwif_ports;
	ppc_ide_md.default_io_base	= mpc5xxx_ide_get_base;
#endif /* CONFIG_BLK_DEV_IDE_MPC5xxx */
}
