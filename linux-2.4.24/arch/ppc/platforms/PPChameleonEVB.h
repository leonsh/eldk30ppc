/*
 * arch/ppc/platforms/PPChameleonEVB.h
 *
 * Support for PPChameleon/PPChameleonEVB boards.
 *
 * Author: llandre
 *         Maintained by DAVE Srl <info@wawnet.biz>
 *
 * 2003 (c) DAVE Srl.  This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifdef __KERNEL__
#ifndef __ASM_PPCCHAMELEONEVB_H__
#define __ASM_PPCCHAMELEONEVB_H__

/* 405EP */
#include <platforms/ibm405ep.h>
/*#include <asm/io.h>*/

#include <asm/ppcboot.h>

#ifndef __ASSEMBLY__

/* Some 4xx parts use a different timebase frequency from the internal clock.
*/
#define bi_tbfreq bi_intfreq


#if 0
/* Memory map for the IBM 405EP evaluation board.
 * Generic 4xx plus RTC.
 */

extern void *ash405_rtc_base;
#define ASH405_RTC_PADDR	((uint)0xf0000000)
#define ASH405_RTC_VADDR	ASH405_RTC_PADDR
#define ASH405_RTC_SIZE		((uint)8*1024)
#endif

/* The UART clock is based off an internal clock -
 * define BASE_BAUD based on the internal clock and divider(s).
 * Since BASE_BAUD must be a constant, we will initialize it
 * using clock/divider values which OpenBIOS initializes
 * for typical configurations at various CPU speeds.
 * The base baud is calculated as (FWDA / EXT UART DIV / 16)
 */
#define BASE_BAUD       0

#define PPC4xx_MACHINE_NAME     "DAVE PPChameleonEVB"


/******************************************************************************
 ******************************************************************************
 ******************************************************************************
 * Some internal regs are not defined elsewhere so we put 'em here            *
 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/




/**************/
/* NAND stuff */
/**************/
/* Internal NAND */
#define CFG_NAND0_PADDR ((uint)0xFF400000)
#define CFG_NAND0_VADDR CFG_NAND0_PADDR
#define CFG_NAND0_CE  (0x80000000 >> 1)  /* our CE is GPIO1 */
#define CFG_NAND0_CLE (0x80000000 >> 2)  /* our CLE is GPIO2 */
#define CFG_NAND0_ALE (0x80000000 >> 3)  /* our ALE is GPIO3 */
#define CFG_NAND0_RDY (0x80000000 >> 4)  /* our RDY is GPIO4 */
/* External NAND */
#define CFG_NAND1_PADDR 0xFF000000
#define CFG_NAND1_VADDR CFG_NAND1_PADDR
#define CFG_NAND1_CE  (0x80000000 >> 14)  /* our CE is GPIO14 */
#define CFG_NAND1_CLE (0x80000000 >> 15)  /* our CLE is GPIO15 */
#define CFG_NAND1_ALE (0x80000000 >> 16)  /* our ALE is GPIO16 */
#define CFG_NAND1_RDY (0x80000000 >> 31)  /* our RDY is GPIO31 */
/* Macros to perform I/O operations */
#define MACRO_NAND_DISABLE_CE(nandptr) do \
{ \
	switch((unsigned long)nandptr) \
        { \
            case CFG_NAND0_PADDR: \
                out_be32((volatile unsigned*)GPIO0_OR, in_be32((volatile unsigned*)GPIO0_OR) | CFG_NAND0_CE); \
                break; \
            case CFG_NAND1_PADDR: \
                out_be32((volatile unsigned*)GPIO0_OR, in_be32((volatile unsigned*)GPIO0_OR) | CFG_NAND1_CE); \
                break; \
        } \
} while(0)

#define MACRO_NAND_ENABLE_CE(nandptr) do \
{ \
	switch((unsigned long)nandptr) \
        { \
            case CFG_NAND0_PADDR: \
                out_be32((volatile unsigned*)GPIO0_OR, in_be32((volatile unsigned*)GPIO0_OR) & ~CFG_NAND0_CE); \
                break; \
            case CFG_NAND1_PADDR: \
                out_be32((volatile unsigned*)GPIO0_OR, in_be32((volatile unsigned*)GPIO0_OR) & ~CFG_NAND1_CE); \
                break; \
        } \
} while(0)



#define MACRO_NAND_CTL_CLRALE(nandptr) do \
{ \
	switch((unsigned long)nandptr) \
        { \
            case CFG_NAND0_PADDR: \
                out_be32((volatile unsigned*)GPIO0_OR, in_be32((volatile unsigned*)GPIO0_OR) & ~CFG_NAND0_ALE); \
                break; \
            case CFG_NAND1_PADDR: \
                out_be32((volatile unsigned*)GPIO0_OR, in_be32((volatile unsigned*)GPIO0_OR) & ~CFG_NAND1_ALE); \
                break; \
        } \
} while(0)

#define MACRO_NAND_CTL_SETALE(nandptr) do \
{ \
	switch((unsigned long)nandptr) \
        { \
            case CFG_NAND0_PADDR: \
                out_be32((volatile unsigned*)GPIO0_OR, in_be32((volatile unsigned*)GPIO0_OR) | CFG_NAND0_ALE); \
                break; \
            case CFG_NAND1_PADDR: \
                out_be32((volatile unsigned*)GPIO0_OR, in_be32((volatile unsigned*)GPIO0_OR) | CFG_NAND1_ALE); \
                break; \
        } \
} while(0)

#define MACRO_NAND_CTL_CLRCLE(nandptr) do \
{ \
	switch((unsigned long)nandptr) \
        { \
            case CFG_NAND0_PADDR: \
                out_be32((volatile unsigned*)GPIO0_OR, in_be32((volatile unsigned*)GPIO0_OR) & ~CFG_NAND0_CLE); \
                break; \
            case CFG_NAND1_PADDR: \
                out_be32((volatile unsigned*)GPIO0_OR, in_be32((volatile unsigned*)GPIO0_OR) & ~CFG_NAND1_CLE); \
                break; \
        } \
} while(0)

#define MACRO_NAND_CTL_SETCLE(nandptr) do \
{ \
	switch((unsigned long)nandptr) \
        { \
            case CFG_NAND0_PADDR: \
                out_be32((volatile unsigned*)GPIO0_OR, in_be32((volatile unsigned*)GPIO0_OR) | CFG_NAND0_CLE); \
                break; \
            case CFG_NAND1_PADDR: \
                out_be32((volatile unsigned*)GPIO0_OR, in_be32((volatile unsigned*)GPIO0_OR) | CFG_NAND1_CLE); \
                break; \
        } \
} while(0)

#define MACRO_NAND_WAIT_READY(nand) \
while ( \
	switch((unsigned long)(((struct nand_chip *)nand)->IO_ADDR)) \
        { \
            case CFG_NAND0_PADDR: \
                !(in_be32((volatile unsigned*)GPIO0_IR) & CFG_NAND0_RDY); \
                break; \
            case CFG_NAND1_PADDR: \
                !(in_be32((volatile unsigned*)GPIO0_IR) & CFG_NAND1_RDY); \
                break; \
        } \
)




#endif /* !__ASSEMBLY__ */
#endif /* __ASM_PPCCHAMELEONEVB_H__ */
#endif /* __KERNEL__ */
