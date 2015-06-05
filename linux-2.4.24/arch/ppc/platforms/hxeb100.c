/*
 * arch/ppc/platforms/hxeb100.c
 *
 * Board setup routines for the Motorola Computer Group HXEB100 Board.
 *
 * Author: Dale Farnsworth <Dale.Farnsworth@mvista.com>
 *
 * Copyright 2002-2003 MontaVista Software Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

/*
 * The HXEB100 port is the result of hard work from many people from
 * many companies.  In particular, employees of Marvell/Galileo, Mission
 * Critical Linux, Xyterra, and MontaVista Software were heavily involved.
 */

#include <linux/config.h>

#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/irq.h>

#include <linux/seq_file.h>
#include <linux/serial.h>

#include <asm/bootinfo.h>
#include <asm/gt64260.h>
#include <asm/todc.h>
#include <asm/time.h>

#include <platforms/hxeb100.h>

#define BOARD_VENDOR	"Motorola Computer Group"
#define BOARD_MACHINE	"HXEB100"

/*
 * Local config options for workarounds and debugging
 */
#define SET_PCI_IDE_NATIVE
#define DISABLE_USB_INTERRUPTS
#define SET_PCI_COMMAND_INVALIDATE
#define HXEB100_PREFETCHEN	/* more stable with this on */
#undef HXEB100_DREADEN	/* fails with e1000 and ide stress */
#undef ENABLE_IO_READ_SYNC_BARRIER /* fails ide/e1000 stress test */
#undef ENABLE_CONFIG_READ_SYNC_BARRIER /* fails ide/e1000 stress test */

ulong	hxeb100_mem_size = 0;
bd_t	board_info;

extern void gen550_progress(char *, unsigned short);
extern void gen550_init(int, struct serial_struct *);

static const unsigned int cpu_745x[2][16] = { /* PLL_EXT 0 & 1 */
	{ 1, 15, 14,  2,  4, 13,  5,  9,  6, 11,  8, 10, 16, 12,  7,  0 },
	{ 0, 30,  0,  2,  0, 26,  0, 18,  0, 22, 20, 24, 28, 32,  0,  0 }
};

TODC_ALLOC();

static void __init
hxeb100_board_info(void *_bi, int size)
{
	bd_t *bi = (bd_t *) _bi;

	if (size == sizeof(*bi) && bi->bi_hxeb == HXEB100_BOARD_INFO_HXEB)
		memcpy(&board_info, bi, sizeof(*bi));
	else
		printk("Invalid HXEB100 VPD\n");
}

static int
hxeb100_get_bus_speed(void)
{
	/* sanity check the bus freq from the vpd */
	if (board_info.bi_busfreq > 60000000
	    && board_info.bi_busfreq < 300000000)
		return board_info.bi_busfreq;
	else
		return 133333333;	/* assume 133MHz if vpd is bad */
}

static int
hxeb100_get_cpu_speed(void)
{
	unsigned long	hid1, pll_ext;

	hid1 = (mfspr(HID1) & 0x0001e000) >> 13;
	pll_ext = 0; /* No way to read; must get from schematic */
	return hxeb100_get_bus_speed() * cpu_745x[pll_ext][hid1]/2;
}

unsigned long __init
hxeb100_find_end_of_memory(void)
{
	return gt64260_get_mem_size();
}

/*
 * Motorola Computer Group HXEB100 PCI interrupt routing.
 */
static int __init
hxeb100_map_irq(struct pci_dev *dev, unsigned char idsel, unsigned char pin)
{
	struct pci_controller	*hose = pci_bus_to_hose(dev->bus->number);

	if (hose->index == 0) {
		static char pci_irq_table[][4] =
		/*
		 *	PCI IDSEL/INTPIN->INTLINE 
		 * 	   A   B   C   D
		 */
		{
			{ 80, 81, 82, 83 },	/* IDSEL 2 - PCI bus 0.0 */
			{  0,  0,  0,  0 },	/* IDSEL 3 */
			{  0,  0,  0,  0 },	/* IDSEL 4 */
			{  0,  0,  0,  0 },	/* IDSEL 5 */
			{ 76, 77, 78, 79 },	/* IDSEL 6 - PCI bus 0.1 */
		};

		const long min_idsel = 2, max_idsel = 6, irqs_per_slot = 4;
		return PCI_IRQ_TABLE_LOOKUP;
	}
	else {
		static char pci_irq_table[][4] =
		/*
		 *	PCI IDSEL/INTPIN->INTLINE 
		 * 	   A   B   C   D
		 */
		{
			{ 84,  0,  0,  0 },	/* IDSEL 2 - 82544 */
			{  0,  0,  0,  0 },	/* IDSEL 3 */
			{ 86, 87,  0,  0 },	/* IDSEL 4 - 53C1010 */
			{  0,  0,  0,  0 },	/* IDSEL 5 */
			{ 72, 73, 74, 75 },	/* IDSEL 6 - PCI bus 1.1 */
		};

		const long min_idsel = 2, max_idsel = 6, irqs_per_slot = 4;
		return PCI_IRQ_TABLE_LOOKUP;
	}
}

