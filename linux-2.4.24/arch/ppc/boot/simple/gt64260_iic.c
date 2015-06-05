/*
 * arch/ppc/boot/simple/gt64260_iic.c
 *
 * Bootloader version of the i2c driver for the GT64260[A].
 *
 * Author: Dale Farnsworth <dfarnsworth@mvista.com>
 *
 * 2003 (c) MontaVista, Software, Inc.  This file is licensed under the terms
 * of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */

#include <linux/config.h>
#include <linux/types.h>
#include <asm/gt64260_defs.h>

extern void udelay(long);

#ifdef	CONFIG_GT64260_NEW_BASE
static u32	gt64260_base = CONFIG_GT64260_NEW_REG_BASE;
#else
static u32	gt64260_base = CONFIG_GT64260_ORIG_REG_BASE;
#endif

static inline unsigned
gt64260_in_le32(volatile unsigned *addr)
{
	unsigned ret;

	__asm__ __volatile__("lwbrx %0,0,%1; eieio" : "=r" (ret) :
				     "r" (addr), "m" (*addr));
	return ret;
}

static inline void
gt64260_out_le32(volatile unsigned *addr, int val)
{
	__asm__ __volatile__("stwbrx %1,0,%2; eieio" : "=m" (*addr) :
				     "r" (val), "r" (addr));
}

#define GT64260_REG_READ(offs)						\
	(gt64260_in_le32((volatile uint *)(gt64260_base + (offs))))
#define GT64260_REG_WRITE(offs, d)					\
	(gt64260_out_le32((volatile uint *)(gt64260_base + (offs)), (int)(d)))

#define READ_ADDR()		(GT64260_REG_READ(GT64260_I2C_ADDR) & 0xff)
#define READ_EX_ADDR()		(GT64260_REG_READ(GT64260_I2C_EX_ADDR) & 0xff)
#define READ_DATA()		(GT64260_REG_READ(GT64260_I2C_DATA) & 0xff)
#define READ_CONTROL()		(GT64260_REG_READ(GT64260_I2C_CONTROL) & 0xff)
#define READ_STATUS()		(GT64260_REG_READ(GT64260_I2C_STATUS) & 0xff)

#define WRITE_ADDR(d)		GT64260_REG_WRITE(GT64260_I2C_ADDR, (d))
#define WRITE_EX_ADDR(d)	GT64260_REG_WRITE(GT64260_I2C_EX_ADDR, (d))
#define WRITE_DATA(d)		GT64260_REG_WRITE(GT64260_I2C_DATA, (d))
#define WRITE_CONTROL(d)	GT64260_REG_WRITE(GT64260_I2C_CONTROL, (d))
#define WRITE_BAUD_RATE(d)	GT64260_REG_WRITE(GT64260_I2C_BAUD_RATE, (d))
#define WRITE_RESET(d)		GT64260_REG_WRITE(GT64260_I2C_RESET, (d))

static int
wait_for_status(int wanted)
{
	int i;
	int status;

	for (i=0; i<1000; i++) {	/* the highest I've seen is 20 */
		udelay(10);
		status = READ_STATUS();
		if (status == wanted)
			return status;
	}
	return -status;
}

static int
iic_control(int control, int status)
{
	WRITE_CONTROL(control);
	return wait_for_status(status);
}

static int
iic_read_byte(int control, int status)
{
	WRITE_CONTROL(control);
	if (wait_for_status(status) < 0)
		return -1;
	return READ_DATA();
}

static int
iic_write_byte(int data, int control, int status)
{
	WRITE_DATA(data & 0xff);
	WRITE_CONTROL(control);
	return wait_for_status(status);
}

int
gt64260_iic_read(uint devaddr, u8 *buf, uint offset, uint offset_size,
		 uint count)
{
	int i;
	int data;
	int control;
	int status;

	/*
	 * send reset
	 */
	WRITE_RESET(0);
	WRITE_ADDR(0);
	WRITE_EX_ADDR(0);
	WRITE_BAUD_RATE((4 << 3) | 4);
	if (iic_control(GT64260_I2C_ENABLE_BIT, GT64260_I2C_STATUS_IDLE) < 0)
		return -1;

	/*
	 * send start
	 */
	control = GT64260_I2C_START_BIT|GT64260_I2C_ENABLE_BIT;
	status = GT64260_I2C_STATUS_SENT_START;
	if (iic_control(control, status) < 0)
		return -1;

	/*
	 * select device for writing
	 */
	data = devaddr & ~GT64260_I2C_DATA_READ_BIT;
	control = GT64260_I2C_ENABLE_BIT;
	status = GT64260_I2C_STATUS_WRITE_ADDR_ACK;
	if (iic_write_byte(data, control, status) < 0)
		return -1;

	/*
	 * send offset of data
	 */
	control = GT64260_I2C_ENABLE_BIT;
	status = GT64260_I2C_STATUS_WRITE_ACK;
	if (offset_size > 1) {
		if (iic_write_byte(offset >> 8, control, status) < 0)
			return -1;
	}
	if (iic_write_byte(offset, control, status) < 0)
		return -1;

	/*
	 * resend start
	 */
	control = GT64260_I2C_START_BIT|GT64260_I2C_ENABLE_BIT;
	status = GT64260_I2C_STATUS_RESENT_START;
	if (iic_control(control, status) < 0)
		return -1;

	/*
	 * select device for reading
	 */
	data = devaddr | GT64260_I2C_DATA_READ_BIT;
	control = GT64260_I2C_ENABLE_BIT;
	status = GT64260_I2C_STATUS_READ_ADDR_ACK;
	if (iic_write_byte(data, control, status) < 0)
		return -1;

	/*
	 * read all but last byte of data
	 */
	for (i=1; i<count; i++) {
		control = GT64260_I2C_ACK_BIT|GT64260_I2C_ENABLE_BIT;
		status = GT64260_I2C_STATUS_READ_ACK;
		data = iic_read_byte(control, status);
		if (data < 0)
			return -1;
		*buf++ = data;
	}

	/*
	 * read last byte of data
	 */
	control = GT64260_I2C_ENABLE_BIT;
	status = GT64260_I2C_STATUS_READ_NO_ACK;
	data = iic_read_byte(control, status);
	if (data < 0)
		return -1;
	*buf++ = data;

	/*
	 * send stop
	 */
	control = GT64260_I2C_STOP_BIT|GT64260_I2C_ENABLE_BIT;
	status = GT64260_I2C_STATUS_IDLE;
	if (iic_control(control, status) < 0)
		return -1;

	return count;
}
