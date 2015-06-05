/*
 * drivers/i2c/i2c-icecube.c
 *
 * I2C adapter driver for Icecube board with MPC5xxx
 *
 * 2003 (c) Wolfgang Denk, DENX Software Engineering, <wd@denx.de>.  This
 * file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of
 * any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/string.h>
#include <asm/mpc5xxx.h>

#include <linux/i2c.h>
#include <linux/i2c-algo-ppc_5xxx.h>

#define MPC5xxx_I2C1_ENABLE	0	/* Disable  */
#define MPC5xxx_I2C1_ADDR	0xFC	/* 1111110x */

#define MPC5xxx_I2C2_ENABLE	1	/* Enable   */
#define MPC5xxx_I2C2_ADDR	0xFE	/* 1111111x */

static struct i2c_algo_mpc5xxx_data icecube_data_1 = {
	(struct mpc5xxx_i2c *)MPC5xxx_I2C1,
	MPC5xxx_I2C1_ADDR,
};

static struct i2c_algo_mpc5xxx_data icecube_data_2 = {
	(struct mpc5xxx_i2c *)MPC5xxx_I2C2,
	MPC5xxx_I2C2_ADDR,
};

static struct i2c_adapter icecube_ops_1 = {
	"Icecube I2C module #1 interface",
	I2C_HW_MPC5xxx_ICECUBE,
	NULL,
	&icecube_data_1,
};

static struct i2c_adapter icecube_ops_2 = {
	"Icecube I2C module #2 interface",
	I2C_HW_MPC5xxx_ICECUBE,
	NULL,
	&icecube_data_2,
};

static int __init i2c_icecube_init(void)
{
	if (MPC5xxx_I2C1_ENABLE) {
		if (i2c_mpc5xxx_add_bus(&icecube_ops_1))
			return -ENODEV;
		printk(KERN_INFO "i2c-icecube.o: I2C module #1 installed\n");
	}
	
	if (MPC5xxx_I2C2_ENABLE) {
		if (i2c_mpc5xxx_add_bus(&icecube_ops_2))
			return -ENODEV;
		printk(KERN_INFO "i2c-icecube.o: I2C module #2 installed\n");
	}
	
	return 0;
}

static void __exit i2c_icecube_exit(void)
{
	if (MPC5xxx_I2C1_ENABLE) {
		if (i2c_mpc5xxx_del_bus(&icecube_ops_1))
			return;
		printk(KERN_INFO "i2c-icecube.o: I2C module #1 uninstalled\n");
	}
	
	if (MPC5xxx_I2C2_ENABLE) {
		if (i2c_mpc5xxx_del_bus(&icecube_ops_2))
			return;
		printk(KERN_INFO "i2c-icecube.o: I2C module #2 uninstalled\n");
	}
}

module_init(i2c_icecube_init);
module_exit(i2c_icecube_exit);

MODULE_LICENSE("GPL");
