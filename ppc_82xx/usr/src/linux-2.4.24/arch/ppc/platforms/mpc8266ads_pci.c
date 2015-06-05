/*
 * arch/ppc/platforms/mpc8266ads_pci.c
 * 
 * PCI setup routines for the Motorola SPS MPC8266ADS-PCI reference board.
 *
 * Author:  andy_lowe@mvista.com
 *
 * 2003 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/irq.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/machdep.h>
#include <asm/pci-bridge.h>
#include <asm/immap_8260.h>
#include <asm/m8260_pci.h>

#include "ads8260.h"

extern void setup_m8260_indirect_pci(struct pci_controller* hose,
				     u32 cfg_addr,
				     u32 cfg_data);

/*
 * interrupt routing
 */

static inline int
mpc8266ads_map_irq(struct pci_dev *dev, unsigned char idsel, unsigned char pin)
{
	static char pci_irq_table[][4] =
	/*
	 *	PCI IDSEL/INTPIN->INTLINE
	 * 	  A      B      C      D
	 */
	{
		{ PIRQA, PIRQB, PIRQC, PIRQD },	/* IDSEL 22 - PCI slot 0 */
		{ PIRQD, PIRQA, PIRQB, PIRQC },	/* IDSEL 23 - PCI slot 1 */
		{ PIRQC, PIRQD, PIRQA, PIRQB },	/* IDSEL 24 - PCI slot 2 */
	};

	const long min_idsel = 22, max_idsel = 24, irqs_per_slot = 4;
	return PCI_IRQ_TABLE_LOOKUP;
}

static void
mpc8266ads_mask_irq(unsigned int irq)
{
	int bit = irq - NR_SIU_INTS;

	*(volatile unsigned long *) PCI_INT_MASK_REG |=  (1 << (31 - bit));
	return;
}

static void
mpc8266ads_unmask_irq(unsigned int irq)
{
	int bit = irq - NR_SIU_INTS;

	*(volatile unsigned long *) PCI_INT_MASK_REG &= ~(1 << (31 - bit));
	return;
}

static void
mpc8266ads_mask_and_ack(unsigned int irq)
{
	int bit = irq - NR_SIU_INTS;

	*(volatile unsigned long *) PCI_INT_MASK_REG |=  (1 << (31 - bit));
	return;
}

static void
mpc8266ads_end_irq(unsigned int irq)
{
	int bit = irq - NR_SIU_INTS;

	*(volatile unsigned long *) PCI_INT_MASK_REG &= ~(1 << (31 - bit));
	return;
}

struct hw_interrupt_type mpc8266ads_ic = {
	" MPC8266ADS",
	NULL,
	NULL,
	mpc8266ads_unmask_irq,
	mpc8266ads_mask_irq,
	mpc8266ads_mask_and_ack,
	mpc8266ads_end_irq,
	0
};

static void
pci_irq_demux(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned long stat, mask, pend;
	int bit;

	for(;;) {
		stat = *(volatile unsigned long *) PCI_INT_STAT_REG;
		mask = *(volatile unsigned long *) PCI_INT_MASK_REG;
		pend = stat & ~mask & 0xf0000000;
		if (!pend)
			break;
		for (bit = 0; pend != 0; ++bit, pend <<= 1) {
			if (pend & 0x80000000)
				ppc_irq_dispatch_handler(regs, NR_SIU_INTS + bit);
		}
	}

	return;
}

void
mpc8266ads_init_irq(void)
{
	int irq;

	for (irq = NR_SIU_INTS; irq < NR_SIU_INTS + 4; irq++)
                irq_desc[irq].handler = &mpc8266ads_ic;

	/* make IRQ6 level sensitive */
	((volatile immap_t *) IMAP_ADDR)->im_intctl.ic_siexr &=
		~(1 << (14 - (SIU_INT_IRQ6 - SIU_INT_IRQ1)));
	
	/* mask all PCI interrupts */
	*(volatile unsigned long *) PCI_INT_MASK_REG |= 0xfff00000;

	/* install the demultiplexer for the PCI cascade interrupt */
	if (request_irq(SIU_INT_IRQ6, pci_irq_demux, SA_INTERRUPT, 
		"PCI IRQ demux", 0))
	{
		printk("Installation of PCI IRQ demux handler failed.\n");
	}
	return;
}

static int                     
mpc8266ads_exclude_device(u_char bus, u_char devfn)
{
	return PCIBIOS_SUCCESSFUL;
}

void __init
m8260_find_bridges(void)
{
	struct pci_controller *hose;
	int host_bridge;

	hose = pcibios_alloc_controller();

	if (!hose)
		return;

	hose->first_busno = 0;
	hose->last_busno = 0xff;

	setup_m8260_indirect_pci(hose,
				 IMAP_ADDR + PCI_CFG_ADDR_REG, 
				 IMAP_ADDR + PCI_CFG_DATA_REG);
	hose->set_cfg_type = 1;

	/* Make sure it is a supported bridge */
	early_read_config_dword(hose,
			        0,
			        PCI_DEVFN(0,0),
			        PCI_VENDOR_ID,
			        &host_bridge);

	switch (host_bridge) {
		case PCI_DEVICE_ID_MPC8265:
			break;
		default:
			printk("Attempting to use unrecognized host bridge ID"
			       " 0x%08x.\n", host_bridge);
			break;
	}

	hose->pci_mem_offset = PCI_MSTR_MEM_LOCAL - PCI_MSTR_MEM_BUS;
	hose->io_space.start = PCI_MSTR_IO_BUS;
	hose->io_space.end = PCI_MSTR_IO_BUS + PCI_MSTR_IO_SIZE - 1;
	hose->mem_space.start = PCI_MSTR_MEM_BUS;
	hose->mem_space.end = PCI_MSTR_MEMIO_BUS + PCI_MSTR_MEMIO_SIZE - 1;
	hose->io_base_virt = (void *)PCI_MSTR_IO_LOCAL;

	/* setup resources */
	pci_init_resource(&hose->io_resource,
			  PCI_MSTR_IO_BUS,
			  PCI_MSTR_IO_BUS + PCI_MSTR_IO_SIZE - 1,
			  IORESOURCE_IO,
			  "PCI host bridge");
	pci_init_resource(&hose->mem_resources[0],
			  PCI_MSTR_MEMIO_BUS,
			  PCI_MSTR_MEMIO_BUS + PCI_MSTR_MEMIO_SIZE - 1,
			  IORESOURCE_MEM,
			  "PCI host bridge");

	ppc_md.pci_exclude_device = mpc8266ads_exclude_device;
	hose->last_busno = pciauto_bus_scan(hose, hose->first_busno);

	ppc_md.pcibios_fixup = NULL;
	ppc_md.pcibios_fixup_bus = NULL;
	ppc_md.pci_swizzle = common_swizzle;
	ppc_md.pci_map_irq = mpc8266ads_map_irq;

	return;
}
