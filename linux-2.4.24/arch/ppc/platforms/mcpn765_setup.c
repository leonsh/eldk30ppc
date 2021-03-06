/*
 * arch/ppc/platforms/mcpn765_setup.c
 *
 * Board setup routines for the Motorola MCG MCPN765 cPCI Board.
 *
 * Author: Mark A. Greer
 *         mgreer@mvista.com
 *
 * 2001-2003 (c) MontaVista Software, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

/*
 * This file adds support for the Motorola MCG MCPN765.
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
#include <linux/ide.h>
#include <linux/irq.h>
#include <linux/seq_file.h>
#include <linux/serial.h>
#include <linux/serialP.h>

#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/time.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/machdep.h>
#include <asm/prom.h>
#include <asm/smp.h>
#include <asm/open_pic.h>
#include <asm/i8259.h>
#include <asm/todc.h>
#include <asm/pci-bridge.h>
#include <asm/bootinfo.h>
#include <asm/serial.h>
#include <asm/pplus.h>

#include "mcpn765.h"
#include "mcpn765_serial.h"

static u_char mcpn765_openpic_initsenses[] __initdata = {
	(IRQ_SENSE_EDGE  | IRQ_POLARITY_POSITIVE),/* 16: i8259 cascade */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),/* 17: COM1,2,3,4 */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),/* 18: Enet 1 (front) */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),/* 19: HAWK WDT XXXX */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),/* 20: 21554 bridge */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),/* 21: cPCI INTA# */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),/* 22: cPCI INTB# */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),/* 23: cPCI INTC# */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),/* 24: cPCI INTD# */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),/* 25: PMC1 INTA#,PMC2 INTB#*/
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),/* 26: PMC1 INTB#,PMC2 INTC#*/
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),/* 27: PMC1 INTC#,PMC2 INTD#*/
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),/* 28: PMC1 INTD#,PMC2 INTA#*/
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),/* 29: Enet 2 (J3) */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),/* 30: Abort Switch */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),/* 31: RTC Alarm */
};

extern void mcpn765_set_VIA_IDE_native(void);

extern u_int openpic_irq(void);
extern char cmd_line[];

extern void gen550_progress(char *, unsigned short);
extern void gen550_init(int, struct serial_struct *);

int use_of_interrupt_tree = 0;

static void mcpn765_halt(void);

TODC_ALLOC();

#ifdef CONFIG_SERIAL
static void __init
mcpn765_early_serial_map(void)
{
	struct serial_struct serial_req;

	/* Setup serial port access */
	memset(&serial_req, 0, sizeof(serial_req));
	serial_req.baud_base = BASE_BAUD;
	serial_req.line = 0;
	serial_req.port = 0;
	serial_req.irq = 17;
	serial_req.flags = STD_COM_FLAGS;
	serial_req.io_type = SERIAL_IO_MEM;
	serial_req.iomem_base = (u_char *)MCPN765_SERIAL_1;
	serial_req.iomem_reg_shift = 4;

#if defined(CONFIG_SERIAL_TEXT_DEBUG) || defined(CONFIG_KGDB)
	gen550_init(0, &serial_req);
#endif

	if (early_serial_setup(&serial_req) != 0)
		printk("Early serial init of port 0 failed\n");

	/* Assume early_serial_setup() doesn't modify serial_req */
	serial_req.line = 1;
	serial_req.port = 1;
	serial_req.irq = 17;
	serial_req.iomem_base = (u_char *)MCPN765_SERIAL_2;

#if defined(CONFIG_SERIAL_TEXT_DEBUG) || defined(CONFIG_KGDB)
	gen550_init(1, &serial_req);
#endif

	if (early_serial_setup(&serial_req) != 0)
		printk("Early serial init of port 1 failed\n");

	/* Assume early_serial_setup() doesn't modify serial_req */
	serial_req.line = 2;
	serial_req.port = 2;
	serial_req.irq = 17;
	serial_req.iomem_base = (u_char *)MCPN765_SERIAL_3;

#if defined(CONFIG_SERIAL_TEXT_DEBUG) || defined(CONFIG_KGDB)
	gen550_init(2, &serial_req);
#endif

	if (early_serial_setup(&serial_req) != 0)
		printk("Early serial init of port 2 failed\n");

	/* Assume early_serial_setup() doesn't modify serial_req */
	serial_req.line = 3;
	serial_req.port = 3;
	serial_req.irq = 17;
	serial_req.iomem_base = (u_char *)MCPN765_SERIAL_4;

#if defined(CONFIG_SERIAL_TEXT_DEBUG) || defined(CONFIG_KGDB)
	gen550_init(3, &serial_req);
#endif

	if (early_serial_setup(&serial_req) != 0)
		printk("Early serial init of port 3 failed\n");
}
#endif

