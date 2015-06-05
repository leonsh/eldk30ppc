/*
 * arch/ppc/boot/simple/misc-ev64260.S
 * 
 * Host bridge init code for the Marvell/Galileo EV-64260-BP evaluation board
 * with a GT64260 onboard.
 *
 * Author: Mark Greer <mgreer@mvista.com>
 *
 * Copyright 2001 MontaVista Software Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <asm/ppc_asm.h>
#include <asm/processor.h>
#include <asm/cache.h>
#include <asm/gt64260_defs.h>
#include <platforms/ev64260.h>

	.globl	gt_board_init
gt_board_init:
	/* DINK doesn't enable 745x timebase, so enable here (Adrian Cox) */
	mfspr	r25,PVR
	srwi	r25,r25,16
	cmplwi	r25,(PVR_7450 >> 16)
	bne	1f
	mfspr	r25,HID0
	oris	r25,r25,(HID0_TBEN >> 16)
	mtspr	HID0,r25

1:
#ifdef	CONFIG_GT64260_NEW_BASE
	li	r23,20

	/*
	 * Change the CS2 window for the UART so that the bootloader
	 * can do I/O thru the UARTs.
	 */
        addis	r25,0,CONFIG_GT64260_NEW_REG_BASE@h
        ori	r25,r25,GT64260_CPU_CS_DECODE_2_BOT             
	addis	r26,0,EV64260_UART_BASE@h
	srw	r26,r26,r23
        stwbrx  r26,0,(r25)
	sync

        addis	r25,0,CONFIG_GT64260_NEW_REG_BASE@h
        ori	r25,r25,GT64260_CPU_CS_DECODE_2_TOP
	addis	r26,0,EV64260_UART_END@h
	srw	r26,r26,r23
        stwbrx  r26,0,(r25)
	sync
#endif
        blr
