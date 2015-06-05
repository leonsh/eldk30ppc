/***********************************************************************
 *
 * (C) Copyright 2002
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 *
 * This module provides support for the VMEBus (and other external)
 * interrupts on CU824 VMEBus systems.
 *
 ***********************************************************************/


/***************************************************************************
 * Exported API functions
 ***************************************************************************
 */
extern int request_cu824_irq (unsigned int, void (*)(int,void *,struct pt_regs *),
			      void *);
extern int free_cu824_irq    (unsigned int);
extern int vme_interrupt_enable  (unsigned int);
extern int vme_interrupt_disable (unsigned int);

/**************************************************************************/
