/* Board specific functions for those embedded 8xx boards that do
 * not have boot monitor support for board information.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/types.h>
#include <linux/config.h>
#include <linux/string.h>
#ifdef CONFIG_8xx
#include <asm/mpc8xx.h>
#endif
#ifdef CONFIG_8260
#include <asm/mpc8260.h>
#include <asm/immap_8260.h>
#endif
#ifdef CONFIG_40x
#include <asm/io.h>
#endif
#if defined(CONFIG_405GP) || defined(CONFIG_NP405H) || defined(CONFIG_NP405L)
#include <linux/netdevice.h>
#endif
extern unsigned long timebase_period_ns;

/* For those boards that don't provide one.
*/
#if !defined(CONFIG_MBX)
static	bd_t	bdinfo;
#endif

/* IIC functions.
 * These are just the basic master read/write operations so we can
 * examine serial EEPROM.
 */
extern void	iic_read(uint devaddr, u_char *buf, uint offset, uint count);

/* Supply a default Ethernet address for those eval boards that don't
 * ship with one.  This is an address from the MBX board I have, so
 * it is unlikely you will find it on your network.
 */
static	ushort	def_enet_addr[] = { 0x0800, 0x3e26, 0x1559 };

extern char *cmd_line;

#if defined(CONFIG_AM8XX)

/* 
 * RedBoot hands us a ready to go board descriptor.
 */
void
embed_config(bd_t **bdp)
{
       bd_t    *bd;

       bd = *bdp;
       if (bd->bi_cmdline) {
           char *s, *d;
           s = bd->bi_cmdline;
           d = cmd_line;
           while (*s) *d++ = *s++;
           *d = '\0';
       }
}
#endif /* CONFIG_AM8XX */


#if defined(CONFIG_MBX)

/* The MBX hands us a pretty much ready to go board descriptor.  This
 * is where the idea started in the first place.
 */
void
embed_config(bd_t **bdp)
{
	u_char	*mp;
	u_char	eebuf[128];
	int i;
	bd_t    *bd;

	bd = *bdp;

	/* Read the first 128 bytes of the EEPROM.  There is more,
	 * but this is all we need.
	 */
	iic_read(0xa4, eebuf, 0, 128);

	/* All we are looking for is the Ethernet MAC address.  The
	 * first 8 bytes are 'MOTOROLA', so check for part of that.
	 * If it's there, assume we have a valid MAC address.  If not,
	 * grab our default one.
	 */
	if ((*(uint *)eebuf) == 0x4d4f544f)
		mp = &eebuf[0x4c];
	else
		mp = (u_char *)def_enet_addr;

	for (i=0; i<6; i++)
		bd->bi_enetaddr[i] = *mp++;

	/* The boot rom passes these to us in MHz.  Linux now expects
	 * them to be in Hz.
	 */
	bd->bi_intfreq *= 1000000;
	bd->bi_busfreq *= 1000000;

	/* Stuff a baud rate here as well.
	*/
	bd->bi_baudrate = 9600;
}
#endif /* CONFIG_MBX */

#if defined(CONFIG_RPXLITE) || defined(CONFIG_RPXCLASSIC) || \
	defined(CONFIG_RPX6) || defined(CONFIG_EP405) || defined(CONFIG_RMU)
/* Helper functions for Embedded Planet boards.
*/
/* Because I didn't find anything that would do this.......
*/
u_char
aschex_to_byte(u_char *cp)
{
	u_char	byte, c;

	c = *cp++;

	if ((c >= 'A') && (c <= 'F')) {
		c -= 'A';
		c += 10;
	} else if ((c >= 'a') && (c <= 'f')) {
		c -= 'a';
		c += 10;
	} else
		c -= '0';

	byte = c * 16;

	c = *cp;

	if ((c >= 'A') && (c <= 'F')) {
		c -= 'A';
		c += 10;
	} else if ((c >= 'a') && (c <= 'f')) {
		c -= 'a';
		c += 10;
	} else
		c -= '0';

	byte += c;

	return(byte);
}

static void
rpx_eth(bd_t *bd, u_char *cp)
{
	int	i;

	for (i=0; i<6; i++) {
		bd->bi_enetaddr[i] = aschex_to_byte(cp);
		cp += 2;
	}
}

#ifdef CONFIG_RPX6
static uint
rpx_baseten(u_char *cp)
{
	uint	retval;

	retval = 0;

	while (*cp != '\n') {
		retval *= 10;
		retval += (*cp) - '0';
		cp++;
	}
	return(retval);
}
#endif

