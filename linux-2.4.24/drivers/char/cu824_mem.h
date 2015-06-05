/***********************************************************************
 *
 * (C) Copyright 2002
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 *
 * This driver provides access to the VMEBus and BCSR memory of
 * CU824 VMEBus systems.
 *
 ***********************************************************************/

#ifndef	__CU824_MEM_H
#define __CU824_MEM_H

typedef	struct	vme_mem_s {
	const char	*name;		/* Device Name						*/
	ulong		phys;		/* physical start of memory region	*/
	ulong		addr;		/* virtual start of memory region	*/
	ulong		size;		/* size of memory region			*/
	ulong		width;		/* bus width of memory region		*/
	uint		flags;		/* Flags							*/
} vme_mem_t;

#define	CU824_FLAGS_BUSY	1	/* Device is BUSY */

/*
 * VME Memory Regions
 */
#define VME_MEM_STD	1		/* VME Standard Memory		*/
#define VME_MEM_SHORT	2		/* VME Short Memory		*/
#define VME_MEM_BCSR	3		/* VME Board control & status	*/
#define VME_MEM_IACK	4		/* VME IACK Range		*/

extern vme_mem_t *vme_mem_dev (int);

#endif	/* __CU824_MEM_H */