static void __init
mcpn765_setup_arch(void)
{
	struct pci_controller *hose;

	if ( ppc_md.progress )
		ppc_md.progress("mcpn765_setup_arch: enter", 0);

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

#ifdef CONFIG_DUMMY_CONSOLE
	conswitchp = &dummy_con;
#endif

	if ( ppc_md.progress )
		ppc_md.progress("mcpn765_setup_arch: find_bridges", 0);

	/* Lookup PCI host bridges */
	mcpn765_find_bridges();

	hose = pci_bus_to_hose(0);
	isa_io_base = (ulong)hose->io_base_virt;

	TODC_INIT(TODC_TYPE_MK48T37,
		  (MCPN765_PHYS_NVRAM_AS0 - isa_io_base),
		  (MCPN765_PHYS_NVRAM_AS1 - isa_io_base),
		  (MCPN765_PHYS_NVRAM_DATA - isa_io_base),
		  8);

	OpenPIC_InitSenses = mcpn765_openpic_initsenses;
	OpenPIC_NumInitSenses = sizeof(mcpn765_openpic_initsenses);

	printk(KERN_INFO "Motorola MCG MCPN765 cPCI Non-System Board\n");
	printk(KERN_INFO "Port by MontaVista Software, Inc. (source@mvista.com)\n");

	if ( ppc_md.progress )
		ppc_md.progress("mcpn765_setup_arch: exit", 0);

	return;
}

/*
 * Initialize the VIA 82c586b.
 */
static void __init
mcpn765_setup_via_82c586b(void)
{
	struct pci_dev	*dev;
	u_char		c;

	if ((dev = pci_find_device(PCI_VENDOR_ID_VIA,
				   PCI_DEVICE_ID_VIA_82C586_0,
				   NULL)) == NULL) {
		printk("No VIA ISA bridge found\n");
		mcpn765_halt();
		/* NOTREACHED */
	}

	/*
	 * If the firmware left the EISA 4d0/4d1 ports enabled, make sure
	 * IRQ 14 is set for edge.
	 */
	pci_read_config_byte(dev, 0x47, &c);

	if (c & (1<<5)) {
		c = inb(0x4d1);
		c &= ~(1<<6);
		outb(c, 0x4d1);
	}

	/* Disable PNP IRQ routing since we use the Hawk's MPIC */
	pci_write_config_dword(dev, 0x54, 0);
	pci_write_config_byte(dev, 0x58, 0);

	if ((dev = pci_find_device(PCI_VENDOR_ID_VIA,
				   PCI_DEVICE_ID_VIA_82C586_1,
				   NULL)) == NULL) {
		printk(KERN_ERR "No VIA ISA bridge found\n");
		mcpn765_halt();
		/* NOTREACHED */
	}

	/*
	 * PPCBug doesn't set the enable bits for the IDE device.
	 * Turn them on now.
	 */
	pcibios_read_config_byte(dev->bus->number, dev->devfn, 0x40, &c);
	c |= 0x03;
	pcibios_write_config_byte(dev->bus->number, dev->devfn, 0x40, c);

	return;
}

static void __init
mcpn765_init2(void)
{
	/* Do MCPN765 board specific initialization.  */
	mcpn765_setup_via_82c586b();

	request_region(0x00,0x20,"dma1");
	request_region(0x20,0x20,"pic1");
	request_region(0x40,0x20,"timer");
	request_region(0x80,0x10,"dma page reg");
	request_region(0xa0,0x20,"pic2");
	request_region(0xc0,0x20,"dma2");

	return;
}

/*
 * Interrupt setup and service.
 * Have MPIC on HAWK and cascaded 8259s on VIA 82586 cascaded to MPIC.
 */
