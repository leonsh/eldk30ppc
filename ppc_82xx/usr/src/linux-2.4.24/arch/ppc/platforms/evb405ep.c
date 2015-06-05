/*
 * arch/ppc/platforms/evb405ep.c
 *
 * Support for IBM PPC 405EP evaluation board ("Elvis").
 *
 * Author: SAW (IBM), derived from walnut.c.
 *         Maintained by MontaVista Software <source@mvista.com>
 *
 * 2003 (c) MontaVista Softare Inc.  This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/threads.h>
#include <linux/param.h>
#include <linux/string.h>
#include <linux/blk.h>
#include <linux/pci.h>
#include <linux/rtc.h>

#include <asm/system.h>
#include <asm/pci-bridge.h>
#include <asm/processor.h>
#include <asm/machdep.h>
#include <asm/page.h>
#include <asm/time.h>
#include <asm/io.h>
#include <platforms/ibm_ocp.h>
#include <platforms/ibm405ep.h>
#include <linux/serial.h>

#ifdef CONFIG_PPC_RTC
#include <asm/todc.h>
#endif

#include <asm/kgdb.h>

#undef DEBUG

#ifdef DEBUG
#define DBG(x...) printk(x)
#else
#define DBG(x...)
#endif


void *evb405ep_rtc_base;

extern void gen550_init(int, struct serial_struct *);

/* Some IRQs unique to the board
 * Used by the generic 405 PCI setup functions in ppc4xx_pci.c
 */
int __init
ppc405_map_irq(struct pci_dev *dev, unsigned char idsel, unsigned char pin)
{
	static char pci_irq_table[][4] =
	    /*
	     *      PCI IDSEL/INTPIN->INTLINE
	     *      A       B       C       D
	     */
	{
		{28, 28, 28, 28},	/* IDSEL 1 - PCI slot 1 */
		{29, 29, 29, 29},	/* IDSEL 2 - PCI slot 2 */
		{30, 30, 30, 30},	/* IDSEL 3 - PCI slot 3 */
		{31, 31, 31, 31},	/* IDSEL 4 - PCI slot 4 */
	};

	const long min_idsel = 1, max_idsel = 4, irqs_per_slot = 4;
	return PCI_IRQ_TABLE_LOOKUP;
};

/* The serial clock for the chip is an internal clock determined by
 * different clock speeds/dividers.
 * Calculate the proper input baud rate and setup the serial driver.
 */
static void __init
evb405ep_early_serial_map(void)
{
	
	u32 uart_div;
	int serial_baud_405ep;
	bd_t *bip = (bd_t *) __res;
	struct serial_struct serialreq = {0};

         /* Calculate the serial clock input frequency
          *
          * The base baud is the PLL OUTA (provided in the board info
          * structure) divided by the external UART Divisor, divided
          * by 16.
          */
	uart_div = (mfdcr(DCRN_CPC0_UCR_BASE) & DCRN_CPC0_UCR_U0DIV);
	serial_baud_405ep = bip->bi_pllouta_freq / uart_div / 16;


	/* Update the serial port attributes */
	serialreq.baud_base = serial_baud_405ep;
	serialreq.flags = (ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST);
	serialreq.io_type = SERIAL_IO_MEM;

	serialreq.line = 0;
	serialreq.port = 0;
	serialreq.irq = ACTING_UART0_INT;
	serialreq.iomem_base = (void*)ACTING_UART0_IO_BASE;

#if defined(CONFIG_SERIAL_TEXT_DEBUG) || defined(CONFIG_KGDB)
	/* Configure debug serial access */
	gen550_init(0, &serialreq);
#endif

	if (early_serial_setup(&serialreq) != 0) {
		printk("Early serial init of port 0 failed\n");
	}	
	
	serialreq.line = 1;
	serialreq.port = 1;
	serialreq.irq = ACTING_UART1_INT;
	serialreq.iomem_base = (void*)ACTING_UART1_IO_BASE;

#if defined(CONFIG_SERIAL_TEXT_DEBUG) || defined(CONFIG_KGDB)
	/* Configure debug serial access */
	gen550_init(1, &serialreq);
#endif

	if (early_serial_setup(&serialreq) != 0) {
		printk("Early serial init of port 1 failed\n");
	}	
}