static void __init
hxeb100_setup_peripherals(void)
{
	u8 *status_reg_1_addr;
	u8 *status_reg_2_addr;
	int status_reg_2;
	int bank_b_boot;
	unsigned boot_bank_base;
	unsigned boot_bank_size;
	unsigned cs0_bank_base;
	unsigned cs0_bank_size;

	/* Set up windows to device bus (status regs, nvram/rtc, uarts), CS 1 */
	gt64260_cpu_cs_set_window(1, HXEB100_DEVICE_BASE, HXEB100_DEVICE_SIZE);

	/* The flashes can swap places based on an on-board jumper */
	status_reg_1_addr = ioremap(HXEB100_STATUS_REG_1, 1);
	bank_b_boot = in_8(status_reg_1_addr) & HXEB100_BANK_SEL_MASK;
	iounmap(status_reg_1_addr);
	if (bank_b_boot) {
		boot_bank_base = HXEB100_BANK_B_FLASH_BASE;
		boot_bank_size = HXEB100_BANK_B_FLASH_SIZE;
		cs0_bank_base  = HXEB100_BANK_A_FLASH_BASE;
		cs0_bank_size  = HXEB100_BANK_A_FLASH_SIZE;
	}
	else {
		boot_bank_base = HXEB100_BANK_A_FLASH_BASE;
		boot_bank_size = HXEB100_BANK_A_FLASH_SIZE;
		cs0_bank_base  = HXEB100_BANK_B_FLASH_BASE;
		cs0_bank_size  = HXEB100_BANK_B_FLASH_SIZE;
	}
	/* set the two flash windows */
	gt64260_cpu_boot_set_window(boot_bank_base, boot_bank_size);
	gt64260_cpu_cs_set_window(0, cs0_bank_base, cs0_bank_size);

	/* disable unused windows */
	gt64260_cpu_cs_set_window(2,0,0);
	gt64260_cpu_cs_set_window(3,0,0);

	/* clear boardfail indicator and flash write protect */
	status_reg_2_addr = ioremap(HXEB100_STATUS_REG_2, 1);
	status_reg_2 = in_8(status_reg_2_addr);
	status_reg_2 &= ~(HXEB100_BD_FAIL_MASK | HXEB100_FLASH_WP_MASK);
	out_8(status_reg_2_addr, status_reg_2);
	iounmap(status_reg_2_addr);

	TODC_INIT(TODC_TYPE_MK48T37, 0, 0,
			ioremap(HXEB100_TODC_BASE, HXEB100_TODC_SIZE), 8);

#ifdef ENABLE_IO_READ_SYNC_BARRIER
	/* enable I/O Read Sync Barrier */
	gt_clr_bits(GT64260_CPU_CONFIG, (1<<29));
#endif

#ifdef ENABLE_CONFIG_READ_SYNC_BARRIER
	/* enable I/O Read Sync Barrier and  Config Read Sync barrier */
	gt_clr_bits(GT64260_CPU_CONFIG, (1<<28));
#endif

	/* set so writes to decode register do not affect remap register */
	gt_set_bits(GT64260_CPU_CONFIG, (1<<27));

	gt_set_bits(GT64260_PCI_0_SLAVE_PCI_DECODE_CNTL, ((1<<0) | (1<<3)));
	gt_set_bits(GT64260_PCI_1_SLAVE_PCI_DECODE_CNTL, ((1<<0) | (1<<3)));

	/* external pci arbiter */
	gt_clr_bits(GT64260_PCI_0_ARBITER_CNTL, (1<<31));
	gt_clr_bits(GT64260_PCI_1_ARBITER_CNTL, (1<<31));

	/* MPP Control 0
	 * MPP funct   MPP programming Usage
	 * 0  GPP[0]   MPPCTL0[3:0]   SERIAL_0/SERIAL_2 interrupt
	 * 1  GPP[1]   MPPCTL0[7:4]   SERIAL_1/SERIAL_3 interrupt
	 * 2  GPP[2]   MPPCTL0[11:8]  Abort switch
	 * 3  GPP[3]   MPPCTL0[15:12] RTC/Thermostat
	 * 4  GPP[4]   MPPCTL0[19:16] Cross-processor 0->1 output
	 * 5  GPP[5]   MPPCTL0[23:20] Cross-processor 0->1 input
	 * 6  GPP[6]   MPPCTL0[27:24] Watchdog timer WDNMI
	 * 7  GPP[7]   MPPCTL0[31:28] LXT971A 0 MDINT/LXT971A 1 MDINT
	*/ 
	gt_write(GT64260_MPP_CNTL_0,(0x00000000));

	/* MPP Control 1
	 * MPP  funct  MPP programming
	 * 8   GPP[8]  MPPCTL1[3:0]   PCI 1.1 INT0
	 * 9   GPP[9]  MPPCTL1[7:4]   PCI 1.1 INT1
	 * 10  GPP[10] MPPCTL1[11:8]  PCI 1.1 INT2
	 * 11  GPP[11] MPPCTL1[15:12] PCI 1.1 INT3
	 * 12  GPP[12] MPPCTL1[19:16] PCI 0.1 INT0
	 * 13  GPP[13] MPPCTL1[23:20] PCI 0.1 INT1
	 * 14  GPP[14] MPPCTL1[27:24] PCI 0.1 INT2
	 * 15  GPP[15] MPPCTL1[31:28] PCI 0.1 INT3
	 */
	gt_write(GT64260_MPP_CNTL_1,(0x00000000));

	/* MPP Control 2
	 * MPP funct   MPP programming Usage
	 * 16  GPP[16] MPPCTL2[3:0]   PCI 0 INT0
	 * 17  GPP[17] MPPCTL2[7:4]   PCI 0 INT1
	 * 18  GPP[18] MPPCTL2[11:8]  PCI 0 INT2
	 * 19  GPP[19] MPPCTL2[15:12] PCI 0 INT3
	 * 20  GPP[20] MPPCTL2[19:16] 82544 INTA
	 * 21  GPP[21] MPPCTL2[23:20] unused
	 * 22  GPP[22] MPPCTL2[27:24] 53C1010 INTA
	 * 23  GPP[23] MPPCTL2[31:28] 53C1010 INTB
	 */
	gt_write(GT64260_MPP_CNTL_2,0x00000000);

	/* MPP Control 3
	 * MPP funct   MPP programming Usage
	 * 24  GPP[24] MPPCTL3[3:0]   Watch dog timer WDNMI ***OUTPUT***
	 * 25  GPP[25] MPPCTL3[7:4]   Watch dog timer WDE ***OUTPUT***
	 * 26  GPP[26] MPPCTL3[11:8]  SROM InitAct ***OUTPUT***
	 * 27  GPP[27] MPPCTL3[15:12] Cross-processor  input to CPU0
	 * 28  GPP[28] MPPCTL3[19:16] Cross-processor OUTPUT to CPU0
	 * 29  GPP[29] MPPCTL3[23:20] Optional external PPC bus arbiter BG1 OUT
	 * 30  GPP[30] MPPCTL3[27:24] unused
	 * 31  GPP[31] MPPCTL3[31:28] unused
	 */
	/* do not enable watch dog timer interrupts for now (was 0x00000599) */
	gt_write(GT64260_MPP_CNTL_3,0x00000500);

	/* set interrupt active levels, then enable them */
	gt_write(GT64260_GPP_LEVEL_CNTL,0x08dfffec);

	/* Setup GPP outputs:
	 * MPP 4:  Cross-processor OUTPUT to CPU1
	 * MPP 26: SROM InitAct
	 * MPP 28: Cross-processor OUTPUT to CPU0
	 * MPP 29: Optional external PPC bus arbiter BG1 enable
	 */
	gt_write(GT64260_GPP_IO_CNTL, 0x34000010);
}

