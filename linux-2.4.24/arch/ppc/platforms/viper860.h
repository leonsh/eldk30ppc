/*
 * BK Id: SCCS/s.am860.h X.XX XX/XX/XX XX:XX:XX gdt
 */
/*
 * A collection of structures, addresses, and values associated with
 * the Analogue & Micro MPC8xx boards. Adapted from the mbx.h version.
 *
 * Copyright (c) 2003 Gary Thomas <gary@chez-thomas.org>
 * Copyright (c) 1997 Dan Malek (dmalek@jlc.net)
 */
#ifdef __KERNEL__
#ifndef __MACH_AM860_DEFS
#define __MACH_AM860_DEFS

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

/* Memory map for the Viper as configured by RedBoot.
 */
#define VIPER_CSR_ADDR		((uint)0xfa100000)         /* LEDs, etc */
#define VIPER_CSR_SIZE		((uint)(1 * 1024 * 1024))
#define IMAP_ADDR		((uint)0xfa200000)         /* Onboard I/O */
#define IMAP_SIZE		((uint)(64 * 1024))

#define FEC_INTERRUPT    SIU_LEVEL1     /* FEC interrupt */
#endif /* !__ASSEMBLY__ */

/* The AM860 does not use the 8259.
*/
#define NR_8259_INTS	0

#endif
#endif /* __KERNEL__ */
