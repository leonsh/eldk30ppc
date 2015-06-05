/***********************************************************************
 *
 * (C) Copyright 2000
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 ************************************************************************/

/* ------------------------------------------------------------------------- */
/* i2c-algo-8240.c i2c driver algorithms for the MPC8240 I2C Interface       */
/* ------------------------------------------------------------------------- */
/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.		     */
/* ------------------------------------------------------------------------- */

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
#include <asm/io.h>

#include <linux/i2c.h>
#include <linux/i2c-algo-8240.h>


#define DEB(x)      if (i2c_debug >= 1) x
#define DEB2(x)     if (i2c_debug >= 2) x
#define DEB3(x)     if (i2c_debug >= 3) x 
#define DEBPROTO(x) if (i2c_debug >= 9) x


#define DEF_TIMEOUT 100


static int i2c_debug  = 0;
static int m8240_scan = 0;

static wait_queue_head_t iic_wait;

static int m8240_i2c_poll = 0;


static u_int m8240_iic_read(volatile u_int * addr)
{
	u_int val;

	val = in_le32(addr);
	asm volatile ("sync");
	return val;
}

static void m8240_iic_write(volatile u_int * addr, u_int val, u_int mask)
{
	u_int tmp;
	
	if (! mask) {
		out_le32(addr, val);
	} else {
		tmp = m8240_iic_read(addr);
		asm volatile ("sync");
		out_le32(addr, (tmp & ~mask) | (val & mask));
	}
	asm volatile ("sync");
}

static void
m8240_iic_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{
	volatile i2c_8240_t *i2c = (i2c_8240_t *)dev_id;

	if (i2c_debug > 1) {
		printk(KERN_DEBUG "m8240_iic_interrupt(dev_id=%p)\n", dev_id);
	}

	m8240_iic_write(&i2c->i2csr, ~M8240_I2CSR_MIF, M8240_I2CSR_MIF);

	wake_up_interruptible(&iic_wait);
}

static void i2c_start(struct i2c_algo_8240_data *adap) 
{
	volatile i2c_8240_t *i2c = (i2c_8240_t *)adap->i2c;

	DEBPROTO(printk("S "));
	m8240_iic_write(&i2c->i2ccr, M8240_I2CCR_MSTA, M8240_I2CCR_MSTA);
}


static void i2c_stop(struct i2c_algo_8240_data *adap) 
{
	volatile i2c_8240_t *i2c = (i2c_8240_t *)adap->i2c;

	DEBPROTO(printk("P\n"));
	m8240_iic_write(&i2c->i2ccr, 0, M8240_I2CCR_MSTA);
}


static int wait_for_bb(struct i2c_algo_8240_data *adap)
{
	volatile i2c_8240_t *i2c = (i2c_8240_t *)adap->i2c;
	int timeout = DEF_TIMEOUT;
	int status;
	status = m8240_iic_read(&i2c->i2csr);
#ifndef STUB_I2C
	while (timeout-- && (status & M8240_I2CSR_MBB)) {
		udelay(1000);
		status = status = m8240_iic_read(&i2c->i2csr);
	}
#endif
	if (timeout <= 0) {
		printk(KERN_INFO "Timeout waiting for Bus Busy\n");
	}

	return (timeout <= 0);
}


static inline void m8240_sleep(unsigned long timeout)
{
	schedule_timeout( timeout * HZ);
}


static inline int wait_for_pin(struct i2c_algo_8240_data *adap, int *status)
{
	int timeout = DEF_TIMEOUT;
	int res     = -1;
	volatile i2c_8240_t *i2c = (i2c_8240_t *)adap->i2c;

	if (! m8240_i2c_poll) {
		interruptible_sleep_on(&iic_wait);
		*status = m8240_iic_read(&i2c->i2csr);
		res = 0;
		goto Done;
	}

	*status = m8240_iic_read(&i2c->i2csr);
#ifndef STUB_I2C
	while (timeout-- && !(*status & M8240_I2CSR_MIF)) {
		udelay(1000);
		*status = m8240_iic_read(&i2c->i2csr);
	}
#endif

	if (!(*status & M8240_I2CSR_MIF)) {
		goto Done;
	}
	
	m8240_iic_write(&i2c->i2csr, ~M8240_I2CSR_MIF, M8240_I2CSR_MIF);

	res = 0;

Done:
	return res;
}


