/*
 * (C) Copyright 2000, 2001
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * This code implements one or more status LEDs which can be switched
 * on or off or set to blink at a specified frequency.
 *
 * For instance you can use a status LED to signal the operational
 * status of a target which usually boots over the network; while
 * running in PPCBoot, the status LED is blinking. As soon as a valid
 * BOOTP reply message has been received, the LED is turned off. The
 * Linux kernel, once it is running, will start blinking the LED
 * again, with another frequency.
 *
 * For IVM* systems, this driver is also used to read and write some
 * special I/O ports:
 *
 * - The "Interlock Switch" and the "Device Reset Monitor" are input
 *   ports (one bit each), which are used to signal status changes to
 *   the applications. It is a requirement to be able to wait for
 *   such a status change using select(). Unfortunately, the hardware
 *   is different for IVMS8 and IVML24, and not all input pins used
 *   can generate interrupts. So we have to poll the ports
 *   periodically - sic!
 * - "Device Reset Enable" is an output port (1 bit) which can be set
 *   and reset from the application. OK, finally we have something
 *   really simple.
 */

#ifndef _PPC_STATUS_LED_H
#define	_PPC_STATUS_LED_H

#include <linux/config.h>
#include <asm/ioctl.h>

/* ioctl's */
#define STATUSLED_GET		_IOR('L', 1, unsigned long)
#define STATUSLED_SET		_IOW('L', 2, unsigned long)
#define STATUSLED_PERIOD	_IOW('L', 3, unsigned long)
#define	STATUS_SWITCH_GET	_IOR('L', 4, unsigned long)
#define STATUS_RESET_GET	_IOR('L', 5, unsigned long)
#define STATUS_RESET_SET	_IOW('L', 6, unsigned long)

#define	STATUS_LED_OFF		0
#define STATUS_LED_BLINKING	1
#define STATUS_LED_ON		2

#ifdef __KERNEL__

/*****  TQM8xxL  ********************************************************/
#if (defined(CONFIG_TQM8xxL) && !defined(CONFIG_ETX094))
# define STATUS_LED_PAR		im_cpm.cp_pbpar
# define STATUS_LED_DIR		im_cpm.cp_pbdir
# define STATUS_LED_ODR		im_cpm.cp_pbodr
# define STATUS_LED_DAT		im_cpm.cp_pbdat

# define STATUS_LED_BIT		0x00000001
# define STATUS_LED_PERIOD	(2 * HZ)
# define STATUS_LED_STATE	STATUS_LED_BLINKING

# define STATUS_LED_ACTIVE	1		/* LED on for bit == 1	*/

# define STATUS_LED_BOOT	0		/* LED 0 used for boot status */

/*****  ETX_094  ********************************************************/
#elif defined(CONFIG_ETX094)

# define STATUS_LED_PAR		im_ioport.iop_pdpar
# define STATUS_LED_DIR		im_ioport.iop_pddir
# undef  STATUS_LED_ODR
# define STATUS_LED_DAT		im_ioport.iop_pddat

# define STATUS_LED_BIT		0x00000001
# define STATUS_LED_PERIOD	(2 * HZ)
# define STATUS_LED_STATE	STATUS_LED_BLINKING

# define STATUS_LED_ACTIVE	0		/* LED on for bit == 0	*/

# define STATUS_LED_BOOT	0		/* LED 0 used for boot status */

/*****  IVMS8  **********************************************************/
#elif defined(CONFIG_IVMS8)

# define STATUS_LED_PAR		im_cpm.cp_pbpar
# define STATUS_LED_DIR		im_cpm.cp_pbdir
# define STATUS_LED_ODR		im_cpm.cp_pbodr
# define STATUS_LED_DAT		im_cpm.cp_pbdat

# define STATUS_LED_BIT		0x00000010	/* LED 0 is on PB.27	*/
# define STATUS_LED_PERIOD	(1 * HZ)
# define STATUS_LED_STATE	STATUS_LED_OFF
# define STATUS_LED_BIT1	0x00000020	/* LED 1 is on PB.26	*/
# define STATUS_LED_PERIOD1	(1 * HZ)
# define STATUS_LED_STATE1	STATUS_LED_OFF
/* IDE LED usable for other purposes, too */
# define STATUS_LED_BIT2	0x00000008	/* LED 2 is on PB.28	*/
# define STATUS_LED_PERIOD2	(1 * HZ)
# define STATUS_LED_STATE2	STATUS_LED_OFF

# define STATUS_LED_ACTIVE	1		/* LED on for bit == 1	*/

# define STATUS_LED_YELLOW	0
# define STATUS_LED_GREEN	1
# define STATUS_LED_BOOT	2		/* IDE LED used for boot status */

# define STATUS_ILOCK_BIT	0x01
# define STATUS_RESET_MON_BIT	0x02

# define STATUS_ILOCK_SWITCH	0x00800000	/* ILOCK switch in IRQ4	*/

# define STATUS_ILOCK_PERIOD	(HZ / 10)	/* about every 100 ms	*/

# define STATUS_RESET_MON	0x0008		/* Reset Mon. on PC.12	*/
						/* polled with ILOCK	*/
