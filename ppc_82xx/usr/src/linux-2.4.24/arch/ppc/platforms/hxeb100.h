/*
 * arch/ppc/platforms/hxeb100.h
 * 
 * Definitions for Motorola Computer Group HXEB100 Board.
 *
 * Authors: Mark A. Greer <mgreer@mvista.com> and
 *          Dale Farnsworth <Dale.Farnsworth@mvista.com>
 *
 * Copyright 2001,2002 MontaVista Software Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
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
 * 	0xffa00000-0xffffffff		- Reserved
 * 	0xff800000-0xffffffff		- bank B FLASH
 * 	0xf2000000-0xff7fffff		- Bank A FLASH
 * 	0xf1100000-0xf11fffff		- GT64260 Device Bus Registers
 * 	0xf1000000-0xf10fffff		- GT64260 Registers
 * 	0xa2000000-0xf0ffffff		- <hole>
 * 	0xa1000000-0xa1ffffff		- PCI 1 I/O (defined in gt64260.h)
 * 	0xa0000000-0xa0ffffff		- PCI 0 I/O (defined in gt64260.h)
 * 	0x90000000-0x9fffffff		- PCI 1 MEM (defined in gt64260.h)
 * 	0x80000000-0x8fffffff		- PCI 0 MEM (defined in gt64260.h)
 *
 */

#ifndef __PPC_PLATFORMS_HXEB100_H
#define __PPC_PLATFORMS_HXEB100_H

#ifndef	MAX
#define	MAX(a,b)	(((a) > (b)) ? (a) : (b))
#endif

#ifdef CONFIG_HXEB100
/*
 * CPU Physical Memory Map setup.
 */
#define HXEB100_GT64260_REG_BASE	0xf1000000
#define	HXEB100_BANK_B_FLASH_BASE	0xff800000
#define	HXEB100_BANK_A_FLASH_BASE	0xf2000000
#define	HXEB100_DEVICE_BASE		0xf1100000

#define	HXEB100_BANK_A_FLASH_SIZE	0x04000000	/* <= 64MB of FLASH */
#define	HXEB100_BANK_B_FLASH_SIZE	0x00200000	/* 2MB of FLASH */
#define HXEB100_BANK_A_FLASH_BUSWIDTH	4
#define HXEB100_BANK_B_FLASH_BUSWIDTH	4

#define	HXEB100_DEVICE_SIZE		0x00100000	/* 1MB */
#define	HXEB100_DEVICE_END		(HXEB100_DEVICE_BASE +	\
						HXEB100_DEVICE_SIZE - 1)

#define	HXEB100_STATUS_REG_1		(HXEB100_DEVICE_BASE + 0x0000)
#define	HXEB100_STATUS_REG_2		(HXEB100_DEVICE_BASE + 0x0001)
#define	HXEB100_STATUS_REG_3		(HXEB100_DEVICE_BASE + 0x0002)
#define	HXEB100_PRESENCE_DETECT_REG	(HXEB100_DEVICE_BASE + 0x0004)
#define	HXEB100_SW_READABLE_SWITCH_REG	(HXEB100_DEVICE_BASE + 0x0005)
#define	HXEB100_TIMEBASE_ENABLE_REG	(HXEB100_DEVICE_BASE + 0x0006)

#define	HXEB100_TODC_BASE		(HXEB100_DEVICE_BASE + 0x10000)
#define	HXEB100_TODC_SIZE		0x8000		/* 32 KB */

#define HXEB100_REF_CLK_MASK		0x80
#define HXEB100_BANK_SEL_MASK		0x40
#define HXEB100_SAFE_START_MASK		0x20
#define HXEB100_ABORT_MASK		0x10
#define HXEB100_FLASH_BSY_MASK		0x08
#define HXEB100_FUSE_STAT_MASK		0x04

#define HXEB100_BD_FAIL_MASK		0x80
#define HXEB100_EEPROM_WP_MASK		0x40
#define HXEB100_FLASH_WP_MASK		0x20
#define HXEB100_TSTAT_MASK_MASK		0x10
#define HXEB100_PCI_1_1_M66EN_MASK	0x08
#define HXEB100_PCI_0_1_M66EN_MASK	0x04
#define HXEB100_PCI_1_0_M66EN_MASK	0x02
#define HXEB100_PCI_0_0_M66EN_MASK	0x01

