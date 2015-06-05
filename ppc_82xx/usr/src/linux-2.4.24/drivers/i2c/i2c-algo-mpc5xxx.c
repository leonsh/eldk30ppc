/*
 * drivers/i2c/i2c-algo-mpc5xxx.c
 *
 * I2C algorithm driver for MPC5XXX
 *
 * 2003 (c) Wolfgang Denk, DENX Software Engineering, <wd@denx.de>.  This
 * file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of
 * any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <asm/io.h>
#include <asm/mpc5xxx.h>
#ifdef CONFIG_UBOOT
#include <asm/ppcboot.h>
#endif

#include <linux/i2c.h>
#include <linux/i2c-algo-ppc_5xxx.h>

#define MPC5xxx_SCAN	1
#define MPC5xxx_TIMEOUT	100
#define MPC5xxx_RETRIES	3

static int speed = 100;

MODULE_PARM(speed, "i");


static int mpc5xxx_in(volatile u32 *reg)
{
	return in_be32(reg) >> 24;
}

static void mpc5xxx_out(volatile u32	*reg,
			int		val,
			int		mask)
{
	int tmp;
	
	if (!mask) {
		out_be32(reg, val << 24);
	} else {
		tmp = mpc5xxx_in(reg);
		out_be32(reg, ((tmp & ~mask) | (val & mask)) << 24);
	}
	
	return;
}

static void i2c_start(struct i2c_algo_mpc5xxx_data *algo_data)
{
	struct mpc5xxx_i2c *regs = algo_data->regs;
	
	mpc5xxx_out(&regs->mcr, MPC5xxx_I2C_STA, MPC5xxx_I2C_STA);
	
	return;
}


static void i2c_stop(struct i2c_algo_mpc5xxx_data *algo_data) 
{
	struct mpc5xxx_i2c *regs = algo_data->regs;
	
	mpc5xxx_out(&regs->mcr, 0, MPC5xxx_I2C_STA);
	
	return;
}

static int wait_for_bb(struct i2c_algo_mpc5xxx_data *algo_data)
{
	struct mpc5xxx_i2c	*regs	= algo_data->regs;
	int			timeout	= MPC5xxx_TIMEOUT;
	int			status;

	status = mpc5xxx_in(&regs->msr);

	while (timeout-- && (status & MPC5xxx_I2C_BB)) {
		udelay(1000);
		status = mpc5xxx_in(&regs->msr);
	}
	
	return (timeout <= 0);
}

static int wait_for_pin(struct i2c_algo_mpc5xxx_data	*algo_data,
			int				*status)
{
	struct mpc5xxx_i2c	*regs	= algo_data->regs;
	int			timeout	= MPC5xxx_TIMEOUT;

	*status = mpc5xxx_in(&regs->msr);
	
	while (timeout-- && !(*status & MPC5xxx_I2C_IF)) {
		udelay(1000);
		*status = mpc5xxx_in(&regs->msr);
	}
	
	if (!(*status & MPC5xxx_I2C_IF)) {
		return -1;
	}
	
	mpc5xxx_out(&regs->msr, 0, MPC5xxx_I2C_IF);
	
	return 0;
}

static int mpc5xxx_get_fdr(void)
{
	int i2c_speed = speed * 1000;
	ulong best_speed = 0;
	ulong divider;
	ulong scl;
#ifdef CONFIG_UBOOT
	extern unsigned char __res[];
	bd_t *bd = (bd_t *)__res;
	ulong ipb = bd->bi_ipbfreq;
#else
	ulong ipb = CONFIG_PPC_5xxx_IPBFREQ
#endif
	ulong bestmatch = 0xffffffffUL;
	int best_i = 0, best_j = 0, i, j;
	int SCL_Tap[] = { 9, 10, 12, 15, 5, 6, 7, 8};
	struct {int scl2tap, tap2tap; } scltap[] = {
		{4, 1},
		{4, 2},
		{6, 4},
		{6, 8},
		{14, 16},
		{30, 32},
		{62, 64},
		{126, 128}
	};
	
	for (i = 7; i >= 0; i--) {
		for (j = 7; j >= 0; j--) {
			scl = 2 * (scltap[j].scl2tap +
				(SCL_Tap[i] - 1) * scltap[j].tap2tap + 2);
			if (ipb <= i2c_speed*scl) {
				if ((i2c_speed*scl - ipb) < bestmatch) {
					bestmatch = i2c_speed*scl - ipb;
					best_i = i;
					best_j = j;
					best_speed = ipb/scl;
				}
			}
		}
	}

	divider = (best_i & 3) | ((best_i & 4) << 3) | (best_j << 2);

	return divider;
}

static void mpc5xxx_init(struct i2c_algo_mpc5xxx_data *algo_data)
{
	struct mpc5xxx_i2c	*regs = algo_data->regs;

	/* Reset to default.
	 */
	mpc5xxx_out(&regs->msr, 0, 0);
	mpc5xxx_out(&regs->mcr, 0, 0);
	
	/* Select an arbitrary address.
	 */
	mpc5xxx_out(&regs->madr, algo_data->addr, 0);
	
	mpc5xxx_out(&regs->mfdr, mpc5xxx_get_fdr(), 0);

	/* Enable module and disable interrupts.
	 */
	mpc5xxx_out(&regs->mcr, MPC5xxx_I2C_EN, MPC5xxx_I2C_INIT_MASK);
	mpc5xxx_out(&regs->msr, 0, MPC5xxx_I2C_IF);
	
	return;
}

