/*
 * i2c-algo-8260.c i2x driver algorithms for MPC8260 CPM
 * Derived from "i2c-algo-8xx.c":
 * Copyright (c) 1999 Dan Malek (dmalek@jlc.net).
 * (C) 2001 Wolfgang Denk, wd@denx.de
 *
 * moved into proper i2c interface; separated out platform specific 
 * parts into i2c-rpx.c
 * Brad Parker (brad@heeltoe.com)
 */

/* $Id$ */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/sched.h>

#include <asm/mpc8260.h>
#include <asm/immap_8260.h>
#include <asm/cpm_8260.h>
#include <asm/irq.h>

#include <linux/i2c.h>
#include <linux/i2c-algo-8260.h>

#define CPM_MAX_READ	513

/* imported from arch/ppc/kernel/misc.S */
extern void invalidate_dcache_range(ulong start, ulong stop);

static wait_queue_head_t iic_wait;
static ushort r_tbase, r_rbase;

int cpm_scan = 0;
int cpm_debug = 0;

static void
cpm_iic_interrupt(int irq, void *dev_id)
{
	volatile i2c8260_t *i2c = (i2c8260_t *)dev_id;

	if (cpm_debug > 1)
		printk(KERN_DEBUG "cpm_iic_interrupt(dev_id=%p)\n", dev_id);

	/* Clear interrupt.
	*/
	i2c->i2c_i2cer = 0xff;

	/* Get 'me going again.
	*/
	wake_up_interruptible(&iic_wait);
}

static void
cpm_iic_init(struct i2c_algo_8260_data *cpm_adap)
{
	volatile iic_t		*iip = cpm_adap->iip;
	volatile i2c8260_t	*i2c = cpm_adap->i2c;

	if (cpm_debug) printk(KERN_DEBUG "cpm_iic_init() - iip=%p\n",iip);

	/* Initialize the parameter ram.
	 * We need to make sure many things are initialized to zero,
	 * especially in the case of a microcode patch.
	 */
	iip->iic_rstate = 0;
	iip->iic_rdp = 0;
	iip->iic_rbptr = 0;
	iip->iic_rbc = 0;
	iip->iic_rxtmp = 0;
	iip->iic_tstate = 0;
	iip->iic_tdp = 0;
	iip->iic_tbptr = 0;
	iip->iic_tbc = 0;
	iip->iic_txtmp = 0;

	/* Set up the IIC parameters in the parameter ram.
	*/
	iip->iic_tbase = r_tbase = cpm_adap->dp_addr;
	iip->iic_rbase = r_rbase = cpm_adap->dp_addr + sizeof(cbd_t)*2;

	iip->iic_tfcr = CPMFCR_GBL | CPMFCR_EB;
	iip->iic_rfcr = CPMFCR_GBL | CPMFCR_EB;

	/* Set maximum receive size.
	*/
	iip->iic_mrblr = CPM_MAX_READ;

	/* Initialize Tx/Rx parameters.
	*/
	{
		volatile cpm8260_t *cp = cpm_adap->cp;

		cp->cp_cpcr =
			mk_cr_cmd(CPM_CR_I2C_PAGE, CPM_CR_I2C_SBLOCK, 0x00, CPM_CR_INIT_TRX) | CPM_CR_FLG;
		while (cp->cp_cpcr & CPM_CR_FLG);
	}

	/* Select an arbitrary address.  Just make sure it is unique.
	*/
	i2c->i2c_i2add = 0x34;

	/* Divider is 2 * ( 15 + 3 )
	*/
	i2c->i2c_i2brg = 0x0f;

	/* Pre-divider is BRGCLK/4
	 */
	i2c->i2c_i2mod = 0x06;

	/* Disable interrupts.
	*/
	i2c->i2c_i2cmr = 0;
	i2c->i2c_i2cer = 0xff;

	init_waitqueue_head(&iic_wait);

	/* Install interrupt handler.
	*/
	if (cpm_debug) {
		printk (KERN_DEBUG "%s[%d] Install ISR for IRQ %d\n",
			__func__,__LINE__, SIU_INT_I2C);
	}
	(*cpm_adap->setisr)(SIU_INT_I2C, cpm_iic_interrupt, (void *)i2c);
}