#if defined(CONFIG_RPXLITE) || defined(CONFIG_RPXCLASSIC) || defined(CONFIG_RMU)
static void
rpx_brate(bd_t *bd, u_char *cp)
{
	uint	rate;

	rate = 0;

	while (*cp != '\n') {
		rate *= 10;
		rate += (*cp) - '0';
		cp++;
	}

	bd->bi_baudrate = rate * 100;
}

static void
rpx_cpuspeed(bd_t *bd, u_char *cp)
{
	uint	num, den;

	num = den = 0;

	while (*cp != '\n') {
		num *= 10;
		num += (*cp) - '0';
		cp++;
		if (*cp == '/') {
			cp++;
			den = (*cp) - '0';
			break;
		}
	}

	/* I don't know why the RPX just can't state the actual
	 * CPU speed.....
	 */
	if (den) {
		num /= den;
		num *= den;
	}
	bd->bi_intfreq = bd->bi_busfreq = num * 1000000;

	/* The 8xx can only run a maximum 50 MHz bus speed (until
	 * Motorola changes this :-).  Greater than 50 MHz parts
	 * run internal/2 for bus speed.
	 */
	if (num > 50)
		bd->bi_busfreq /= 2;
}
#endif

#if defined(CONFIG_RPXLITE) || defined(CONFIG_RPXCLASSIC) || \
    defined(CONFIG_EP405) || defined(CONFIG_RMU)
static void
rpx_memsize(bd_t *bd, u_char *cp)
{
	uint	size;

	size = 0;

	while (*cp != '\n') {
		size *= 10;
		size += (*cp) - '0';
		cp++;
	}

	bd->bi_memsize = size * 1024 * 1024;
}
#endif /* LITE || CLASSIC || EP405 || RMU */
#if defined(CONFIG_EP405)
static void
rpx_nvramsize(bd_t *bd, u_char *cp)
{
	uint	size;

	size = 0;

	while (*cp != '\n') {
		size *= 10;
		size += (*cp) - '0';
		cp++;
	}

	bd->bi_nvramsize = size * 1024;
}
#endif /* CONFIG_EP405 */

#endif	/* Embedded Planet boards */

#if defined(CONFIG_RPXLITE) || defined(CONFIG_RPXCLASSIC) || defined(CONFIG_RMU)

/* Read the EEPROM on the RPX-Lite board.
*/
void
embed_config(bd_t **bdp)
{
	u_char	eebuf[256], *cp;
	bd_t	*bd;

	/* Read the first 256 bytes of the EEPROM.  I think this
	 * is really all there is, and I hope if it gets bigger the
	 * info we want is still up front.
	 */
	bd = &bdinfo;
	*bdp = bd;

#if 1
	iic_read(0xa8, eebuf, 0, 128);
	iic_read(0xa8, &eebuf[128], 128, 128);

	/* We look for two things, the Ethernet address and the
	 * serial baud rate.  The records are separated by
	 * newlines.
	 */
	cp = eebuf;
	for (;;) {
		if (*cp == 'E') {
			cp++;
			if (*cp == 'A') {
				cp += 2;
				rpx_eth(bd, cp);
			}
		}
		if (*cp == 'S') {
			cp++;
			if (*cp == 'B') {
				cp += 2;
				rpx_brate(bd, cp);
			}
		}
		if (*cp == 'D') {
			cp++;
			if (*cp == '1') {
				cp += 2;
				rpx_memsize(bd, cp);
			}
		}
		if (*cp == 'H') {
			cp++;
			if (*cp == 'Z') {
				cp += 2;
				rpx_cpuspeed(bd, cp);
			}
		}

		/* Scan to the end of the record.
		*/
		while ((*cp != '\n') && (*cp != 0xff))
			cp++;

		/* If the next character is a 0 or ff, we are done.
		*/
		cp++;
		if ((*cp == 0) || (*cp == 0xff))
			break;
	}
	bd->bi_memstart = 0;
#else
	/* For boards without initialized EEPROM.
	*/
	bd->bi_memstart = 0;
	bd->bi_memsize = (8 * 1024 * 1024);
	bd->bi_intfreq = 48000000;
	bd->bi_busfreq = 48000000;
	bd->bi_baudrate = 9600;
#endif
}
#endif /* RPXLITE || RPXCLASSIC */