static int try_address(struct i2c_algo_mpc5xxx_data	*algo_data,
		       unsigned char			addr,
		       int				retries)
{
	struct mpc5xxx_i2c	*regs = algo_data->regs;
	int			status;
	int			i;
	
	for (i = 0; i < retries; i++) {
		i2c_start(algo_data);
		
		mpc5xxx_out(&regs->mcr, MPC5xxx_I2C_TX, MPC5xxx_I2C_TX);
		mpc5xxx_out(&regs->mdr, addr, 0);
		
		if (!wait_for_pin(algo_data, &status)) {
			if (!(status & MPC5xxx_I2C_RXAK)) { 
				printk("(0x%02x)", addr >> 1);
				i2c_stop(algo_data);
				return 0;
			} else {
				printk(".");
			}
		}
		
		i2c_stop(algo_data);
		udelay(500);
	}
	
	return -1;
}

static int mpc5xxx_sendbytes(struct i2c_adapter	*adap,
			     char		*buf,
			     int		count)
{
	struct i2c_algo_mpc5xxx_data	*algo_data	= adap->algo_data;
	struct mpc5xxx_i2c		*regs		= algo_data->regs;
	int				wrcount;
	int				status;
    
	for (wrcount = 0; wrcount < count; ++wrcount) {
		mpc5xxx_out(&regs->mdr, buf[wrcount], 0);
		
		if (wait_for_pin(algo_data, &status)) {
			i2c_stop(algo_data);
			return -EREMOTEIO;
		}
		
		if (status & MPC5xxx_I2C_RXAK) {
			i2c_stop(algo_data);
			return -EREMOTEIO;
		}
	}
	
	return wrcount;
}

static int mpc5xxx_readbytes(struct i2c_adapter	*adap,
			     char		*buf,
			     int		count)
{
	struct i2c_algo_mpc5xxx_data	*algo_data	= adap->algo_data;
	struct mpc5xxx_i2c		*regs		= algo_data->regs;
	int				rdcount		= 0;
	int				dummy		= 1;
	int				status;
	int				i;
	
	mpc5xxx_out(&regs->mcr, 0, MPC5xxx_I2C_TX);
	mpc5xxx_out(&regs->mcr, 0, MPC5xxx_I2C_TXAK);

	for (i = 0; i < count; ++i) {
		buf[rdcount] = mpc5xxx_in(&regs->mdr);
		
		if (dummy) {
			dummy = 0;
		} else {
			rdcount++;
		}
		
		if (wait_for_pin(algo_data, &status)) {
			i2c_stop(algo_data);
			return -EREMOTEIO;
		}
	}
	
	mpc5xxx_out(&regs->mcr, MPC5xxx_I2C_TXAK, MPC5xxx_I2C_TXAK);
	buf[rdcount++] = mpc5xxx_in(&regs->mdr);
	
	if (wait_for_pin(algo_data, &status)) {
		i2c_stop(algo_data);
		return -EREMOTEIO;
	}
	
	return rdcount;
}