static void __init
hxeb100_setup_bridge(void)
{
	gt64260_bridge_info_t 	info;
	int			i;
	u16			val16;
	u8			save_exclude;

	GT64260_BRIDGE_INFO_DEFAULT(&info, hxeb100_find_end_of_memory());

	/*
	 * Assume that GT64260_CPU_SCS_DECODE_WINDOWS,
	 * GT64260_CPU_SNOOP_WINDOWS, GT64260_PCI_SCS_WINDOWS, and
	 * GT64260_PCI_SNOOP_WINDOWS are all the equal.
	 * Also assume that GT64260_CPU_PROT_WINDOWS >=
	 * GT64260_CPU_SCS_DECODE_WINDOWS and
	 * GT64260_PCI_ACC_CNTL_WINDOWS >= GT64260_PCI_SCS_WINDOWS.
	 */
	for (i=0; i<GT64260_CPU_SCS_DECODE_WINDOWS; i++) {
		info.cpu_prot_options[i] = 0;
		info.cpu_snoop_options[i] = GT64260_CPU_SNOOP_WB;

		info.pci_0_acc_cntl_options[i] = 
#ifdef HXEB100_PREFETCHEN
			GT64260_PCI_ACC_CNTL_PREFETCHEN |
#endif
#ifdef HXEB100_DREADEN
			GT64260_PCI_ACC_CNTL_DREADEN |
#endif
			GT64260_PCI_ACC_CNTL_SWAP_NONE |
			GT64260_PCI_ACC_CNTL_MBURST_4_WORDS;
		info.pci_0_snoop_options[i] = GT64260_PCI_SNOOP_WB;

		info.pci_1_acc_cntl_options[i] = 
#ifdef HXEB100_PREFETCHEN
			GT64260_PCI_ACC_CNTL_PREFETCHEN |
#endif
#ifdef HXEB100_DREADEN
			GT64260_PCI_ACC_CNTL_DREADEN |
#endif
			GT64260_PCI_ACC_CNTL_SWAP_NONE |
			GT64260_PCI_ACC_CNTL_MBURST_4_WORDS;
		info.pci_1_snoop_options[i] = GT64260_PCI_SNOOP_WB;
	}

	info.pci_0_latency = 0x8;
	info.pci_1_latency = 0x8;

	info.hose_a = pcibios_alloc_controller();
	if (!info.hose_a) {
		if (ppc_md.progress)
			ppc_md.progress("hxeb100_setup_bridge: hose_a failed", 0);
		printk("hxeb100_setup_bridge: can't set up first hose\n");
		return;
	}

	info.hose_b = pcibios_alloc_controller();
	if (!info.hose_b) {
		if (ppc_md.progress)
			ppc_md.progress("hxeb100_setup_bridge: hose_b failed", 0);
		printk("hxeb100_setup_bridge: can't set up second hose\n");
		return;
	}

	info.phys_base_addr = gt64260_base;

	/* Map in the bridge's registers */
	gt64260_base = (u32)ioremap(gt64260_base, GT64260_INTERNAL_SPACE_SIZE);

	/* Lookup PCI host bridges */
	if (gt64260_find_bridges(&info, hxeb100_map_irq)) {
		printk("Bridge initialization failed.\n");
		iounmap((void *)gt64260_base);
	}

	/*
	 * Dave Wilhardt found that bit 4 (PCI_COMMAND_INVALIDATE) in
	 * the PCI Command registers must be set if you are using
	 * cache coherency.
	 */
	save_exclude = gt64260_pci_exclude_bridge;
	gt64260_pci_exclude_bridge = FALSE;

	early_read_config_word(info.hose_a,
			info.hose_a->first_busno,
			PCI_DEVFN(0,0),
			PCI_COMMAND,
			&val16);
	val16 |= PCI_COMMAND_INVALIDATE;
	early_write_config_word(info.hose_a,
			info.hose_a->first_busno,
			PCI_DEVFN(0,0),
			PCI_COMMAND,
			val16);

	early_read_config_word(info.hose_b,
			info.hose_b->first_busno,
			PCI_DEVFN(0,0),
			PCI_COMMAND,
			&val16);
	val16 |= PCI_COMMAND_INVALIDATE;
	early_write_config_word(info.hose_b,
			info.hose_b->first_busno,
			PCI_DEVFN(0,0),
			PCI_COMMAND,
			val16);

	gt64260_pci_exclude_bridge = save_exclude;
	
	return;
}

