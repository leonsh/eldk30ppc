/*
 * Simple CPM SPI interface for the MPC 8260.
 *
 * based on the version for MPC860 by:
 * 
 * Copyright (c) 2002 Wolfgang Grandegger (wg@denx.de)
 *
 * This interface is partially derived from code copyrighted
 * by Navin Boppuri (nboppuri@trinetcommunication.co) and
 * Prashant Patel (pmpatel@trinetcommunication.com).
 *
 * This driver implements the function "cpm_spi_io()" to be 
 * used by other drivers and a simple read/write interface 
 * for user-land applications. The latter is mainly useful 
 * for debugging purposes. Some further remarks:
 *
 * - Board specific definitions and code should go into
 *   the file "cpm_spi.h".
 *
 * - For the moment, no interrupts are used. This be useful 
 *   for (very) long transfers.
 */

#ifndef EXPORT_SYMTAB
#  define EXPORT_SYMTAB /* need this one 'cause we export symbols */
#endif

#include <linux/config.h>
#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <asm/immap_8260.h>
#include <asm/mpc8260.h>

#include <asm/cpm_8260.h>
#include <asm/cpm_spi.h>

#define DRIVER_NAME "cpm_spi"

MODULE_AUTHOR("Wolfgang Grandegger (wg@denx.de)");
MODULE_DESCRIPTION("Simple Driver for the CPM SPI");

#undef DEBUG
#ifdef DEBUG
# define debugk(fmt,args...)    printk(fmt ,##args)
#else
# define debugk(fmt,args...)
#endif

#define CPM_SPI_MAJOR 65	/* "borrowed" from "plink" driver */

static volatile immap_t *immap = (immap_t *)IMAP_ADDR;

static unsigned int dp_addr;
static cbd_t *tx_bdf, *rx_bdf;
static u_char *tx_buf, *rx_buf;

extern void invalidate_dcache_range(unsigned long start, unsigned long end);

#if CPM_SPI_SWAP_BYTES
static void swap_bytes(u_char *buf, int len)
{
	u_short *sbuf = (u_short *)buf;
	while (len > 0) {
		*sbuf = cpu_to_le16(*sbuf);
		sbuf++; 
		len -= 2;
	}
}
#else
#define swap_bytes(buf, len)
#endif

/*
 * CPM SPI Kernel API function(s)
 */
ssize_t cpm_spi_io (int chip_id, int serial,
		    u_char *tx_buffer, int tx_size, 
		    u_char *rx_buffer, int rx_size)
{
	unsigned long flags;
	int i;

	/* 
	 * Serialize access to the SPI. We have to disable interrupts
	 * because we may need to call it from interrupt handlers.
	 */
	save_flags(flags); cli();

	rx_bdf->cbd_datlen = 0;
	if (serial)
		tx_bdf->cbd_datlen = tx_size + rx_size;
	else
		tx_bdf->cbd_datlen = tx_size > rx_size ? tx_size : rx_size;
	if (tx_bdf->cbd_datlen > CONFIG_CPM_SPI_BDSIZE) {
		printk("cpm_spi_io: Invalid size\n");
		restore_flags(flags);
		return -EINVAL;
	}
	if (tx_size > 0) {
		memcpy(tx_buf, tx_buffer, tx_size);
#ifdef DEBUG
		printk("Tx:");
		for (i = 0; i < tx_size; i++)
			printk(" %02x", tx_buf[i]);
		printk("\n");
#endif
		swap_bytes(tx_buf, tx_size);
	}

	flush_dcache_range((unsigned long) tx_buf, 
			   (unsigned long) (tx_buf+CONFIG_CPM_SPI_BDSIZE-1));
	flush_dcache_range((unsigned long) rx_buf, 
			   (unsigned long) (rx_buf+CONFIG_CPM_SPI_BDSIZE-1));
	invalidate_dcache_range((unsigned long) rx_buf, 
				(unsigned long) (rx_buf+
						 CONFIG_CPM_SPI_BDSIZE-1));

	/* Setting Rx and Tx BD status and data length */
	tx_bdf->cbd_sc = BD_SC_READY | BD_SC_LAST | BD_SC_WRAP;
	rx_bdf->cbd_sc = BD_SC_EMPTY | BD_SC_WRAP;

    	/* Chip select for device */
	cpm_spi_set_cs(immap, chip_id, 1);

	immap->im_spi.spi_spie = 0xff; 		/* Clear all SPI events */
        immap->im_spi.spi_spim = 0x00;		/* Mask  all SPI events */

	/* Start SPI Tx/Rx transfer */
	immap->im_spi.spi_spcom |= 0x80;

	/* 
	 * Wait until the Tx/Rx transfer is done. 
	 */
	for (i = 0; i < CPM_SPI_POLL_RETRIES; i++) {
                udelay(5);
		if ((tx_bdf->cbd_sc & BD_SC_EMPTY) == 0 && 
		        (rx_bdf->cbd_sc & BD_SC_EMPTY) == 0)
			break;		    
        }

	/* De-select device */
	cpm_spi_set_cs(immap, chip_id, 0);

	/* Check for timeout */
	if (i == CPM_SPI_POLL_RETRIES) {
		printk("cpm_spi_io: Tx/Rx transfer timeout\n");
		restore_flags(flags);
		return -EIO;
	}

#ifdef DEBUG
	printk("Transfer time approx. %d us\n", i*5);
	if (rx_bdf->cbd_datlen > 0) {
		printk("Rx:");
		for (i = 0; i < rx_bdf->cbd_datlen; i++)
			printk(" %02x", rx_buf[i]);
		printk("\n");
	}
#endif

	/* Copy receive data if appropriate */
	i = rx_bdf->cbd_datlen;
	swap_bytes(rx_buf, i);
	if (rx_size > 0) {
		if (serial) {
			i -= tx_size;
			if (i != rx_size)
				printk("i=%d rx_size=%d\n", i, rx_size);
			memcpy(rx_buffer, rx_buf + tx_size, i);
		} else {
			if (i != (rx_size > rx_size ? tx_size : rx_size))
				printk("i=%d rx_size=%d tx_size=%d\n", 
				       i, tx_size, rx_size);
			memcpy(rx_buffer, rx_buf, i);
		}
	}
	
	restore_flags(flags);
	
	return i;
}

