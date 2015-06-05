
#ifndef _PPC_KERNEL_M8260_PCI_H
#define _PPC_KERNEL_M8260_PCI_H

#include <asm/m8260_pci.h>

/*
 *   Local->PCI map (from CPU)                             controlled by
 *   MPC826x master window
 *
 *   0x80000000 - 0xBFFFFFFF    Total CPU2PCI space        PCIBR0
 *                       
 *   0x80000000 - 0x8FFFFFFF    PCI Mem with prefetch      (Outbound ATU #1)
 *   0x90000000 - 0x9FFFFFFF    PCI Mem w/o  prefetch      (Outbound ATU #2)
 *   0xA0000000 - 0xAFFFFFFF    32-bit PCI IO              (Outbound ATU #3)
 *                      
 *   PCI->Local map (from PCI)
 *   MPC826x slave window                                  controlled by
 *
 *   0x00000000 - 0x07FFFFFF    MPC826x local memory       (Inbound ATU #1)
 */

/* 
 * Slave window that allows PCI masters to access MPC826x local memory. 
 * This window is set up using the first set of Inbound ATU registers
 */

#define PCI_SLV_MEM_LOCAL	0x00000000		/* Local base */
#define PCI_SLV_MEM_BUS		0x00000000		/* PCI base */
#define PICMR0_MASK_ATTRIB	(PICMR_MASK_512MB | PICMR_ENABLE | \
                          	 PICMR_PREFETCH_EN)

/* 
 * This is the window that allows the CPU to access PCI address space.
 * It will be setup with the SIU PCIBR0 register. All three PCI master
 * windows, which allow the CPU to access PCI prefetch, non prefetch,
 * and IO space (see below), must all fit within this window. 
 */

#define PCI_MSTR_LOCAL		0x80000000		/* Local base */
#define PCIMSK0_MASK		PCIMSK_1GB		/* Size of window */

/* 
 * Master window that allows the CPU to access PCI Memory (prefetch).
 * This window will be setup with the first set of Outbound ATU registers
 * in the bridge.
 */

#define PCI_MSTR_MEM_LOCAL	0x10000000          /* Local base */
#define PCI_MSTR_MEM_BUS	0x10000000          /* PCI base   */
#define CPU_PCI_MEM_START	PCI_MSTR_MEM_LOCAL
#define PCI_MSTR_MEM_SIZE	0x10000000          /* 256MB */
#define POCMR0_MASK_ATTRIB	(POCMR_MASK_256MB | POCMR_ENABLE | POCMR_PREFETCH_EN)

/* 
 * Master window that allows the CPU to access PCI Memory (non-prefetch).
 * This window will be setup with the second set of Outbound ATU registers
 * in the bridge.
 */

#define PCI_MSTR_MEMIO_LOCAL    0x90000000          /* Local base */
#define PCI_MSTR_MEMIO_BUS      0x90000000          /* PCI base   */
#define CPU_PCI_MEMIO_START     PCI_MSTR_MEMIO_LOCAL
#define PCI_MSTR_MEMIO_SIZE     0x10000000          /* 256MB */
#define POCMR1_MASK_ATTRIB      (POCMR_MASK_256MB | POCMR_ENABLE)

/* 
 * Master window that allows the CPU to access PCI IO space.
 * This window will be setup with the third set of Outbound ATU registers
 * in the bridge.
 */

#define PCI_MSTR_IO_LOCAL       0xA0000000          /* Local base */
#ifdef CONFIG_ATC
#define PCI_MSTR_IO_BUS         0x00000000          /* PCI base   */
#else
#define PCI_MSTR_IO_BUS         0xA0000000          /* PCI base   */
#endif
#define CPU_PCI_IO_START        PCI_MSTR_IO_LOCAL
#define PCI_MSTR_IO_SIZE        0x10000000          /* 256MB */
#define POCMR2_MASK_ATTRIB      (POCMR_MASK_256MB | POCMR_ENABLE | POCMR_PCI_IO)


#define _IO_BASE		isa_io_base
#define PCIBIOS_MIN_IO		PCI_MSTR_IO_BUS
#define PCIBIOS_MIN_MEM		0x80000000

#endif /* _PPC_KERNEL_M8260_PCI_H */
