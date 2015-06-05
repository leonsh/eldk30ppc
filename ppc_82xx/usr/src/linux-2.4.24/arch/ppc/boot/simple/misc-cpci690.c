/*
 * arch/ppc/boot/simple/misc-cpci690.c
 * 
 * Add birec data for Force CPCI690 board.
 *
 * Author: Mark A. Greer <source@mvista.com>
 *
 * Copyright 2003 MontaVista Software, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/types.h>
#include <asm/bootinfo.h>

#include <cpci690.h>

#include "nonstdio.h"

extern struct bi_record *decompress_kernel(unsigned long load_addr,
						int num_words,
						unsigned long cksum);

static bd_t	board_info;

/*
 * The MAC addresses are store in the RTC's NVRAM.  We need to get them out
 * from there, convert to the proper format for the kernel driver,
 * and pass them into the kernel.
 */
struct bi_record *
load_kernel(u32 load_addr, int num_words, u32 chksum)
{
	struct bi_record	*bi_recs;
	bd_t			*bip = &board_info;

	bi_recs = decompress_kernel(load_addr, num_words, chksum);

	bip->bi_magic = CPCI690_BI_MAGIC;
	memcpy(bip->bi_enetaddr[0],
		(void *)(CPCI690_TODC_BASE+CPCI690_MAC_OFFSET),
		sizeof(bip->bi_enetaddr));

	bootinfo_append(BI_BOARD_INFO, sizeof(board_info), bip);

	return bi_recs;
}
