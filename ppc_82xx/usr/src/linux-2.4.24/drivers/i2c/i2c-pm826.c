/*
 * PM826 CPM I2C interface.
 * Copyright (c) 2001 DENX Software Engineering, Wolfgang Denk, wd@denx.de
 *
 * PM826 specific parts of the i2c interface
 */

#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/stddef.h>
#include <linux/parport.h>

#include <asm/mpc8260.h>
#include <asm/cpm_8260.h>

#include <linux/i2c.h>
#include <linux/i2c-algo-8260.h>

static void
pm826_iic_init(struct i2c_algo_8260_data *data)
{
	volatile cpm8260_t *cp;
	volatile immap_t *immap;

	cp = cpmp;	/* Get pointer to Communication Processor */
	immap = (immap_t *)IMAP_ADDR;	/* and to internal registers */

	*(ushort *)(&immap->im_dprambase[PROFF_I2C_BASE]) = PROFF_I2C;
	data->iip = (iic_t *)&immap->im_dprambase[PROFF_I2C];

	data->i2c = (i2c8260_t *)&(immap->im_i2c);
	data->cp = cp;

	/* Initialize Port D IIC pins.
	*/
	immap->im_ioport.iop_ppard |= 0x00030000;
	immap->im_ioport.iop_pdird &= ~0x00030000;
	immap->im_ioport.iop_podrd |= 0x00030000;
	immap->im_ioport.iop_psord |= 0x00030000;

	/* Allocate space for two transmit and two receive buffer
	 * descriptors in the DP ram.
	 */
	data->dp_addr = m8260_cpm_dpalloc(sizeof(cbd_t) * 4, 8);

	/* ptr to i2c area */
	data->i2c = (i2c8260_t *)&(((immap_t *)IMAP_ADDR)->im_i2c);
}

static int pm826_install_isr(int irq, void (*func)(void *), void *data)
{
	/* install interrupt handler */
	request_8xxirq(irq, func, 0, "i2c", data);

	return 0;
}

static int pm826_reg(struct i2c_client *client)
{
	return 0;
}

static int pm826_unreg(struct i2c_client *client)
{
	return 0;
}

static void pm826_inc_use(struct i2c_adapter *adap)
{
#ifdef MODULE
	MOD_INC_USE_COUNT;
#endif
}

static void pm826_dec_use(struct i2c_adapter *adap)
{
#ifdef MODULE
	MOD_DEC_USE_COUNT;
#endif
}

static struct i2c_algo_8260_data pm826_data = {
	setisr: pm826_install_isr
};


static struct i2c_adapter pm826_ops = {
	"pm826",
	I2C_HW_MPC8260_PM826,
	NULL,
	&pm826_data,
	pm826_inc_use,
	pm826_dec_use,
	pm826_reg,
	pm826_unreg,
};

int __init i2c_pm826_init(void)
{
	printk(KERN_INFO "i2c-pm826.o: i2c PM826 module\n");

	/* reset hardware to sane state */
	pm826_iic_init(&pm826_data);

	if (i2c_8260_add_bus(&pm826_ops) < 0) {
		printk(KERN_INFO "i2c-pm826: Unable to register with I2C\n");
		return -ENODEV;
	}

	return 0;
}

void __exit i2c_pm826_exit(void)
{
	i2c_8260_del_bus(&pm826_ops);
}

#ifdef MODULE
MODULE_AUTHOR("Wolfgang Denk <wd@denx.de>");
MODULE_DESCRIPTION("I2C-Bus adapter routines for PM826");

module_init(i2c_pm826_init);
module_exit(i2c_pm826_exit);
#endif
