/*
 * arch/ppc/platforms/ppmc260.c
 *
 * Board setup routines for the Force PowerPMC260.
 *
 * Derived from ev64260_setup.c
 * Author: Randy Vinson <rvinson@mvista.com.
 *
 * 2003 (c) MontaVista Software, Inc. This file is licensed under the terms
 * of the GNU General Public License version 2. This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */

#include <linux/config.h>

#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/ide.h>
#include <linux/irq.h>

#include <linux/fs.h>
#include <linux/seq_file.h>

#if     !defined(CONFIG_GT64260_CONSOLE)
#include <linux/serial.h>
#endif

#include <asm/bootinfo.h>
#include <asm/machdep.h>
#include <asm/gt64260.h>
#include <asm/todc.h>
#include <asm/time.h>

#include <platforms/ppmc260.h>

#define BOARD_VENDOR	"Force Computers"
#define BOARD_MACHINE	"ProcessorPMC260"

ulong	ppmc260_mem_size = 0;
bd_t	board_info;

static const unsigned int cpu_7xx[16] = {
    0, 15, 14, 0, 0, 13, 5, 9, 6, 11, 8, 10, 16, 12, 7, 0
};

static void __init
ppmc260_board_info(void *_bi, int size)
{
	bd_t *bi = (bd_t *) _bi;

	if (size == sizeof(*bi))
		memcpy(&board_info, bi, sizeof(*bi));
	else
		printk("GT64260 MAC Addresses not passed\n");
}

static unsigned int  ppmc260_get_bus_speed(void)
{
    return 133333333;
}

static unsigned int ppmc260_get_cpu_speed(void)
{
    unsigned long hid1 = mfspr(HID1) >> 28;
    return ppmc260_get_bus_speed()*cpu_7xx[hid1]/2;
}

unsigned long __init
ppmc260_find_end_of_memory(void)
{
	return gt64260_get_mem_size();
}

/*
 * Force Computer Test Card PCI interrupt routing.
 */
static int __init
ppmc260_map_irq(struct pci_dev *dev, unsigned char idsel, unsigned char pin)
{
	struct pci_controller	*hose = pci_bus_to_hose(dev->bus->number);

	if (hose->index == 0) {
		static char pci_irq_table[][4] =
		/*
		 *	PCI IDSEL/INTPIN->INTLINE 
		 * 	   A   B   C   D
		 */
		{
			{ 74, 75, 76, 77 },/* IDSEL 7  (AD17) - PCI Slot */
			{ 75, 76, 77, 74 },/* IDSEL 8  (AD18) - PMC1B */
			{ 75, 76, 77, 74 },/* IDSEL 9  (AD19) - PMC2B */
			{ 74, 75, 76, 77 },/* IDSEL 10 (AD20) - PMC1A */
			{ 74, 75, 76, 77 },/* IDSEL 11 (AD21) - PMC2A */
			{  0,  0,  0, 0  },/* IDSEL 12 (AD22) - Not Used */
			{  0,  0,  0, 0  },/* IDSEL 13 (AD23) - Not Used */
			{ 74, 75, 76, 77 },/* IDSEL 14 (AD24) - 21154/cPCI */
		};

		const long min_idsel = 7, max_idsel = 14, irqs_per_slot = 4;
		return PCI_IRQ_TABLE_LOOKUP;
	}
	else {
		static char pci_irq_table[][4] =
		/*
		 *	PCI IDSEL/INTPIN->INTLINE 
		 * 	   A   B   C   D
		 */
		{
			{ 0, 0, 0, 0 },/* IDSEL 7 - PCI bus 1 (Not Used)*/
		};

		const long min_idsel = 7, max_idsel = 7, irqs_per_slot = 4;
		return PCI_IRQ_TABLE_LOOKUP;
	}
}

