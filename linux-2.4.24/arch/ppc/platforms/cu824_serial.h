#ifndef __ASMPPC_CU824_SERIAL_H
#define __ASMPPC_CU824_SERIAL_H

#include <linux/config.h>

#ifdef CONFIG_SERIAL_MANY_PORTS
#define RS_TABLE_SIZE  64
#else
#define RS_TABLE_SIZE  2
#endif

#define BASE_BAUD 460800

#define STD_SERIAL_PORT_DFNS			\
	{ 0, BASE_BAUD, 0, 2, ASYNC_BOOT_AUTOCONF, 0, 0, 0, 0, 0, 0, 0,		\
	(void*)0xFE800083, 2, 0, 0, {}, {}, {}, SERIAL_IO_MEM }, /* ttyS0 */	\
	{ 0, BASE_BAUD, 0, 2, ASYNC_BOOT_AUTOCONF, 0, 0, 0, 0, 0, 0, 0,		\
	(void*)0xFE8000C3, 2, 0, 0, {}, {}, {}, SERIAL_IO_MEM }, /* ttyS1 */

#define SERIAL_PORT_DFNS \
	STD_SERIAL_PORT_DFNS
	
#endif /* __ASMPPC_CU824_SERIAL_H */
