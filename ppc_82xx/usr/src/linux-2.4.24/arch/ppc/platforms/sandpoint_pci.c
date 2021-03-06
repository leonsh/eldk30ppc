/*
 * arch/ppc/platforms/sandpoint_pci.c
 * 
 * PCI setup routines for the Motorola SPS Sandpoint Test Platform
 *
 * Author: Mark A. Greer
 *         mgreer@mvista.com
 *
 * Copyright 2000-2002 MontaVista Software Inc.
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
#include <asm/mpc10x.h>

#include "sandpoint.h"

/*
 * Motorola SPS Sandpoint interrupt routing.
 */
static inline int
sandpoint_map_irq(struct pci_dev *dev, unsigned char idsel, unsigned char pin)
{
	static char pci_irq_table[][4] =
	/*
	 *	PCI IDSEL/INTPIN->INTLINE 
	 * 	   A   B   C   D
	 */
	{
		{ 16,  0,  0,  0 },	/* IDSEL 11 - i8259 on Windbond */
		{  0,  0,  0,  0 },	/* IDSEL 12 - unused */
		{ 17, 18, 19, 20 },     /* IDSEL 13 - PCI slot 1 */
		{ 18, 19, 20, 17 },     /* IDSEL 14 - PCI slot 2 */
		{ 19, 20, 17, 18 },     /* IDSEL 15 - PCI slot 3 */
		{ 20, 17, 18, 19 },     /* IDSEL 16 - PCI slot 4 */
	};

	const long min_idsel = 11, max_idsel = 16, irqs_per_slot = 4;
	return PCI_IRQ_TABLE_LOOKUP;
}

static void __init
sandpoint_setup_winbond_83553(struct pci_controller *hose)
{
	int		devfn;

	/*
	 * Route IDE interrupts directly to the 8259's IRQ 14 & 15.
	 * We can't route the IDE interrupt to PCI INTC# or INTD# because those
	 * woule interfere with the PMC's INTC# and INTD# lines.
	 */
	/*
	 * Winbond Fcn 0
	 */
	devfn = PCI_DEVFN(11,0);

	early_write_config_byte(hose,
				0,
				devfn,
				0x43, /* IDE Interrupt Routing Control */
				0xef);
	early_write_config_word(hose,
				0,
				devfn,
				0x44, /* PCI Interrupt Routing Control */
				0x0000);

	/* Want ISA memory cycles to be forwarded to PCI bus */
	early_write_config_byte(hose,
				0,
				devfn,
				0x48, /* ISA-to-PCI Addr Decoder Control */
				0xf0);

	/* Enable RTC and Keyboard address locations.  */
	early_write_config_byte(hose,
				0,
				devfn,
				0x4d,	/* Chip Select Control Register */
				0x00);

	/* Enable Port 92.  */
	early_write_config_byte(hose,
				0,
				devfn,
				0x4e,	/* AT System Control Register */
				0x06);
	/*
	 * Winbond Fcn 1
	 */
	devfn = PCI_DEVFN(11,1);

	/* Put IDE controller into native mode. */
	early_write_config_byte(hose,
				0,
				devfn,
				0x09,	/* Programming interface Register */
				0x8f);

	/* Init IRQ routing, enable both ports, disable fast 16 */
	early_write_config_dword(hose,
				0,
				devfn,
				0x40,	/* IDE Control/Status Register */
				0x00ff0011);
	return;
}

void __init
sandpoint_find_bridges(void)
{
	struct pci_controller	*hose;

	hose = pcibios_alloc_controller();

	if (!hose)
		return;

	hose->first_busno = 0;
	hose->last_busno = 0xff;

	if (mpc10x_bridge_init(hose,
			       MPC10X_MEM_MAP_B,
			       MPC10X_MEM_MAP_B,
			       MPC10X_MAPB_EUMB_BASE) == 0) {

		/* Do early winbond init, then scan PCI bus */
		sandpoint_setup_winbond_83553(hose);
		hose->last_busno = pciauto_bus_scan(hose, hose->first_busno);

		ppc_md.pcibios_fixup = NULL;
		ppc_md.pcibios_fixup_bus = NULL;
		ppc_md.pci_swizzle = common_swizzle;
		ppc_md.pci_map_irq = sandpoint_map_irq;
	}
	else {
		if (ppc_md.progress)
			ppc_md.progress("Bridge init failed", 0x100);
		printk("Host bridge init failed\n");
	}

	return;
}
