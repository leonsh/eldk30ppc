/*
 * Generic xmit layer for the ISP1362 USB client function.
 * Copyright (C) 2003 DENX Software Engineering, Wolfgang Denk, wd@denx.de
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include "isp1362.h"

/* #define	DEBUG */

#ifdef DEBUG
#define	debug(fmt...)	printk(## fmt)
#else
#define	debug(fmt...)
#endif

static char *isp1362_ep2_buf;
static int isp1362_ep2_len = 0;
static int isp1362_ep2_curlen;
static int isp1362_ep2_remain;
static int isp1362_ep2_pktsize;
static usb_callback_t isp1362_ep2_callback;

static void isp1362_ep2_start(void)
{
	if (!isp1362_ep2_len)
		return;

	isp1362_ep2_curlen = isp1362_ep2_pktsize;
	if (isp1362_ep2_curlen > isp1362_ep2_remain)
		isp1362_ep2_curlen = isp1362_ep2_remain;

	isp1362_write_buffer (2, isp1362_ep2_buf, isp1362_ep2_curlen);
}

static void isp1362_ep2_done(int flag)
{
	int size = isp1362_ep2_len - isp1362_ep2_remain;
	if (isp1362_ep2_len) {
		isp1362_ep2_len = 0;
		if (isp1362_ep2_callback)
			isp1362_ep2_callback(flag, size);
	}
}

void isp1362_ep2_int_hndlr (void)
{
	ulong status = isp1362_read_status (2, EP_DIR_IN);

	debug ("isp1362: EP2 status: %02X\n", (uint)status);
	if (!(status & EPSTAT_EPFULL0) && isp1362_ep2_len)
	{
		isp1362_ep2_buf += isp1362_ep2_curlen;
		isp1362_ep2_remain -= isp1362_ep2_curlen;

		if (isp1362_ep2_remain != 0) {
			isp1362_ep2_start();
		} else {
			isp1362_ep2_done(0);
		}
	}
}

int isp1362_send(char *buf, int len, usb_callback_t callback)
{
	int flags;

	if (isp1362_ep2_len)
		return -EBUSY;

	local_irq_save(flags);
	isp1362_ep2_buf = buf;
	isp1362_ep2_len = len;
	isp1362_ep2_callback = callback;
	isp1362_ep2_remain = len;
	isp1362_ep2_start();
	local_irq_restore(flags);

	return 0;
}

int isp1362_ep2_init(void)
{
	desc_t * pd = isp1362_get_descriptor_ptr();
	isp1362_ep2_pktsize = __le16_to_cpu( pd->b.ep2.wMaxPacketSize );
	isp1362_ep2_done(-EAGAIN);
	return 0;
}

void isp1362_ep2_reset(void)
{
	desc_t * pd = isp1362_get_descriptor_ptr();
	isp1362_ep2_pktsize = __le16_to_cpu( pd->b.ep2.wMaxPacketSize );
	isp1362_ep2_done(-EINTR);
}

void isp1362_send_reset(void)
{
	isp1362_ep2_reset();
}

EXPORT_SYMBOL(isp1362_send);
EXPORT_SYMBOL(isp1362_send_reset);
