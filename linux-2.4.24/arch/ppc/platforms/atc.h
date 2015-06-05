/*
 * ATC board specific definitions
 * 
 * Copyright (c) 2003 Wolfgang Denk (wd@denx.de)
 */

#ifndef __MACH_ATC_H
#define __MACH_ATC_H

#include <linux/config.h>
 
#include <asm/ppcboot.h>

#if defined(CONFIG_PCI)
#include <platforms/m8260_pci.h>
#endif

#define IMAP_ADDR		((uint)0xF0000000)
#define RTC_PORT		0xF5000000
#define RTC_ALWAYS_BCD		0

#endif	/* __MACH_ATC_H */