static void __init
hxeb100_init_caches(int cpu_nr)
{
	volatile static	uint	cpu0_l3val;

	/* Enable L2 cache */
	_set_L2CR(L2CR_L2E);

	/*
	 * L3CR on cpu1 is not initialized by firmware,
	 * so we propagate the L3CR value from cpu0 to cpu1
	 */
	if (cpu_nr == 0)
		cpu0_l3val = _get_L3CR() | L3CR_L3E;

	/* Enable L3 cache */
	_set_L3CR(cpu0_l3val);
}

#ifdef	CONFIG_SERIAL
static void __init
hxeb100_early_serial_map(void)
{
	struct serial_struct	serial_req;
	static char		first_time = 1;

	if (first_time) {
		memset(&serial_req, 0, sizeof(serial_req));

		serial_req.line = 0;
		serial_req.baud_base = BASE_BAUD;
		serial_req.port = 0;
		serial_req.irq = HXEB100_UART_0_IRQ;
		serial_req.flags = STD_COM_FLAGS;
		serial_req.io_type = SERIAL_IO_MEM;
		serial_req.iomem_base = ioremap(HXEB100_SERIAL_0,
							HXEB100_UART_SIZE);
		serial_req.iomem_reg_shift = 0;
#if defined(CONFIG_SERIAL_TEXT_DEBUG) || defined(CONFIG_KGDB)
		gen550_init(0, &serial_req);
#endif

		if (early_serial_setup(&serial_req) != 0)
			printk("Early serial init of port 0 failed\n");

		/* Assume early_serial_setup() doesn't modify serial_req */
		serial_req.line = 1;
		serial_req.port = 1;
		serial_req.irq = HXEB100_UART_1_IRQ;
		serial_req.iomem_base = ioremap(HXEB100_SERIAL_1,
							HXEB100_UART_SIZE);
#if defined(CONFIG_SERIAL_TEXT_DEBUG) || defined(CONFIG_KGDB)
		gen550_init(1, &serial_req);
#endif

		if (early_serial_setup(&serial_req) != 0)
			printk("Early serial init of port 1 failed\n");

		first_time = 0;
	}
}
#endif

