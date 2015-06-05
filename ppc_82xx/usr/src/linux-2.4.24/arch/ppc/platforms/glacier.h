/*
 * arch/ppc/platforms/glacier.h
 *
 * Definitions for the Motorola Glacier platform
 *
 * Author: Dale Farnsworth	<dfarnsworth@mvista.com>
 *
 * 2003 (c) MontaVista, Software, Inc.  This file is licensed under the terms
 * of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */
 /*
 * From Processor to PCI:
 *   PCI Mem Space: 0x40000000 - 0x50000000 -> 0x40000000 - 0x50000000 (256 MB)
 *   PCI I/O Space: 0x50000000 - 0x51000000 -> 0x50000000 - 0x51000000 (16 MB)
 *
 * From PCI to Processor:
 *   System Memory: 0x00000000 -> 0x00000000
 */

#ifndef __PLATFORMS_GLACIER_H
#define __PLATFORMS_GLACIER_H

#define MPC5xxx_PROC_PCI_MEM_START	0x40000000U
#define MPC5xxx_PROC_PCI_MEM_END	0x4fffffffU
#define MPC5xxx_PCI_MEM_START		0x40000000U
#define MPC5xxx_PCI_MEM_END		0x4fffffffU

#define MPC5xxx_PROC_PCI_IO_START	0x50000000U
#define MPC5xxx_PROC_PCI_IO_END		0x50ffffffU
#define MPC5xxx_PCI_IO_START		0x50000000U
#define MPC5xxx_PCI_IO_END		0x50ffffffU

#define MPC5xxx_PCI_DRAM_OFFSET		0x00000000U
#define MPC5xxx_PCI_PHY_MEM_OFFSET	0x00000000U

#define MPC5xxx_ISA_IO_BASE		MPC5xxx_PROC_PCI_IO_START
#define MPC5xxx_ISA_MEM_BASE		MPC5xxx_PROC_PCI_MEM_START

#define RS_TABLE_SIZE  3

#ifdef CONFIG_UBOOT
#define	MPC5xxx_INIT_BAUD_BASE		0
#else
#define	MPC5xxx_INIT_BAUD_BASE		CONFIG_PPC_5xxx_IPBFREQ
#endif

#define STD_PSC_OP(num)					\
{							\
	.magic = 0,					\
	.baud_base = MPC5xxx_INIT_BAUD_BASE,		\
	.irq = MPC5xxx_PSC##num##_IRQ,			\
	.flags = ASYNC_BOOT_AUTOCONF|ASYNC_SKIP_TEST, 	\
	.iomem_base = (void *)MPC5xxx_PSC##num##,	\
	.io_type = SERIAL_IO_MEM,			\
},

#define SERIAL_PORT_DFNS	\
	STD_PSC_OP(1)		\
	STD_PSC_OP(2)		\
	STD_PSC_OP(3)

#endif /* __PLATFORMS_GLACIER_H */