static int
cpm_iic_shutdown(struct i2c_algo_8260_data *cpm_adap)
{
	volatile i2c8260_t *i2c = cpm_adap->i2c;

	/* Shut down IIC.
	*/
	i2c->i2c_i2mod = 0;
	i2c->i2c_i2cmr = 0;
	i2c->i2c_i2cer = 0xff;

	return(0);
}

static void 
cpm_reset_iic_params(volatile iic_t *iip)
{
	iip->iic_tbase = r_tbase;
	iip->iic_rbase = r_rbase;

	iip->iic_tfcr = CPMFCR_GBL | CPMFCR_EB;
	iip->iic_rfcr = CPMFCR_GBL | CPMFCR_EB;

	iip->iic_mrblr = CPM_MAX_READ;

	iip->iic_rstate = 0;
	iip->iic_rdp = 0;
	iip->iic_rbptr = 0;
	iip->iic_rbc = 0;
	iip->iic_rxtmp = 0;
	iip->iic_tstate = 0;
	iip->iic_tdp = 0;
	iip->iic_tbptr = 0;
	iip->iic_tbc = 0;
	iip->iic_txtmp = 0;
}

#define BD_SC_NAK		((ushort)0x0004) /* NAK - did not respond */
#define CPM_CR_CLOSE_RXBD	((ushort)0x0007)

static void force_close(struct i2c_algo_8260_data *cpm)
{
	volatile cpm8260_t *cp = cpm->cp;

	if (cpm_debug) printk(KERN_DEBUG "force_close()\n");
#if 0
	cp->cp_cpcr =
		mk_cr_cmd(CPM_CR_CH_I2C, CPM_CR_CLOSE_RXBD) |
		CPM_CR_FLG;

	while (cp->cp_cpcr & CPM_CR_FLG);
#endif
}


/* Read from IIC...
 * abyte = address byte, with r/w flag already set
 */
static int
cpm_iic_read(struct i2c_algo_8260_data *cpm, u_char abyte, char *buf, int count)
{
	volatile immap_t *immap = (immap_t *)IMAP_ADDR;
	volatile iic_t *iip = cpm->iip;
	volatile i2c8260_t *i2c = cpm->i2c;
	volatile cpm8260_t *cp = cpm->cp;
	volatile cbd_t	*tbdf, *rbdf;
	u_char *tb;
	unsigned long flags;

	if (count >= CPM_MAX_READ)
		return -EINVAL;

	tbdf = (cbd_t *)&immap->im_dprambase[iip->iic_tbase];
	rbdf = (cbd_t *)&immap->im_dprambase[iip->iic_rbase];

	/* To read, we need an empty buffer of the proper length.
	 * All that is used is the first byte for address, the remainder
	 * is just used for timing (and doesn't really have to exist).
	 */
	if (/*cpm->reloc*/0) {
		cpm_reset_iic_params(iip);
	}
	tb = cpm->temp;
	tb = (u_char *)(((uint)tb + 15) & ~15);
	tb[0] = abyte;		/* Device address byte w/rw flag */

	flush_dcache_range((unsigned long)tb, (unsigned long)tb+1);

	if (cpm_debug) printk(KERN_DEBUG "cpm_iic_read(abyte=0x%x)\n", abyte);

	tbdf->cbd_bufaddr = __pa(tb);
	tbdf->cbd_datlen = count + 1;
	tbdf->cbd_sc =
		BD_SC_READY | BD_SC_INTRPT | BD_SC_LAST |
		BD_SC_WRAP | BD_IIC_START;

	rbdf->cbd_datlen = 0;
	rbdf->cbd_bufaddr = __pa(buf);
	rbdf->cbd_sc = BD_SC_EMPTY | BD_SC_WRAP;

	if (count > 0 && count < CPM_MAX_READ)
		iip->iic_mrblr = count;	/* prevent excessive read */

	invalidate_dcache_range(buf, buf+count);

	/* Chip bug, set enable here */
	save_flags(flags); cli();
	i2c->i2c_i2cmr = 0x13;	/* Enable some interupts */
	i2c->i2c_i2cer = 0xff;
	i2c->i2c_i2mod |= 1;	/* Enable */
	i2c->i2c_i2com = 0x81;	/* Start master */

	/* Wait for IIC transfer */
	interruptible_sleep_on(&iic_wait);
	restore_flags(flags);
	if (signal_pending(current))
		return -EIO;

	if (cpm_debug) {
		printk(KERN_DEBUG "tx sc %04x, rx sc %04x\n",
		       tbdf->cbd_sc, rbdf->cbd_sc);
	}

	if (tbdf->cbd_sc & BD_SC_NAK) {
		printk(KERN_INFO "IIC read; no ack\n");
		return 0;
	}

	if (rbdf->cbd_sc & BD_SC_EMPTY) {
		printk(KERN_INFO "IIC read; complete but rbuf empty\n");
		force_close(cpm);
		printk(KERN_INFO "tx sc %04x, rx sc %04x\n",
		       tbdf->cbd_sc, rbdf->cbd_sc);
	}

	if (cpm_debug) printk(KERN_DEBUG "read %d bytes\n", rbdf->cbd_datlen);

	if (rbdf->cbd_datlen < count) {
		printk(KERN_INFO "IIC read; short, wanted %d got %d\n",
		       count, rbdf->cbd_datlen);
		return 0;
	}
	if (cpm_debug) printk(KERN_DEBUG "buf[0] = 0x%x\n", buf[0]);


	return count;
}