static void __init
hxeb100_setup_arch(void)
{
	if ( ppc_md.progress )
		ppc_md.progress("hxeb100_setup_arch: enter", 0);

	loops_per_jiffy = hxeb100_get_cpu_speed() / HZ;

#ifdef CONFIG_BLK_DEV_INITRD
	if (initrd_start)
		ROOT_DEV = MKDEV(RAMDISK_MAJOR, 0);
	else
#endif
#ifdef	CONFIG_ROOT_NFS
		ROOT_DEV = to_kdev_t(0x00FF);	/* /dev/nfs pseudo device */
#else
		ROOT_DEV = to_kdev_t(0x1602);	/* /dev/hdc2 IDE disk */
#endif

	if ( ppc_md.progress )
		ppc_md.progress("hxeb100_setup_arch: Enabling L2 cache", 0);

	hxeb100_init_caches(0);

	if ( ppc_md.progress )
		ppc_md.progress("hxeb100_setup_arch: Initializing bridge", 0);

	hxeb100_setup_bridge();		/* set up PCI bridge(s) */
	hxeb100_setup_peripherals();	/* set up chip selects/GPP/MPP etc */

	if ( ppc_md.progress )
		ppc_md.progress("hxeb100_setup_arch: bridge init complete", 0);

#ifdef	CONFIG_DUMMY_CONSOLE
	conswitchp = &dummy_con;
#endif

#ifdef	CONFIG_SERIAL
	hxeb100_early_serial_map();
#endif

	gt64260_set_mac_addr(0, board_info.bi_enetaddr[0]);
	gt64260_set_mac_addr(1, board_info.bi_enetaddr[1]);

	printk(BOARD_VENDOR " " BOARD_MACHINE "\n");
	printk("HXEB100 port (C) 2002 MontaVista Software, Inc. (source@mvista.com)\n");

	if ( ppc_md.progress )
		ppc_md.progress("hxeb100_setup_arch: exit", 0);

	return;
}

static void
hxeb100_restart(char *cmd)
{
	ulong	i = 10000000;
	u8 *status_reg_3_addr;

	__cli();
	status_reg_3_addr = ioremap(HXEB100_STATUS_REG_3, 1);
	out_8(status_reg_3_addr, HXEB100_BOARD_RESET_MASK);

	while (i-- > 0);
	panic("restart failed\n");
}

static void
hxeb100_halt(void)
{
	__cli();
	while (1);
	/* NOTREACHED */
}

static void
hxeb100_power_off(void)
{
	hxeb100_halt();
	/* NOTREACHED */
}

static int
hxeb100_show_cpuinfo(struct seq_file *m)
{
	uint pvid;

	pvid = mfspr(PVR);
	seq_printf(m, "vendor\t\t: " BOARD_VENDOR "\n");
	seq_printf(m, "machine\t\t: " BOARD_MACHINE "\n");
	seq_printf(m, "cpu MHz\t\t: %d\n", hxeb100_get_cpu_speed()/1000/1000);
	seq_printf(m, "bus MHz\t\t: %d\n", hxeb100_get_bus_speed()/1000/1000);

	return 0;
}

static void __init
hxeb100_calibrate_decr(void)
{
	ulong freq;

	freq = hxeb100_get_bus_speed()/4;

	printk("time_init: decrementer frequency = %lu.%.6lu MHz\n",
		freq/1000000, freq%1000000);

	tb_ticks_per_jiffy = freq / HZ;
	tb_to_us = mulhwu_scale_factor(freq, 1000000);

	return;
}

#ifdef	CONFIG_SERIAL
#if defined(CONFIG_SERIAL_TEXT_DEBUG) || defined(CONFIG_KGDB)
static void __init
hxeb100_map_io(void)
{
	io_block_mapping(0xf0000000, 0xf0000000, 0x10000000, _PAGE_IO);
}
#endif
#endif