static void __init
ppmc260_setup_peripherals(void)
{
	/*
	 * Set up windows for BOOT FLASH (using boot CS window),
	 * USER FLASH0 and USER FLASH1.
	 *
	 * Assumes that firmware has set up the Device/Boot Bank Param regs.
	 */
	gt64260_cpu_boot_set_window(PPMC260_BOOT_FLASH_BASE,
				    PPMC260_BOOT_FLASH_SIZE);
	gt64260_cpu_cs_set_window(1, 0, 0); /* disable */
	gt64260_cpu_cs_set_window(2, PPMC260_USER_FLASH0_BASE,
				  PPMC260_USER_FLASH0_SIZE);
	gt64260_cpu_cs_set_window(3, PPMC260_USER_FLASH1_BASE,
				  PPMC260_USER_FLASH1_SIZE);

	gt_clr_bits(GT64260_CPU_CONFIG, ((1<<28) | (1<<29)));
	gt_set_bits(GT64260_CPU_CONFIG, (1<<27));

	gt_set_bits(GT64260_PCI_0_SLAVE_PCI_DECODE_CNTL, ((1<<0) | (1<<3)));

        gt_set_bits(GT64260_CPU_MASTER_CNTL, (1<<9)); /* Only 1 cpu */

	/*
	 * The PPMC260 uses several Multi-Purpose Pins (MPP) on the 64260
	 * bridge as interrupt inputs (via the General Purpose Ports (GPP)
	 * register).  Need to route the MPP inputs to the GPP and set the
	 * polarity correctly.
	 *
	 * In MPP Control 3 Register
	 *   MPP 10 -> GPP 10 (PCI 0 INTA)
	 *   MPP 11 -> GPP 11 (PCI 0 INTB)
	 *   MPP 12 -> GPP 12 (PCI 0 INTC)
	 *   MPP 13 -> GPP 13 (PCI 0 INTD)
	 */
	gt_clr_bits(GT64260_MPP_CNTL_3,
			       ((1<<8) | (1<<9) | (1<<10) | (1<<11) |
			        (1<<12) | (1<<13) | (1<<14) | (1<<15) |
				(1<<16) | (1<<17) | (1<<18) | (1<<19) |
				(1<<20) | (1<<21) | (1<<22) | (1<<23)));

	/* PCI interrupts are active low */
	gt_set_bits(GT64260_GPP_LEVEL_CNTL,
			     ((1<<10) | (1<<11) | (1<<12) | (1<<13)));

	/* Clear any pending interrupts for these inputs and enable them. */
	gt_write(GT64260_GPP_INTR_CAUSE,
			  ~((1<<10) | (1<<11) | (1<<12) | (1<<13)));
	gt_set_bits(GT64260_GPP_INTR_MASK,
			     ((1<<10) | (1<<11)| (1<<12) | (1<<13)));
	gt_set_bits(GT64260_IC_CPU_INTR_MASK_HI, (1<<25));

	return;
}


static void __init
ppmc260_setup_bridge(void)
{
        gt64260_bridge_info_t           info;
	int				i;
	u32				val;
	u8				save_exclude;

        GT64260_BRIDGE_INFO_DEFAULT(&info, 0);

	info.phys_base_addr = gt64260_base;

	/* Map in the bridge's registers */
	gt64260_base = (u32)ioremap(gt64260_base, GT64260_INTERNAL_SPACE_SIZE);

        info.mem_size = ppmc260_find_end_of_memory();

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
			GT64260_PCI_ACC_CNTL_DREADEN |
			GT64260_PCI_ACC_CNTL_RDPREFETCH |
			GT64260_PCI_ACC_CNTL_RDLINEPREFETCH |
			GT64260_PCI_ACC_CNTL_RDMULPREFETCH |
			GT64260_PCI_ACC_CNTL_SWAP_BYTE |
			GT64260_PCI_ACC_CNTL_MBURST_4_WORDS;
		info.pci_0_snoop_options[i] = GT64260_PCI_SNOOP_WB;
	}

	info.pci_0_latency = 0x8;

	info.hose_a = pcibios_alloc_controller();
	if (!info.hose_a) {
		if (ppc_md.progress)
			ppc_md.progress("ppmc260_setup_bridge: hose_a failed", 0);
		printk("ppmc260_setup_bridge: can't set up first hose\n");
		return;
	}

        /* Lookup PCI host bridges */
        if (gt64260_find_bridges(&info, ppmc260_map_irq)) {
                printk("Bridge initialization failed.\n");
		iounmap((void *)gt64260_base);
        }

	/*
	 * Dave Wilhardt found that bit 4 in the PCI Command registers must
	 * be set if you are using cache coherency.
	 *
	 * Note: he also said that bit 4 must be on in all PCI devices but
	 *       that has not been implemented yet.
	 */
	save_exclude = gt64260_pci_exclude_bridge;
	gt64260_pci_exclude_bridge = FALSE;

	early_read_config_dword(info.hose_a,
			info.hose_a->first_busno,
			PCI_DEVFN(0,0),
			PCI_COMMAND,
			&val);
	val |= 0x10;
	early_write_config_dword(info.hose_a,
			info.hose_a->first_busno,
			PCI_DEVFN(0,0),
			PCI_COMMAND,
			val);

	gt64260_pci_exclude_bridge = save_exclude;

