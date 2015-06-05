/*
 * arch/ppc/platforms/pcore.h
 * 
 * Definitions for Force PowerCore board support
 *
 * Author: Matt Porter <mporter@mvista.com>
 *
 * Copyright 2001 MontaVista Software Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __PPC_PLATFORMS_PCORE_H
#define __PPC_PLATFORMS_PCORE_H

#include <asm/mpc10x.h>

#define PCORE_TYPE_6750			1
#define PCORE_TYPE_680			2

#define PCORE_NVRAM_AS0			0x73
#define PCORE_NVRAM_AS1			0x75
#define PCORE_NVRAM_DATA		0x77    

#define PCORE_DCCR_REG			(MPC10X_MAPB_ISA_IO_BASE + 0x308)
#define PCORE_DCCR_L2_MASK		0xc0
#define PCORE_DCCR_L2_0KB		0x00
#define PCORE_DCCR_L2_256KB		0x40
#define PCORE_DCCR_L2_512KB		0xc0
#define PCORE_DCCR_L2_1MB		0x80
#define PCORE_DCCR_L2_2MB		0x00

#define PCORE_WINBOND_IDE_INT		0x43
#define PCORE_WINBOND_PCI_INT		0x44
#define PCORE_WINBOND_PRI_EDG_LVL	0x4d0
#define PCORE_WINBOND_SEC_EDG_LVL	0x4d1

#endif /* __PPC_PLATFORMS_PCORE_H */