/*
 * Set BAT 3 to map 0xf0000000 to end of physical memory space.
 */
static __inline__ void
hxeb100_set_bat(void)
{
	mb();
	mtspr(DBAT1U, 0xf0001ffe);
	mtspr(DBAT1L, 0xf000002a);
	mb();

	return;
}

#ifdef DISABLE_USB_INTERRUPTS
/* The MOTLoad firmware on the HXEB100 leaves USB interrupts enabled.
 * This workaround finds the USB devices and disables their interrupts.
 */
/* copied from drivers/usb/usb-ohci.h */
#define MAX_ROOT_PORTS	15		/* maximum OHCI root hub ports */
#define OHCI_INTR_MIE	(1 << 31)	/* master interrupt enable */
/*
 * This is the structure of the OHCI controller's memory mapped I/O
 * region.  This is Memory Mapped I/O.  You must use the readl() and
 * writel() macros defined in asm/io.h to access these!!
 */
struct ohci_regs {
	/* control and status registers */
	__u32	revision;
	__u32	control;
	__u32	cmdstatus;
	__u32	intrstatus;
	__u32	intrenable;
	__u32	intrdisable;
	/* memory pointers */
	__u32	hcca;
	__u32	ed_periodcurrent;
	__u32	ed_controlhead;
	__u32	ed_controlcurrent;
	__u32	ed_bulkhead;
	__u32	ed_bulkcurrent;
	__u32	donehead;
	/* frame counters */
	__u32	fminterval;
	__u32	fmremaining;
	__u32	fmnumber;
	__u32	periodicstart;
	__u32	lsthresh;
	/* Root hub ports */
	struct	ohci_roothub_regs {
		__u32	a;
		__u32	b;
		__u32	status;
		__u32	portstatus[MAX_ROOT_PORTS];
	} roothub;
} __attribute((aligned(32)));
#endif

#ifdef SET_PCI_IDE_NATIVE
static void __init
set_pci_native_mode(void)
{
	struct pci_dev *dev;

	/* Better way of doing this ??? */
	pci_for_each_dev(dev) {	
		int class = dev->class >> 8;

		/* enable pci native mode */
		if (class == PCI_CLASS_STORAGE_IDE) {
			u8 reg;

			pci_read_config_byte(dev, 0x9, &reg);
			if (reg == 0x8a) {
				printk("PCI: Enabling PCI IDE native mode on %s\n", dev->slot_name); 
				pci_write_config_byte(dev, 0x9,  0x8f);

				/* let the pci code set this device up after we change it */
				pci_setup_device(dev); 
			} else if (reg != 0x8f) {
				printk("PCI: IDE chip in unknown mode 0x%02x on %s", reg, dev->slot_name);
			}
		}
	}
}
#endif

#ifdef DISABLE_USB_INTERRUPTS
/* The MOTLoad firmware on the HXEB100 leaves USB interrupts enabled.
 * This workaround finds the USB devices and disables their interrupts.
 */
static void __init
disable_usb_interrupts(void)
{
	struct pci_dev *dev;
	unsigned long mem_resource, mem_len;
	void *mem_base;
	struct ohci_regs *regs;

	pci_for_each_dev(dev) {	
		int class = dev->class >> 8;

		if (class == PCI_CLASS_SERIAL_USB
				&& dev->vendor == PCI_VENDOR_ID_NEC
				&& dev->device == 0x0035) {
			if (pci_enable_device(dev) < 0)
				continue;

			mem_resource = pci_resource_start(dev, 0);
			mem_len = pci_resource_len(dev, 0);
			if (!request_mem_region(mem_resource, mem_len, "usb")) {
				pci_disable_device(dev);
				continue;
			}
			mem_base = ioremap_nocache(mem_resource, mem_len);
			if (!mem_base) {
				release_mem_region(mem_resource, mem_len);
				pci_disable_device(dev);
				continue;
			}
			regs = mem_base;
			if (readl(&regs->intrenable)) {
				printk("Disabling interrupts on USB device %d.%d.%d\n",
					dev->bus->number, dev->devfn >> 3,
					dev->devfn & 7);
				writel(~0, &regs->intrdisable);
			}
			iounmap(mem_base);
			release_mem_region(mem_resource, mem_len);
			pci_disable_device(dev);
		}
	}
}
#endif

#ifdef SET_PCI_COMMAND_INVALIDATE
/*
 * Dave Wilhardt found that PCI_COMMAND_INVALIDATE must
 * be set for each device if you are using cache coherency.
 */
