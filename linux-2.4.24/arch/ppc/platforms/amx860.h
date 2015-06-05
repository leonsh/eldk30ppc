/*
 * Westel Controller Board AMX860 specific definitions
 * 
 * Copyright (c) 2001-2003 Wolfgang Denk (wd@denx.de)
 */
#ifndef __MACH_AMX860_DEFS
#define __MACH_AMX860_DEFS

#include  <linux/config.h>
#include  <asm/ppcboot.h>

#define IMAP_ADDR		((uint)0xff000000)
#define IMAP_SIZE		((uint)(64 * 1024))

/* We don't use the 8259.
 */
#define NR_8259_INTS	0

#endif
