/*
 * arch/ppc/platforms/glacier.c
 *
 * Core support for the Motorola Glacier platform
 *
 * Author: Dale Farnsworth	<dfarnsworth@mvista.com>
 *
 * 2003 (c) MontaVista, Software, Inc.  This file is licensed under the terms
 * of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
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
#include <asm/mpc5xxx.h>

extern unsigned long mpc5xxx_find_end_of_memory(void);
extern void mpc5xxx_find_bridges(void);
extern void mpc5xxx_set_bat(void);
extern void mpc5xxx_map_io(void);
extern void mpc5xxx_restart(char *cmd);
extern void mpc5xxx_halt(void);
extern void mpc5xxx_power_off(void);
extern void mpc5xxx_ide_init_hwif_ports(hw_regs_t *hw,
				ide_ioreg_t data_port, ide_ioreg_t ctrl_oort,
				int *irq);
extern ide_ioreg_t mpc5xxx_ide_get_base(int index);

static int
glacier_show_cpuinfo(struct seq_file *m)
{
	seq_printf(m, "machine\t\t: Motorola Glacier\n");

	return 0;
}

static void __init
glacier_setup_arch(void)
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

static void __init
glacier_calibrate_decr(void)
{
	int freq, divisor;

	freq = 231000000/3.5;
	printk("hardcoded clock to 231MHz, need to calculate it\n");
	divisor = 4;
	tb_ticks_per_jiffy = freq / HZ / divisor;
	tb_to_us = mulhwu_scale_factor(freq / divisor, 1000000);
}

void __init
platform_init(unsigned long r3, unsigned long r4, unsigned long r5,
	unsigned long r6, unsigned long r7)
{
	parse_bootinfo(find_bootinfo());

	mpc5xxx_set_bat();

	isa_io_base		= MPC5xxx_ISA_IO_BASE;
	isa_mem_base		= MPC5xxx_ISA_MEM_BASE;
	pci_dram_offset		= MPC5xxx_PCI_DRAM_OFFSET;

	ppc_md.setup_arch	= glacier_setup_arch;
	ppc_md.show_cpuinfo	= glacier_show_cpuinfo;
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
	ppc_md.calibrate_decr	= glacier_calibrate_decr;
#ifdef	CONFIG_SERIAL_TEXT_DEBUG
	ppc_md.progress = mpc5xxx_progress;
#endif	/* CONFIG_SERIAL_TEXT_DEBUG */

#ifdef CONFIG_VT
	ppc_md.kbd_setkeycode	= mpc5xxx_setkeycode;
	ppc_md.kbd_getkeycode	= mpc5xxx_getkeycode;
	ppc_md.kbd_translate	= mpc5xxx_translate;
	ppc_md.kbd_unexpected_up = mpc5xxx_unexpected_up;
	ppc_md.kbd_leds		= NULL;
	ppc_md.kbd_init_hw	= mpc5xxx_kbd_init_hw;
#ifdef CONFIG_MAGIC_SYSRQ
	ppc_md.ppc_kbd_sysrq_xlate = mpc5xxx_sysrq_xlate;
	SYSRQ_KEY = 0x54;
#endif
#endif

#ifdef CONFIG_BLK_DEV_IDE_MPC5xxx
	ppc_ide_md.ide_init_hwif = mpc5xxx_ide_init_hwif_ports;
	ppc_ide_md.default_io_base = mpc5xxx_ide_get_base;
#endif /* CONFIG_BLK_DEV_IDE_MPC5xxx */
}