#ifdef CONFIG_BSEIP
/* Build a board information structure for the BSE ip-Engine.
 * There is more to come since we will add some environment
 * variables and a function to read them.
 */
void
embed_config(bd_t **bdp)
{
	u_char	*cp;
	int	i;
	bd_t	*bd;

	bd = &bdinfo;
	*bdp = bd;

	/* Baud rate and processor speed will eventually come
	 * from the environment variables.
	 */
	bd->bi_baudrate = 9600;

	/* Get the Ethernet station address from the Flash ROM.
	*/
	cp = (u_char *)0xfe003ffa;
	for (i=0; i<6; i++) {
		bd->bi_enetaddr[i] = *cp++;
	}

	/* The rest of this should come from the environment as well.
	*/
	bd->bi_memstart = 0;
	bd->bi_memsize = (16 * 1024 * 1024);
	bd->bi_intfreq = 48000000;
	bd->bi_busfreq = 48000000;
}
#endif /* BSEIP */

#ifdef CONFIG_FADS
/* Build a board information structure for the FADS.
 */
void
embed_config(bd_t **bdp)
{
	u_char	*cp;
	int	i;
	bd_t	*bd;

	bd = &bdinfo;
	*bdp = bd;

	/* Just fill in some known values.
	 */
	bd->bi_baudrate = 9600;

	/* Use default enet.
	*/
	cp = (u_char *)def_enet_addr;
	for (i=0; i<6; i++) {
		bd->bi_enetaddr[i] = *cp++;
	}

	bd->bi_memstart = 0;
	bd->bi_memsize = (8 * 1024 * 1024);
	bd->bi_intfreq = 48000000;
	bd->bi_busfreq = 48000000;
}
#endif /* FADS */

#ifdef CONFIG_8260
/* Compute 8260 clock values if the rom doesn't provide them.
 * We can't compute the internal core frequency (I don't know how to
 * do that).
 */
static void
clk_8260(bd_t *bd)
{
	uint	scmr, vco_out, clkin;
	uint	plldf, pllmf, busdf, brgdf, cpmdf;
	volatile immap_t	*ip;

	ip = (immap_t *)IMAP_ADDR;
	scmr = ip->im_clkrst.car_scmr;

	/* The clkin is always bus frequency.
	*/
	clkin = bd->bi_busfreq;

	/* Collect the bits from the scmr.
	*/
	plldf = (scmr >> 12) & 1;
	pllmf = scmr & 0xfff;
	cpmdf = (scmr >> 16) & 0x0f;
	busdf = (scmr >> 20) & 0x0f;

	/* This is arithmetic from the 8260 manual.
	*/
	vco_out = clkin / (plldf + 1);
	vco_out *= 2 * (pllmf + 1);
	bd->bi_vco = vco_out;		/* Save for later */

	bd->bi_cpmfreq = vco_out / 2;	/* CPM Freq, in MHz */

	/* Set Baud rate divisor.  The power up default is divide by 16,
	 * but we set it again here in case it was changed.
	 */
	ip->im_clkrst.car_sccr = 1;	/* DIV 16 BRG */
	bd->bi_brgfreq = vco_out / 16;
}
#endif

#ifdef CONFIG_EST8260
void
embed_config(bd_t **bdp)
{
	u_char	*cp;
	int	i;
	bd_t	*bd;

	bd = *bdp;
#if 0
	/* This is here for debugging or for boot roms that don't
	 * provide a bd_t when starting us.  Hack to suit your needs.
	 */
	bd = &bdinfo;
	*bdp = bd;
	bd->bi_baudrate = 115200;
	bd->bi_intfreq = 200000000;
	bd->bi_busfreq = 66666666;
	bd->bi_cpmfreq = 66666666;
	bd->bi_brgfreq = 33333333;
	bd->bi_memsize = 16 * 1024 * 1024;

	cp = (u_char *)def_enet_addr;
	for (i=0; i<6; i++) {
		bd->bi_enetaddr[i] = *cp++;
	}
#else
	/* The boot rom passes these to us in MHz.  Linux now expects
	 * them to be in Hz.
	 */
	bd->bi_intfreq *= 1000000;
	bd->bi_busfreq *= 1000000;
	bd->bi_cpmfreq *= 1000000;
	bd->bi_brgfreq *= 1000000;
#endif

	clk_8260(bd);
}
#endif /* EST8260 */

