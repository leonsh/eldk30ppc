/*
 * xuartlite_tty.c
 *
 * Xilinx UART Lite support.
 *
 * Author: MontaVista Software, Inc.
 *         source@mvista.com
 *
 * 2003 (c) MontaVista, Software, Inc.  This file is licensed under the terms
 * of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */

#include <linux/config.h>
#include "xuartlite_l.h"
#include "xparameters.h"

static u32 base_addresses[XPAR_XUARTLITE_NUM_INSTANCES] = {
	XPAR_UARTLITE_0_BASEADDR,
#ifdef XPAR_UARTLITE_1_BASEADDR
	XPAR_UARTLITE_1_BASEADDR,
#ifdef XPAR_UARTLITE_2_BASEADDR
	XPAR_UARTLITE_2_BASEADDR,
#ifdef XPAR_UARTLITE_3_BASEADDR
	XPAR_UARTLITE_3_BASEADDR,
#ifdef XPAR_UARTLITE_4_BASEADDR
#error Edit this file to add more devices.
#endif				/* 4 */
#endif				/* 3 */
#endif				/* 2 */
#endif				/* 1 */
};

unsigned long
serial_init(int chan, void *ignored)
{
	unsigned long com_port;

	if (chan < 0 || chan >= XPAR_XUARTLITE_NUM_INSTANCES) {
		com_port = 0;	/* default to the 1st port */
	} else {
		com_port = (unsigned long) chan;
	}

	/* Clear FIFOs */
	XUartLite_mSetControlReg(base_addresses[com_port],
				 XUL_CR_FIFO_RX_RESET | XUL_CR_FIFO_TX_RESET);

	return com_port;
}

void
serial_putc(unsigned long com_port, unsigned char c)
{
	XUartLite_SendByte(base_addresses[com_port], c);
}

unsigned char
serial_getc(unsigned long com_port)
{
	return XUartLite_RecvByte(base_addresses[com_port]);
}

int
serial_tstc(unsigned long com_port)
{
	return !XUartLite_mIsReceiveEmpty(base_addresses[com_port]);
}
