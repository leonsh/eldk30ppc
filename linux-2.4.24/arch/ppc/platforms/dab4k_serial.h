/*
 * include/asm-ppc/dab4k_serial.h
 * 
 * Definitions for IMC DAB4K2 board
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __H_DAB4K_SERIAL
#define __H_DAB4K_SERIAL

#define RS_TABLE_SIZE 2

#define UARTA_ADDR 0xD0000000
#define UARTB_ADDR 0xD0000080
#define UART_BASE_BAUD 20000000

#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF|ASYNC_SKIP_TEST)

#define SERIAL_PORT_DFNS \
         { 0, (UART_BASE_BAUD/16), UARTA_ADDR, SIU_IRQ3, STD_COM_FLAGS, \
           iomem_base: (u8 *) UARTA_ADDR, \
           iomem_reg_shift:0, \
           io_type: SERIAL_IO_MEM }, \
         { 0, (UART_BASE_BAUD/16), UARTB_ADDR, SIU_IRQ4, STD_COM_FLAGS, \
           iomem_base: (u8 *) UARTB_ADDR, \
           iomem_reg_shift:0, \
           io_type: SERIAL_IO_MEM }

#endif /* DAB4K_SERIAL_H */
