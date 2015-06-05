/*
 * PM826 board specific definitions
 * 
 * Copyright (c) 2001 Wolfgang Denk (wd@denx.de)
 */

#ifndef __MACH_PM826_H
#define __MACH_PM826_H

#include <linux/config.h>
 
#include <asm/ppcboot.h>

#if defined(CONFIG_PCI)
#include <platforms/m8260_pci.h>
#endif

#define IMAP_ADDR		((uint)0xF0000000)
#define	I2C_ADDR_RTC		0x51

#endif	/* __MACH_PM826_H */
