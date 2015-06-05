/*
 * arch/ppc/platforms/k2_setup.c
 *
 * Board setup routines for SBS K2
 *
 * Author: Matt Porter <mporter@mvista.com>
 *
 * 2001-2003 (c) MontaVista, Software, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
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
#include <linux/ide.h>
#include <linux/irq.h>
#include <linux/seq_file.h>
#include <linux/serialP.h>

#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/machdep.h>
#include <asm/time.h>
#include <asm/i8259.h>
#include <asm/todc.h>
#include <asm/bootinfo.h>

#include "cpc710.h"
#include "k2.h"

extern void k2_setup_hoses(void);
extern unsigned long loops_per_jiffy;
extern void gen550_progress(char *, unsigned short);
extern void gen550_init(int, struct serial_struct *);

static unsigned int cpu_7xx[16] = {
	0, 15, 14, 0, 0, 13, 5, 9, 6, 11, 8, 10, 16, 12, 7, 0
};
static unsigned int cpu_6xx[16] = {
	0, 0, 14, 0, 0, 13, 5, 9, 6, 11, 8, 10, 0, 12, 7, 0
};

#if defined(CONFIG_BLK_DEV_IDE) || defined(CONFIG_BLK_DEV_IDE_MODULE)
static void __init
k2_ide_init_hwif_ports (hw_regs_t *hw, ide_ioreg_t data_port,
		ide_ioreg_t ctrl_port, int *irq)
{
	ide_ioreg_t reg = data_port;
	int i = 8;

	for (i = IDE_DATA_OFFSET; i <= IDE_STATUS_OFFSET; i++) {
		hw->io_ports[i] = reg;
		reg += 1;
	}
	if (ctrl_port)
		hw->io_ports[IDE_CONTROL_OFFSET] = ctrl_port;
	else
		hw->io_ports[IDE_CONTROL_OFFSET] =
			hw->io_ports[IDE_DATA_OFFSET] + 0x206;

	if (irq != NULL)
		*irq = 0;
}
#endif

static int
k2_get_bus_speed(void)
{
	int bus_speed;
	unsigned char board_id;

	board_id = *(unsigned char *)K2_BOARD_ID_REG;

	switch( K2_BUS_SPD(board_id) ) {

		case 0:
		default:
			bus_speed = 100000000;
			break;

		case 1:
			bus_speed = 83333333;
			break;

		case 2:
			bus_speed = 75000000;
			break;

		case 3:
			bus_speed = 66666666;
			break;
	}
	return bus_speed;
}

static int
k2_get_cpu_speed(void)
{
	unsigned long hid1;
	int cpu_speed;

	hid1 = mfspr(HID1) >> 28;

	if ((mfspr(PVR) >> 16) == 8)
		hid1 = cpu_7xx[hid1];
	else
		hid1 = cpu_6xx[hid1];

	cpu_speed = k2_get_bus_speed()*hid1/2;
	return cpu_speed;
}

static void __init
k2_calibrate_decr(void)
{
	int freq, divisor = 4;

	/* determine processor bus speed */
	freq = k2_get_bus_speed();
	tb_ticks_per_jiffy = freq / HZ / divisor;
	tb_to_us = mulhwu_scale_factor(freq/divisor, 1000000);
}

static int
k2_show_cpuinfo(struct seq_file *m)
{
	unsigned char k2_geo_bits, k2_system_slot;

	seq_printf(m, "vendor\t\t: SBS\n");
	seq_printf(m, "machine\t\t: K2\n");
	seq_printf(m, "cpu speed\t: %dMhz\n", k2_get_cpu_speed()/1000000);
	seq_printf(m, "bus speed\t: %dMhz\n", k2_get_bus_speed()/1000000);
	seq_printf(m, "memory type\t: SDRAM\n");

	k2_geo_bits = readb(K2_MSIZ_GEO_REG) & K2_GEO_ADR_MASK;
	k2_system_slot = !(readb(K2_MISC_REG) & K2_SYS_SLOT_MASK);
	seq_printf(m, "backplane\t: %s slot board", 
		k2_system_slot ? "System" : "Non system");
	seq_printf(m, "with geographical address %x\n",	k2_geo_bits);

	return 0;
}

extern char cmd_line[];

TODC_ALLOC();

static void __init
k2_setup_arch(void)
{
	unsigned int cpu;

	/* Setup TODC access */
	TODC_INIT(TODC_TYPE_MK48T37, 0, 0,
		  ioremap(K2_RTC_BASE_ADDRESS, K2_RTC_SIZE),
		  8);

	/* init to some ~sane value until calibrate_delay() runs */
        loops_per_jiffy = 50000000/HZ;

	/* make FLASH transactions higher priority than PCI to avoid deadlock */
	__raw_writel(__raw_readl(SIOC1) | 0x80000000, SIOC1);

	/* Set hardware to access FLASH page 2*/
	__raw_writel(1<<29, GPOUT);

	/* Setup PCI host bridges */
        k2_setup_hoses();

#ifdef CONFIG_BLK_DEV_INITRD
	if (initrd_start)
		ROOT_DEV = MKDEV(RAMDISK_MAJOR, 0); /* /dev/ram */
	else
#endif
#ifdef CONFIG_ROOT_NFS
		ROOT_DEV = to_kdev_t(0x00FF);	/* /dev/nfs pseudo device */
#else
		ROOT_DEV = to_kdev_t(0x1601);	/* /dev/hdc1 */
#endif

#ifdef CONFIG_DUMMY_CONSOLE
	conswitchp = &dummy_con;
#endif

	/* Identify the system */
	printk("System Identification: SBS K2 - PowerPC 750 @ %d Mhz\n", k2_get_cpu_speed()/1000000);
	printk("SBS K2 port (C) 2001 MontaVista Software, Inc. (source@mvista.com)\n");

	/* Identify the CPU manufacturer */
	cpu = PVR_REV(mfspr(PVR));
	printk("CPU manufacturer: %s [rev=%04x]\n", (cpu & (1<<15)) ? "IBM" :
	       "Motorola", cpu);
}