EXPORT_SYMBOL(cpm_spi_io);

/*
 * Prototypes for driver entry functions.
 */
static int
cpm_spi_open(struct inode *inode, struct file *filp);
static int
cpm_spi_release(struct inode *inode, struct file *filp);
static ssize_t
cpm_spi_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t
cpm_spi_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
static int
cpm_spi_init(void);
static void
cpm_spi_cleanup(void);

/*
 * File operations supported by this driver.
 */
struct file_operations cpm_spi_fops = {
	owner:   THIS_MODULE,
	open:    cpm_spi_open,
	release: cpm_spi_release,
	read:    cpm_spi_read,
	write:   cpm_spi_write,
};


static int 
cpm_spi_open (struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);

	if (minor >= CPM_SPI_MAX_CHIPS)
		return -ENODEV;

	filp->private_data = (void *)minor;
	MOD_INC_USE_COUNT;
	return 0;
}

static int 
cpm_spi_release(struct inode *inode, struct file *filp)
{
	MOD_DEC_USE_COUNT;
	return 0;
}

static ssize_t 
cpm_spi_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	u_char tx_kbuf[CONFIG_CPM_SPI_BDSIZE];
	u_char rx_kbuf[CONFIG_CPM_SPI_BDSIZE];
	int chip_id = (int)filp->private_data;
	ssize_t size;
	
	debugk(__FUNCTION__ ": count=%d, chip_id=%d\n", count, chip_id);

	if (count > CONFIG_CPM_SPI_BDSIZE)
		return -ENXIO;
	
	if (copy_from_user(tx_kbuf, buf, count))
		return -EFAULT;
	
	size = cpm_spi_io(chip_id, 0, tx_kbuf, count, rx_kbuf, count);
	if (size < 0)
		return size;
	
	if (copy_to_user(buf, rx_kbuf, size))
		return -EFAULT;
	
	return size;
}

static ssize_t 
cpm_spi_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int chip_id = (int)filp->private_data;
	u_char tx_kbuf[CONFIG_CPM_SPI_BDSIZE];

	if (count > CONFIG_CPM_SPI_BDSIZE)
		return -ENXIO;

	if (copy_from_user(tx_kbuf, buf, count))
		return -EFAULT;

	return cpm_spi_io(chip_id, 0, tx_kbuf, count, NULL, 0);
}