/* Write to IIC...
 * addr = address byte, with r/w flag already set
 */
static int
cpm_iic_write(struct i2c_algo_8260_data *cpm, u_char abyte, char *buf,int count)
{
	volatile immap_t *immap = (immap_t *)IMAP_ADDR;
	volatile iic_t *iip = cpm->iip;
	volatile i2c8260_t *i2c = cpm->i2c;
	volatile cpm8260_t *cp = cpm->cp;
	volatile cbd_t	*tbdf;
	u_char *tb;
	unsigned long flags;

	tb = cpm->temp;
	tb = (u_char *)(((uint)tb + 15) & ~15);
	*tb = abyte;		/* Device address byte w/rw flag */

	flush_dcache_range((unsigned long)tb, (unsigned long)tb+1);
	flush_dcache_range((unsigned long)buf, (unsigned long)buf+count);

	if (cpm_debug) printk(KERN_DEBUG "cpm_iic_write(abyte=0x%x)\n", abyte);
	if (cpm_debug) printk(KERN_DEBUG "buf[0] = 0x%x, buf[1] = 0x%x\n", buf[0], buf[1]);

	/* set up 2 descriptors */
	tbdf = (cbd_t *)&immap->im_dprambase[iip->iic_tbase];

	tbdf[0].cbd_bufaddr = __pa(tb);
	tbdf[0].cbd_datlen = 1;
	tbdf[0].cbd_sc = BD_SC_READY | BD_IIC_START;

	tbdf[1].cbd_bufaddr = __pa(buf);
	tbdf[1].cbd_datlen = count;
	tbdf[1].cbd_sc = BD_SC_READY | BD_SC_INTRPT | BD_SC_LAST | BD_SC_WRAP;

	/* Chip bug, set enable here */
	save_flags(flags); cli();
	i2c->i2c_i2cmr = 0x13;	/* Enable some interupts */
	i2c->i2c_i2cer = 0xff;
	i2c->i2c_i2mod |= 1;	/* Enable */
	i2c->i2c_i2com = 0x81;	/* Start master */

	/* Wait for IIC transfer */
	interruptible_sleep_on(&iic_wait);
	restore_flags(flags);
	if (signal_pending(current))
		return -EIO;

	if (cpm_debug) {
		printk(KERN_DEBUG "tx0 sc %04x, tx1 sc %04x\n",
		       tbdf[0].cbd_sc, tbdf[1].cbd_sc);
	}

	if (tbdf->cbd_sc & BD_SC_NAK) {
		printk(KERN_INFO "IIC write; no ack\n");
		return 0;
	}
	  
	if (tbdf->cbd_sc & BD_SC_READY) {
		printk(KERN_INFO "IIC write; complete but tbuf ready\n");
		return 0;
	}

	return count;
}

