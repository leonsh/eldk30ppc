/*
 * arch/ppc/platforms/cpci690.h
 *
 * Definitions for Force CPCI690
 *
 * Author: Mark A. Greer <mgreer@mvista.com>
 *
 * 2003 (c) MontaVista, Software, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

/*
 * The GT64260 has 2 PCI buses each with 1 window from the CPU bus to
 * PCI I/O space and 4 windows from the CPU bus to PCI MEM space.
 * We'll only use one PCI MEM window on each PCI bus.
 */

#ifndef __PPC_PLATFORMS_CPCI690_H
#define __PPC_PLATFORMS_CPCI690_H

#ifndef	MAX
#define	MAX(a,b)	(((a) > (b)) ? (a) : (b))
#endif
#ifndef	MIN
#define	MIN(a,b)	(((a) < (b)) ? (a) : (b))
#endif

#define KB              (1024UL)
#define MB              (KB * KB)
#define GB              (MB * KB)

/*
 * Define bd_t to pass in the MAC addresses used by the GT64260's enet ctlrs.
 */
#define	CPCI690_BI_MAGIC		0xFE8765DC

typedef struct board_info {
	u32	bi_magic;
	u8	bi_enetaddr[3][6];
} bd_t;

/*
 * CPU Physical Memory Map setup.
 */
#define	CPCI690_REG_BASE		0xf0000000
#define	CPCI690_REG_SIZE_ACTUAL		0x08
#define	CPCI690_REG_SIZE		MAX(GT64260_WINDOW_SIZE_MIN,	\
						CPCI690_REG_SIZE_ACTUAL)
/* Board reg offsets */
#define	CPCI690_LED_CNTL		0x00
#define	CPCI690_SW_RESET		0x01
#define	CPCI690_MISC_STATUS		0x02
#define	CPCI690_SWITCH_STATUS		0x03
#define	CPCI690_MEM_CTLR		0x04
#define	CPCI690_LAST_RESET_1		0x05
#define	CPCI690_LAST_RESET_2		0x06

#define	CPCI690_MAC_OFFSET		0x7c10 /* MAC addrs in RTC NVRAM */

#define	CPCI690_TODC_BASE		0xf0100000
#define	CPCI690_TODC_SIZE_ACTUAL	0x8000	/* Size or NVRAM + RTC regs */
#define	CPCI690_TODC_SIZE		MAX(GT64260_WINDOW_SIZE_MIN,	\
						CPCI690_TODC_SIZE_ACTUAL)

#define	CPCI690_IPMI_BASE		0xf0200000
#define	CPCI690_IPMI_SIZE_ACTUAL	0x10	/* 16 bytes of IPMI regs */
#define	CPCI690_IPMI_SIZE		MAX(GT64260_WINDOW_SIZE_MIN,	\
						CPCI690_IPMI_SIZE_ACTUAL)

#endif /* __PPC_PLATFORMS_CPCI690_H */
