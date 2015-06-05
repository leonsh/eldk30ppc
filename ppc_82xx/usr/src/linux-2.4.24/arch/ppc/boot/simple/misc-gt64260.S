/*
 * arch/ppc/boot/simple/misc-gt64260.S
 * 
 * Code to change the base address for the GT64260 et. al. host bridge and call
 * board specific init routine.
 *  - GT64260_FW_REG_BASE is the current reg base
 *  - GT64260_BRIDGE_REG_BASE is the new reg base
 *
 * Author: Mark Greer <mgreer@mvista.com>
 *
 * 2002 (c) MontaVista, Software, Inc.  This file is licensed under the terms
 * of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */

#include <linux/config.h>
#include <asm/ppc_asm.h>
#include <asm/processor.h>
#include <asm/cache.h>
#include <asm/gt64260_defs.h>

	.globl	gt64260_init
gt64260_init:
	mflr	r27

#ifdef	CONFIG_GT64260_NEW_BASE
	bl	move_base
#endif
	bl	gt_board_init

	mtlr	r27
	blr

#ifdef	CONFIG_GT64260_NEW_BASE
move_base:

	li	r20,0
	li	r23,20

	/* Relocate gt64260's regs */
        addis	r25,0,CONFIG_GT64260_ORIG_REG_BASE@h
        ori	r25,r25,GT64260_INTERNAL_SPACE_DECODE             
        lwbrx	r26,0,(r25)
	lis	r24,0xffff
	and	r26,r26,r24
	addis	r24,0,CONFIG_GT64260_NEW_REG_BASE@h
	srw	r24,r24,r23
	or	r26,r26,r24
        stwbrx  r26,0,(r25)
	sync

	/* Wait for write to take effect */
        addis	r25,0,CONFIG_GT64260_NEW_REG_BASE@h
	ori	r25,r25,GT64260_INTERNAL_SPACE_DECODE
1:	lwbrx	r24,0,(r25)
	cmpw	r24,r26
	bne	1b

	blr
#endif