static void
m8240_iic_init(struct i2c_algo_8240_data *adap)
{
	volatile i2c_8240_t	*i2c = adap->i2c;

	if (i2c_debug) {
		printk(KERN_DEBUG "m8240_iic_init() - I2C registers at %p\n", i2c);
	}

		/* Select an arbitrary address.  Just make sure it is unique.
		 */
	m8240_iic_write(&i2c->i2cadr, 0x34, M8240_I2CADR_ADDR_MASK);

		/* Make clock run maximum slow.
		 */
	m8240_iic_write(&i2c->i2cfdr, 0x3f, M8240_I2CFDR_FDR_MASK);

		/* Enable module and disable interrupts.
		 */
	m8240_iic_write(&i2c->i2ccr, M8240_I2CCR_MEN, M8240_I2CCR_INIT_MASK);

	m8240_iic_write(&i2c->i2csr, ~M8240_I2CSR_MIF, M8240_I2CSR_MIF);

	init_waitqueue_head(&iic_wait);

		/* Install interrupt handler.
		 */
	if (i2c_debug) {
		printk (KERN_DEBUG "%s[%d] Install ISR for IRQ %d\n",
			__func__,__LINE__, M8240_IRQ_I2C);
	}

	if (request_irq(M8240_IRQ_I2C, m8240_iic_interrupt, 0, "I2C", (void *)i2c)) {
		printk(KERN_WARNING "Failed to install an interrupt handler "
		       " for MPC8240 I2C contorller IRQ%d\n", M8240_IRQ_I2C);
		m8240_i2c_poll = 1;
	} else {
		m8240_iic_write(&i2c->i2ccr, M8240_I2CCR_MIEN,
		                M8240_I2CCR_MIEN);
	}
}


static int try_address(struct i2c_algo_8240_data *adap,
		       unsigned char addr, int retries)
{
	volatile i2c_8240_t	*i2c = adap->i2c;
	int i, status, ret = -1;

	for (i = 0; i < retries; i++) {
		i2c_start(adap);
		m8240_iic_write(&i2c->i2ccr, M8240_I2CCR_MTX, M8240_I2CCR_MTX);
		m8240_iic_write(&i2c->i2cdr, addr, 0);
		
		if (wait_for_pin(adap, &status) >= 0) {
			if (!(status & M8240_I2CSR_RXAK)) { 
				printk(KERN_INFO "(0x%02x)", addr >> 1);
				i2c_stop(adap);
				ret = 0;
				goto Done;
			} else {
				printk(".");
			}
		}
		i2c_stop(adap);
		udelay(500);
	}
Done:
	DEB2(if (i < retries)
	      printk(KERN_DEBUG "i2c-algo-8240.o: needed %d retries for %d\n",i,
	                   addr));
	return ret;
}


static int m8240_sendbytes(struct i2c_adapter *i2c_adap, const char *buf,
                         int count)
{
	struct i2c_algo_8240_data *adap = i2c_adap->algo_data;
	volatile i2c_8240_t	  *i2c  = adap->i2c;
	int wrcount, status, timeout;
    
	for (wrcount = 0; wrcount < count; ++wrcount) {
		DEB2(printk(KERN_INFO "i2c-algo-8240.o: %s i2c_write: writing %2.2X\n",
		      i2c_adap->name, buf[wrcount]&0xff));
		m8240_iic_write(&i2c->i2cdr, buf[wrcount], 0);
		timeout = wait_for_pin(adap, &status);
		if (timeout) {
			i2c_stop(adap);
			printk(KERN_INFO "i2c-algo-8240.o: %s i2c_write: "
			       "error - timeout.\n", i2c_adap->name);
			return -EREMOTEIO; /* got a better one ?? */
		}
#ifndef STUB_I2C
		if (status & M8240_I2CSR_RXAK) {
			i2c_stop(adap);
			printk(KERN_INFO "i2c-algo-8240.o: %s i2c_write: "
			       "error - no ack.\n", i2c_adap->name);
			return -EREMOTEIO; /* got a better one ?? */
		}
#endif
	}
	return (wrcount);
}


