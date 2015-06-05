/*
 * arch/ppc/platforms/mcpn765_pci.c
 * 
 * PCI setup routines for the Motorola MCG MCPN765 cPCI board.
 *
 * Author: Mark A. Greer
 *         mgreer@mvista.com
 *
 * Copyright 2001 MontaVista Software Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/slab.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/machdep.h>
#include <asm/pci-bridge.h>
#include <asm/pplus.h>

#include "mcpn765.h"


/*
 * Motorola MCG MCPN765 interrupt routing.
 */
static inline int
mcpn765_map_irq(struct pci_dev *dev, unsigned char idsel, unsigned char pin)
{
	static char pci_irq_table[][4] =
	/*
	 *	PCI IDSEL/INTPIN->INTLINE 
	 * 	   A   B   C   D
	 */
	{
		{ 14,  0,  0,  0 },	/* IDSEL 11 - have to manually set */
		{  0,  0,  0,  0 },	/* IDSEL 12 - unused */
		{  0,  0,  0,  0 },	/* IDSEL 13 - unused */
		{ 18,  0,  0,  0 },	/* IDSEL 14 - Enet 0 */
		{  0,  0,  0,  0 },	/* IDSEL 15 - unused */
		{ 25, 26, 27, 28 },	/* IDSEL 16 - PMC Slot 1 */
		{ 28, 25, 26, 27 },	/* IDSEL 17 - PMC Slot 2 */
		{  0,  0,  0,  0 },	/* IDSEL 18 - PMC 2B Connector XXXX */
		{ 29,  0,  0,  0 },	/* IDSEL 19 - Enet 1 */
		{ 20,  0,  0,  0 },	/* IDSEL 20 - 21554 cPCI bridge */
	};

	const long min_idsel = 11, max_idsel = 20, irqs_per_slot = 4;
	return PCI_IRQ_TABLE_LOOKUP;
}

void __init
mcpn765_set_VIA_IDE_legacy(void)
{
	unsigned short vend, dev;

	early_read_config_word(0, 0, PCI_DEVFN(0xb, 1), PCI_VENDOR_ID, &vend);
	early_read_config_word(0, 0, PCI_DEVFN(0xb, 1), PCI_DEVICE_ID, &dev);

	if ((vend == PCI_VENDOR_ID_VIA) &&
	    (dev == PCI_DEVICE_ID_VIA_82C586_1)) {

		unsigned char temp;

		/* put back original "standard" port base addresses */
		early_write_config_dword(0, 0, PCI_DEVFN(0xb, 1),
				         PCI_BASE_ADDRESS_0, 0x1f1);
		early_write_config_dword(0, 0, PCI_DEVFN(0xb, 1),
				         PCI_BASE_ADDRESS_1, 0x3f5);
		early_write_config_dword(0, 0, PCI_DEVFN(0xb, 1),
				         PCI_BASE_ADDRESS_2, 0x171);
		early_write_config_dword(0, 0, PCI_DEVFN(0xb, 1),
				         PCI_BASE_ADDRESS_3, 0x375);
		early_write_config_dword(0, 0, PCI_DEVFN(0xb, 1),
				         PCI_BASE_ADDRESS_4, 0xcc01);

		/* put into legacy mode */
		early_read_config_byte(0, 0, PCI_DEVFN(0xb, 1), PCI_CLASS_PROG,
				       &temp);
		temp &= ~0x05;
		early_write_config_byte(0, 0, PCI_DEVFN(0xb, 1), PCI_CLASS_PROG,
					temp);
	}
}

void
mcpn765_set_VIA_IDE_native(void)
{
	unsigned short vend, dev;

	early_read_config_word(0, 0, PCI_DEVFN(0xb, 1), PCI_VENDOR_ID, &vend);
	early_read_config_word(0, 0, PCI_DEVFN(0xb, 1), PCI_DEVICE_ID, &dev);

	if ((vend == PCI_VENDOR_ID_VIA) &&
	    (dev == PCI_DEVICE_ID_VIA_82C586_1)) {

		unsigned char temp;

		/* put into native mode */
		early_read_config_byte(0, 0, PCI_DEVFN(0xb, 1), PCI_CLASS_PROG,
				       &temp);
		temp |= 0x05;
		early_write_config_byte(0, 0, PCI_DEVFN(0xb, 1), PCI_CLASS_PROG,
					temp);
	}
}
void __init
mcpn765_find_bridges(void)
{
	struct pci_controller	*hose;

	hose = pcibios_alloc_controller();

	if (!hose)
		return;

	hose->first_busno = 0;
	hose->last_busno = 0xff;
	hose->pci_mem_offset = MCPN765_PCI_PHY_MEM_OFFSET;

	pci_init_resource(&hose->io_resource,
			MCPN765_PCI_IO_START,
			MCPN765_PCI_IO_END,
			IORESOURCE_IO,
			"PCI host bridge");

	pci_init_resource(&hose->mem_resources[0],
			MCPN765_PCI_MEM_START,
			MCPN765_PCI_MEM_END,
			IORESOURCE_MEM,
			"PCI host bridge");

	hose->io_space.start = MCPN765_PCI_IO_START;
	hose->io_space.end = MCPN765_PCI_IO_END;
	hose->mem_space.start = MCPN765_PCI_MEM_START;
	hose->mem_space.end = MCPN765_PCI_MEM_END - PPLUS_MPIC_SIZE;

	if (pplus_init(hose,
		       MCPN765_HAWK_PPC_REG_BASE,
		       MCPN765_PROC_PCI_MEM_START,
		       MCPN765_PROC_PCI_MEM_END - PPLUS_MPIC_SIZE,
		       MCPN765_PROC_PCI_IO_START,
		       MCPN765_PROC_PCI_IO_END,
		       MCPN765_PCI_MEM_END - PPLUS_MPIC_SIZE + 1) != 0) {
		printk("Could not initialize HAWK bridge\n");
	}

	/* VIA IDE BAR decoders are only 16-bits wide. PCI Auto Config
	 * will reassign the bars outside of 16-bit I/O space, which will 
	 * "break" things. To prevent this, we'll set the IDE chip into
	 * legacy mode and seed the bars with their legacy addresses (in 16-bit
	 * I/O space). The Auto Config code will skip the IDE contoller in 
	 * legacy mode, so our bar values will stick.
	 */
	mcpn765_set_VIA_IDE_legacy();

	hose->last_busno = pciauto_bus_scan(hose, hose->first_busno);

	/* Now that we've got 16-bit addresses in the bars, we can switch the
	 * IDE controller back into native mode so we can do "modern" resource
	 * and interrupt management.
	 */
	mcpn765_set_VIA_IDE_native();

	ppc_md.pcibios_fixup = NULL;
	ppc_md.pcibios_fixup_bus = NULL;
	ppc_md.pci_swizzle = common_swizzle;
	ppc_md.pci_map_irq = mcpn765_map_irq;

	return;
}