void __init
board_setup_arch(void)
{
#define EVB405EP_FPGA_BASE      0xF0300000

        void *fpga_reg0;
        void *fpga_reg1;

        evb405ep_early_serial_map();
        fpga_reg0 = ioremap(EVB405EP_FPGA_BASE, 8);
        if (!fpga_reg0) {
                printk(KERN_CRIT
                       "evb405ep_setup_arch() fpga_reg0 ioremap failed\n");
                return;
        }

        fpga_reg1 = fpga_reg0 + 1;

#ifdef CONFIG_PPC_RTC
        /* RTC step for the evb405ep */
        evb405ep_rtc_base = (void *) EVB405EP_RTC_VADDR;
        TODC_INIT(TODC_TYPE_DS1743, evb405ep_rtc_base, evb405ep_rtc_base,
                  evb405ep_rtc_base, 8);
#endif                          /* CONFIG_PPC_RTC */
        /* Identify the system */
        printk("IBM 405EP Evaluation Board port (C) 2003 MontaVista Software, Inc. (source@mvista.com)\n");

	
}

void __init
bios_fixup(struct pci_controller *hose, struct pcil0_regs *pcip)
{

	unsigned int bar_response, bar;
	/*
	 * Expected PCI mapping:
	 *
	 *  PLB addr             PCI memory addr
	 *  ---------------------       ---------------------
	 *  0000'0000 - 7fff'ffff <---  0000'0000 - 7fff'ffff
	 *  8000'0000 - Bfff'ffff --->  8000'0000 - Bfff'ffff
	 *
	 *  PLB addr             PCI io addr
	 *  ---------------------       ---------------------
	 *  e800'0000 - e800'ffff --->  0000'0000 - 0001'0000
	 *
	 * The following code is simplified by assuming that the bootrom
	 * has been well behaved in following this mapping.
	 */

#ifdef DEBUG
	int i;

	printk("ioremap PCLIO_BASE = 0x%x\n", pcip);
	printk("PCI bridge regs before fixup \n");
	for (i = 0; i <= 3; i++) {
		printk(" pmm%dma\t0x%x\n", i, in_le32(&(pcip->pmm[i].ma)));
		printk(" pmm%dma\t0x%x\n", i, in_le32(&(pcip->pmm[i].la)));
		printk(" pmm%dma\t0x%x\n", i, in_le32(&(pcip->pmm[i].pcila)));
		printk(" pmm%dma\t0x%x\n", i, in_le32(&(pcip->pmm[i].pciha)));
	}
	printk(" ptm1ms\t0x%x\n", in_le32(&(pcip->ptm1ms)));
	printk(" ptm1la\t0x%x\n", in_le32(&(pcip->ptm1la)));
	printk(" ptm2ms\t0x%x\n", in_le32(&(pcip->ptm2ms)));
	printk(" ptm2la\t0x%x\n", in_le32(&(pcip->ptm2la)));

#endif

	/* added for IBM boot rom version 1.15 bios bar changes  -AK */

	/* Disable region first */
	out_le32((void *) &(pcip->pmm[0].ma), 0x00000000);
	/* PLB starting addr, PCI: 0x80000000 */
	out_le32((void *) &(pcip->pmm[0].la), 0x80000000);
	/* PCI start addr, 0x80000000 */
	out_le32((void *) &(pcip->pmm[0].pcila), PPC405_PCI_MEM_BASE);
	/* 512MB range of PLB to PCI */
	out_le32((void *) &(pcip->pmm[0].pciha), 0x00000000);
	/* Enable no pre-fetch, enable region */
	out_le32((void *) &(pcip->pmm[0].ma), ((0xffffffff -
						(PPC405_PCI_UPPER_MEM -
						 PPC405_PCI_MEM_BASE)) | 0x01));

	/* Disable region one */
	out_le32((void *) &(pcip->pmm[1].ma), 0x00000000);
	out_le32((void *) &(pcip->pmm[1].la), 0x00000000);
	out_le32((void *) &(pcip->pmm[1].pcila), 0x00000000);
	out_le32((void *) &(pcip->pmm[1].pciha), 0x00000000);
	out_le32((void *) &(pcip->pmm[1].ma), 0x00000000);
	out_le32((void *) &(pcip->ptm1ms), 0x00000001);

	/* Disable region two */
	out_le32((void *) &(pcip->pmm[2].ma), 0x00000000);
	out_le32((void *) &(pcip->pmm[2].la), 0x00000000);
	out_le32((void *) &(pcip->pmm[2].pcila), 0x00000000);
	out_le32((void *) &(pcip->pmm[2].pciha), 0x00000000);
	out_le32((void *) &(pcip->pmm[2].ma), 0x00000000);
	out_le32((void *) &(pcip->ptm2ms), 0x00000000);
	out_le32((void *) &(pcip->ptm2la), 0x00000000);

	/* Zero config bars */
	for (bar = PCI_BASE_ADDRESS_1; bar <= PCI_BASE_ADDRESS_2; bar += 4) {
		early_write_config_dword(hose, hose->first_busno,
					 PCI_FUNC(hose->first_busno), bar,
					 0x00000000);
		early_read_config_dword(hose, hose->first_busno,
					PCI_FUNC(hose->first_busno), bar,
					&bar_response);
		DBG("BUS %d, device %d, Function %d bar 0x%8.8x is 0x%8.8x\n",
		    hose->first_busno, PCI_SLOT(hose->first_busno),
		    PCI_FUNC(hose->first_busno), bar, bar_response);
	}
	/* end work arround */

#ifdef DEBUG
	printk("PCI bridge regs after fixup \n");
	for (i = 0; i <= 3; i++) {
		printk(" pmm%dma\t0x%x\n", i, in_le32(&(pcip->pmm[i].ma)));
		printk(" pmm%dma\t0x%x\n", i, in_le32(&(pcip->pmm[i].la)));
		printk(" pmm%dma\t0x%x\n", i, in_le32(&(pcip->pmm[i].pcila)));
		printk(" pmm%dma\t0x%x\n", i, in_le32(&(pcip->pmm[i].pciha)));
	}
	printk(" ptm1ms\t0x%x\n", in_le32(&(pcip->ptm1ms)));
	printk(" ptm1la\t0x%x\n", in_le32(&(pcip->ptm1la)));
	printk(" ptm2ms\t0x%x\n", in_le32(&(pcip->ptm2ms)));
	printk(" ptm2la\t0x%x\n", in_le32(&(pcip->ptm2la)));

#endif
}

void __init
board_io_mapping(void)
{
     io_block_mapping(EVB405EP_RTC_VADDR,
                      EVB405EP_RTC_PADDR, EVB405EP_RTC_SIZE, _PAGE_IO);
}

void __init
board_setup_irq(void)
{
}

void __init
board_init(void)
{
#ifdef CONFIG_PPC_RTC
	ppc_md.time_init = todc_time_init;
	ppc_md.set_rtc_time = todc_set_rtc_time;
	ppc_md.get_rtc_time = todc_get_rtc_time;
	ppc_md.nvram_read_val = todc_direct_read_val;
	ppc_md.nvram_write_val = todc_direct_write_val;
#endif
#ifdef CONFIG_KGDB
	ppc_md.early_serial_map = evb405ep_early_serial_map;
#endif
}