static int m8240_readbytes(struct i2c_adapter *i2c_adap, char *buf, int count)
{
	int rdcount = 0, i, status, timeout, dummy = 1;
	struct i2c_algo_8240_data *adap = i2c_adap->algo_data;
	volatile i2c_8240_t	  *i2c  = adap->i2c;
    
	m8240_iic_write(&i2c->i2ccr, 0, M8240_I2CCR_MTX);

	for (i = 0; i < count; ++i) {
		buf[rdcount] = m8240_iic_read(&i2c->i2cdr);
		if (dummy) {
			dummy = 0;
		} else {
			rdcount++;
		}
		timeout = wait_for_pin(adap, &status);
		if (timeout) {
			i2c_stop(adap);
			printk(KERN_INFO "i2c-algo-8240.o: i2c_read: "
			       "i2c_inb timed out.\n");
			return -EREMOTEIO;
		}
	}
	m8240_iic_write(&i2c->i2ccr, M8240_I2CCR_TXAK, M8240_I2CCR_TXAK);
	buf[rdcount++] = m8240_iic_read(&i2c->i2cdr);
	timeout = wait_for_pin(adap, &status);
	if (timeout) {
		i2c_stop(adap);
		printk(KERN_INFO "i2c-algo-8240.o: i2c_read: i2c_inb timed out.\n");
		return -EREMOTEIO;
	}
    
	return rdcount;
}


static inline void m8240_doAddress(struct i2c_algo_8240_data *adap,
                                struct i2c_msg *msg, int retries) 
{
	volatile i2c_8240_t *i2c = (i2c_8240_t *)adap->i2c;
	unsigned short flags = msg->flags;
	unsigned char addr;

	addr = ( msg->addr << 1 );
	if (flags & I2C_M_RD ) {
		addr |= 1;
	}
	if (flags & I2C_M_REV_DIR_ADDR ) {
			addr ^= 1;
	}

	m8240_iic_write(&i2c->i2ccr, M8240_I2CCR_MTX, M8240_I2CCR_MTX);
	m8240_iic_write(&i2c->i2cdr, addr, 0);
}

static int m8240_xfer(struct i2c_adapter *i2c_adap,
		    struct i2c_msg msgs[], 
		    int num)
{
	struct i2c_algo_8240_data * adap            = i2c_adap->algo_data;
	struct i2c_msg            * pmsg;
	int                         i, ret;
	int                         timeout, status;

	for (i = 0; i < num; i++) {
		pmsg = &msgs[i];

		DEB3(printk(KERN_DEBUG "i2c-algo-8240.o: Msg %d, addr=0x%x, flags=0x%x, "
		            "len=%d\n",
		            i, msgs[i].addr, msgs[i].flags, msgs[i].len);)

			/* Check for bus busy.
			 */
		timeout = wait_for_bb(adap);
		if (timeout) {
			DEB2(printk(KERN_DEBUG "i2c-algo-8240.o: "
			            "Timeout waiting for BB in m8240_xfer\n");)
			ret = -EIO;
			goto Done;
		}

		i2c_start(adap);
  
		m8240_doAddress(adap, pmsg, i2c_adap->retries);
    
		timeout = wait_for_pin(adap, &status);
		if (timeout) {
			i2c_stop(adap);
			DEB2(printk(KERN_DEBUG "i2c-algo-8240.o: Timeout waiting "
			            "for PIN(1) in m8240_xfer\n");)
			ret = EREMOTEIO;
			goto Done;
		}
    
#ifndef STUB_I2C
		if (status & M8240_I2CSR_RXAK) {
			i2c_stop(adap);
			DEB2(printk(KERN_DEBUG "i2c-algo-8240.o: "
			            "No RXAK in m8240_xfer\n");)
			ret = -EREMOTEIO;
			goto Done;
		}
#endif

		if (pmsg->flags & I2C_M_RD) {
			ret = m8240_readbytes(i2c_adap, pmsg->buf, pmsg->len);
        
			if (ret != pmsg->len) {
				DEB2(printk(KERN_DEBUG "i2c-algo-8240.o: fail: "
				            "only read %d bytes.\n", ret));
				ret = (ret < 0) ? ret : -EREMOTEIO;
				goto Done;
			} else {
				DEB2(printk(KERN_DEBUG "i2c-algo-8240.o: read %d bytes.\n",
				            ret));
			}
		} else {
			ret = m8240_sendbytes(i2c_adap, pmsg->buf, pmsg->len);

			if (ret != pmsg->len) {
				DEB2(printk(KERN_DEBUG "i2c-algo-8240.o: fail: "
				            "only wrote %d bytes.\n", ret));
				ret = (ret < 0) ? ret : -EREMOTEIO;
				goto Done;
			} else {
				DEB2(printk(KERN_DEBUG "i2c-algo-8240.o: wrote %d "
				            "bytes.\n", ret));
			}
		}

		i2c_stop(adap);
	}
	ret = num;
Done:
	return ret;
}

