/***********************************************************************
 *
 * (C) Copyright 2000
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 ***********************************************************************/

/* ------------------------------------------------------------------------- */
/* i2c-algo-8240.h i2c driver defines for the MPC8240 I2C Interface          */
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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                */
/* ------------------------------------------------------------------------- */


#ifndef I2C_ALGO_8240_H
#define I2C_ALGO_8240_H 1

#include <linux/i2c.h>

#define M8240_I2C_OFFSET  0x3000
#define M8240_I2C_SIZE    0x1000

#define M8240_IRQ_I2C     16

typedef struct i2c_8240_s
{
	u_int i2cadr;
	u_int i2cfdr;
	u_int i2ccr;
	u_int i2csr;
	u_int i2cdr;
}
i2c_8240_t;

	/* Defines for the I2C Address Register.
	 */
#define M8240_I2CADR_ADDR_MASK 0x000000FE

	/* Defines for the I2C Frequency Divider Register.
	 */
#define M8240_I2CFDR_FDR_MASK  0x0000003F

	/* Defines for the I2C Control Register.
	 */
#define M8240_I2CCR_RSTA       0x00000004
#define M8240_I2CCR_TXAK       0x00000008
#define M8240_I2CCR_MTX        0x00000010
#define M8240_I2CCR_MSTA       0x00000020
#define M8240_I2CCR_MIEN       0x00000040
#define M8240_I2CCR_MEN        0x00000080

#define M8240_I2CCR_INIT_MASK  (M8240_I2CCR_MEN | M8240_I2CCR_MSTA | \
                                M8240_I2CCR_MTX | M8240_I2CCR_RSTA)

	/* Defines for the I2C Status Register
	 */
#define M8240_I2CSR_RXAK       0x00000001
#define M8240_I2CSR_MIF        0x00000002
#define M8240_I2CSR_MBB        0x00000020

struct i2c_algo_8240_data
{
	volatile i2c_8240_t * i2c;
};

extern int i2c_8240_add_bus(struct i2c_adapter *);
extern int i2c_8240_del_bus(struct i2c_adapter *);

#endif /* I2C_ALGO_8240_H */
