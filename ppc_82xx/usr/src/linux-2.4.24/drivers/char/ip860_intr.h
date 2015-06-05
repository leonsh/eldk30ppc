/***********************************************************************
 *
 * (C) Copyright 2001-2004
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 *
 * This module provides support for the VMEBus (and other external)
 * interrupts on IP860 VMEBus systems.
 *
 ***********************************************************************/


/***************************************************************************
 * Exported API functions
 ***************************************************************************
 */
extern int request_ip860_irq (unsigned int, void (*)(int,void *,struct pt_regs *),
			      void *);
extern int free_ip860_irq    (unsigned int);
extern int vme_interrupt_enable  (unsigned int);
extern int vme_interrupt_disable (unsigned int);


/***************************************************************************
 * Hardware related stuff
 ***************************************************************************
 */
#define	IP860_IRQ_ABORT		SIU_IRQ1	/* Abort Key		*/
#define IP860_IRQ_VME_INTR	SIU_IRQ3	/* VMEBus Interrupt	*/
#define IP860_IRQ_IPMOD_A	SIU_IRQ4	/* IP Module A		*/
#define IP860_IRQ_IPMOD_B	SIU_IRQ5	/* IP Module B		*/
#define IP860_IRQ_VME_MBSF	SIU_IRQ6	/* Mailbox & SYSFAIL	*/

#define IP860_IPA_IRQ0_ENABLE	0x01
#define IP860_IPA_IRQ1_ENABLE	0x02
#define IP860_IPB_IRQ0_ENABLE	0x04
#define IP860_IPB_IRQ1_ENABLE	0x08
#define IP860_MBOX_IRQ_ENABLE	0x10
#define IP860_ABORT_ENABLE	0x80

#define IP860_IPA_IACK0_INDX	0x00000080	/* Index in short array */
#define IP860_IPA_IACK1_INDX	0x00000081
#define IP860_IPB_IACK0_INDX	0x00800080
#define IP860_IPB_IACK1_INDX	0x00800081

/*
 * On the IP860 here are 2 types of interrupts:
 *	256 VME Bus Interrupts and
 *	  6 Onboard Interrupts (Abort Key, IP Modules, etc.)
 */
#define	NR_VME_IVECT		8
#define	NR_VME_INTERRUPTS	256

#define	IP860_ONBOARD_IRQ_ABORT		(NR_VME_INTERRUPTS + 0)
#define IP860_ONBOARD_IRQ_MBSF		(NR_VME_INTERRUPTS + 1)
#define IP860_ONBOARD_IRQ_IP_B_1	(NR_VME_INTERRUPTS + 2)
#define IP860_ONBOARD_IRQ_IP_B_0	(NR_VME_INTERRUPTS + 3)
#define IP860_ONBOARD_IRQ_IP_A_1	(NR_VME_INTERRUPTS + 4)
#define IP860_ONBOARD_IRQ_IP_A_0	(NR_VME_INTERRUPTS + 5)

/**************************************************************************/

