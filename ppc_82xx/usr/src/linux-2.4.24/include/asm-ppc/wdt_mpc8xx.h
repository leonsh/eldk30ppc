/*
 * (C) Copyright 2000
 * Jörg Haider, SIEMENS AG
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
 *
 * (C) 2002 Detlev Zundel, dzu@denx.de -- Added "watchdog chains"
 * (C) 2001 Wolfgang Denk, wd@denx.de -- Cleanup, Modifications for 2.4 kernels
 * (C) 2001 Wolfgang Denk, wd@denx.de -- Adaption for MAX706TESA Watchdog (LWMON)
 * (C) 2001 Steven Hein,  ssh@sgi.com -- Added timeout configuration option
 */

/*
 * The purpose of this header file is to provide an interface for the
 * driver of the watchdog timer wdt_mpc8xx. In essence this interface
 * consists of the three macros WDT_MPC8XX_INIT, WDT_MPC8XX_SERVICE,
 * WDT_MPC8XX_CLOSE, and its functionality is described as follows:
 *
 * WDT_MPC8XX_INIT:     opens the driver and initializes the timer to
 *                      300 seconds;
 *
 * WDT_MPC8XX_SERVICE:  writes the value defined by the macro
 *                      WDT_MPC8XX_DEF_SERVICE_TIME to the variable,
 *                      which serves as a timer counter;
 *
 * WDT_MPC8XX_CLOSE:    closes the watchdog driver;
 *
 * Finally there is a macro called WDT_MPC8XX_SET_SERVICE_TIME(sec)
 * for altering the value written to the timer counter to a value,
 * which is specified by sec.
 */


#ifndef _wdt_mpc8xx_h
#define _wdt_mpc8xx_h

typedef struct	wdt_mpc8xx_param {
	unsigned chainid;
	unsigned long timer_count[3];
	int action[3];
	int signal;
} wdt_mpc8xx_param_t;

/* Constants for the action[] fields */
#define WDT_MPC8XX_ACTION_SIGNAL  1
#define WDT_MPC8XX_ACTION_KILL    2
#define WDT_MPC8XX_ACTION_REBOOT  3
#define WDT_MPC8XX_ACTION_RESET   4

#define	WDT_MPC8XX_IOCTL_BASE	'W'

#define WDT_MPC8XX_OPEN_ONLY	_IO (WDT_MPC8XX_IOCTL_BASE, 0)
#define WDT_MPC8XX_ALWAYS	_IO (WDT_MPC8XX_IOCTL_BASE, 1)
#define WDT_MPC8XX_REGISTER	_IOW(WDT_MPC8XX_IOCTL_BASE, 2, wdt_mpc8xx_param_t)
#define WDT_MPC8XX_RESET	_IOW(WDT_MPC8XX_IOCTL_BASE, 3, int)
#define WDT_MPC8XX_UNREGISTER	_IOW(WDT_MPC8XX_IOCTL_BASE, 4, int)

#ifndef	__KERNEL__

#include <fcntl.h>
#include <unistd.h>
#include <linux/ioctl.h>

#define WDT_MPC8XX_DEVICE "/dev/watchdog"
#define WDT_MPC8XX_DEF_SERVICE_TIME 300

int wdt_mpc8xx_fd;
int wdt_mpc8xx_value = WDT_MPC8XX_DEF_SERVICE_TIME;

#define WDT_MPC8XX_INIT (wdt_mpc8xx_fd = open(WDT_MPC8XX_DEVICE, O_RDWR, 0))

#define WDT_MPC8XX_SET_SERVICE_TIME(sec) wdt_mpc8xx_value = (sec);

#define WDT_MPC8XX_SERVICE write(wdt_mpc8xx_fd, (char *) &wdt_mpc8xx_value, sizeof (wdt_mpc8xx_value))

#define WDT_MPC8XX_CLOSE close(wdt_mpc8xx_fd)

#endif	/* __KERNEL__ */

#endif	/* _wdt_mpc8xx_h */