/* See if an IIC address exists..
 * addr = 7 bit address, unshifted
 */
static int
cpm_iic_tryaddress(struct i2c_algo_8260_data *cpm, int addr)
{
	volatile immap_t *immap = (immap_t *)IMAP_ADDR;
	volatile iic_t *iip = cpm->iip;
	volatile i2c8260_t *i2c = cpm->i2c;
	volatile cpm8260_t *cp = cpm->cp;
	volatile cbd_t *tbdf, *rbdf;
	u_char *tb;
	unsigned long flags, len;

	if (cpm_debug > 1)
		printk(KERN_DEBUG "cpm_iic_tryaddress(cpm=%p,addr=%d)\n", cpm, addr);

	if (cpm_debug && addr == 0) {
		printk(KERN_DEBUG "iip %p, dp_addr 0x%x\n", cpm->iip, cpm->dp_addr);
		printk(KERN_DEBUG "iic_tbase %d, r_tbase %d\n", iip->iic_tbase, r_tbase);
	}

	tbdf = (cbd_t *)&immap->im_dprambase[iip->iic_tbase];
	rbdf = (cbd_t *)&immap->im_dprambase[iip->iic_rbase];

	tb = cpm->temp;
	tb = (u_char *)(((uint)tb + 15) & ~15);

	/* do a simple read */
	tb[0] = (addr << 1) | 1;	/* device address (+ read) */
	len = 2;

	flush_dcache_range((unsigned long)tb, (unsigned long)tb+1);

	tbdf->cbd_bufaddr = __pa(tb);
	tbdf->cbd_datlen = len;
	tbdf->cbd_sc =
		BD_SC_READY | BD_SC_INTRPT | BD_SC_LAST |
		BD_SC_WRAP | BD_IIC_START;

	rbdf->cbd_datlen = 0;
	rbdf->cbd_bufaddr = __pa(tb+2);
	rbdf->cbd_sc = BD_SC_EMPTY | BD_SC_WRAP;

	save_flags(flags); cli();
	i2c->i2c_i2cmr = 0x13;	/* Enable some interupts */
	i2c->i2c_i2cer = 0xff;
	i2c->i2c_i2mod |= 1;	/* Enable */
	i2c->i2c_i2com = 0x81;	/* Start master */

	if (cpm_debug > 1) printk(KERN_DEBUG "about to sleep\n");

	/* wait for IIC transfer */
	interruptible_sleep_on(&iic_wait);
	restore_flags(flags);
	if (signal_pending(current))
		return -EIO;

	if (cpm_debug > 1) printk(KERN_DEBUG "back from sleep\n");

	if (tbdf->cbd_sc & BD_SC_NAK) {
		if (cpm_debug > 1) printk(KERN_DEBUG "IIC try; no ack\n");
		return 0;
	}
	  
	if (tbdf->cbd_sc & BD_SC_READY) {
		printk(KERN_INFO "IIC try; complete but tbuf ready\n");
	}
	
	return 1;
}

