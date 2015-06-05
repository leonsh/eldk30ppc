/*
 * Simple CPM SPI interface for the MPC 8260.
 * 
 * Copyright (c) 2002 Wolfgang Grandegger (wg@denx.de)
 *
 * This interface is partly derived from code copyrighted
 * by Navin Boppuri (nboppuri@trinetcommunication.co) and
 * Prashant Patel (pmpatel@trinetcommunication.com).
 */

#ifndef __CPM_SPI_H
#define __CPM_SPI_H 

#define SPMODE_LEN(x) (((x) - 1) << 4)

#define CPM_SPI_POLL_RETRIES 1000 /* in micro-seconds */

/*
 * Board specific setting and functions:
 *
 * CPM_SPI_MAX_CHIPS: Maximum number of chips on the SPI.
 *
 * CPM_SPI_CLOCK: the bitwise or of the DIV16 and PM bits,
 *      see 16.12.4.1 in the MPC823e User's Manual.
 *
 * CPM_SPI_MODE : SPI mode setting,
 *      see 16.12.4.1 in the MPC823e User's Manual.
 * 
 */

#ifdef CONFIG_PM826
/* There are two MPC2510 connected to the SPI */

#define CPM_SPI_MAX_CHIPS	32
#if 0
#define CPM_SPI_BITS_PER_CHAR	8
#define CPM_SPI_SWAP_BYTES	0
#else
#define CPM_SPI_BITS_PER_CHAR	16
#define CPM_SPI_SWAP_BYTES	1
#endif

/* There are read timeouts with a clock value of 2 */ 
#define CPM_SPI_CLOCK 		0

#define CPM_SPI_SPMODE (SPMODE_REV | SPMODE_MSTR | \
			SPMODE_LEN(CPM_SPI_BITS_PER_CHAR) |\
	                CPM_SPI_CLOCK)

/*
 * Pin configuration:
 *
 * SEL:
 *   PD19
 *
 * CS:
 *   PA9 PA8 PB17 PB16 PB13 PB12
 *   High                   Low
 *
 * IRQ:
 *   PC7 PC6 PC5 PC4 PC3 PC2 PC1
 *   High                    Low 
 */

static inline void
cpm_spi_init_ports (volatile immap_t *immap)
{
	volatile iop8260_t *iop = &immap->im_ioport;

	/* Configure CS pins */
	iop->iop_ppara &= ~0x00c00000;
	iop->iop_pdira |=  0x00c00000;
	iop->iop_podra &= ~0x00c00000;
	iop->iop_pdata &= ~0x00c00000;

	iop->iop_pparb &= ~0x000cc000;
	iop->iop_pdirb |=  0x000cc000;
	iop->iop_podrb &= ~0x000cc000;
	iop->iop_pdatb &= ~0x000cc000;

	iop->iop_ppard &= ~0x00001000;
	iop->iop_pdird |=  0x00001000;
	iop->iop_podrd &= ~0x00001000;
	iop->iop_podrd |=  0x00001000;

	/* Configure IRQ pins */
	iop->iop_pparc &= ~0x7f000000;
	iop->iop_pdirc &= ~0x7f000000;
	iop->iop_podrc |=  0x7f000000;
}

static inline void
cpm_spi_set_cs (volatile immap_t *immap, int id, int select)
{
	volatile iop8260_t *iop = &immap->im_ioport;
	unsigned long portb, porta;

	if (select) {
		iop->iop_pdatd |= 0x00001000;
		
		porta = portb = 0;
		if (id & 0x01) portb |= 0x00080000;
		if (id & 0x02) portb |= 0x00040000;
		if (id & 0x04) portb |= 0x00008000;
		if (id & 0x08) portb |= 0x00004000;
		if (id & 0x10) porta |= 0x00800000;
		if (id & 0x20) porta |= 0x00400000;

		iop->iop_pdata  = (iop->iop_pdata & ~0x00c00000) | porta;
		iop->iop_pdatb  = (iop->iop_pdatb & ~0x000cc000) | portb;
		iop->iop_pdatd &= ~0x00001000;

	} else {
#if 0 /* SEL line won't work correctly */
		iop->iop_pdatd |= 0x00001000;
#else
		iop->iop_pdata &= ~0x00c00000;
		iop->iop_pdatb &= ~0x000cc000;
#endif
	}
}

static inline int
cpm_spi_get_irq_cs (void)
{
	volatile immap_t *immap = (immap_t *)IMAP_ADDR;
	volatile iop8260_t *iop = &immap->im_ioport;
	unsigned long portc;
	int irq;

	portc = iop->iop_pdatc;

	irq = 0;
	if (portc & 0x01000000) irq += 64;
	if (portc & 0x02000000) irq += 32;
	if (portc & 0x04000000) irq += 16;
	if (portc & 0x08000000) irq += 8;
	if (portc & 0x10000000) irq += 4;
	if (portc & 0x20000000) irq += 2;
	if (portc & 0x40000000) irq += 1;

	return irq;
}

#elif defined (CONFIG_ATC)

#define CPM_SPI_MAX_CHIPS	32
#define CPM_SPI_SWAP_BYTES	0

static inline void
cpm_spi_init_ports (volatile immap_t *immap)
{

	immap->im_ioport.iop_ppard &= ~0x00080000;
	immap->im_ioport.iop_pdird |= 0x00080000;
	immap->im_ioport.iop_podrd &= ~0x00080000;
	immap->im_ioport.iop_pdatd |= 0x00080000;
}

static inline void
cpm_spi_set_cs (volatile immap_t *immap, int id, int select)
{

	if (select) {
		immap->im_ioport.iop_pdatd &= ~0x00080000;
	} else {
		immap->im_ioport.iop_pdatd |= 0x00080000;
	}
}
    
#else
#error "CPM SPI support is not implemented for your board"
#endif

#endif
