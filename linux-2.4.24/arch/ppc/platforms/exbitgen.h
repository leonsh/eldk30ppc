/*
 *
 *    Copyright (c) 1999 Grant Erickson <grant@lcse.umn.edu>
 *
 *    Copyright 2000 MontaVista Software Inc.
 *	PPC405 modifications
 * 	Author: MontaVista Software, Inc.
 *         	frank_rowand@mvista.com or source@mvista.com
 * 	   	debbie_chu@mvista.com
 *
 *    Copyright 2001 Vitesse Semiconductor Corp.
 *      Based on walnut.h.
 *
 *    Module name: exbitgen.h
 *    (Why does this say "ppc405.h" in walnut.h)
 *
 *    Description:
 *      Macros, definitions, and data structures specific to the IBM PowerPC
 *      based boards.
 *
 *      This includes:
 *
 *         VITESSE "ExbitGen" 405GP generic CPU board.
 *
 */

#ifdef __KERNEL__
#ifndef	__EXBITGEN_H__
#define	__EXBITGEN_H__

/* We have a 405GP core */
#include <platforms/ibm405gp.h>

#ifndef __ASSEMBLY__

/* bd_t structure */
#include <asm/ppcboot.h>
#define bi_tbfreq bi_intfreq

#ifdef __cplusplus
extern "C" {
#endif

#define BASE_BAUD		691200
	
/* Memory map for the VITESSE "ExbitGen" 405GP generic CPU board.
 * Generic 4xx.
 */

#define EXBITGEN_CPLD_ADDR    ((uint)0x10000000)
#define EXBITGEN_CPLD_SIZE    ((uint)1*1024*1024)
#define EXBITGEN_FPGA8BIT_ADDR    ((uint)0x10100000)
#define EXBITGEN_FPGA8BIT_SIZE    ((uint)1*1024*1024)
#define EXBITGEN_FPGA32BIT_ADDR    ((uint)0x10200000)
#define EXBITGEN_FPGA32BIT_SIZE    ((uint)1*1024*1024)

#define EXBITGEN_FPGA_ADDR    EXBITGEN_FPGA32BIT_ADDR
#define EXBITGEN_FPGA_SIZE    EXBITGEN_FPGA32BIT_SIZE

#define EXBITGEN_CS7_TARGET_ADDR  ((uint)0x40000000)
#define EXBITGEN_CS6_TARGET_ADDR  ((uint)0x48000000)
#define EXBITGEN_CS2_TARGET_ADDR  ((uint)0x50000000)
#define EXBITGEN_CS3_TARGET_ADDR  ((uint)0x58000000)
#define EXBITGEN_CS2367_TARGET_BS 5 /* 0: 1MB, 1: 2MB, 2: 4MB, 3: 8MB, 4: 16MB, 5: 32MB, 6: 64MB, 7: 128MB (max) */
#define EXBITGEN_CS2367_TARGET_SIZE  ((uint)(1<<EXBITGEN_CS2367_TARGET_BS)*1024*1024)

#define VITGEN_CPLD_ADDR		EXBITGEN_CPLD_ADDR
#define VITGEN_CPLD_SIZE		EXBITGEN_CPLD_SIZE
#define VITGEN_FPGA_ADDR		EXBITGEN_FPGA_ADDR
#define VITGEN_FPGA_SIZE		EXBITGEN_FPGA_SIZE
#define VITGEN_CS2_TARGET_ADDR		EXBITGEN_CS2_TARGET_ADDR
#define VITGEN_CS2367_TARGET_SIZE	EXBITGEN_CS2367_TARGET_SIZE
#define VITGEN_CS3_TARGET_ADDR		EXBITGEN_CS3_TARGET_ADDR
#define VITGEN_CS6_TARGET_ADDR		EXBITGEN_CS6_TARGET_ADDR
#define VITGEN_CS7_TARGET_ADDR		EXBITGEN_CS7_TARGET_ADDR
#define VITGEN_CS2367_TARGET_BS		EXBITGEN_CS2367_TARGET_BS

extern char pci_irq_table[][4];

#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */

#define PPC4xx_MACHINE_NAME	"Vitesse ExbitGen"

#endif /* __EXBITGEN_H__ */
#endif /* __KERNEL__ */


