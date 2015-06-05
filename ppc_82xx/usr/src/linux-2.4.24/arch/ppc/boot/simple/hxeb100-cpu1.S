/*
 * arch/ppc/boot/simple/hxeb100-cpu1.S
 * 
 * Code that runs on cpu1 before the kernel proper is loaded in low memory.
 * it sets up a stack for cpu1, then waits for a command from cpu0 telling
 * which address to branch to.
 *
 * Author: Dale Farnsworth <dfarnsworth@mvista.com>
 *
 * 2003 (c) MontaVista, Software, Inc.  This file is licensed under the terms
 * of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */


#include <asm/ppc_asm.h>
#include <platforms/hxeb100.h>

	.globl	hxeb100_cpu1_wait_for_kernel
hxeb100_cpu1_wait_for_kernel:
	lis	r1,.stack@h	/* Set initial stack pointer for cpu 1 */
	ori	r1,r1,.stack@l
	addi	r1,r1,256
	li	r2,0x000F	/* Mask pointer to 16-byte boundary */
	andc	r1,r1,r2

	lis	r6,hxeb100_wait_for_kernel_mailbox@h
	ori	r6,r6,hxeb100_wait_for_kernel_mailbox@l
	lis	r7,MOTLOAD_SMP_IDLE@h
	ori	r7,r7,MOTLOAD_SMP_IDLE@l
	stw	r7,MAILBOX_LOGIN_OFFSET(r6)	/* Set status to IDLE */
	li	r5,0
	stw	r5,MAILBOX_CMD_OFFSET(r6)	/* Set current cmd to null */
10:
	li	r3,1000				/* 1 millisecond delay */
	bl	udelay
	lis	r6,hxeb100_wait_for_kernel_mailbox@h
	ori	r6,r6,hxeb100_wait_for_kernel_mailbox@l
	lwz	r4,MAILBOX_CMD_OFFSET(r6)	/* load current cmd */	
	lis	r5,MOTLOAD_SMP_START_CMD@h
	ori	r5,r5,MOTLOAD_SMP_START_CMD@l
	cmpw	0,r4,r5
	bne	10b				/* wait for start cmd */

	lis	r7,MOTLOAD_SMP_EXEC@h
	ori	r7,r7,MOTLOAD_SMP_EXEC@l
	stw	r7,MAILBOX_LOGIN_OFFSET(r6)	/* set status to EXEC */

	bl	disable_6xx_mmu
	bl	disable_6xx_l1cache
	bl	_setup_L2CR
	bl	_setup_L3CR

	lis	r6,hxeb100_wait_for_kernel_mailbox@h
	ori	r6,r6,hxeb100_wait_for_kernel_mailbox@l
	lwz	r5,MAILBOX_FUNC_OFFSET(r6)	/* get start address */
	mtlr	r5
	blr

	.comm	.stack,4096*2,4
