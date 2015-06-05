/*
 * arch/ppc/platforms/cpci690.c
 *
 * Board setup routines for the Force CPCI690 board.
 *
 * Author: Mark A. Greer <mgreer@mvista.com>
 *
 * 2003 (c) MontaVista Software, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This programr
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#include <linux/config.h>

#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/ide.h>
#include <linux/irq.h>
#include <linux/fs.h>
#include <linux/seq_file.h>

#include <asm/bootinfo.h>
#include <asm/machdep.h>
#include <asm/gt64260.h>
#include <asm/todc.h>
#include <asm/time.h>

#include <platforms/cpci690.h>

#define BOARD_VENDOR	"Force"
#define BOARD_MACHINE	"CPCI690"

/* Set IDE controllers into Native mode? */
#define SET_PCI_IDE_NATIVE

static u32	board_reg_base;	/* Virtual addr of board regs */

static unsigned long gt64260_phys_base;


static const unsigned int cpu_7xx[16] = { /* 7xx & 74xx (but not 745x) */
	18, 15, 14, 2, 4, 13, 5, 9, 6, 11, 8, 10, 16, 12, 7, 0
};

bd_t	board_info;


TODC_ALLOC();

static void __init
cpci690_extract_board_info(void *bi_rec, int size)
{
	bd_t	*bip = bi_rec;


	if ((size == sizeof(bd_t)) && (bip->bi_magic == CPCI690_BI_MAGIC)) {
		memcpy(&board_info, bip, sizeof(bd_t));
	}
	else {
		printk(KERN_NOTICE "Invalid BOARD_INFO bi_rec\n");
	}

	return;
}

static int
cpci690_get_bus_speed(void)
{
	return 133333333;
}

static int
cpci690_get_cpu_speed(void)
{
	unsigned long	hid1;

	hid1 = mfspr(HID1) >> 28;
	return cpci690_get_bus_speed() * cpu_7xx[hid1]/2;
}

unsigned long __init
cpci690_find_end_of_memory(void)
{
	u32		mem_ctlr_size;
	static u32	board_size;
	static u8	first_time = 1;

	if (first_time) {
		/* Using cpci690_set_bat() mapping ==> virt addr == phys addr */
		switch (in_8((u8 *)(CPCI690_REG_BASE+CPCI690_MEM_CTLR))&0x07) {
			case 0x01:
				board_size = 256*MB;
				break;
			case 0x02:
				board_size = 512*MB;
				break;
			case 0x03:
				board_size = 768*MB;
				break;
			case 0x04:
				board_size = 1*GB;
				break;
			case 0x05:
				board_size = 1*GB + 512*MB;
				break;
			case 0x06:
				board_size = 2*GB;
				break;
			default:
				board_size = 0xffffffff; /* use mem ctlr size */
		} /* switch */

		mem_ctlr_size = gt64260_get_mem_size();

		/* Check that mem ctlr & board reg agree.  If not, pick MIN. */
		if (board_size != mem_ctlr_size) {
			printk(KERN_WARNING "Board register & memory controller mem size disagree (board reg: 0x%lx, mem ctlr: 0x%lx)\n",
				(ulong)board_size, (ulong)mem_ctlr_size);
			board_size = MIN(board_size, mem_ctlr_size);
		}

		first_time = 0;
	} /* if */

	return board_size;
}