static void
k2_restart(char *cmd)
{
	__cli();

	/* Flip FLASH back to page 1 to access firmware image */
	__raw_writel(0, GPOUT);

	mtspr(SRR0, 0xfff00100);
	mtspr(SRR1, 0);
	__asm__ __volatile__ ("rfi\n\t");
	for(;;);
}

static void
k2_power_off(void)
{
	for(;;);
}

static void
k2_halt(void)
{
	k2_restart(NULL);
}

/*
 * Set BAT 3 to map PCI32 I/O space.
 */
static __inline__ void
k2_set_bat(void)
{
	/* wait for all outstanding memory accesses to complete */
	mb();

	/* setup DBATs */
	mtspr(DBAT2U, 0x80001ffe);
	mtspr(DBAT2L, 0x8000002a);
	mtspr(DBAT3U, 0xf0001ffe);
	mtspr(DBAT3L, 0xf000002a);

	/* wait for updates */
	mb();

	return;
}

static unsigned long __init
k2_find_end_of_memory(void)
{
	unsigned long total;
	unsigned char msize = 7;        /* Default to 128MB */

	msize = K2_MEM_SIZE(readb(K2_MSIZ_GEO_REG));

	switch (msize)
	{
		case 2:
			/*
			 * This will break without a lowered
			 * KERNELBASE or CONFIG_HIGHMEM on.
			 * It seems non 1GB builds exist yet,
			 * though.
			 */
			total = K2_MEM_SIZE_1GB;
			break;
		case 3:
		case 4:
			total = K2_MEM_SIZE_512MB;
			break;
		case 5:
		case 6:
			total = K2_MEM_SIZE_256MB;
			break;
		case 7:
			total = K2_MEM_SIZE_128MB;
			break;
		default:
			printk("K2: Invalid memory size detected, defaulting to 128MB\n");
				total = K2_MEM_SIZE_128MB;
			break;
	}
	return total;
}

static void __init
k2_map_io(void)
{
	io_block_mapping(K2_PCI32_IO_BASE,
			K2_PCI32_IO_BASE,
			0x00200000,
			_PAGE_IO);
	io_block_mapping(0xff000000,
			0xff000000,
			0x01000000,
			_PAGE_IO);
}

static void __init
k2_init_irq(void)
{
	int i;

	for (i = 0; i < 16; i++)
		irq_desc[i].handler = &i8259_pic;

	i8259_init(0);
}

#if defined(CONFIG_SERIAL) && defined(CONFIG_SERIAL_TEXT_DEBUG)
extern struct serial_state rs_table[];

static void __init
k2_early_serial_map(void)
{
	struct serial_struct serial_req;

	/* Setup serial port access */
	memset(&serial_req, 0, sizeof(serial_req));

	/*
	 * rs_table[] already set up by <asm/pc_serial.h> so use that info for
	 * gen550_init().  This also means early_serial_setup() doesn't
	 * have to be called.
	 */
	serial_req.io_type = rs_table[0].io_type;
	serial_req.port = rs_table[0].port;
	serial_req.iomem_base = rs_table[0].iomem_base;
	serial_req.iomem_reg_shift = rs_table[0].iomem_reg_shift;
#if defined(CONFIG_SERIAL_TEXT_DEBUG) || defined(CONFIG_KGDB)
	gen550_init(0, &serial_req);
#endif
}
#endif

void __init platform_init(unsigned long r3, unsigned long r4,
		unsigned long r5, unsigned long r6, unsigned long r7)
{
	parse_bootinfo((struct bi_record *) (r3 + KERNELBASE));

	isa_io_base = K2_ISA_IO_BASE;
	isa_mem_base = K2_ISA_MEM_BASE;
	pci_dram_offset = K2_PCI32_SYS_MEM_BASE;

	k2_set_bat();

	ppc_md.setup_arch = k2_setup_arch;
	ppc_md.show_cpuinfo = k2_show_cpuinfo;
	ppc_md.init_IRQ = k2_init_irq;
	ppc_md.get_irq = i8259_irq;

	ppc_md.find_end_of_memory = k2_find_end_of_memory;
	ppc_md.setup_io_mappings = k2_map_io;

	ppc_md.restart = k2_restart;
	ppc_md.power_off = k2_power_off;
	ppc_md.halt = k2_halt;

	ppc_md.time_init = todc_time_init;
	ppc_md.set_rtc_time = todc_set_rtc_time;
	ppc_md.get_rtc_time = todc_get_rtc_time;
	ppc_md.calibrate_decr = k2_calibrate_decr;

	ppc_md.nvram_read_val = todc_direct_read_val;
	ppc_md.nvram_write_val = todc_direct_write_val;

#if defined(CONFIG_BLK_DEV_IDE) || defined(CONFIG_BLK_DEV_IDE_MODULE)
	ppc_ide_md.ide_init_hwif = k2_ide_init_hwif_ports;
#endif

#if defined(CONFIG_SERIAL) && defined(CONFIG_SERIAL_TEXT_DEBUG)
	k2_early_serial_map();
	ppc_md.progress = gen550_progress;
#endif
}

