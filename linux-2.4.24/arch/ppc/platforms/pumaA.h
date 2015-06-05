/*
 * arch/ppc/platforms/pumaA.h
 * 
 * Definitions for Momentum PUMA-A (PPC 750) board
 * Adapted from ev64260.h (Marvell/Galileo EV-64260-BP Evaluation Board)
 *
 * Author: Gary Thomas <gary@mlbassoc.com>
 * Copyright (C) 2003 MLB Associates
 *
 * Original version
 * Author: Mark A. Greer <mgreer@mvista.com>
 * Copyright 2001 MontaVista Software Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

/*
 * The GT64260 has 2 PCI buses each with 1 window from the CPU bus to
 * PCI I/O space and 4 windows from the CPU bus to PCI MEM space.
 * We'll only use one PCI MEM window on each PCI bus.
 *
 * This is the CPU physical memory map (windows must be at least 1MB and start
 * on a boundary that is a multiple of the window size):
 *
 * 	0xfc000000-0xffffffff		- External FLASH on device module
 * 	0xfbf00000-0xfbffffff		- Embedded (on board) FLASH
 * 	0xfbe00000-0xfbefffff		- GT64260 Registers
 * 	0xfbd00000-0xfbdfffff		- External SRAM on device module
 * 	0xfbc00000-0xfbcfffff		- TODC chip on device module
 * 	0xa1000000-0xa1ffffff		- PCI 1 I/O (defined in gt64260.h)
 * 	0xa0000000-0xa0ffffff		- PCI 0 I/O (defined in gt64260.h)
 * 	0x90000000-0x9fffffff		- PCI 1 MEM (defined in gt64260.h)
 * 	0x80000000-0x8fffffff		- PCI 0 MEM (defined in gt64260.h)
 */

#ifndef __PPC_PLATFORMS_PUMA_H
#define __PPC_PLATFORMS_PUMA_H

#ifndef	MAX
#define	MAX(a,b)	(((a) > (b)) ? (a) : (b))
#endif

/*
 * CPU Physical Memory Map setup.
 */
#define PUMA_CPLD_BASE                  0x70100000
#define	PUMA_TODC_BASE			0x70200000
#define	PUMA_EXT_FLASH_BASE		0x70400000
#define	PUMA_EMB_FLASH_BASE		0xfbf00000
#define	PUMA_BRIDGE_REG_BASE		0xfbe00000

// Serial port setup (via bridge)
#define GT64260_CLK_SOURCE   8           // clock routing (determined by hardware)
#define GT64260_CAN_READ_REGS

#define PUMA_CPLD_SIZE_ACTUAL           0x00000007
#define	PUMA_EXT_FLASH_SIZE_ACTUAL	0x01000000  /* <= 64MB Extern FLASH */
#define	PUMA_EMB_FLASH_SIZE_ACTUAL	0x00080000  /* 512KB of Embed FLASH */
#define	PUMA_TODC_SIZE_ACTUAL		0x00000080  /* 128 bytes for TODC/NVRAM */

#define	PUMA_CPLD_SIZE			MAX(GT64260_WINDOW_SIZE_MIN,	\
						PUMA_CPLD_SIZE_ACTUAL)
#define	PUMA_EXT_FLASH_SIZE		MAX(GT64260_WINDOW_SIZE_MIN,	\
						PUMA_EXT_FLASH_SIZE_ACTUAL)
#define	PUMA_EMB_FLASH_SIZE		MAX(GT64260_WINDOW_SIZE_MIN,	\
						PUMA_EMB_FLASH_SIZE_ACTUAL)
#define	PUMA_TODC_SIZE			MAX(GT64260_WINDOW_SIZE_MIN,	\
						PUMA_TODC_SIZE_ACTUAL)

#ifndef __ASSEMBLY__
// Layout of CPLD device
struct puma_cpld {
    volatile unsigned char board_revision;
    volatile unsigned char cpld_revision;
    volatile unsigned char memory_size;
    volatile unsigned char reset_status;
    volatile unsigned char flash_ctl;
    volatile unsigned char monarch_functions;
    volatile unsigned char gpio_ctl;
};
#endif


// Only a single serial port is available
#define RS_TABLE_SIZE	1

#endif /* __PPC_PLATFORMS_PUMA_H */
