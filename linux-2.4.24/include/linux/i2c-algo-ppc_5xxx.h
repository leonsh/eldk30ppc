/*
 * include/linux/i2c-algo-mpc5xxx.h
 *
 * I2C support for MPC5xxx processors
 *
 * 2003 (c) Wolfgang Denk, DENX Software Engineering, <wd@denx.de>.  This
 * file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of
 * any kind, whether express or implied.
 */
 
#ifndef _I2C_ALGO_PPC_5XXX_h_
#define _I2C_ALGO_PPC_5XXX_h_

#include <asm/mpc5xxx.h>

struct i2c_algo_mpc5xxx_data {
    struct mpc5xxx_i2c	*regs;	/* I2C module registers  */
    unsigned char	addr;	/* I2C module address */
};

int i2c_mpc5xxx_add_bus(struct i2c_adapter *);
int i2c_mpc5xxx_del_bus(struct i2c_adapter *);

#endif /* _I2C_ALGO_PPC_5XXX_H_ */
