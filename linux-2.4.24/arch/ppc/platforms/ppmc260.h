/*
 * arch/ppc/platforms/ppmc260.h
 * 
 * Definitions for Force Processor PMC260.
 *
 * Derived from: arch/ppc/platforms/ev64260.h
 * Author: Randy Vinson <rvinson@mvista.com>
 *
 * 2003 (c) MontaVista Software, Inc. This file is licensed under the terms
 * of the GNU General Public License version 2. This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */

/*
 * The GT64260 has 2 PCI buses each with 1 window from the CPU bus to
 * PCI I/O space and 4 windows from the CPU bus to PCI MEM space.
 * We'll only use one PCI MEM window on each PCI bus.
 *
 * This is the CPU physical memory map (windows must be at least 1MB and start
 * on a boundary that is a multiple of the window size):
 *
 * 	0xfff00000-0xffffffff		- Boot Flash (1MB)
 * 	0xf0100000-0xffefffff		- <hole>
 * 	0xf0000000-0xf00fffff		- GT64260 Registers
 * 	0x54000000-0xefffffff		- <hole>
 * 	0x52000000-0x53ffffff		- FLASH Bank 1 - 32MB (CS3)
 * 	0x50000000-0x51ffffff		- FLASH Bank 0 - 32MB (CS2)
 * 	0xa2000000-0x4fffffff		- <hole>
 * 	0xa1000000-0xa1ffffff		- PCI 1 I/O (defined in gt64260.h)
 * 	0xa0000000-0xa0ffffff		- PCI 0 I/O (defined in gt64260.h)
 * 	0x90000000-0x9fffffff		- PCI 1 MEM (defined in gt64260.h)
 * 	0x80000000-0x8fffffff		- PCI 0 MEM (defined in gt64260.h)
 */

#ifndef __PPC_PLATFORMS_PPMC260_H
#define __PPC_PLATFORMS_PPMC260_H

#ifndef	MAX
#define	MAX(a,b)	(((a) > (b)) ? (a) : (b))
#endif

/*
 * CPU Physical Memory Map setup.
 */
#define	PPMC260_BOOT_FLASH_SIZE_ACTUAL	0x00100000  /* 1MB AMD (BOOTCS)*/
#define PPMC260_USER_FLASH0_SIZE_ACTUAL	0x02000000  /* 32MB Intel (CS2) */
#define PPMC260_USER_FLASH1_SIZE_ACTUAL 0x02000000  /* 32MB Intel (CS3) */

#define	PPMC260_BOOT_FLASH_SIZE		MAX(GT64260_WINDOW_SIZE_MIN,	\
						PPMC260_BOOT_FLASH_SIZE_ACTUAL)
#define	PPMC260_USER_FLASH0_SIZE	MAX(GT64260_WINDOW_SIZE_MIN,	\
						PPMC260_USER_FLASH0_SIZE_ACTUAL)
#define	PPMC260_USER_FLASH1_SIZE	MAX(GT64260_WINDOW_SIZE_MIN,	\
						PPMC260_USER_FLASH1_SIZE_ACTUAL)

#define PPMC260_BOOT_FLASH_BASE		0xfff00000
#define PPMC260_USER_FLASH0_BASE	0x50000000
#define PPMC260_USER_FLASH1_BASE	(PPMC260_USER_FLASH0_BASE + \
					 PPMC260_USER_FLASH0_SIZE)

#define PPMC260_I2C_MAC_EEPROM_ADDR	0xa2

#ifndef __ASSEMBLY__
/*
 *  * Data passed from the boot loader to the kernel
 *   */
typedef struct board_info {
	unsigned char   bi_enetaddr[3][6];
} bd_t;

#endif /* __ASSEMBLY__ */

#endif /* __PPC_PLATFORMS_PPMC260_H */
