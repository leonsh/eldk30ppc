/* ------------------------------------------------------------------------- */
/* i2c-algo-8260.h i2c driver algorithms for MPX8260 CPM		     */
/* ------------------------------------------------------------------------- */

/* $Id$ */

#ifndef I2C_ALGO_8260_H
#define I2C_ALGO_8260_H 1

#include <linux/i2c.h>

struct i2c_algo_8260_data {
	uint dp_addr;
	int reloc;
	volatile i2c8260_t *i2c;
	volatile iic_t	*iip;
	volatile cpm8260_t *cp;

	int	(*setisr) (int irq,
			   void (*func)(int, void (*)(void *), void *),
			   void *data);

	u_char	temp[513];
};

int i2c_8260_add_bus(struct i2c_adapter *);
int i2c_8260_del_bus(struct i2c_adapter *);

#endif /* I2C_ALGO_8260_H */