static void __init
mcpn765_init_IRQ(void)
{
	int i;

	if ( ppc_md.progress )
		ppc_md.progress("init_irq: enter", 0);

	openpic_init(NUM_8259_INTERRUPTS);
	openpic_hookup_cascade(NUM_8259_INTERRUPTS, "82c59 cascade",
			&i8259_irq);

	for(i=0; i < NUM_8259_INTERRUPTS; i++)
		irq_desc[i].handler = &i8259_pic;

	i8259_init(0);

	if ( ppc_md.progress )
		ppc_md.progress("init_irq: exit", 0);
}

static u32
mcpn765_irq_cannonicalize(u32 irq)
{
	if (irq == 2)
		return 9;
	else
		return irq;
}

static unsigned long __init
mcpn765_find_end_of_memory(void)
{
	return pplus_get_mem_size(MCPN765_HAWK_SMC_BASE);
}

static void __init
mcpn765_map_io(void)
{
	io_block_mapping(0xfe800000, 0xfe800000, 0x00800000, _PAGE_IO);
}

static void
mcpn765_reset_board(void)
{
	__cli();

	/* set VIA IDE controller into native mode */
	mcpn765_set_VIA_IDE_native();

	/* Set exception prefix high - to the firmware */
	_nmask_and_or_msr(0, MSR_IP);

	out_8((u_char *)MCPN765_BOARD_MODRST_REG, 0x01);

	return;
}

static void
mcpn765_restart(char *cmd)
{
	volatile ulong	i = 10000000;

	mcpn765_reset_board();

	while (i-- > 0);
	panic("restart failed\n");
}

static void
mcpn765_power_off(void)
{
	mcpn765_halt();
	/* NOTREACHED */
}

static void
mcpn765_halt(void)
{
	__cli();
	while (1);
	/* NOTREACHED */
}

static int
mcpn765_show_cpuinfo(struct seq_file *m)
{
	seq_printf(m, "vendor\t\t: Motorola MCG\n");
	seq_printf(m, "machine\t\t: MCPN765\n");

	return 0;
}

/*
 * Set BAT 3 to map 0xf0000000 to end of physical memory space.
 */
static __inline__ void
mcpn765_set_bat(void)
{
	mb();
	mtspr(DBAT1U, 0xfe8000fe);
	mtspr(DBAT1L, 0xfe80002a);
	mb();
}

void __init
platform_init(unsigned long r3, unsigned long r4, unsigned long r5,
		unsigned long r6, unsigned long r7)
{
	parse_bootinfo(find_bootinfo());

	/* Map in board regs, etc. */
	mcpn765_set_bat();

	isa_mem_base = MCPN765_ISA_MEM_BASE;
	pci_dram_offset = MCPN765_PCI_DRAM_OFFSET;
	ISA_DMA_THRESHOLD = 0x00ffffff;
	DMA_MODE_READ = 0x44;
	DMA_MODE_WRITE = 0x48;

	ppc_md.setup_arch = mcpn765_setup_arch;
	ppc_md.show_cpuinfo = mcpn765_show_cpuinfo;
	ppc_md.irq_cannonicalize = mcpn765_irq_cannonicalize;
	ppc_md.init_IRQ = mcpn765_init_IRQ;
	ppc_md.get_irq = openpic_get_irq;
	ppc_md.init = mcpn765_init2;

	ppc_md.restart = mcpn765_restart;
	ppc_md.power_off = mcpn765_power_off;
	ppc_md.halt = mcpn765_halt;

	ppc_md.find_end_of_memory = mcpn765_find_end_of_memory;
	ppc_md.setup_io_mappings = mcpn765_map_io;

	ppc_md.time_init = todc_time_init;
	ppc_md.set_rtc_time = todc_set_rtc_time;
	ppc_md.get_rtc_time = todc_get_rtc_time;
	ppc_md.calibrate_decr = todc_calibrate_decr;

	ppc_md.nvram_read_val = todc_m48txx_read_val;
	ppc_md.nvram_write_val = todc_m48txx_write_val;

#if defined(CONFIG_SERIAL) && \
	(defined(CONFIG_SERIAL_TEXT_DEBUG) || defined(CONFIG_KGDB))
	mcpn765_early_serial_map();
#ifdef CONFIG_SERIAL_TEXT_DEBUG
	ppc_md.progress = gen550_progress;
#endif
#endif
}
