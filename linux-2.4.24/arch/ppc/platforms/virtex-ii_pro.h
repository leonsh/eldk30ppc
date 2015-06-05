/*
 * virtex-ii_pro.h
 *
 * Include file that defines the Xilinx Virtex-II Pro processor
 *
 * Author: MontaVista Software, Inc.
 *         source@mvista.com
 *
 * 2002 (c) MontaVista, Software, Inc.  This file is licensed under the terms
 * of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */

#ifdef __KERNEL__
#ifndef __ASM_VIRTEXIIPRO_H__
#define __ASM_VIRTEXIIPRO_H__

#include <linux/config.h>
#include <platforms/xilinx_ocp.h>

#ifdef CONFIG_SERIAL_MANY_PORTS
# define RS_TABLE_SIZE  64
#else
# define RS_TABLE_SIZE  4
#endif

#include <platforms/ibm405.h>

#ifndef __ASSEMBLY__
unsigned char x8042_read_input(void);
#define kbd_read_input()	x8042_read_input()

unsigned char x8042_read_status(void);
#define kbd_read_status()	x8042_read_status()

void x8042_write_output(unsigned char val);
#define kbd_write_output(val)	x8042_write_output(val)

void x8042_write_command(unsigned char val);
#define kbd_write_command(val)	x8042_write_command(val)
#endif				/* !__ASSEMBLY__ */

#endif				/* __ASM_VIRTEXIIPRO_H__ */
#endif				/* __KERNEL__ */