#ifdef CONFIG_SBS8260
void
embed_config(bd_t **bdp)
{
	u_char	*cp;
	int	i;
	bd_t	*bd;

	/* This should provided by the boot rom.
	 */
	bd = &bdinfo;
	*bdp = bd;
	bd->bi_baudrate = 9600;
	bd->bi_memsize = 64 * 1024 * 1024;

	/* Set all of the clocks.  We have to know the speed of the
	 * external clock.  The development board had 66 MHz.
	 */
	bd->bi_busfreq = 66666666;
	clk_8260(bd);

	/* I don't know how to compute this yet.
	*/
	bd->bi_intfreq = 133000000;


	cp = (u_char *)def_enet_addr;
	for (i=0; i<6; i++) {
		bd->bi_enetaddr[i] = *cp++;
	}
}
#endif /* SBS8260 */

#ifdef CONFIG_RPX6
void
embed_config(bd_t **bdp)
{
	u_char	*cp, *keyvals;
	int	i;
	bd_t	*bd;

	keyvals = (u_char *)*bdp;

	bd = &bdinfo;
	*bdp = bd;

	/* This is almost identical to the RPX-Lite/Classic functions
	 * on the 8xx boards.  It would be nice to have a key lookup
	 * function in a string, but the format of all of the fields
	 * is slightly different.
	 */
	cp = keyvals;
	for (;;) {
		if (*cp == 'E') {
			cp++;
			if (*cp == 'A') {
				cp += 2;
				rpx_eth(bd, cp);
			}
		}
		if (*cp == 'S') {
			cp++;
			if (*cp == 'B') {
				cp += 2;
				bd->bi_baudrate = rpx_baseten(cp);
			}
		}
		if (*cp == 'D') {
			cp++;
			if (*cp == '1') {
				cp += 2;
				bd->bi_memsize = rpx_baseten(cp) * 1024 * 1024;
			}
		}
		if (*cp == 'X') {
			cp++;
			if (*cp == 'T') {
				cp += 2;
				bd->bi_busfreq = rpx_baseten(cp);
			}
		}
		if (*cp == 'N') {
			cp++;
			if (*cp == 'V') {
				cp += 2;
				bd->bi_nvsize = rpx_baseten(cp) * 1024 * 1024;
			}
		}

		/* Scan to the end of the record.
		*/
		while ((*cp != '\n') && (*cp != 0xff))
			cp++;

		/* If the next character is a 0 or ff, we are done.
		*/
		cp++;
		if ((*cp == 0) || (*cp == 0xff))
			break;
	}
	bd->bi_memstart = 0;

	/* The memory size includes both the 60x and local bus DRAM.
	 * I don't want to use the local bus DRAM for real memory,
	 * so subtract it out.  It would be nice if they were separate
	 * keys.
	 */
	bd->bi_memsize -= 32 * 1024 * 1024;

	/* Set all of the clocks.  We have to know the speed of the
	 * external clock.
	 */
	clk_8260(bd);

	/* I don't know how to compute this yet.
	*/
	bd->bi_intfreq = 200000000;
}
#endif /* RPX6 for testing */

#ifdef CONFIG_ADS8260
void
embed_config(bd_t **bdp)
{
	u_char	*cp;
	int	i;
	bd_t	*bd;

	/* This should provided by the boot rom.
	 */
	bd = &bdinfo;
	*bdp = bd;
	bd->bi_baudrate = 9600;
	bd->bi_memsize = 16 * 1024 * 1024;

	/* Set all of the clocks.  We have to know the speed of the
	 * external clock.  The development board had 66 MHz.
	 */
	bd->bi_busfreq = 66666666;
	clk_8260(bd);

	/* I don't know how to compute this yet.
	*/
	bd->bi_intfreq = 200000000;


	cp = (u_char *)def_enet_addr;
	for (i=0; i<6; i++) {
		bd->bi_enetaddr[i] = *cp++;
	}
}
#endif /* ADS8260 */

#ifdef CONFIG_WILLOW
void
embed_config(bd_t **bdp)
{
	u_char	*cp;
	int	i;
	bd_t	*bd;

	/* Willow has Open Firmware....I should learn how to get this
	 * information from it.
	 */
	bd = &bdinfo;
	*bdp = bd;
	bd->bi_baudrate = 9600;
	bd->bi_memsize = 32 * 1024 * 1024;

	/* Set all of the clocks.  We have to know the speed of the
	 * external clock.  The development board had 66 MHz.
	 */
	bd->bi_busfreq = 66666666;
	clk_8260(bd);

	/* I don't know how to compute this yet.
	*/
	bd->bi_intfreq = 200000000;


	cp = (u_char *)def_enet_addr;
	for (i=0; i<6; i++) {
		bd->bi_enetaddr[i] = *cp++;
	}
}
#endif /* WILLOW */

