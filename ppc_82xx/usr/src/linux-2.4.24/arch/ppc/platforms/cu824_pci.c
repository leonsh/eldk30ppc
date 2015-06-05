/* 
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

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

void cu824_pcibios_fixup(void)
{
        struct pci_dev *dev;

	pci_for_each_dev(dev)
	{
		if (dev->devfn == 0x78)
		{
			dev->irq = 1;
			pcibios_write_config_byte(dev->bus->number, dev->devfn,
			                          PCI_INTERRUPT_LINE, dev->irq);
		}
	}
}

void __init cu824_find_bridges(void)
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

		hose->mem_resources[0].end = 0xffffffff;
	
		hose->last_busno = pciauto_bus_scan(hose, hose->first_busno);

		ppc_md.pcibios_fixup = cu824_pcibios_fixup;
		ppc_md.pcibios_fixup_bus = NULL;
	}
	else {
		if (ppc_md.progress)
			ppc_md.progress("Bridge init failed", 0x100);
		printk("Host bridge init failed\n");
	}

	return;
}