static void __init
set_pci_command_invalidate(void)
{
	struct pci_dev *dev;
	u16 val;

	pci_for_each_dev(dev) {	
		pci_read_config_word(dev, PCI_COMMAND, &val);
		val |= PCI_COMMAND_INVALIDATE;
		pci_write_config_word(dev, PCI_COMMAND, val);
	}
}
#endif

static void __init
hxeb100_pci_fixups(void)
{
#ifdef SET_PCI_IDE_NATIVE
	set_pci_native_mode();
#endif
#ifdef DISABLE_USB_INTERRUPTS
	disable_usb_interrupts();
#endif
#ifdef SET_PCI_COMMAND_INVALIDATE
	set_pci_command_invalidate();
#endif
}

#ifdef CONFIG_SMP
#include <asm/smp.h>
struct smp_ops_t hxeb100_smp_ops;

/*
 * Set and clear IPIs for Motorola HXEB100
 */
static inline void hxeb100_set_ipi(int cpu)
{
	int ipi_out_gpp_bit;

	if (cpu == 0)
		ipi_out_gpp_bit = 1 << HXEB100_CPU0_IPI_OUT;
	else if (cpu == 1)
		ipi_out_gpp_bit = 1 << HXEB100_CPU1_IPI_OUT;
	else
		panic("hxeb100_set_ipi called with invalid cpu #\n");

	gt_clr_bits(GT64260_GPP_VALUE, ipi_out_gpp_bit);
}

static inline void hxeb100_clr_ipi(int cpu)
{
	int ipi_out_gpp_bit;
	int ipi_in_gpp_bit;

	if (cpu == 0) {
		ipi_out_gpp_bit = 1 << HXEB100_CPU0_IPI_OUT;
		ipi_in_gpp_bit = 1 << HXEB100_CPU0_IPI_IN;
	}
	else if (cpu == 1) {
		ipi_out_gpp_bit = 1 << HXEB100_CPU1_IPI_OUT;
		ipi_in_gpp_bit = 1 << HXEB100_CPU1_IPI_IN;
	}
	else {
		panic("hxeb100_clr_ipi called with invalid cpu #\n");
	}

	gt_set_bits(GT64260_GPP_VALUE, ipi_out_gpp_bit);
	/* FEr MISC-4 */
	gt_write(GT64260_GPP_INTR_CAUSE, ~ipi_in_gpp_bit);
}

/*
 * We don't have separate IPIs for separate messages like openpic does.
 * Instead we have a bitmap for each processor, where a 1 bit means that
 * the corresponding message is pending for that processor.
 * Ideally each cpu's entry would be in a different cache line.
 *  -- paulus.
 */
static unsigned long hxeb100_smp_message[NR_CPUS];

int cpu0 = 0;
int cpu1 = 1;

void
hxeb100_primary_intr(int irq, void *pcpu, struct pt_regs *regs)
{
	int cpu = smp_processor_id();
	int targeted_cpu = *(int *)pcpu;
	int msg;
	
	if (smp_num_cpus < 2)
		return;

	if (cpu != targeted_cpu)
		panic("IPI for cpu %d received by cpu %d\n", targeted_cpu, cpu);

	/* clear interrupt */
	hxeb100_clr_ipi(cpu);
	barrier();

	/* make sure there is a message there */
	for (msg = 0; msg < 4; msg++){
		if (test_and_clear_bit(msg, &hxeb100_smp_message[cpu])){
			smp_message_recv(msg, regs);
		}
	}
}

static void
smp_hxeb100_message_pass(int target, int msg, unsigned long data, int wait)
{
	int i;

	if (smp_num_cpus < 2)
		return;

	for (i = 0; i < smp_num_cpus; i++) {
		if (target == MSG_ALL
		    || (target == MSG_ALL_BUT_SELF && i != smp_processor_id())
		    || target == i) {
			set_bit(msg, &hxeb100_smp_message[i]);
			barrier();
			hxeb100_set_ipi(i);
		}
	}
}

static int
smp_hxeb100_probe(void)
{
	int cpu_nr = 2;
	return cpu_nr;
}

static void
smp_hxeb100_kick_cpu(int cpu_nr)
{
	*(unsigned long *)KERNELBASE = cpu_nr;
	asm volatile("dcbf 0,%0"::"r"(KERNELBASE):"memory");
}

