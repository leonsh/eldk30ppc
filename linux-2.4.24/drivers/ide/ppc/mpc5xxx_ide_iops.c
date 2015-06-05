/*
 * drivers/ide/ppc/mpc5xxx_ide_iops.c
 *
 * Utility functions for MPC5xxx on-chip IDE interface
 *
 *  Copyright (c) 2003 Mipsys - Benjamin Herrenschmidt
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mpc5xxx.h>

#include "mpc5xxx_ide.h"

#undef WHACKY_DEBUG

static u8 ide_mpc5xxx_inb (unsigned long port)
{
#ifdef WHACKY_DEBUG
	u8 data;
	printk("ide_inb(%x) : ", port);
	WAIT_TIP_BIT_CLEAR(port);
	data = readb(port);
	printk("%02x\n", data);
	return data;
#else
	WAIT_TIP_BIT_CLEAR(port);
	return (u8) readb(port);
#endif
}

static u16 ide_mpc5xxx_inw (unsigned long port)
{
#ifdef WHACKY_DEBUG
	u16 data;
	printk("ide_inw(%x) : ", port);
	WAIT_TIP_BIT_CLEAR(port);
	data = readw(port);
	printk("%04x\n", data);
	return data;
#else
	WAIT_TIP_BIT_CLEAR(port);
	return (u16) readw(port);
#endif
}

static void ide_mpc5xxx_insw (unsigned long port, void *addr, u32 count)
{
#ifdef WHACKY_DEBUG
	printk("ide_insw(%x,%d)\n", port, count);
#endif
	WAIT_TIP_BIT_CLEAR(port);
	__ide_mm_insw(port, addr, count);
}

static u32 ide_mpc5xxx_inl (unsigned long port)
{
#ifdef WHACKY_DEBUG
	u32 data;
	printk("ide_inl(%x) : ", port);
	WAIT_TIP_BIT_CLEAR(port);
	data = readl(port);
	printk("%08x\n", data);
	return data;
#else
	WAIT_TIP_BIT_CLEAR(port);
	return (u32) readl(port);
#endif
}

static void ide_mpc5xxx_insl (unsigned long port, void *addr, u32 count)
{
	WAIT_TIP_BIT_CLEAR(port);
	__ide_mm_insl(port, addr, count);
}

static void ide_mpc5xxx_outb (u8 value, unsigned long port)
{
#ifdef WHACKY_DEBUG
	printk("ide_outb(%x, %02x)\n", port, value);
#endif
	WAIT_TIP_BIT_CLEAR(port);
	writeb(value, port);
}

static void ide_mpc5xxx_outbsync (ide_drive_t *drive, u8 value, unsigned long port)
{
#ifdef WHACKY_DEBUG
	printk("ide_outbsync(%x, %02x)\n", port, value);
#endif
	WAIT_TIP_BIT_CLEAR(port);
	writeb(value, port);	
}


static void ide_mpc5xxx_outw (u16 value, unsigned long port)
{
#ifdef WHACKY_DEBUG
	printk("ide_outw(%x, %04x)\n", port, value);
#endif
	WAIT_TIP_BIT_CLEAR(port);
	writew(value, port);
}

static void ide_mpc5xxx_outsw (unsigned long port, void *addr, u32 count)
{
#ifdef WHACKY_DEBUG
	printk("ide_outsw(%x, %d)\n", port, count);
#endif
	WAIT_TIP_BIT_CLEAR(port);
	__ide_mm_outsw(port, addr, count);
}

static void ide_mpc5xxx_outl (u32 value, unsigned long port)
{
#ifdef WHACKY_DEBUG
	printk("ide_outl(%x, %08x)\n", port, value);
#endif
	WAIT_TIP_BIT_CLEAR(port);
	writel(value, port);
}

static void ide_mpc5xxx_outsl (unsigned long port, void *addr, u32 count)
{
	WAIT_TIP_BIT_CLEAR(port);
	__ide_mm_outsl(port, addr, count);
}

void setup_hwif_mpc5xxx_iops (ide_hwif_t *hwif)
{
	hwif->OUTB	= ide_mpc5xxx_outb;
	hwif->OUTBSYNC	= ide_mpc5xxx_outbsync;
	hwif->OUTW	= ide_mpc5xxx_outw;
	hwif->OUTL	= ide_mpc5xxx_outl;
	hwif->OUTSW	= ide_mpc5xxx_outsw;
	hwif->OUTSL	= ide_mpc5xxx_outsl;
	hwif->INB	= ide_mpc5xxx_inb;
	hwif->INW	= ide_mpc5xxx_inw;
	hwif->INL	= ide_mpc5xxx_inl;
	hwif->INSW	= ide_mpc5xxx_insw;
	hwif->INSL	= ide_mpc5xxx_insl;
}
