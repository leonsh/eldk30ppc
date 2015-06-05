/*
 * SL8245 board specific definitions
 * 
 * Copyright (c) 2003 Wolfgang Denk (wd@denx.de)
 */

#ifndef __ASMPPC_SL8245_SERIAL_H
#define __ASMPPC_SL8245_SERIAL_H

#include <linux/config.h>

#define SL8245_SERIAL_0		0xFDF04500

#ifdef CONFIG_SERIAL_MANY_PORTS
#define RS_TABLE_SIZE  64
#else
#define RS_TABLE_SIZE  1
#endif

#define BASE_BAUD ( 133333333 / 16 )

#ifdef CONFIG_SERIAL_DETECT_IRQ
#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF|ASYNC_SKIP_TEST|ASYNC_AUTO_IRQ)
#else
#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF|ASYNC_SKIP_TEST)
#endif

#define STD_SERIAL_PORT_DFNS \
        { 0, BASE_BAUD, SL8245_SERIAL_0, 3, STD_COM_FLAGS, /* ttyS0 */ \
		iomem_base: (u8 *)SL8245_SERIAL_0,			  \
		io_type: SERIAL_IO_MEM },

#define SERIAL_PORT_DFNS \
        STD_SERIAL_PORT_DFNS

#endif /* __ASMPPC_SL8245_SERIAL_H */