static void mpc5xxx_do_address(struct i2c_algo_mpc5xxx_data	*algo_data,
			       struct i2c_msg			*msg,
			       int				retries)
{
	struct mpc5xxx_i2c	*regs = algo_data->regs;
	unsigned char		addr;

	addr = (msg->addr << 1);
	
	if (msg->flags & I2C_M_RD ) {
		addr |= 1;
	}
	if (msg->flags & I2C_M_REV_DIR_ADDR ) {
		addr ^= 1;
	}
	
	mpc5xxx_out(&regs->mcr, MPC5xxx_I2C_TX, MPC5xxx_I2C_TX);
	mpc5xxx_out(&regs->mdr, addr, 0);
	
	return;
}

static int mpc5xxx_xfer(struct i2c_adapter	*adap,
			struct i2c_msg		msgs[], 
			int			num)
{
	struct i2c_algo_mpc5xxx_data	*algo_data = adap->algo_data;
	struct i2c_msg			*pmsg;
	int				ret;
	int				status;
	int				i;

	for (i = 0; i < num; i++) {
		pmsg = &msgs[i];
		
		if (wait_for_bb(algo_data))
			return -EREMOTEIO;

		i2c_start(algo_data);
  		mpc5xxx_do_address(algo_data, pmsg, adap->retries);
		
		if (wait_for_pin(algo_data, &status)) {
			i2c_stop(algo_data);
			return -EREMOTEIO;
		}
		
   		if (status & MPC5xxx_I2C_RXAK) {
			i2c_stop(algo_data);
			return -EREMOTEIO;
		}
		
		if (pmsg->flags & I2C_M_RD) {
			ret = mpc5xxx_readbytes(adap, pmsg->buf, pmsg->len);  
			if (ret != pmsg->len)
				return (ret < 0) ? ret : -EREMOTEIO;
		} else {
			ret = mpc5xxx_sendbytes(adap, pmsg->buf, pmsg->len);
			if (ret != pmsg->len)
				return (ret < 0) ? ret : -EREMOTEIO;
		}
		
		i2c_stop(algo_data);
	}
	
	return num;
}

static u32 mpc5xxx_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_SMBUS_EMUL;
}

static struct i2c_algorithm mpc5xxx_algo = {
	"MPC5xxx algorithm",
	I2C_ALGO_MPC5xxx,
	mpc5xxx_xfer,
	NULL,
	NULL,
	NULL,
	NULL,
	mpc5xxx_func,
};

int i2c_mpc5xxx_add_bus(struct i2c_adapter *adap)
{
	struct i2c_algo_mpc5xxx_data	*algo_data = adap->algo_data;
	int				i;

	adap->id |= mpc5xxx_algo.id;
	adap->algo = &mpc5xxx_algo;
	
	adap->timeout = MPC5xxx_TIMEOUT;
	adap->retries = MPC5xxx_RETRIES;
	
	mpc5xxx_init(algo_data);
	
	if (i2c_add_adapter(adap))
		return -1;

	if (MPC5xxx_SCAN) {
		printk(KERN_INFO "i2c-algo-5xxx.o: scanning bus %s...\n", adap->name);
		
		for (i = 0x00; i < 0xff; i+=2) {
			try_address(algo_data, i, 1);
			udelay(500);
		}
		
		printk("\n");
	}
	
	return 0;
}

int i2c_mpc5xxx_del_bus(struct i2c_adapter *adap)
{
	if (i2c_del_adapter(adap))
		return -1;
	
	return 0;
}

EXPORT_SYMBOL(i2c_mpc5xxx_add_bus);
EXPORT_SYMBOL(i2c_mpc5xxx_del_bus);
MODULE_LICENSE("GPL");
