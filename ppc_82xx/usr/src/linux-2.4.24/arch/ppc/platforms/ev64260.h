/*
 * arch/ppc/platforms/ev64260.h
 * 
 * Definitions for Marvell/Galileo EV-64260-BP Evaluation Board.
 *
 * Author: Mark A. Greer <mgreer@mvista.com>
 *
 * 2001-2002 (c) MontaVista, Software, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
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
 * 	0xfbe00000-0xfbefffff		- GT64260 Registers (preferably)
 * 					  but really a config option
 * 	0xfbd00000-0xfbdfffff		- External SRAM on device module
 * 	0xfbc00000-0xfbcfffff		- TODC chip on device module
 * 	0xfbb00000-0xfbbfffff		- External UART on device module
 * 	0xa2000000-0xfbafffff		- <hole>
 * 	0xa1000000-0xa1ffffff		- PCI 1 I/O (defined in gt64260.h)
 * 	0xa0000000-0xa0ffffff		- PCI 0 I/O (defined in gt64260.h)
 * 	0x90000000-0x9fffffff		- PCI 1 MEM (defined in gt64260.h)
 * 	0x80000000-0x8fffffff		- PCI 0 MEM (defined in gt64260.h)
 *
 * 	XXX: FIXME: PPCBoot uses EVB64260_BRIDGE_REG_BASE as 0xf8000000
 * 	  This happens to not conflict with the current memory map, but
 * 	  it should get fixed eventually anyway.
 */

#ifndef __PPC_PLATFORMS_EV64260_H
#define __PPC_PLATFORMS_EV64260_H

#ifndef	MAX
#define	MAX(a,b)	(((a) > (b)) ? (a) : (b))
#endif

#ifdef CONFIG_EV64260
/*
 * CPU Physical Memory Map setup.
 */
#define	EV64260_EXT_FLASH_BASE		0xfc000000
#define	EV64260_EMB_FLASH_BASE		0xfbf00000
#define	EV64260_EXT_SRAM_BASE		0xfbd00000
#define	EV64260_TODC_BASE		0xfbc00000
#define	EV64260_UART_BASE		0xfbb00000

#define	EV64260_EXT_FLASH_SIZE_ACTUAL	0x04000000  /* <= 64MB Extern FLASH */
#define	EV64260_EMB_FLASH_SIZE_ACTUAL	0x00080000  /* 512KB of Embed FLASH */
#define	EV64260_EXT_SRAM_SIZE_ACTUAL	0x00100000  /* 1MB SDRAM */
#define	EV64260_TODC_SIZE_ACTUAL	0x00000020  /* 32 bytes for TODC */
#define	EV64260_UART_SIZE_ACTUAL	0x00000040  /* 64 bytes for DUART */

#define	EV64260_EXT_FLASH_SIZE		MAX(GT64260_WINDOW_SIZE_MIN,	\
						EV64260_EXT_FLASH_SIZE_ACTUAL)
#define	EV64260_EMB_FLASH_SIZE		MAX(GT64260_WINDOW_SIZE_MIN,	\
						EV64260_EMB_FLASH_SIZE_ACTUAL)
#define	EV64260_EXT_SRAM_SIZE		MAX(GT64260_WINDOW_SIZE_MIN,	\
						EV64260_EXT_SRAM_SIZE_ACTUAL)
#define	EV64260_TODC_SIZE		MAX(GT64260_WINDOW_SIZE_MIN,	\
						EV64260_TODC_SIZE_ACTUAL)
#if 0	/* blows up assembler in bootloader */
#define	EV64260_UART_SIZE		MAX(GT64260_WINDOW_SIZE_MIN,	\
						EV64260_UART_SIZE_ACTUAL)
#else
#define	EV64260_UART_SIZE		GT64260_WINDOW_SIZE_MIN
#endif
#define	EV64260_UART_END		((EV64260_UART_BASE +		\
					EV64260_UART_SIZE - 1) & 0xfff00000)

/*
 * Board-specific IRQ info
 */
#define	EV64260_UART_0_IRQ		85
#define	EV64260_UART_1_IRQ		86

#define	EV64260_PCI_0_IRQ		91
#define	EV64260_PCI_1_IRQ		93

#if	!defined(CONFIG_GT64260_CONSOLE)
/*
 * Serial driver setup.
 */
#define EV64260_SERIAL_0		(EV64260_UART_BASE + 0x20)
#define EV64260_SERIAL_1		EV64260_UART_BASE

#define BASE_BAUD	( 3686400 / 16 )	/* 115200 */

#ifdef CONFIG_SERIAL_MANY_PORTS
#define RS_TABLE_SIZE	64
#else
#define RS_TABLE_SIZE	2
#endif

#ifdef CONFIG_SERIAL_DETECT_IRQ
#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF|ASYNC_SKIP_TEST|ASYNC_AUTO_IRQ)
#else
#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF|ASYNC_SKIP_TEST)
#endif

/* Required for bootloader's ns16550.c code */
#define STD_SERIAL_PORT_DFNS 						\
        { 0, BASE_BAUD, EV64260_SERIAL_0, EV64260_UART_0_IRQ, STD_COM_FLAGS, \
	iomem_base: (u8 *)EV64260_SERIAL_0,	/* ttyS0 */		\
	iomem_reg_shift: 2,						\
	io_type: SERIAL_IO_MEM },					\
        { 0, BASE_BAUD, EV64260_SERIAL_1, EV64260_UART_1_IRQ, STD_COM_FLAGS, \
	iomem_base: (u8 *)EV64260_SERIAL_1,	/* ttyS1 */		\
	iomem_reg_shift: 2,						\
	io_type: SERIAL_IO_MEM },

#define SERIAL_PORT_DFNS \
        STD_SERIAL_PORT_DFNS
#else /* if defined(CONFIG_GT64260_CONSOLE) */
#define SERIAL_PORT_DFNS
#endif /* !defined(CONFIG_GT64260_CONSOLE) */

#endif /* CONFIG_EV64260 */

#endif /* __PPC_PLATFORMS_EV64260_H */