#define HXEB100_BOARD_RESET_MASK	0x80
#define HXEB100_CFLASH_DMA_MASK		0x04
#define HXEB100_PCI1_RST_MASK		0x02
#define HXEB100_PCI0_RST_MASK		0x01

#define HXEB100_VPD_IIC_DEV		0xa8

/* IPI to cpu1: bit  4 is connected to bit 27 */
/* IPI to cpu0: bit 28 is connected to bit  5 */

#define HXEB100_CPU0_IPI_OUT		28
#define HXEB100_CPU1_IPI_OUT		4
#define HXEB100_CPU0_IPI_IN		5
#define HXEB100_CPU1_IPI_IN		27
#define HXEB100_CPU0_IPI 		(HXEB100_CPU0_IPI_IN+64)
#define HXEB100_CPU1_IPI		(HXEB100_CPU1_IPI_IN+64)

#ifndef	__ASSEMBLY__
/*
 * Data passed from the boot loader to the kernel
 */
typedef struct board_info {
    	unsigned int	bi_hxeb;
    	unsigned int	bi_busfreq;
	unsigned char	bi_enetaddr[2][6];
} bd_t;

#define HXEB100_BOARD_INFO_HXEB		0x48584542	/* 'HXEB' */

/*
 * Bootloader SMP setup.
 */
extern void hxeb100_boot_start_cpu1(void);
extern void hxeb100_kernel_start_cpu1(void);

struct motload_mailbox {
            u32 login;
            void (*func)(void);
            u32 cmd;
};
#endif /* __ASSEMBLY__ */

#define MOTLOAD_SMP_IDLE		0x49444c45      /* "IDLE" */
#define MOTLOAD_SMP_EXEC		0x45584543      /* "EXEC" */
#define MOTLOAD_SMP_START_CMD		0xa             /* start cpu1 */
#define MOTLOAD_SMP_MAILBOX_ADDR	0x0000a2d8
#define MAILBOX_LOGIN_OFFSET		0x0
#define MAILBOX_FUNC_OFFSET		0x4
#define MAILBOX_CMD_OFFSET		0x8

#define MOTLOAD_SMP_MAILBOX ((volatile struct motload_mailbox *)MOTLOAD_SMP_MAILBOX_ADDR)
#define KERNEL_SECONDARY_HOLD ((void (*)(void)) 0xc0)

/*
 * Serial driver setup.
 */
#define HXEB100_SERIAL_0		(HXEB100_DEVICE_BASE + 0x20000)
#define HXEB100_SERIAL_1		(HXEB100_DEVICE_BASE + 0x21000)
#define HXEB100_SERIAL_2		(HXEB100_DEVICE_BASE + 0x22000)
#define HXEB100_SERIAL_3		(HXEB100_DEVICE_BASE + 0x23000)

#define	HXEB100_UART_0_IRQ		(0+64)
#define	HXEB100_UART_1_IRQ		(1+64)
#define	HXEB100_UART_2_IRQ		(0+64)
#define	HXEB100_UART_3_IRQ		(1+64)

#define	HXEB100_UART_SIZE		0x00000008

#define BASE_BAUD ( 1843200 / 16 )

#ifdef CONFIG_SERIAL_MANY_PORTS
#define RS_TABLE_SIZE	64
#else
#define RS_TABLE_SIZE	4
#endif

#ifdef CONFIG_SERIAL_DETECT_IRQ
#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF|ASYNC_SKIP_TEST|ASYNC_AUTO_IRQ)
#else
#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF|ASYNC_SKIP_TEST)
#endif

/* Required for bootloader's ns16550.c code */
#define STD_SERIAL_PORT_DFNS 						\
        { 0, BASE_BAUD, HXEB100_SERIAL_0, HXEB100_UART_0_IRQ,/* ttyS0 */\
	STD_COM_FLAGS, iomem_base: (u8 *)HXEB100_SERIAL_0,		\
	iomem_reg_shift: 0,						\
	io_type: SERIAL_IO_MEM },					\
        { 0, BASE_BAUD, HXEB100_SERIAL_1, HXEB100_UART_1_IRQ,/* ttyS1 */\
	STD_COM_FLAGS, iomem_base: (u8 *)HXEB100_SERIAL_1,		\
	iomem_reg_shift: 0,						\
	io_type: SERIAL_IO_MEM },

#define SERIAL_PORT_DFNS \
        STD_SERIAL_PORT_DFNS

#endif /* CONFIG_HXEB100 */

#endif /* __PPC_PLATFORMS_HXEB100_H */
