/***********************************************************************
 *
 * (C) Copyright 2000
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 ***********************************************************************/

/* ------------------------------------------------------------------------- */
/* i2c-cu824.c I2C Interface driver module for the CU824 board               */
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
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/init.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <platforms/cu824.h>

#include <linux/i2c.h>
#include <linux/i2c-algo-8240.h>


static int cu824_i2c_reg(struct i2c_client *client)
{
	return 0;
}


static int cu824_i2c_unreg(struct i2c_client *client)
{
	return 0;
}

static void cu824_i2c_inc_use(struct i2c_adapter *adap)
{
#ifdef MODULE
	MOD_INC_USE_COUNT;
#endif
}

static void cu824_i2c_dec_use(struct i2c_adapter *adap)
{
#ifdef MODULE
	MOD_DEC_USE_COUNT;
#endif
}


static struct i2c_algo_8240_data cu824_i2c_data = {
	NULL
};

static struct i2c_adapter cu824_i2c_ops = {
	"CU824 I2C Interface",
	I2C_HW_MPC8240_CU824,
	NULL,
	&cu824_i2c_data,
	cu824_i2c_inc_use,
	cu824_i2c_dec_use,
	cu824_i2c_reg,
	cu824_i2c_unreg,
};

int __init i2c_cu824_init(void) 
{
	int res = -ENODEV;

	printk(KERN_INFO "i2c-cu824.o: i2c CU824 module\n");

	cu824_i2c_data.i2c = ioremap(M824O_EUMB_BASE + M8240_I2C_OFFSET,
	                             M8240_I2C_SIZE);

	if (i2c_8240_add_bus(&cu824_i2c_ops) < 0) {
		goto Done;
	}

	printk(KERN_INFO "i2c-cu824.o: found device at 0x%08x.\n",
	       (unsigned int)cu824_i2c_data.i2c);

	res = 0;
Done:
	return res;
}


EXPORT_NO_SYMBOLS;

#ifdef MODULE
MODULE_AUTHOR("Wolfgang Denk <wd@denx.de>");
MODULE_DESCRIPTION("I2C-Bus driver routines for the CU824 board");


int init_module(void) 
{
	return i2c_cu824_init();
}

void cleanup_module(void) 
{
	i2c_8240_del_bus(&cu824_i2c_ops);
}

#endif
