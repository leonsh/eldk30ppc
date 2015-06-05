/*
 * CU824 board specific definitions
 *
 * Copyright (c) 2001 Wolfgang Denk (wd@denx.de)
 */

#ifndef __PPC_PLATFORMS_CU824_H
#define __PPC_PLATFORMS_CU824_H

#include <asm/ppcboot.h>

#define M824O_EUMB_BASE		0xFCE00000

/*
 * CU824 local interrupts
 */
#define	CU824_IRQ_ABORT_AC	0	/* Abort Key / VMEbus ACFail */
#define	CU824_IRQ_PCI		1	/* PCI-IRQ(0-3)		     */
#define	CU824_IRQ_SIO		2	/* SIO-A / SIO-B	     */
#define	CU824_IRQ_VME_INTR	3	/* VMEIRQ(1-7)		     */
#define	CU824_IRQ_MAIL_LM_RTC	4	/* Mailbox / LM81 / RTC	     */

/*
 * On the CU824 here are 2 types of interrupts:
 *	  7 VME Bus Interrupts and
 *	  4 Onboard Interrupts
 */
#define	NR_VME_INTERRUPTS	7

#define	CU824_ONBOARD_IRQ_ABORT_AC		(NR_VME_INTERRUPTS + 0)	
#define	CU824_ONBOARD_IRQ_PCI			(NR_VME_INTERRUPTS + 1)
#define	CU824_ONBOARD_IRQ_SIO			(NR_VME_INTERRUPTS + 2)
#define	CU824_ONBOARD_IRQ_MAIL_LM_RTC		(NR_VME_INTERRUPTS + 3)

typedef volatile unsigned char	vuchar;

/*
 * CU824 Board Control and Status registers
 */
typedef struct cu824_bcsr_s {
	vuchar	reserved0 [3], sac;	/* Address Compare Register			*/
	vuchar	reserved1 [3], vie;	/* VMEBus Interrupt Enable Register		*/
	vuchar	reserved2 [3], lie;	/* Local Interrupt Enable Register			*/
	vuchar	reserved3 [3], saa;	/* System Address Register			*/
	vuchar	reserved4 [3], bcr;	/* Board Control Register			*/
	vuchar	reserved5 [3], vis;	/* VMEBus Interrupt Status Register		*/
	vuchar	reserved6 [3], lis;	/* Local Interrupt Status Register		*/
	vuchar	reserved7 [3], pis;	/* PCI Interrupt Status Register		*/
	vuchar	reserved8 [3], wdr;	/* Watchdog Retriggered Port			*/
	vuchar	reserved9 [3], cmi;	/* Clear Mailbox Interrupt Register		*/
	vuchar	reserveda [3], brr;	/* Board Revision Register			*/
	vuchar	reservedb [3], pll;	/* PLL State Register				*/
	vuchar	reservedc [3], led;	/* LED Control Register				*/
} cu824_bcsr_t;

#define	CU824_LED_U		0x01	/* "U" LED Bit	*/
#define	CU824_LED_R		0x02	/* "R" LED Bit	*/

/*
 * Onboard interrupt mask registers bit map (BCR|LIE)
 */
#define CU824_ONBOARD_PCI0		0x01
#define CU824_ONBOARD_PCI1		0x02
#define CU824_ONBOARD_PCI2		0x04
#define CU824_ONBOARD_PCI3		0x08
#define CU824_ONBOARD_SIO_B		0x10
#define CU824_ONBOARD_SIO_A		0x20
#define CU824_ONBOARD_MAILBOX		0x40
#define CU824_ONBOARD_ABORT_AC		0x80
#define CU824_ONBOARD_RTC		0x4000
#define CU824_ONBOARD_LM		0x8000

#endif /* __PPC_PLATFORMS_CU824_H */
