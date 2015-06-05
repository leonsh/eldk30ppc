/*
 * A collection of structures, addresses, and values associated with
 * the Analogue & Micro Adder-II MPC852T boards. Adapted from "mbx.h"
 *
 * Copyright (c) 2003 Gary Thomas <gary@mlbassoc.com>
 * Copyright (c) 1997 Dan Malek (dmalek@jlc.net)
 */
#ifdef __KERNEL__
#ifndef __MACH_ADDERII_DEFS
#define __MACH_ADDERII_DEFS

#ifndef __ASSEMBLY__
/* A Board Information structure that is given to a program when
 * RedBoot starts it up.
 */
typedef struct bd_info {
	unsigned int	bi_tag;		/* Should be 0x42444944 "BDID" */
	unsigned int	bi_size;	/* Size of this structure */
	unsigned int	bi_revision;	/* revision of this structure */
	unsigned int	bi_bdate;	/* EPPCbug date, i.e. 0x11061997 */
	unsigned int	bi_memstart;	/* Memory start address */
	unsigned int	bi_memsize;	/* Memory (end) size in bytes */
	unsigned int	bi_intfreq;	/* Internal Freq, in Hz */
	unsigned int	bi_busfreq;	/* Bus Freq, in Hz */
	unsigned int	bi_clun;	/* Boot device controller */
	unsigned int	bi_dlun;	/* Boot device logical dev */
	unsigned char	bi_enetaddr[6];
	unsigned int	bi_baudrate;    
        unsigned char   *bi_cmdline;    /*  Pointer to command line */
} bd_t;

/* 
 * Memory map for the Adder-II as configured by RedBoot.
 */
#define IMAP_ADDR		((uint)0xfa200000)         /* Onboard I/O */
#define IMAP_SIZE		((uint)(64 * 1024))

#define FEC_INTERRUPT    SIU_LEVEL1     /* FEC interrupt */
#endif /* !__ASSEMBLY__ */

/* The Adder-II does not use the 8259.
*/
#define NR_8259_INTS	0

#endif
#endif /* __KERNEL__ */