#ifdef CONFIG_XILINX_ML300
#if !defined(XPAR_IIC_0_BASEADDR) || !defined(XPAR_PERSISTENT_0_IIC_0_BASEADDR)
/*
 * The ML300 uses an I2C SEEPROM to store the Ethernet MAC address, but
 * either an I2C interface or the SEEPROM aren't configured in.  If you
 * are in this situation, you'll need to define an alternative way of
 * storing the Ethernet MAC address.  To temporarily work around the
 * situation, you can simply comment out the following #error and a
 * hard-coded MAC will be used.
 */
#error I2C needed for obtaining the Ethernet MAC address
#define get_mac_addr(mac) 1
#else
static int
hexdigit(char c)
{
	if ('0' <= c && c <= '9')
		return c - '0';
	else if ('a' <= c && c <= 'f')
		return c - 'a' + 10;
	else if ('A' <= c && c <= 'F')
		return c - 'A' + 10;
	else
		return -1;
}

#include <xiic_l.h>
static int get_mac_addr(unsigned char *mac)
{
#define SEEPROM_DATA_SIZE \
 (XPAR_PERSISTENT_0_IIC_0_HIGHADDR - XPAR_PERSISTENT_0_IIC_0_BASEADDR + 1)
	unsigned char sdata[SEEPROM_DATA_SIZE], *cp, *cksum_val, *enet_val;
	unsigned char cksum;
	int i, done, msn, lsn;
	enum { BEGIN_KEY, IN_KEY, IN_VALUE } state;

	/*
	 * Fill our SEEPROM data array (sdata) from address
         * XPAR_PERSISTENT_0_IIC_0_BASEADDR of the SEEPROM at slave
         * address XPAR_PERSISTENT_0_IIC_0_EEPROMADDR.  We'll then parse
         * that data looking for a MAC address. */
	sdata[0] = XPAR_PERSISTENT_0_IIC_0_BASEADDR >> 8;
	sdata[1] = XPAR_PERSISTENT_0_IIC_0_BASEADDR & 0xFF;
	i = XIic_Send(XPAR_IIC_0_BASEADDR,
		      XPAR_PERSISTENT_0_IIC_0_EEPROMADDR>>1, sdata, 2);
	if (i != 2)
		return 1;	/* Couldn't send the address.  Return error. */
	i = XIic_Recv(XPAR_IIC_0_BASEADDR,
		      XPAR_PERSISTENT_0_IIC_0_EEPROMADDR>>1,
		      sdata, sizeof(sdata));
	if (i != sizeof(sdata))
		return 1;	/* Didn't read all the data.  Return error. */
	
	/* The SEEPROM should contain a series of KEY=VALUE parameters.
	 * Each KEY=VALUE is followed by a NULL character.  After the
	 * last one will be an extra NULL character.  Valid characters
	 * for KEYs are A to Z, 0 to 9 and underscore.  Any character
	 * other than NULL is valid for VALUEs.  In addition there is a
	 * checksum.  Do an initial pass to make sure the key/values
	 * look good and to find the C= (checksum) and E= (ethernet MAC
	 * address parameters. */
	cp = sdata;
	cksum_val = enet_val = NULL;
	cksum = 0;
	done = 0;
	state = BEGIN_KEY;
	while (!done) {
		/* Error if we didn't find the end of the data. */
		if (cp >= sdata + sizeof(sdata))
			return 1;

		switch (state) {
		case BEGIN_KEY:
			state = IN_KEY;
			if (*cp == 'C' && *(cp+1) == '=') {
				cksum_val = cp + 2;
				break;
			} else if (*cp == 'E' && *(cp+1) == '=') {
				enet_val = cp + 2;
				break;
			} else if (*cp == '\0') {
				/* Found the end of the data. */
				done = 1;
				break;
			}
			/* otherwise, fall through to validate the char. */
		case IN_KEY:
			switch (*cp) {
			case 'A'...'Z':
			case '0'...'9':
			case '_':
				break; /* Valid char.  Do nothing. */
			case '=':
				state = IN_VALUE;
				break;
			default:
				return 1; /* Invalid character.  Error. */
			}
			break;
		case IN_VALUE:
			if (*cp == '\0')
				state = BEGIN_KEY;
			break;
		}

		cksum += *(cp++);
	}

	/* Error if we couldn't find the checksum and MAC. */
	if (!cksum_val || !enet_val)
		return 1;

	/* At this point, we know that the structure of the data was
	 * correct and we have found where the checksum and MAC address
	 * values are. */

	/* Validate the checksum. */
	msn = hexdigit(cksum_val[0]);
	lsn = hexdigit(cksum_val[1]);
	if (cksum_val[2] != '\0' || msn < 0 || lsn < 0)
		return 1;	/* Error because it isn't two hex digits. */
	/* The sum of all the characters except for the two checksum
	 * digits should be the value of the two checksum digits.
	 */
	cksum -= cksum_val[0];
	cksum -= cksum_val[1];
	if (cksum != (msn << 4 | lsn))
		return 1;	/* Bad checksum. */

	/* Validate and set the MAC. */
	cp = enet_val;
	while (cp < enet_val + 12) {
		msn = hexdigit(*cp++);
		lsn = hexdigit(*cp++);
		if (msn < 0 || lsn < 0)
			return 1;
		*mac++ = msn << 4 | lsn;
	}
	if (*cp != '\0')
		return 1;

	/* Success */
	return 0;
}
#endif