#if 0
	/* initialize the gt64260 wdt */
	gt64260_wdt_init(GT64260_WDT_WDC_VAL_MSK | GT64260_WDT_WDC_EN);
#endif
	return;
}

static void __init
ppmc260_early_serial_map(void)
{
#ifdef	CONFIG_KGDB
	static char	first_time = 1;


#if defined(CONFIG_KGDB_TTYS0)
#define KGDB_PORT 0
#elif defined(CONFIG_KGDB_TTYS1)
#define KGDB_PORT 1
#else
#error "Invalid kgdb_tty port"
#endif

	if (first_time) {
		gt_early_mpsc_init(KGDB_PORT, B115200|CS8|CREAD|HUPCL|CLOCAL);
		first_time = 0;
	}

	return;
#endif
}

#define IS_PVR_750CX(p) (						\
	   PVR_REV((p) & 0xFFF0) == 0x0100				\
	|| PVR_REV((p) & 0xFF00) == 0x2200				\
	|| PVR_REV((p) & 0xFF00) == 0x3300				\
	)

static void __init
ppmc260_setup_arch(void)
{
	uint	val;

	if ( ppc_md.progress )
		ppc_md.progress("ppmc260_setup_arch: enter", 0);

	loops_per_jiffy = ppmc260_get_cpu_speed() / HZ;

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
		ppc_md.progress("ppmc260_setup_arch: Enabling L2 cache", 0);

	/* Enable L2 cache */
	val = _get_L2CR();
	val |= L2CR_L2E;
	_set_L2CR(val);

	if ( ppc_md.progress )
		ppc_md.progress("ppmc260_setup_arch: Initializing bridge", 0);

	ppmc260_setup_bridge();		/* set up PCI bridge(s) */
	ppmc260_setup_peripherals();	/* set up chip selects/GPP/MPP etc */

	if ( ppc_md.progress )
		ppc_md.progress("ppmc260_setup_arch: bridge init complete", 0);

#ifdef	CONFIG_DUMMY_CONSOLE
	conswitchp = &dummy_con;
#endif

#if defined(CONFIG_GT64260_CONSOLE)
	ppmc260_early_serial_map();
#endif
	gt64260_set_mac_addr(0, board_info.bi_enetaddr[0]);
	gt64260_set_mac_addr(1, board_info.bi_enetaddr[1]);
	gt64260_set_mac_addr(2, board_info.bi_enetaddr[2]);


	printk(BOARD_VENDOR " " BOARD_MACHINE "\n");
	printk("Force ProcessorPMC260 port by MontaVista Software, Inc. (source@mvista.com)\n");

	if ( ppc_md.progress )
		ppc_md.progress("ppmc260_setup_arch: exit", 0);

	return;
}

static void
ppmc260_reset_board(void *addr)
{
#if 0
	u32 temp = 0;

	__cli();

	/* setup MPP 19 as WDE# output - active low */
	gt_modify(GT64260_MPP_CNTL_2, (0x9<<12), (0xf<<12));

	/* set wdt timeout */
	gt64260_wdt_load(200000);

	/* enable the wdt */
	gt64260_wdt_ena();

	/* wait for wdt */
	printk("Using Watchdog timer to reset board.\n");
	printk("Waiting for WDT to expire");

	while (1) {
		temp++;
		if (temp % 4000000 == 0)
			printk(".");
	}
	/* not reached */
#else
	__cli();

	printk("Auto-Reset not supported. Press reset button to reset board.\n");
	while(1);
	/* not reached */

#endif
}

static void
ppmc260_restart(char *cmd)
{
	volatile ulong	i = 10000000;

	ppmc260_reset_board((void *)0xfff00100);

	while (i-- > 0);
	panic("restart failed\n");
}

static void
ppmc260_halt(void)
{
	__cli();
	while (1);
	/* NOTREACHED */
}

static void
ppmc260_power_off(void)
{
	ppmc260_halt();
	/* NOTREACHED */
}

static int
ppmc260_show_cpuinfo(struct seq_file *m)
{
	uint pvid;

	pvid = mfspr(PVR);
	seq_printf(m, "vendor\t\t: " BOARD_VENDOR "\n");
	seq_printf(m, "machine\t\t: " BOARD_MACHINE "\n");

	seq_printf(m, "PVID\t\t: 0x%x, vendor: %s\n",
			pvid,
			((pvid & (1<<15)) || IS_PVR_750CX(pvid))
			    ? "IBM" : "Motorola" );
	seq_printf(m, "cpu MHz\t\t: %d\n", ppmc260_get_cpu_speed()/1000/1000);
	seq_printf(m, "bus MHz\t\t: %d\n", ppmc260_get_bus_speed()/1000/1000);

	return 0;
}

