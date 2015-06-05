/*
 * DAB4K board specific definitions (based on TQM860L)
 * 
 * Copyright (c) 1999,2000,2001 Wolfgang Denk (wd@denx.de)
 * Modified  (m) 2002 Steven Scholz (steven.scholz@imc-berlin.de)
 */

#ifndef __MACH_DAB4K_H
#define __MACH_DAB4K_H

#include <linux/config.h>
 
#include <asm/ppcboot.h>

#define __BOARD__		"DAB4K"
#define CONFIG_DAB4K_VERSION	2

#define DAB4K_IMMR_BASE	0xFFF00000	/* phys. addr of IMMR			*/
#define DAB4K_IMAP_SIZE	(64 * 1024)	/* size of mapped area			*/

#define IMAP_ADDR		DAB4K_IMMR_BASE	/* physical base address of IMMR area	*/
#define IMAP_SIZE		DAB4K_IMAP_SIZE	/* mapped size of IMMR area		*/

/*-----------------------------------------------------------------------
 * PCMCIA stuff
 *-----------------------------------------------------------------------
 *
 */
#define PCMCIA_MEM_SIZE		( 64 << 20 )

#if defined(CONFIG_PCMCIA_M8XX) || defined(CONFIG_PCMCIA_M8XX_MODULE)
/* define IO_BASE for pcmcia */
#define _IO_BASE                0x80000000
#define _IO_BASE_SIZE           0x1000

#ifdef CONFIG_IDE
#define ide_request_irq(irq,hand,flg,dev,id)    request_8xxirq((irq),(hand),(flg),(dev),(id))
#endif

#endif

/*
 * Definitions for IDE0 Interface
 */

#define	MAX_HWIFS	1	/* overwrite default in include/asm-ppc/ide.h	*/

#if defined(CONFIG_IDE_8xx_PCCARD)
# define IDE0_BASE_OFFSET		0
# define IDE0_DATA_REG_OFFSET		(PCMCIA_MEM_SIZE + 0x320)
# define IDE0_ERROR_REG_OFFSET		(2 * PCMCIA_MEM_SIZE + 0x320 + 1)
# define IDE0_NSECTOR_REG_OFFSET	(2 * PCMCIA_MEM_SIZE + 0x320 + 2)
# define IDE0_SECTOR_REG_OFFSET		(2 * PCMCIA_MEM_SIZE + 0x320 + 3)
# define IDE0_LCYL_REG_OFFSET		(2 * PCMCIA_MEM_SIZE + 0x320 + 4)
# define IDE0_HCYL_REG_OFFSET		(2 * PCMCIA_MEM_SIZE + 0x320 + 5)
# define IDE0_SELECT_REG_OFFSET		(2 * PCMCIA_MEM_SIZE + 0x320 + 6)
# define IDE0_STATUS_REG_OFFSET		(2 * PCMCIA_MEM_SIZE + 0x320 + 7)
# define IDE0_CONTROL_REG_OFFSET	0x0106
# define IDE0_IRQ_REG_OFFSET		0x000A	/* not used			*/
#elif defined(CONFIG_IDE_EXT_DIRECT)
/*# define CFG_ATA_BASE_ADDR		0xFE000000*/
# define CFG_ATA_BASE_ADDR		0x90000000
# define IDE0_BASE_OFFSET		0x0000
# define IDE0_DATA_REG_OFFSET		0x0000
# define IDE0_ERROR_REG_OFFSET		0x0002
# define IDE0_NSECTOR_REG_OFFSET	0x0004
# define IDE0_SECTOR_REG_OFFSET		0x0006
# define IDE0_LCYL_REG_OFFSET		0x0008
# define IDE0_HCYL_REG_OFFSET		0x000a
# define IDE0_SELECT_REG_OFFSET		0x000c
# define IDE0_STATUS_REG_OFFSET		0x000e
/*# define IDE0_CONTROL_REG_OFFSET	0x0106*/
# define IDE0_CONTROL_REG_OFFSET	0x0000	/* NOT USED!!!		*/
# define IDE0_IRQ_REG_OFFSET		0x000A  /* not used             */
#endif

/*
 * Interrupts
 */

#define FEC_INTERRUPT		SIU_LEVEL3	/*  7 */
#define PHY_INTERRUPT		SIU_IRQ5	/* 10 */

#if defined(CONFIG_IDE_8xx_PCCARD)
#define IDE0_INTERRUPT		SIU_LEVEL6	/* 13, i.e. PCMCIA interrupt	*/
#else
#define IDE0_INTERRUPT		SIU_IRQ1	/*  2, i.e. external interrupt	*/
#endif

#define IMCDEV_INTERRUPT	SIU_IRQ2	/* Power fail etc.*/

#define IMCDEV_BASE_ADDR	0xC0000000

/*
 * Status LEDs
 */
# define STATUS_LED_PAR		im_ioport.iop_pcpar
# define STATUS_LED_DIR		im_ioport.iop_pcdir
# define STATUS_LED_DAT		im_ioport.iop_pcdat
# undef STATUS_LED_ODR

# define STATUS_LED_BIT		0x0800	/* LED 0 is on PC.4	PCMCIA_LED	*/
# define STATUS_LED_PERIOD	(HZ)
# define STATUS_LED_STATE	STATUS_LED_OFF

# define STATUS_LED_BIT1	0x0400	/* LED 1 is on PC.5	STATUS_LED	*/
# define STATUS_LED_PERIOD1	(HZ)
# define STATUS_LED_STATE1	STATUS_LED_BLINKING

# define STATUS_LED_BIT2	0x0200	/* LED 2 is on PC.6	FRONT_LED	*/
# define STATUS_LED_PERIOD2	(HZ)
# define STATUS_LED_STATE2	STATUS_LED_ON

# define STATUS_LED_ACTIVE	1		/* LED on for bit == 1	*/
# define STATUS_LED_BOOT	1		/* LED 0 used for boot status */

#define PCMCIA_LED		0
#define STATUS_LED		1
#define FRONT_LED		2

/* We don't use the 8259.
*/
#define NR_8259_INTS	0

/* Generic 8xx type
#define _MACH_8xx (_MACH_tqm8xxL)
*/

#endif	/* __MACH_DAB4K_H */