static int algo_control(struct i2c_adapter *adapter, 
	unsigned int cmd, unsigned long arg)
{
	return 0;
}

static u32 m8240_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_SMBUS_EMUL; 
}

/* -----exported algorithm data: -------------------------------------	*/

static struct i2c_algorithm m8240_algo = {
	"MPC8240 algorithm",
	I2C_ALGO_MPC8240,
	m8240_xfer,
	NULL,
	NULL,				/* slave_xmit		*/
	NULL,				/* slave_recv		*/
	algo_control,			/* ioctl		*/
	m8240_func,			/* functionality	*/
};

/* 
 * registering functions to load algorithms at runtime 
 */
int i2c_8240_add_bus(struct i2c_adapter *adap)
{
	int i;
	struct i2c_algo_8240_data *m8240_adap = adap->algo_data;

	DEB2(printk(KERN_DEBUG "i2c-algo-8240.o: hw routines for %s registered.\n",
	            adap->name));

	adap->id |= m8240_algo.id;
	adap->algo = &m8240_algo;

	adap->timeout = 100;
	adap->retries = 3;

#ifdef MODULE
	MOD_INC_USE_COUNT;
#endif

	i2c_add_adapter(adap);
	m8240_iic_init(m8240_adap);

	if (m8240_scan) {
		printk(KERN_INFO " i2c-algo-8240.o: scanning bus %s.\n",
		       adap->name);
		for (i = 0x00; i < 0xff; i+=2) {
			try_address(m8240_adap, i, 1);
			udelay(500);
		}
		printk("\n");
	}
	return 0;
}


int i2c_8240_del_bus(struct i2c_adapter *adap)
{
	int res;
	if ((res = i2c_del_adapter(adap)) < 0)
		return res;
	DEB2(printk(KERN_DEBUG "i2c-algo-8240.o: adapter unregistered: %s\n",adap->name));

#ifdef MODULE
	MOD_DEC_USE_COUNT;
#endif
	return 0;
}

int __init i2c_algo_8240_init (void)
{
	printk(KERN_INFO "i2c-algo-8240.o: i2c MPC8240 algorithm module\n");
	return 0;
}


EXPORT_SYMBOL(i2c_8240_add_bus);
EXPORT_SYMBOL(i2c_8240_del_bus);

#ifdef MODULE
MODULE_AUTHOR("Wolfgang Denk <wd@denx.de>");
MODULE_DESCRIPTION("I2C-Bus MPC8240 algorithm");

MODULE_PARM(m8240_scan, "i");
MODULE_PARM(i2c_debug,"i");

MODULE_PARM_DESC(m8240_scan, "Scan for active chips on the bus");
MODULE_PARM_DESC(i2c_debug,
        "debug level - 0 off; 1 normal; 2,3 more verbose; 9 m8240-protocol");


int init_module(void) 
{
	return i2c_algo_8240_init();
}

void cleanup_module(void) 
{
}
#endif