/* DS1501 RTC has too much variation to use RTC for calibration */
static void __init
ppmc260_calibrate_decr(void)
{
	ulong freq;

	freq = ppmc260_get_bus_speed()/4;

	printk("time_init: decrementer frequency = %lu.%.6lu MHz\n",
	       freq/1000000, freq%1000000);

	tb_ticks_per_jiffy = freq / HZ;
	tb_to_us = mulhwu_scale_factor(freq, 1000000);

	return;
}

/*
 * Set BAT 3 to map 0xf0000000 to end of physical memory space.
 */
static __inline__ void
ppmc260_set_bat(void)
{
	unsigned long   bat3u, bat3l;
	static int	mapping_set = 0;

	if (!mapping_set) {

		__asm__ __volatile__(
		" lis %0,0xf000\n \
		  ori %1,%0,0x002a\n \
		  ori %0,%0,0x1ffe\n \
		  mtspr 0x21e,%0\n \
		  mtspr 0x21f,%1\n \
		  isync\n \
		  sync "
		: "=r" (bat3u), "=r" (bat3l));

		mapping_set = 1;
	}

	return;
}

#if defined(CONFIG_SERIAL_TEXT_DEBUG) || defined(CONFIG_KGDB)
static void __init
ppmc260_map_io(void)
{
	io_block_mapping(0xf0000000, 0xf0000000, 0x10000000, _PAGE_IO);
}
#endif

void __init
platform_init(unsigned long r3, unsigned long r4, unsigned long r5,
	      unsigned long r6, unsigned long r7)
{
	extern char cmd_line[];

	cmd_line[0]=0;
#ifdef CONFIG_BLK_DEV_INITRD
	initrd_start=initrd_end=0;
	initrd_below_start_ok=0;
#endif /* CONFIG_BLK_DEV_INITRD */

	ppc_md.board_info = ppmc260_board_info;

	parse_bootinfo(find_bootinfo());

	isa_mem_base = 0;
	isa_io_base = GT64260_PCI_0_IO_START_PROC;
	pci_dram_offset = GT64260_PCI_0_MEM_START_PROC;

	ppc_md.setup_arch = ppmc260_setup_arch;
	ppc_md.show_cpuinfo = ppmc260_show_cpuinfo;
	ppc_md.irq_cannonicalize = NULL;
	ppc_md.init_IRQ = gt64260_init_irq;
	ppc_md.get_irq = gt64260_get_irq;

	ppc_md.restart = ppmc260_restart;
	ppc_md.power_off = ppmc260_power_off;
	ppc_md.halt = ppmc260_halt;

	ppc_md.find_end_of_memory = ppmc260_find_end_of_memory;

	ppc_md.init = NULL;

	ppc_md.time_init = NULL;
	ppc_md.set_rtc_time = NULL;
	ppc_md.get_rtc_time = NULL;

	ppc_md.nvram_read_val = NULL;
	ppc_md.nvram_write_val = NULL;

	ppc_md.calibrate_decr = ppmc260_calibrate_decr;

#ifdef	CONFIG_GT64260_NEW_BASE
	gt64260_base = CONFIG_GT64260_NEW_REG_BASE;
#else
	gt64260_base = CONFIG_GT64260_ORIG_REG_BASE;
#endif

	/*
	 * We need to map
	 * in the mem ctlr's regs so we can determine the amount of memory
	 * in the system.
	 * Also, if progress msgs are being used, have to map in the
	 * MPSC's regs.
	 * All of this is done by ppmc260_set_bat() and ppmc260_map_io().
	 */
	/* Sanity check so ppmc260_set_bat(), etc. work */
	if (gt64260_base < 0xf0000000) {
		printk("Bridge's Base Address (0x%x) should be >= 0xf0000000\n",
			gt64260_base);
	}
	ppmc260_set_bat();

#if defined(CONFIG_GT64260_CONSOLE)
#ifdef	CONFIG_SERIAL_TEXT_DEBUG
	ppc_md.setup_io_mappings = ppmc260_map_io;
	ppc_md.progress = gt64260_mpsc_progress; /* embedded UART */
#endif	/* CONFIG_SERIAL_TEXT_DEBUG */
#ifdef	CONFIG_KGDB
	ppc_md.setup_io_mappings = ppmc260_map_io;
	ppc_md.early_serial_map = ppmc260_early_serial_map;
#endif	/* CONFIG_KGDB */
#endif
	return;
}