void
embed_config(bd_t ** bdp)
{
	static const unsigned long line_size = 32;
	static const unsigned long congruence_classes = 256;
	unsigned long addr;
	unsigned long dccr;
	bd_t *bd;

	/*
	 * Invalidate the data cache if the data cache is turned off.
	 * - The 405 core does not invalidate the data cache on power-up
	 *   or reset but does turn off the data cache. We cannot assume
	 *   that the cache contents are valid.
	 * - If the data cache is turned on this must have been done by
	 *   a bootloader and we assume that the cache contents are
	 *   valid.
	 */
	__asm__("mfdccr %0": "=r" (dccr));
	if (dccr == 0) {
		for (addr = 0;
		     addr < (congruence_classes * line_size);
		     addr += line_size) {
			__asm__("dccci 0,%0": :"b"(addr));
		}
	}

	bd = &bdinfo;
	*bdp = bd;
	bd->bi_memsize = XPAR_DDR_0_SIZE;
	bd->bi_intfreq = XPAR_CORE_CLOCK_FREQ_HZ;
	bd->bi_busfreq = XPAR_PLB_CLOCK_FREQ_HZ;
	bd->bi_pci_busfreq = XPAR_PCI_0_CLOCK_FREQ_HZ;

	if (get_mac_addr(bd->bi_enetaddr)) {
		/* The SEEPROM is corrupted.  Set the address to
		 * Xilinx's preferred default.  However, first to
		 * eliminate a compiler warning because we don't really
		 * use def_enet_addr, we'll reference it.  The compiler
		 * optimizes it away so no harm done. */
		bd->bi_enetaddr[0] = def_enet_addr[0];
		bd->bi_enetaddr[0] = 0x00;
		bd->bi_enetaddr[1] = 0x0A;
		bd->bi_enetaddr[2] = 0x35;
		bd->bi_enetaddr[3] = 0x00;
		bd->bi_enetaddr[4] = 0x22;
		bd->bi_enetaddr[5] = 0x00;
	}
	timebase_period_ns = 1000000000 / bd->bi_tbfreq;
}
#endif /* CONFIG_XILINX_ML300 */

#ifdef CONFIG_IBM_OPENBIOS
/* This could possibly work for all treeboot roms.
*/
#if defined(CONFIG_ASH) || defined(CONFIG_BEECH) || defined(CONFIG_EVB405EP)
#define BOARD_INFO_VECTOR       0xFFF80B50 /* openbios 1.19 moved this vector down  - armin */
#else
#define BOARD_INFO_VECTOR	0xFFFE0B50
#endif

#ifdef CONFIG_BEECH
/* Several bootloaders have been used on Beech. We assume either
 * SSX or OpenBIOS */

#define SSX_BIOS_ADDR 		0xFFFF0000
#define SSX_BIOS_GET_BOARD_INFO 0

struct ssx_bios_id {
	unsigned int boot_branch;	/* Branch to bootcode */
	char ssx_bios[8];		/* "SSX BIOS" (no \0) */
	void (*bios_entry_point)(unsigned int, bd_t *); /* Call bios_entry_point(cmd, &data) */
};