static int 
cpm_spi_init(void)
{
	int err = 0;
	volatile spi_ram_t    *spi;
	volatile cpm8260_t    *cp;
    
        printk (KERN_INFO "CPM SPI Driver: $Revision: 1.0 $ wd@denx.de\n");

	/* Global pointer to internal registers */
	immap = (immap_t *)IMAP_ADDR;
        cp = (cpm8260_t *) &immap->im_cpm;
    
	*(ushort *)(&immap->im_dprambase[PROFF_SPI_BASE]) = PROFF_SPI;
	spi = (spi_ram_t *)&immap->im_dprambase[PROFF_SPI];   
	

/* 1 */
	/* 
	 * Initiliaze port pins for SPI
	 *                  par dir odr
	 * PD16 -> SPIMISO:  1   0   1
	 * PD17 -> SPIMOSI:  1   0   1
	 * PD18 -> SPICLK :  1   0   1
	 */
	immap->im_ioport.iop_ppard |=  0x0000e000;
	immap->im_ioport.iop_psord |=  0x0000e000;
	immap->im_ioport.iop_pdird &= ~0x0000e000;
					
	/* 
	 * Initialize board-specific port for chip select etc.
	 * Note: the board-specific definitions are in cpm_spi.h. 
	 */
	cpm_spi_init_ports(immap);

        /* 
	 * Initialize the parameter RAM. We need to make sure
	 * many things are initialized to zero, especially in 
	 * the case of a microcode patch.
         */
        spi->spi_rdp = 0;
        spi->spi_rbptr = 0;
        spi->spi_rbc = 0;   
        spi->spi_rxtmp = 0;
        spi->spi_tstate = 0;
        spi->spi_tdp = 0;
        spi->spi_tbptr = 0;
        spi->spi_tbc = 0;
        spi->spi_txtmp = 0;

	/* 
	 * Allocate space for one transmit and one receive buffer
         * descriptor in the DP RAM.  
         */
        dp_addr = m8260_cpm_dpalloc(sizeof(cbd_t) * 2, 8);
        if (dp_addr == CPM_DP_NOSPACE) {
		printk("cpm_spi: m8260_cpm_dpalloc() failed\n");
                return -ENOMEM;
        }
	
/* 3 */
        /* Set up the SPI parameters in the parameter ram */
	spi->spi_rbase = dp_addr;
        spi->spi_tbase = dp_addr + sizeof (cbd_t);
	    
	/***********IMPORTANT******************/
		
        /*
         * Setting transmit and receive buffer descriptor pointers
         * initially to rbase and tbase. Only the microcode patches
         * documentation talks about initializing this pointer. This
	 * is missing from the sample I2C driver. If you dont
         * initialize these pointers, the kernel hangs.
         */
        spi->spi_rbptr = spi->spi_rbase;
	spi->spi_tbptr = spi->spi_tbase;
										
/* 4 */
        /* Init SPI Tx + Rx Parameters */

	while (cp->cp_cpcr & CPM_CR_FLG)
		;
        cp->cp_cpcr = mk_cr_cmd(CPM_CR_SPI_PAGE, CPM_CR_SPI_SBLOCK,
				    0, CPM_CR_INIT_TRX) | CPM_CR_FLG;

	while (cp->cp_cpcr & CPM_CR_FLG)
		;
						
/* 6 */
        /* Set to big endian. */
	spi->spi_tfcr = CPMFCR_EB;
	spi->spi_rfcr = CPMFCR_EB;
	    
/* 7 */
	/* Set maximum receive size. */
	spi->spi_mrblr = CONFIG_CPM_SPI_BDSIZE;
		    
/* 8 + 9 */
        /* tx and rx buffer descriptors */
	tx_bdf = (cbd_t *) & immap->im_dprambase[spi->spi_tbase];
        rx_bdf = (cbd_t *) & immap->im_dprambase[spi->spi_rbase];
	    
        tx_bdf->cbd_sc &= ~BD_SC_READY;
	rx_bdf->cbd_sc &= ~BD_SC_EMPTY;
	
	/* Allocate memory for Rx and Tx buffers */
	tx_buf = (u_char *)m8260_cpm_hostalloc(CONFIG_CPM_SPI_BDSIZE, 16);
	rx_buf = (u_char *)m8260_cpm_hostalloc(CONFIG_CPM_SPI_BDSIZE, 16);

	/* Set the bd's rx and tx buffer address pointers */
	rx_bdf->cbd_bufaddr = __pa(rx_buf);
	tx_bdf->cbd_bufaddr = __pa(tx_buf);
		    
/* 10 + 11 */
	immap->im_spi.spi_spmode = SPMODE_REV	| /* reverse data*/
				   SPMODE_MSTR	| /* SPI is master */
				   SPMODE_EN	| /* enable SPI */
  				   SPMODE_LEN(8)| /* 8 Bits per char */
			           0x8 ;	  /* medium speed */
					    							    
	immap->im_spi.spi_spie = 0x37; 		/* Clear all SPI events */
	immap->im_spi.spi_spim = 0x00;		/* Mask  all SPI events */
				    	    																
	/* 
	 * Finally register the driver.
	 */
	err = register_chrdev(CPM_SPI_MAJOR, DRIVER_NAME, &cpm_spi_fops);
	if (err < 0) {
		printk("cpm_spi: Couldn't register driver (major=%d)\n", 
		       CPM_SPI_MAJOR);
		return err;
	}

	return 0;
}

static void 
cpm_spi_cleanup(void)
{
	unregister_chrdev(CPM_SPI_MAJOR, DRIVER_NAME);
}

module_init(cpm_spi_init);
module_exit(cpm_spi_cleanup);