#ifdef SET_PCI_IDE_NATIVE
static void __init
set_pci_native_mode(void)
{
	struct pci_dev *dev;

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

static void __init
cpci690_pci_fixups(void)
{
#ifdef SET_PCI_IDE_NATIVE
	set_pci_native_mode();
#endif
}

static int __init
cpci690_map_irq(struct pci_dev *dev, unsigned char idsel, unsigned char pin)
{
	struct pci_controller	*hose = pci_bus_to_hose(dev->bus->number);

	if (hose->index == 0) {
		static char pci_irq_table[][4] =
		/*
		 *	PCI IDSEL/INTPIN->INTLINE
		 * 	   A   B   C   D
		 */
		{
			{ 90, 91, 88, 89}, /* IDSEL 30/20 - Sentinel */
		};

		const long min_idsel = 20, max_idsel = 20, irqs_per_slot = 4;
		return PCI_IRQ_TABLE_LOOKUP;
	}
	else {
		static char pci_irq_table[][4] =
		/*
		 *	PCI IDSEL/INTPIN->INTLINE
		 * 	   A   B   C   D
		 */
		{
			{ 93, 94, 95, 92}, /* IDSEL 28/18 - PMC slot 2 */
			{  0,  0,  0,  0}, /* IDSEL 29/19 - Not used */
			{ 94, 95, 92, 93}, /* IDSEL 30/20 - PMC slot 1 */
		};

		const long min_idsel = 18, max_idsel = 20, irqs_per_slot = 4;
		return PCI_IRQ_TABLE_LOOKUP;
	}
}

static void __init
cpci690_cleanup_bits(void)
{
	/*
	 * Force firmware, for some reason, enables a bunch of intr sources
	 * for the PCI INT output pins.  Must mask them off b/c the PCI0/1
	 * Int pins are wired to INTD# for their respective buses.
	 */
	gt_write(GT64260_IC_CPU_INTR_MASK_LO, 0);
	gt_write(GT64260_IC_CPU_INTR_MASK_HI, 0);
	gt_write(GT64260_IC_PCI_0_INTR_MASK_LO, 0);
	gt_write(GT64260_IC_PCI_0_INTR_MASK_HI, 0);
	gt_write(GT64260_IC_PCI_1_INTR_MASK_LO, 0);
	gt_write(GT64260_IC_PCI_1_INTR_MASK_HI, 0);
	gt_write(GT64260_IC_CPU_INT_0_MASK, 0x00000000);
	gt_write(GT64260_IC_CPU_INT_1_MASK, 0x00000000);
	gt_write(GT64260_IC_CPU_INT_2_MASK, 0x00000000);
	gt_write(GT64260_IC_CPU_INT_3_MASK, 0x00000000);

	/* Bit 23 MUST be 1 for >100MHZ bus and all 690s are 133MHZ.
	   Some cards have this set to zero by the firmware,
	   though. */
	gt_set_bits(GT64260_CPU_CONFIG, (1<<23));
}

static void __init
cpci690_setup_bridge(void)
{
        gt64260_bridge_info_t	info;
	u8			save_exclude;
	u32			val;
	int			i;

        GT64260_BRIDGE_INFO_DEFAULT(&info, cpci690_find_end_of_memory());

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
			/* GT64260_PCI_ACC_CNTL_PREFETCHEN | */
			GT64260_PCI_ACC_CNTL_DREADEN |
			GT64260_PCI_ACC_CNTL_RDPREFETCH |
			GT64260_PCI_ACC_CNTL_RDLINEPREFETCH |
			GT64260_PCI_ACC_CNTL_RDMULPREFETCH |
			GT64260_PCI_ACC_CNTL_SWAP_NONE |
			GT64260_PCI_ACC_CNTL_MBURST_4_WORDS;
		info.pci_0_snoop_options[i] = GT64260_PCI_SNOOP_WB;
		info.pci_1_acc_cntl_options[i] =
			/* GT64260_PCI_ACC_CNTL_PREFETCHEN | */
			GT64260_PCI_ACC_CNTL_DREADEN |
			GT64260_PCI_ACC_CNTL_RDPREFETCH |
			GT64260_PCI_ACC_CNTL_RDLINEPREFETCH |
			GT64260_PCI_ACC_CNTL_RDMULPREFETCH |
			GT64260_PCI_ACC_CNTL_SWAP_NONE |
			GT64260_PCI_ACC_CNTL_MBURST_4_WORDS;
		info.pci_1_snoop_options[i] = GT64260_PCI_SNOOP_WB;
	}

	info.pci_0_latency = 0x4;
	info.pci_1_latency = 0x4;

	info.hose_a = pcibios_alloc_controller();
	if (!info.hose_a) {
		if (ppc_md.progress)
			ppc_md.progress("cpci690_setup_bridge: hose_a failed", 0);
		printk("cpci690_setup_bridge: can't set up first hose\n");
		return;
	}

	info.hose_b = pcibios_alloc_controller();
	if (!info.hose_b) {
		if (ppc_md.progress)
			ppc_md.progress("cpci690_setup_bridge: hose_b failed", 0);
		printk("cpci690_setup_bridge: can't set up second hose\n");
		return;
	}

	info.phys_base_addr = gt64260_phys_base;

        /* Lookup PCI host bridges */
        if (gt64260_find_bridges(&info, cpci690_map_irq)) {
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

	early_read_config_dword(info.hose_b,
			info.hose_b->first_busno,
			PCI_DEVFN(0,0),
			PCI_COMMAND,
			&val);
	val |= 0x10;
	early_write_config_dword(info.hose_b,
			info.hose_b->first_busno,
			PCI_DEVFN(0,0),
			PCI_COMMAND,
			val);

	gt64260_pci_exclude_bridge = save_exclude;

	return;
}

static void __init
cpci690_setup_peripherals(void)
{
	/*
	 * Set up windows to SRAM, RTC/TODC and DUART on device module
	 * (CS 0, 1 & 2)
	 * */
	gt64260_cpu_cs_set_window(0, CPCI690_REG_BASE, CPCI690_REG_SIZE);
	gt64260_cpu_cs_set_window(1, CPCI690_TODC_BASE, CPCI690_TODC_SIZE);
	gt64260_cpu_cs_set_window(2, CPCI690_IPMI_BASE, CPCI690_IPMI_SIZE);

	TODC_INIT(TODC_TYPE_MK48T35, 0, 0,
			ioremap(CPCI690_TODC_BASE, CPCI690_TODC_SIZE), 8);

	/* Map in board registers */
	board_reg_base = (u32)ioremap(CPCI690_REG_BASE, CPCI690_REG_SIZE_ACTUAL);

	gt_set_bits(GT64260_PCI_0_ARBITER_CNTL, (1<<31));
	gt_set_bits(GT64260_PCI_1_ARBITER_CNTL, (1<<31));

        gt_set_bits(GT64260_CPU_MASTER_CNTL, (1<<9)); /* Only 1 cpu */

#define GPP_EXTERNAL_INTERRUPTS \
		((1<<24) | (1<<25) | (1<<26) | (1<<27) | \
		 (1<<28) | (1<<29) | (1<<30) | (1<<31))
	/* PCI interrupts are inputs */
	gt_clr_bits(GT64260_GPP_IO_CNTL, GPP_EXTERNAL_INTERRUPTS);
	/* PCI interrupts are active low */
	gt_set_bits(GT64260_GPP_LEVEL_CNTL, GPP_EXTERNAL_INTERRUPTS);

	/* Clear any pending interrupts for these inputs and enable them. */
	gt_write(GT64260_GPP_INTR_CAUSE, ~GPP_EXTERNAL_INTERRUPTS);
	gt_set_bits(GT64260_GPP_INTR_MASK, GPP_EXTERNAL_INTERRUPTS);

	/* Route MPP interrupt inputs to GPP */
	gt_write(GT64260_MPP_CNTL_2, 0x00000000);
	gt_write(GT64260_MPP_CNTL_3, 0x00000000);

	/*
	 * Set MPSC Multiplex RMII
	 * NOTE: ethernet driver modifies bit 0 and 1
	 */
	gt_write(GT64260_MPP_SERIAL_PORTS_MULTIPLEX, 0x00001102);

	/* Set up the MAC addresses for the enet ctlrs. */
	gt64260_set_mac_addr(0, board_info.bi_enetaddr[0]);
	gt64260_set_mac_addr(1, board_info.bi_enetaddr[1]);
	gt64260_set_mac_addr(2, board_info.bi_enetaddr[2]);

	return;
}

#ifdef	CONFIG_KGDB
static void __init
cpci690_early_serial_map(void)
{
	static char	first_time = 1;

#if defined(CONFIG_KGDB_TTYS0)
#define KGDB_PORT 0
#elif defined(CONFIG_KGDB_TTYS1)
#define KGDB_PORT 1
#else
#error "Invalid kgdb_tty port"
#endif

	if (first_time) {
		gt_early_mpsc_init(KGDB_PORT, B9600|CS8|CREAD|HUPCL|CLOCAL);
		first_time = 0;
	}
}
#endif

static void __init
cpci690_setup_arch(void)
{
	uint	val;

	if ( ppc_md.progress )
		ppc_md.progress("cpci690_setup_arch: enter", 0);

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
		ppc_md.progress("cpci690_setup_arch: Enabling L2 cache", 0);

	/* Enable L2 and L3 caches (if 745x) */
	val = _get_L2CR();
	val |= L2CR_L2E;
	_set_L2CR(val);

	if ( ppc_md.progress )
		ppc_md.progress("cpci690_setup_arch: Initializing bridge", 0);

	/* Map in the bridge's registers */
	gt64260_base = (u32)ioremap(gt64260_base, GT64260_INTERNAL_SPACE_SIZE);

	cpci690_cleanup_bits();		/* clear registers */
	cpci690_setup_bridge();		/* set up PCI bridge(s) */
	cpci690_setup_peripherals();	/* set up chip selects/GPP/MPP etc */

	if ( ppc_md.progress )
		ppc_md.progress("cpci690_setup_arch: bridge init complete", 0);

#ifdef	CONFIG_DUMMY_CONSOLE
	conswitchp = &dummy_con;
#endif

	printk(BOARD_VENDOR " " BOARD_MACHINE "\n");
	printk("CPCI690 port (C) 2003 MontaVista Software, Inc. (source@mvista.com)\n");

	if ( ppc_md.progress )
		ppc_md.progress("cpci690_setup_arch: exit", 0);

	return;
}

static void
cpci690_reset_board(void)
{
	unsigned long i = 10000;

	__cli();

	/* set exception prefix high - to the prom */
	_nmask_and_or_msr(0, MSR_IP);

	out_8((u8 *)(board_reg_base + CPCI690_SW_RESET), 0x11);

	while ( i != 0 ) i++;
	panic("restart failed\n");
}

static void
cpci690_restart(char *cmd)
{
	cpci690_reset_board();
}

static void
cpci690_halt(void)
{
	__cli();
	while (1);
	/* NOTREACHED */
}

static void
cpci690_power_off(void)
{
	cpci690_halt();
	/* NOTREACHED */
}

static int
cpci690_show_cpuinfo(struct seq_file *m)
{
	seq_printf(m, "vendor\t\t: " BOARD_VENDOR "\n");
	seq_printf(m, "machine\t\t: " BOARD_MACHINE "\n");
	seq_printf(m, "cpu MHz\t\t: %d\n", cpci690_get_cpu_speed()/1000/1000);
	seq_printf(m, "bus MHz\t\t: %d\n", cpci690_get_bus_speed()/1000/1000);

	return 0;
}

static void __init
cpci690_calibrate_decr(void)
{
	ulong freq;

	freq = cpci690_get_bus_speed()/4;

	printk("time_init: decrementer frequency = %lu.%.6lu MHz\n",
	       freq/1000000, freq%1000000);

	tb_ticks_per_jiffy = freq / HZ;
	tb_to_us = mulhwu_scale_factor(freq, 1000000);

	return;
}

#if defined(CONFIG_BLK_DEV_IDE) || defined(CONFIG_BLK_DEV_IDE_MODULE)
static int
cpci690_ide_check_region(ide_ioreg_t from, unsigned int extent)
{
        return check_region(from, extent);
}

static void
cpci690_ide_request_region(ide_ioreg_t from,
                        unsigned int extent,
                        const char *name)
{
        request_region(from, extent, name);
	return;
}

static void
cpci690_ide_release_region(ide_ioreg_t from,
                        unsigned int extent)
{
        release_region(from, extent);
	return;
}

static void __init
cpci690_ide_pci_init_hwif_ports(hw_regs_t *hw, ide_ioreg_t data_port,
                ide_ioreg_t ctrl_port, int *irq)
{
	struct pci_dev	*dev;
#if 1 /* NTL */
        int i;

	//printk("regs %d to %d @ 0x%x\n", IDE_DATA_OFFSET, IDE_STATUS_OFFSET, data_port);
        for (i = IDE_DATA_OFFSET; i <= IDE_STATUS_OFFSET; i++) {
                hw->io_ports[i] = data_port;
                data_port++;
        }

	//printk("ctrl %d @ 0x%x\n", IDE_CONTROL_OFFSET, ctrl_port);
        hw->io_ports[IDE_CONTROL_OFFSET] = ctrl_port;
#endif

	pci_for_each_dev(dev) {
		if (((dev->class >> 8) == PCI_CLASS_STORAGE_IDE) ||
		    ((dev->class >> 8) == PCI_CLASS_STORAGE_RAID)) {
			hw->irq = dev->irq;

			if (irq != NULL) {
				*irq = dev->irq;
			}
		}
	}

	return;
}
#endif

/*
 * Set BAT 3 to map 0xf0000000 to end of physical memory space.
 */
static __inline__ void
cpci690_set_bat(void)
{
	mb();
	mtspr(DBAT2U, 0xf0001ffe);
	mtspr(DBAT2L, 0xf000002a);
	mb();

	return;
}

#if	defined(CONFIG_SERIAL_TEXT_DEBUG) || defined(CONFIG_KGDB)
static void __init
cpci690_map_io(void)
{
	io_block_mapping(gt64260_base, gt64260_base, 0x00020000, _PAGE_IO);
}
#endif

void __init
platform_init(unsigned long r3, unsigned long r4, unsigned long r5,
	      unsigned long r6, unsigned long r7)
{
	extern char cmd_line[];

#ifdef	CONFIG_GT64260_NEW_BASE
	gt64260_base = CONFIG_GT64260_NEW_REG_BASE;
#else
	gt64260_base = CONFIG_GT64260_ORIG_REG_BASE;
#endif
	gt64260_phys_base = gt64260_base;


	cmd_line[0]=0;
#ifdef CONFIG_BLK_DEV_INITRD
	initrd_start=initrd_end=0;
	initrd_below_start_ok=0;
#endif /* CONFIG_BLK_DEV_INITRD */

	ppc_md.board_info = cpci690_extract_board_info;
	parse_bootinfo(find_bootinfo());

	cpci690_set_bat(); /* Need for cpci690_find_end_of_memory & progress */
	loops_per_jiffy = cpci690_get_cpu_speed() / HZ;

	isa_mem_base = 0;
	isa_io_base = GT64260_PCI_0_IO_START_PROC;
	pci_dram_offset = GT64260_PCI_0_MEM_START_PROC;

	ppc_md.setup_arch = cpci690_setup_arch;
	ppc_md.show_cpuinfo = cpci690_show_cpuinfo;
	ppc_md.irq_cannonicalize = NULL;
	ppc_md.init_IRQ = gt64260_init_irq;
	ppc_md.get_irq = gt64260_get_irq;

	ppc_md.pcibios_fixup = cpci690_pci_fixups;

	ppc_md.restart = cpci690_restart;
	ppc_md.power_off = cpci690_power_off;
	ppc_md.halt = cpci690_halt;

	ppc_md.find_end_of_memory = cpci690_find_end_of_memory;

	ppc_md.init = NULL;

	ppc_md.time_init = todc_time_init;
	ppc_md.set_rtc_time = todc_set_rtc_time;
	ppc_md.get_rtc_time = todc_get_rtc_time;

	ppc_md.nvram_read_val = todc_direct_read_val;
	ppc_md.nvram_write_val = todc_direct_write_val;

	ppc_md.calibrate_decr = cpci690_calibrate_decr;

#ifdef	CONFIG_SERIAL_TEXT_DEBUG
	ppc_md.setup_io_mappings = cpci690_map_io;
	ppc_md.progress = gt64260_mpsc_progress;
#endif	/* CONFIG_SERIAL_TEXT_DEBUG */
#ifdef	CONFIG_KGDB
	ppc_md.early_serial_map = cpci690_early_serial_map;
	ppc_md.setup_io_mappings = cpci690_map_io;
#endif	/* CONFIG_KGDB */

#if defined(CONFIG_BLK_DEV_IDE) || defined(CONFIG_BLK_DEV_IDE_MODULE)
        ppc_ide_md.ide_init_hwif = cpci690_ide_pci_init_hwif_ports;
#endif

	return;
}