extern int memcmp(const void *s1, const void *s2, size_t n);

static void get_board_info(bd_t **bdp)
{
	struct ssx_bios_id *ssx = (struct ssx_bios_id *)SSX_BIOS_ADDR;

	/* Check for SSX signature */

	if (memcmp(&ssx->ssx_bios, "SSX BIOS", 8) == 0) {
		ssx->bios_entry_point(SSX_BIOS_GET_BOARD_INFO, *bdp);
	} else {
		/* It's not SSX, so assume OpenBIOS */
		typedef void (*PFV)(bd_t *bd);
		((PFV)(*(unsigned long *)BOARD_INFO_VECTOR))(*bdp);
	}
}

void
embed_config(bd_t **bdp)
{
        *bdp = &bdinfo;
	get_board_info(bdp);
}
#else /* !CONFIG_BEECH */
void
embed_config(bd_t **bdp)
{
	u_char	*cp;
	int	i;
	bd_t	*bd, *treeboot_bd;
	bd_t *(*get_board_info)(void) =
	    (bd_t *(*)(void))(*(unsigned long *)BOARD_INFO_VECTOR);
#if !defined(CONFIG_STB03xxx)
	/* shut down the Ethernet controller that the boot rom
	 * sometimes leaves running.
	 */
	mtdcr(DCRN_MALCR(DCRN_MAL_BASE), MALCR_MMSR);                /* 1st reset MAL */
	while (mfdcr(DCRN_MALCR(DCRN_MAL_BASE)) & MALCR_MMSR) {};    /* wait for the reset */
	out_be32((u32 *)EMAC0_BASE, 0x20000000);        /* then reset EMAC */
#endif

	bd = &bdinfo;
	*bdp = bd;
	if ((treeboot_bd = get_board_info()) != NULL) {
		memcpy(bd, treeboot_bd, sizeof(bd_t));
	}
	else {
		/* Hmmm...better try to stuff some defaults.
		*/
		bd->bi_memsize = 16 * 1024 * 1024;
		cp = (u_char *)def_enet_addr;
		for (i=0; i<6; i++) {
			/* I should probably put different ones here,
			 * hopefully only one is used.
			 */
			bd->BD_EMAC_ADDR(0,i) = *cp;

#ifdef CONFIG_PCI
			bd->bi_pci_enetaddr[i] = *cp++;
#endif
		}
		bd->bi_tbfreq = 200 * 1000 * 1000;
		bd->bi_intfreq = 200000000;
		bd->bi_busfreq = 100000000;
#ifdef CONFIG_PCI
		bd->bi_pci_busfreq = 66666666;
#endif
	}
	/* Yeah, this look weird, but on Redwood 4 they are
	 * different object in the structure.  Sincr Redwwood 5
	 * and Redwood 6 use OpenBIOS, it requires a special value.
	 */
#if defined(CONFIG_REDWOOD_5) || defined (CONFIG_REDWOOD_6)
	bd->bi_tbfreq = 27 * 1000 * 1000;
#endif
	timebase_period_ns = 1000000000 / bd->bi_tbfreq;
}
#endif /* CONFIG_BEECH */
#endif /* CONFIG_IBM_OPENBIOS */

#ifdef CONFIG_ARCTIC2
/* Several bootloaders have been used on the Arctic.  We assume either
 * SSX or PIBS */

#define SSX_BIOS_ADDR 		0xFFFF0000
#define SSX_BIOS_GET_BOARD_INFO 0
#define	PIBS_BOARD_INFO_VECTOR	0xFFF62004

struct ssx_bios_id {
	unsigned int boot_branch;	/* Branch to bootcode */
	char ssx_bios[8];		/* "SSX BIOS" (no \0) */
	void (*bios_entry_point)(unsigned int, bd_t *); /* Call bios_entry_point(cmd, &data) */
};

extern int memcmp(const void *s1, const void *s2, size_t n);

static void get_board_info(bd_t **bdp)
{
	struct ssx_bios_id *ssx = (struct ssx_bios_id *)SSX_BIOS_ADDR;

	/* Check for SSX signature */

	if (memcmp(&ssx->ssx_bios, "SSX BIOS", 8) == 0) {
		ssx->bios_entry_point(SSX_BIOS_GET_BOARD_INFO, *bdp);
	} else {
		/* It's not SSX, so assume PIBS */
		typedef void (*PFV)(bd_t *bd);
		((PFV)(*(unsigned long *)PIBS_BOARD_INFO_VECTOR))(*bdp);
	}
}

