/*
 * arch/ppc/boot/simple/pumaA_tty.c
 * 
 * Bootloader version of the embedded MPSC/UART driver for the GT64260.
 * Simplified version which does not use DMA.
 *
 * Author: Gary Thomas <gary@mlbassoc.com>
 * Copyright 2003 MLB Associates
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

/* This code assumes that the data cache has been disabled (L1, L2, L3). */

#include <linux/config.h>
#include <linux/serialP.h>
#include <linux/serial_reg.h>
#include <asm/serial.h>
#include <asm/gt64260_defs.h>

extern void udelay(long);

#ifdef	CONFIG_GT64260_NEW_BASE
static u32	gt64260_base = CONFIG_GT64260_NEW_REG_BASE;
#else
static u32	gt64260_base = CONFIG_GT64260_ORIG_REG_BASE;
#endif

inline unsigned
gt64260_in_le32(volatile unsigned *addr)
{
	unsigned ret;

	__asm__ __volatile__("lwbrx %0,0,%1; eieio" : "=r" (ret) :
				     "r" (addr), "m" (*addr));
	return ret;
}

inline void
gt64260_out_le32(volatile unsigned *addr, int val)
{
	__asm__ __volatile__("stwbrx %1,0,%2; eieio" : "=m" (*addr) :
				     "r" (val), "r" (addr));
}

#define GT64260_REG_READ(offs)						\
	(gt64260_in_le32((volatile uint *)(gt64260_base + (offs))))
#define GT64260_REG_WRITE(offs, d)					\
        (gt64260_out_le32((volatile uint *)(gt64260_base + (offs)), (int)(d)))



unsigned long
serial_init(int chan, void *ignored)
{
        // Assume that the port is already set up
	return (ulong)chan;
}

void
serial_putc(unsigned long com_port, unsigned char c)
{
	GT64260_REG_WRITE(GT64260_MPSC_0_CHR_1, c);
	GT64260_REG_WRITE(GT64260_MPSC_0_CHR_2, 0x200);
        while ((GT64260_REG_READ(GT64260_MPSC_0_CHR_2) & 0x200)) ;  // Wait for character to go out
}

unsigned char
serial_getc(unsigned long com_port)
{
        unsigned char c = '\0';
        int tmp;
        if ((GT64260_REG_READ(GT64260_MPSC_0_RCVR_CAUSE) & 0x0040) != 0) {
            tmp = GT64260_REG_READ(GT64260_MPSC_0_CHR_10);            
            GT64260_REG_WRITE(GT64260_MPSC_0_CHR_10, tmp);  // ACK
            c = (tmp >> 16) & 0xFF;
        }
	return c;
}

int
serial_tstc(unsigned long com_port)
{
        return ((GT64260_REG_READ(GT64260_MPSC_0_RCVR_CAUSE) & 0x0040) != 0);
}

void
serial_close(unsigned long com_port)
{
}
