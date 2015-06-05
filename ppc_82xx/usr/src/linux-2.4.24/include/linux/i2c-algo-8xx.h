/* ------------------------------------------------------------------------- */
/* i2c-algo-8xx.h i2c driver algorithms for MPX8XX CPM			     */
/* ------------------------------------------------------------------------- */

/* $Id$ */

#ifndef I2C_ALGO_8XX_H
#define I2C_ALGO_8XX_H 1

#include <linux/i2c.h>

struct i2c_algo_8xx_data {
	uint dp_addr;
	int reloc;
	volatile i2c8xx_t *i2c;
	volatile iic_t	*iip;
	volatile cpm8xx_t *cp;

	u_char	temp[513];
};

int i2c_8xx_add_bus(struct i2c_adapter *);
int i2c_8xx_del_bus(struct i2c_adapter *);

#endif /* I2C_ALGO_8XX_H */