static void
smp_hxeb100_setup_cpu(int cpu_nr)
{
	int irq;
	int group_irq_base;
		
	hxeb100_clr_ipi(0);
	hxeb100_clr_ipi(1); /* paranoia */
	
	switch(cpu_nr) {
	case 0:
		irq = HXEB100_CPU0_IPI;
		request_irq(irq, hxeb100_primary_intr,
				SA_INTERRUPT, "CPU0 IPI", &cpu0);
		/* clear interrupt cause register */
		gt_write(GT64260_GPP_INTR_CAUSE, ~(1<<HXEB100_CPU0_IPI_IN));

		irq_desc[irq].handler->set_affinity(irq, 1<<0);
		unmask_irq(irq);
		
		break;
	case 1:
		hxeb100_init_caches(cpu_nr);

		irq = HXEB100_CPU1_IPI;
		request_irq(irq, hxeb100_primary_intr,
				SA_INTERRUPT, "CPU1 IPI", &cpu1);

		/* clear interrupt cause register */
		gt_write(GT64260_GPP_INTR_CAUSE, ~(1<<HXEB100_CPU1_IPI_IN));

		irq_desc[irq].handler->set_affinity(irq, 1<<1);
		unmask_irq(irq);
		/*
		 * since gpp irqs are steered in groups of 8
		 * and, since we know that cpu1's ipi is the only irq
		 * actually used in it's group,
		 * force the entire group of irqs to this processor
		 */
		group_irq_base = irq & ~7;
		for (irq=group_irq_base; irq < (group_irq_base+8); irq++)
			irq_desc[irq].handler->set_affinity(irq, 1<<1);
		break;
	default:
		panic("smp_hxeb100_setup_cpu: cpu %d doesn't exist\n", cpu_nr);
	}
}

struct smp_ops_t hxeb100_smp_ops = {
	smp_hxeb100_message_pass,
	smp_hxeb100_probe,
	smp_hxeb100_kick_cpu,
	smp_hxeb100_setup_cpu,
};
#endif

void __init
platform_init(unsigned long r3, unsigned long r4, unsigned long r5,
		unsigned long r6, unsigned long r7)
{
	extern char cmd_line[];

	cmd_line[0] = '\0';
#ifdef CONFIG_BLK_DEV_INITRD
	initrd_start=initrd_end=0;
	initrd_below_start_ok=0;
#endif
	ppc_md.board_info = hxeb100_board_info;

	parse_bootinfo(find_bootinfo());

	isa_mem_base = 0;
	isa_io_base = GT64260_PCI_0_IO_START_PROC;
	pci_dram_offset = GT64260_PCI_0_MEM_START_PROC;

	ppc_md.setup_arch = hxeb100_setup_arch;
	ppc_md.show_cpuinfo = hxeb100_show_cpuinfo;
	ppc_md.irq_cannonicalize = NULL;
	ppc_md.init_IRQ = gt64260_init_irq;
	ppc_md.get_irq = gt64260_get_irq;

	ppc_md.pcibios_fixup = hxeb100_pci_fixups;

	ppc_md.restart = hxeb100_restart;
	ppc_md.power_off = hxeb100_power_off;
	ppc_md.halt = hxeb100_halt;

	ppc_md.find_end_of_memory = hxeb100_find_end_of_memory;
	ppc_md.init = NULL;

	ppc_md.time_init = todc_time_init;
	ppc_md.set_rtc_time = todc_set_rtc_time;
	ppc_md.get_rtc_time = todc_get_rtc_time;

	ppc_md.nvram_read_val = todc_direct_read_val;
	ppc_md.nvram_write_val = todc_direct_write_val;

	ppc_md.calibrate_decr = hxeb100_calibrate_decr;

#ifdef CONFIG_SMP
	ppc_md.smp_ops = &hxeb100_smp_ops;
#endif

#ifdef	CONFIG_GT64260_NEW_BASE
	gt64260_base = CONFIG_GT64260_NEW_REG_BASE;
#else
	gt64260_base = CONFIG_GT64260_ORIG_REG_BASE;
#endif
	/*
	 * We need to map in the mem ctlr's regs so we can determine the
	 * amount of memory in the system.
	 * Also, if progress msgs are being used, have to map in the
	 * UART regs.
	 * All of this is done by hxeb100_set_bat() and hxeb100_map_io().
	 */
	/* Sanity check so hxeb100_set_bat(), etc. work */
	if (gt64260_base < 0xf0000000) {
		printk("Bridge's Base Address (0x%x) should be >= 0xf0000000\n",
			gt64260_base);
	}

	hxeb100_set_bat();

#ifdef	CONFIG_SERIAL
#if defined(CONFIG_SERIAL_TEXT_DEBUG)
	ppc_md.setup_io_mappings = hxeb100_map_io;
	ppc_md.progress = gen550_progress;
#endif
#if defined(CONFIG_KGDB)
	ppc_md.setup_io_mappings = hxeb100_map_io;
	ppc_md.early_serial_map = hxeb100_early_serial_map;
#endif
#endif

	return;
}
