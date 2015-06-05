/* Board information for the EST8260, which should be generic for
 * all 8260 boards.  The IMMR is now given to us so the hard define
 * will soon be removed.  All of the clock values are computed from
 * the configuration SCMR and the Power-On-Reset word.
 */

#define IMAP_ADDR	((uint)0xf0000000)    // hwuang, 10/28-04

#define PHY_INTERRUPT	SIU_INT_IRQ7   //dupeng, 01/13/04
/* A Board Information structure that is given to a program when
 * prom starts it up.
 */
typedef struct bd_info {
	unsigned long	bi_memstart;	/* Memory start address */
	unsigned long	bi_memsize;	/* Memory (end) size in bytes */
	unsigned long	bi_flashstart;	/* start of FLASH memory */
	unsigned long	bi_flashsize;	/* size	 of FLASH memory */
	unsigned long	bi_flashoffset; /* reserved area for startup monitor */
        unsigned long   bi_sramstart;   /* start of SRAM memory */
	unsigned long   bi_sramsize;    /* size of SRAM memory */

	unsigned long	bi_immr;	/* IMMR when called from boot rom */

	unsigned long   bi_bootflags;   /* boot flag (for Lynx OS) */

	unsigned long   bi_ip_addr;     /* IP Address */
	unsigned char   bi_enetaddr[6]; /* Ethernet address */
	unsigned short  bi_ethspeed;    /* Ethernet speed in Mbps */

	unsigned long	bi_intfreq;	/* Internal Freq, in Hz */
	unsigned long	bi_busfreq;	/* Bus Freq, in MHz */
	unsigned long	bi_cpmfreq;	/* CPM Freq, in MHz */
	unsigned long	bi_brgfreq;	/* BRG Freq, in MHz */
	unsigned long	bi_sccfreq;	/* SCC Freq, in MHz */
	unsigned long	bi_vco;		/* VCO Out from PLL */
	unsigned long	bi_baudrate;	/* Default console baud rate */
} bd_t;

extern bd_t m8xx_board_info;