void embed_config(bd_t **bdp)
{
        *bdp = &bdinfo;
	get_board_info(bdp);
#if 0
	/* Enable RefClk/4 mode for both UARTs */
	mtdcr(DCRN_CPC0_CR0, mfdcr(DCRN_CPC0_CR0) | 0x30000000);
#endif
	timebase_period_ns = 1000000000 / bdinfo.bi_tbfreq;
}

#endif

#ifdef CONFIG_EP405
#include <linux/serial_reg.h>

void
embed_config(bd_t **bdp)
{
	u32 chcr0;
	u_char *cp;
	bd_t	*bd;

	/* Different versions of the PlanetCore firmware vary in how
	   they set up the serial port - in particular whether they
	   use the internal or external serial clock for UART0.  Make
	   sure the UART is in a known state. */
	/* FIXME: We should use the board's 11.0592MHz external serial
	   clock - it will be more accurate for serial rates.  For
	   now, however the baud rates in ep405.h are for the internal
	   clock. */
	chcr0 = mfdcr(DCRN_CHCR0);
	if ( (chcr0 & 0x1fff) != 0x103e ) {
		mtdcr(DCRN_CHCR0, (chcr0 & 0xffffe000) | 0x103e);
		/* The following tricks serial_init() into resetting
		 * the baud rate */
		writeb(0, UART0_IO_BASE + UART_LCR);
	}

	/* We haven't seen actual problems with the EP405 leaving the
	 * EMAC running (as we have on Walnut).  But the registers
	 * suggest it may not be left completely quiescent.  Reset it
	 * just to be sure. */
	mtdcr(DCRN_MALCR(DCRN_MAL_BASE), MALCR_MMSR);     /* 1st reset MAL */
	while (mfdcr(DCRN_MALCR(DCRN_MAL_BASE)) & MALCR_MMSR)
		; /* wait for the reset */	
	out_be32((u32 *)EMAC0_BASE,0x20000000);        /* then reset EMAC */

	bd = &bdinfo;
	*bdp = bd;
#if 1
	        cp = (u_char *)0xF0000EE0;
	        for (;;) {
	                if (*cp == 'E') {
	                        cp++;
	                        if (*cp == 'A') {
                                  cp += 2;
                                  rpx_eth(bd, cp);
	                        }
		         }

	         	if (*cp == 'D') {
	                        	cp++;
	                        	if (*cp == '1') {
		                                cp += 2;
		                                rpx_memsize(bd, cp);
	        	                }
                	}

			if (*cp == 'N') {
				cp++;
				if (*cp == 'V') {
					cp += 2;
					rpx_nvramsize(bd, cp);
				}
			}
			while ((*cp != '\n') && (*cp != 0xff))
			      cp++;

	                cp++;
	                if ((*cp == 0) || (*cp == 0xff))
	                   break;
	       }
	bd->bi_intfreq   = 200000000;
	bd->bi_busfreq   = 100000000;
	bd->bi_pci_busfreq= 33000000 ;
#else

	bd->bi_memsize   = 64000000;
	bd->bi_intfreq   = 200000000;
	bd->bi_busfreq   = 100000000;
	bd->bi_pci_busfreq= 33000000 ;
#endif
	timebase_period_ns = 1000000000 / bd->bi_tbfreq;
}
#endif

#ifdef CONFIG_RAINIER
/* Rainier uses vxworks bootrom */
void
embed_config(bd_t **bdp)
{
	u_char	*cp;
	int	i;
	bd_t	*bd;
	
	bd = &bdinfo;
	*bdp = bd;
	
	for(i=0;i<8192;i+=32) {
		__asm__("dccci 0,%0" :: "r" (i));
	}
	__asm__("iccci 0,0");
	__asm__("sync;isync");

	/* init ram for parity */
	memset(0, 0,0x400000);  /* Lo memory */


	bd->bi_memsize   = (32 * 1024 * 1024) ;
	bd->bi_intfreq = 133000000; //the internal clock is 133 MHz
	bd->bi_busfreq   = 100000000;
	bd->bi_pci_busfreq= 33000000;

	cp = (u_char *)def_enet_addr;
	for (i=0; i<6; i++) {
		bd->bi_enetaddr[i] = *cp++;
	}

}
#endif
