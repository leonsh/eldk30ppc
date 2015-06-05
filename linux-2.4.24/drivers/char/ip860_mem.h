/***********************************************************************
 *
 * (C) Copyright 2000-2004
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 *
 * This driver provides access to the VMEBus and IP Module memory of
 * IP860 VMEBus systems.
 *
 ***********************************************************************/

#ifndef	__IP860_MEM_H
#define __IP860_MEM_H

typedef	struct	vme_mem_s {
	const char	*name;		/* Device Name				*/
	ulong		phys;		/* physical start of memory reagion	*/
	ulong		addr;		/* virtual  start of memory reagion	*/
	ulong		size;		/*	    size  of memory reagion	*/
	ulong		width;		/*	bus width of memory reagion	*/
	uint		flags;		/* Flags				*/
	uint		cs;		/* Chip Select line used		*/
} vme_mem_t;

#define	IP860_FLAGS_BUSY	1	/* Device is BUSY			*/

/*
 * VME Memory Regions
 */
#define VME_MEM_STD	1		/* VME Standard Memory			*/
#define VME_MEM_SHORT	2		/* VME Short Memory			*/
#define VME_MEM_SRAM	3		/* Static RAM				*/
#define VME_MEM_BCSR	4		/* Board Control & Status		*/
#define VME_MEM_IPMOD	8		/* IP Module Memory			*/

extern vme_mem_t *vme_mem_dev (int);

typedef volatile unsigned char	vu_char;
typedef volatile unsigned short	vu_short;
/*
 * IP860 Board Control and Status registers
 */
typedef struct	ip860_bcsr_s {
	vu_char	srm_addr;		/* Shared Memory Address Register	*/
	vu_char		reserved_01;
	vu_char	mbox_addr;		/* Mailbox Address Register		*/
	vu_char		reserved_03;
	vu_char	vme_imask;		/* VMEBus  Interrupt  Mask   Register	*/
	vu_char		reserved_05;
	vu_char	vme_ipend;		/* VMEBus  Interrupt Pending Register	*/
	vu_char		reserved_07;
	vu_char	onbd_imask;		/* Onboard Interrupt  Mask   Register	*/
	vu_char		reserved_09;
	vu_char	onbd_ipend;		/* Onboard Interrupt Pending Register	*/
	vu_char		reserved_0B;
	vu_char	bd_ctrl;		/* Board Control Register		*/
	vu_char		reserved_0D;
	vu_char	bd_status;		/* Board Status  Register		*/
	vu_char		reserved_0F;
	vu_char	vme_irq;		/* VMEBus  Interrupt Request Register	*/
	vu_char		reserved_11;
	vu_char	vme_ivec;		/* VMEBus  Interrupt Vector  Register	*/
	vu_char		reserved_13;
	vu_char	clr_mb_int;		/* Clear Mailbox Interrupt Register	*/
	vu_char		reserved_15;
	vu_char	rtc;			/* Real Time Clock			*/
	vu_char		reserved_17;
	vu_char	mbox;			/* Mailbox				*/
	vu_char		reserved_19;
	vu_char	wdog_trigger;		/* Watchdog Trigger Register		*/
	vu_char		reserved_1B;
	vu_char	rmw_req;		/* RMW Request register			*/
	vu_char		reserved_1D;
	vu_char		reserved_1E;
	vu_char		reserved_1F;
} ip860_bcsr_t;

#endif	/* __IP860_MEM_H */

