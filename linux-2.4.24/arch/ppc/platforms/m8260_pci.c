/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/machdep.h>
#include <asm/pci-bridge.h>
#include <asm/immap_8260.h>
#include <asm/mpc8260.h>

#include "m8260_pci.h"


/* PCI bus configuration registers.
 */
#define PCI_CLASS_BRIDGE_CTLR	0x06

#define pci_out_le32(reg,val)	out_le32 \
				((volatile unsigned *)(immr_addr|reg),val)


void m8260_pcibios_fixup(void)
{ 
    struct pci_dev *dev;

    pci_for_each_dev(dev)
    {
#ifdef CONFIG_ATC
	if (dev->bus->number == 0 && (dev->devfn & ~0x7) == (0x1E << 3))
	{
		dev->irq = SIU_INT_IRQ2;
	}
#endif
#ifdef CONFIG_PM826
	if (dev->bus->number == 0 && (dev->devfn & ~0x7) == (0xD << 3))
	{
		dev->irq = SIU_INT_IRQ5;
	}
#endif
    }
}

void __init m8260_find_bridges(void)
{
    extern int pci_assign_all_busses;
    u32 immr_addr = (u32) immr;
    volatile immap_t *immap = immr;
    struct pci_controller * hose;
    u16 tempShort;

    pci_assign_all_busses = 1;
    
    hose = pcibios_alloc_controller();

    if (!hose)
	return;

    hose->first_busno = 0;
    hose->last_busno = 0xff;

    isa_io_base = (ulong) ioremap(0xa0000000, 0x1000) - PCIBIOS_MIN_IO;

    hose->io_base_virt = (void *)isa_io_base;

    setup_indirect_pci(hose, immr_addr + PCI_CFG_ADDR_REG,
	                         immr_addr + PCI_CFG_DATA_REG);

#ifndef CONFIG_ATC 	/* already done in U-Boot */
    /* 
     * Setting required to enable IRQ1-IRQ7 (SIUMCR [DPPC]), 
     * and local bus for PCI (SIUMCR [LBPC]).
     */
    immap->im_siu_conf.sc_siumcr = 0x00640000;
#endif

    /* Make PCI lowest priority */
    /* Each 4 bits is a device bus request  and the MS 4bits 
       is highest priority */
    /* Bus               4bit value 
	   ---               ----------
       CPM high          0b0000
       CPM middle        0b0001
	   CPM low           0b0010
       PCI reguest       0b0011
       Reserved          0b0100
       Reserved          0b0101
       Internal Core     0b0110
       External Master 1 0b0111
       External Master 2 0b1000
       External Master 3 0b1001
       The rest are reserved */
    immap->im_siu_conf.sc_ppc_alrh = 0x61207893;

    /* Park bus on core while modifying PCI Bus accesses */
    immap->im_siu_conf.sc_ppc_acr = 0x6;

    /* 
     * Set up master window that allows the CPU to access PCI space. This 
     * window is set up using the first SIU PCIBR registers.
     */
    *(volatile unsigned long*)(immr_addr + M8265_PCIMSK0) = PCIMSK0_MASK;
    *(volatile unsigned long*)(immr_addr + M8265_PCIBR0) =
	    PCI_MSTR_LOCAL | PCIBR_ENABLE;

    /* Release PCI RST (by default the PCI RST signal is held low)  */
    pci_out_le32 (PCI_GCR_REG, PCIGCR_PCI_BUS_EN);

    /* give it some time */
    udelay(1000);

    /* 
     * Set up master window that allows the CPU to access PCI Memory (prefetch) 
     * space. This window is set up using the first set of Outbound ATU registers.
     */
    pci_out_le32 (POTAR_REG0, PCI_MSTR_MEM_BUS >> 12);      /* PCI base */
    pci_out_le32 (POBAR_REG0, PCI_MSTR_MEM_LOCAL >> 12);    /* Local base */
    pci_out_le32 (POCMR_REG0, POCMR0_MASK_ATTRIB);    /* Size & attribute */

    /* 
     * Set up master window that allows the CPU to access PCI Memory (non-prefetch) 
     * space. This window is set up using the second set of Outbound ATU registers.
     */
    pci_out_le32 (POTAR_REG1, PCI_MSTR_MEMIO_BUS >> 12);    /* PCI base */
    pci_out_le32 (POBAR_REG1, PCI_MSTR_MEMIO_LOCAL >> 12);  /* Local base */
    pci_out_le32 (POCMR_REG1, POCMR1_MASK_ATTRIB);    /* Size & attribute */
    
    /* 
     * Set up master window that allows the CPU to access PCI IO space. This window
     * is set up using the third set of Outbound ATU registers.
     */
    pci_out_le32 (POTAR_REG2, PCI_MSTR_IO_BUS >> 12);       /* PCI base */
    pci_out_le32 (POBAR_REG2, PCI_MSTR_IO_LOCAL >> 12);     /* Local base */
    pci_out_le32 (POCMR_REG2, POCMR2_MASK_ATTRIB);    /* Size & attribute */

    /* 
     * Set up slave window that allows PCI masters to access MPC826x local memory. 
     * This window is set up using the first set of Inbound ATU registers
     */
    pci_out_le32 (PITAR_REG0, PCI_SLV_MEM_LOCAL >> 12);     /* Local base */
    pci_out_le32 (PIBAR_REG0, PCI_SLV_MEM_BUS >> 12);       /* PCI base */
    pci_out_le32 (PICMR_REG0, PICMR0_MASK_ATTRIB);    /* Size & attribute */

    /* See above for description - puts PCI request as highest priority */
    immap->im_siu_conf.sc_ppc_alrh = 0x03124567;

    /* Park the bus on the PCI */
    immap->im_siu_conf.sc_ppc_acr = PPC_ACR_BUS_PARK_PCI;

    /* Host mode - specify the bridge as a host-PCI bridge */
    early_write_config_byte(hose, 0, 0, PCI_CLASS_CODE,
	                           PCI_CLASS_BRIDGE_CTLR);

    /* Enable the host bridge to be a master on the PCI bus, and to act as a PCI memory target */

    early_read_config_word(hose, 0, 0, PCI_COMMAND, &tempShort);
    early_write_config_word(hose, 0, 0, PCI_COMMAND,
		         tempShort | PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY);

    ppc_md.pcibios_fixup = m8260_pcibios_fixup;
}
