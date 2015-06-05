/*
 * Generic receive layer for the ISP1362 USB client function.
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

static char *isp1362_ep1_buf;
static int isp1362_ep1_len;
static int isp1362_ep1_curlen;
static int isp1362_ep1_remain;
static int isp1362_ep1_pktsize;
static usb_callback_t isp1362_ep1_callback;

static void isp1362_ep1_start(void)
{
	isp1362_ep1_curlen = isp1362_ep1_pktsize;
	if (isp1362_ep1_curlen > isp1362_ep1_remain)
		isp1362_ep1_curlen = isp1362_ep1_remain;
}

static void isp1362_ep1_done(int flag)
{
	int size = isp1362_ep1_len - isp1362_ep1_remain;

	if (!isp1362_ep1_len)
		return;
	isp1362_ep1_len = isp1362_ep1_curlen = 0;
	if (isp1362_ep1_callback) {
		isp1362_ep1_callback(flag, size);
	}
}

void isp1362_ep1_int_hndlr(void)
{
	ulong status = isp1362_read_status (1, EP_DIR_OUT);
	unsigned int len;

	debug ("isp1362: EP1 status: %02X\n", (uint)status);
	if (status & EPSTAT_EPFULL0)
	{
		if (!isp1362_ep1_curlen) {
			isp1362_clear_buffer (1);
			return;
		}

		len = isp1362_read_buffer (1, isp1362_ep1_buf, isp1362_ep1_curlen);
		isp1362_ep1_curlen = 0;
		isp1362_ep1_remain -= len;
		isp1362_ep1_done(len > 0 ? 0 : -EPIPE);
	}
}

int isp1362_recv(char *buf, int len, usb_callback_t callback)
{
	int flags;

	if (isp1362_ep1_len)
		return -EBUSY;

	local_irq_save(flags);
	isp1362_ep1_len = len;
	isp1362_ep1_callback = callback;
	isp1362_ep1_remain = len;
	isp1362_ep1_buf = buf;
	isp1362_ep1_curlen = 0;
	isp1362_ep1_start();
	local_irq_restore(flags);

	return 0;
}

int isp1362_ep1_init(void)
{
	desc_t * pd = isp1362_get_descriptor_ptr();
	isp1362_ep1_pktsize = __le16_to_cpu( pd->b.ep1.wMaxPacketSize );
	isp1362_ep1_done(-EAGAIN);
	return 0;
}

void isp1362_ep1_reset(void)
{
	desc_t * pd = isp1362_get_descriptor_ptr();
	isp1362_ep1_pktsize = __le16_to_cpu( pd->b.ep1.wMaxPacketSize );
	isp1362_ep1_done(-EINTR);
}

void isp1362_recv_reset(void)
{
	isp1362_ep1_reset();
}

EXPORT_SYMBOL(isp1362_recv);
EXPORT_SYMBOL(isp1362_recv_reset);