# define STATUS_RESET_ENA	0x0004		/* Reset enable  PC.13	*/

/*****  IVML24  *********************************************************/
#elif defined(CONFIG_IVML24)

# define STATUS_LED_PAR		im_cpm.cp_pbpar
# define STATUS_LED_DIR		im_cpm.cp_pbdir
# define STATUS_LED_ODR		im_cpm.cp_pbodr
# define STATUS_LED_DAT		im_cpm.cp_pbdat

# define STATUS_LED_BIT		0x00000010
# define STATUS_LED_PERIOD	(1 * HZ)
# define STATUS_LED_STATE	STATUS_LED_OFF
# define STATUS_LED_BIT1	0x00000020
# define STATUS_LED_PERIOD1	(1 * HZ)
# define STATUS_LED_STATE1	STATUS_LED_OFF
/* IDE LED usable for other purposes, too */
# define STATUS_LED_BIT2	0x00000008	/* LED 2 is on PB.28	*/
# define STATUS_LED_PERIOD2	(1 * HZ)
# define STATUS_LED_STATE2	STATUS_LED_OFF

# define STATUS_LED_ACTIVE	1		/* LED on for bit == 1	*/

# define STATUS_LED_YELLOW	0
# define STATUS_LED_GREEN	1
# define STATUS_LED_BOOT	2		/* IDE LED used for boot status */

# define STATUS_ILOCK_BIT	0x01
# define STATUS_RESET_MON_BIT	0x02

# define STATUS_ILOCK_SWITCH	0x00004000	/* ILOCK is on PB.17	*/

# define STATUS_ILOCK_PERIOD	(HZ / 10)	/* about every 100 ms	*/

# define STATUS_RESET_MON	0x0008		/* Reset Mon. on PC.12	*/
						/* polled with ILOCK	*/
# define STATUS_RESET_ENA	0x0004		/* Reset enable  PC.13	*/

/*****  PCU E  and  CCM  ************************************************/
#elif defined(CONFIG_PCU_E) || defined(CONFIG_CCM)

# define STATUS_LED_PAR		im_cpm.cp_pbpar
# define STATUS_LED_DIR		im_cpm.cp_pbdir
# undef  STATUS_LED_ODR		im_cpm.cp_pbodr
# define STATUS_LED_DAT		im_cpm.cp_pbdat

# define STATUS_LED_BIT		0x00010000	/* green LED is on PB.15 */
# define STATUS_LED_PERIOD	(2 * HZ)
# define STATUS_LED_STATE	STATUS_LED_BLINKING
# define STATUS_LED_BIT1	0x00020000	/*  red  LED is on PB.14 */
# define STATUS_LED_PERIOD1	(1 * HZ)
# define STATUS_LED_STATE1	STATUS_LED_OFF

# define STATUS_LED_ACTIVE	1		/* LED on for bit == 1	*/

# define STATUS_LED_BOOT	0		/* LED 0 used for boot status */
# define STATUS_LED_GREEN	STATUS_LED_BOOT
# define STATUS_LED_RED		1

/*****  LANTEC  *********************************************************/
#elif defined(CONFIG_LANTEC)

# define STATUS_LED_PAR		im_ioport.iop_pdpar
# define STATUS_LED_DIR		im_ioport.iop_pddir
# undef  STATUS_LED_ODR
# define STATUS_LED_DAT		im_ioport.iop_pddat

# define STATUS_LED_BIT		0x0800
# define STATUS_LED_PERIOD	(HZ / 4)
# define STATUS_LED_STATE	STATUS_LED_BLINKING

# define STATUS_LED_ACTIVE	1		/* LED on for bit == 0 */

# define STATUS_LED_BOOT	0		/* LED 0 used for boot status */

/*****  ICU862   ********************************************************/
#elif defined(CONFIG_ICU862)

# define STATUS_LED_PAR		im_ioport.iop_papar
# define STATUS_LED_DIR		im_ioport.iop_padir
# define STATUS_LED_ODR		im_ioport.iop_paodr
# define STATUS_LED_DAT		im_ioport.iop_padat

# define STATUS_LED_BIT		0x4000		/* LED 0 is on PA.1 */
# define STATUS_LED_PERIOD	(2 * HZ)
# define STATUS_LED_STATE	STATUS_LED_BLINKING
# define STATUS_LED_BIT1	0x1000		/* LED 1 is on PA.3 */
# define STATUS_LED_PERIOD1	(1 * HZ)
# define STATUS_LED_STATE1	STATUS_LED_OFF

# define STATUS_LED_ACTIVE	1		/* LED on for bit == 1	*/

# define STATUS_LED_BOOT	0		/* LED 0 used for boot status */

/*****  DAB4K    ********************************************************/
/* STATUS_LED_XXXs are defined in arch/ppc/platforms/dab4k.h            */
#elif defined(CONFIG_DAB4K)


/************************************************************************/
#else
# error Status LED configuration missing
#endif

void status_led_tick (unsigned long timestamp);
void status_led_set  (int state);

#endif	/* __KERNEL__ */

#endif	/* _PPC_STATUS_LED_H */
