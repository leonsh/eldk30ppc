/*
 * (C) Copyright 2003
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 *
 * Based on linux/drivers/acorn/char/pcf8583.h
 *
 *  Copyright (C) 2000 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
struct rtc_registers {
	unsigned char	secs;
	unsigned char	mins;
	unsigned char	hours;
	unsigned char	wday;
	unsigned char	mday;
	unsigned char	mon;
	unsigned char	year;
	unsigned char	cs;
};

struct rtc_memory {
	unsigned int	loc;
	unsigned int	nr;
	unsigned char	*data;
};

#define RTC_GETDATETIME	0
#define RTC_SETTIME	1
#define RTC_SETDATETIME	2
#define RTC_GETCTRL	3
#define RTC_SETCTRL	4
#define MEM_READ	5
#define MEM_WRITE	6

#define CTRL_STOP	0x80
#if	0
#define CTRL_HOLD	0x40
#define CTRL_32KHZ	0x00
#define CTRL_MASK	0x08
#define CTRL_ALARMEN	0x04
#define CTRL_ALARM	0x02
#define CTRL_TIMER	0x01
#endif