static int cpm_xfer(struct i2c_adapter *i2c_adap,
		    struct i2c_msg msgs[], 
		    int num)
{
	struct i2c_algo_8260_data *adap = i2c_adap->algo_data;
	struct i2c_msg *pmsg;
	int i, ret;
	u_char addr;
    
	for (i = 0; i < num; i++) {
		pmsg = &msgs[i];

		if (cpm_debug)
			printk(KERN_DEBUG "i2c-algo-8260.o: "
			       "#%d addr=0x%x flags=0x%x len=%d\n",
			       i, pmsg->addr, pmsg->flags, pmsg->len);

		addr = pmsg->addr << 1;
		if (pmsg->flags & I2C_M_RD )
			addr |= 1;
		if (pmsg->flags & I2C_M_REV_DIR_ADDR )
			addr ^= 1;
    
		if (!(pmsg->flags & I2C_M_NOSTART)) {
		}
		if (pmsg->flags & I2C_M_RD ) {
			/* read bytes into buffer*/
			ret = cpm_iic_read(adap, addr, pmsg->buf, pmsg->len);
			if (cpm_debug)
				printk(KERN_DEBUG "i2c-algo-8260.o: read %d bytes\n", ret);
			if (ret < pmsg->len ) {
				return (ret<0)? ret : -EREMOTEIO;
			}
		} else {
			/* write bytes from buffer */
			ret = cpm_iic_write(adap, addr, pmsg->buf, pmsg->len);
			if (cpm_debug)
				printk(KERN_DEBUG "i2c-algo-8260.o: wrote %d\n", ret);
			if (ret < pmsg->len ) {
				return (ret<0) ? ret : -EREMOTEIO;
			}
		}
	}
	return (num);
}

static int algo_control(struct i2c_adapter *adapter, 
	unsigned int cmd, unsigned long arg)
{
	return 0;
}

static u32 cpm_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_SMBUS_EMUL | I2C_FUNC_10BIT_ADDR | 
	       I2C_FUNC_PROTOCOL_MANGLING; 
}

/* -----exported algorithm data: -------------------------------------	*/

static struct i2c_algorithm cpm_algo = {
	"MPC8260 CPM algorithm",
	I2C_ALGO_MPC8260,
	cpm_xfer,
	NULL,
	NULL,				/* slave_xmit		*/
	NULL,				/* slave_recv		*/
	algo_control,			/* ioctl		*/
	cpm_func,			/* functionality	*/
};

/* 
 * registering functions to load algorithms at runtime 
 */
int i2c_8260_add_bus(struct i2c_adapter *adap)
{
	int i;
	struct i2c_algo_8260_data *cpm_adap = adap->algo_data;

	if (cpm_debug)
		printk(KERN_DEBUG "i2c-algo-8260.o: hw routines for %s registered.\n",
		       adap->name);

	/* register new adapter to i2c module... */

	adap->id |= cpm_algo.id;
	adap->algo = &cpm_algo;

#ifdef MODULE
	MOD_INC_USE_COUNT;
#endif

	i2c_add_adapter(adap);
	cpm_iic_init(cpm_adap);

	/* scan bus */
	if (cpm_scan) {
		printk(KERN_INFO " i2c-algo-8260.o: scanning bus %s...\n",
		       adap->name);
		for (i = 0; i < 128; i++) {
			if (cpm_iic_tryaddress(cpm_adap, i)) {
				printk("(%02x)",i<<1); 
			}
		}
		printk("\n");
	}
	return 0;
}


int i2c_8260_del_bus(struct i2c_adapter *adap)
{
	int res;
	struct i2c_algo_8260_data *cpm_adap = adap->algo_data;

	cpm_iic_shutdown(cpm_adap);

	if ((res = i2c_del_adapter(adap)) < 0)
		return res;

	printk(KERN_INFO "i2c-algo-8260.o: adapter unregistered: %s\n",adap->name);

#ifdef MODULE
	MOD_DEC_USE_COUNT;
#endif
	return 0;
}

EXPORT_SYMBOL(i2c_8260_add_bus);
EXPORT_SYMBOL(i2c_8260_del_bus);

int __init i2c_algo_8260_init (void)
{
	printk(KERN_INFO "i2c-algo-8260.o: i2c mpc8260 algorithm module\n");
	return 0;
}


#ifdef MODULE
MODULE_AUTHOR("Brad Parker <brad@heeltoe.com>");
MODULE_DESCRIPTION("I2C-Bus MPC8260 algorithm");

int init_module(void) 
{
	return i2c_algo_8260_init();
}

void cleanup_module(void) 
{
}
#endif
