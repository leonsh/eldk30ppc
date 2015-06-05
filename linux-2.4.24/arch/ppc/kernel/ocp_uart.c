/*
 * ocp_uart.c
 *
 *
 * 	Current Maintainer
 *      Armin Kuster akuster@pacbell.net
 *      April, 2002
 *
 * This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR   IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT,  INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Version 1.0 (04/22/02) - A. Kuster
 *	Initial version	 -
 *
 *  Version 1.1 (05/02/02) - Armin & David Mueller
 *	Fixes problem with "ghost" uart device
 *	I'm not sure if this is the correct fix or if
 *	the semantic of ocp_register() should be changed? 
 *
 *  Version 1.2 (05/15/02) - David Mueller
 *	Various fixes to opc_uart_init():
 *	  Initialize curr_uart, free previously allocated driver.
 *	  Object and fix return value.
 *
 *  Version 1.3 (05/25/02) - Armin
 *      name change *_driver to *_dev
 *	DBU:[dave@cray.com] ask the regs if internally clocked; get the info from 
 *      there for what the divisor is supposed to be.
 *  Version 1.4 06/14/02 - Armin
 *      changed irq_resource to irq
 *  Version 1.5 07/24/02 - Armin
 *      added IBM_CPM_UART* support at init
 *  Version 1.6 07/25/02 -armin
 *      minor fix in exit
 *  Version 1.6.1 - Armin
 *  	addded CONFIG_PM arround DCR bits, some 4xx platorms don't
 *  	have CPM registers, Xilinx for example
 *     
 *
 */

#include <linux/config.h>
#include <linux/stddef.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/serial.h>
#include <linux/serialP.h>
#include <linux/serial_reg.h>
#include <asm/ibm4xx.h>

#include <asm/serial.h>
#include <asm/ocp.h>

#define VER "1.6.2"

extern struct serial_state rs_table[];
/*
 * This function is used to setup the uart at an early boot time, 
 * default is 0
 */

void
early_uart_init(void)
{
	struct serial_struct serial_req;

	memset(&serial_req, 0, sizeof (serial_req));
	serial_req.line = rs_table[0].port;
	serial_req.baud_base = rs_table[0].baud_base;
	serial_req.port = rs_table[0].port;
	serial_req.irq = rs_table[0].irq;
	serial_req.flags = rs_table[0].flags;
	serial_req.io_type = rs_table[0].io_type;
	serial_req.iomem_base = rs_table[0].iomem_base;
	serial_req.iomem_reg_shift = rs_table[0].iomem_reg_shift;
	if (early_serial_setup(&serial_req) != 0)
		printk(KERN_ERR
		       "Early serial init of uart:%lu failed.\n", rs_table[0].port);
}

/* this function is to help setup the /proc for the ocp uart */

static int __init
ocp_uart_init(void)
{
	struct ocp_dev *uart_drv;
	int curr_uart = 0;

	while (curr_uart != -ENXIO) {
		if (!(uart_drv = ocp_alloc_dev(sizeof (struct serial_struct))))
			return -ENOMEM;
		uart_drv->type = UART;
		/* this returns the next uart number */
		if ((curr_uart = ocp_register(uart_drv)) != -ENXIO) {
			uart_drv->irq = rs_table[curr_uart].irq;
#ifdef CONFIG_PM
			mtdcr(DCRN_CPMFR,
			      mfdcr(DCRN_CPMFR) & ~ocp_get_pm(UART, curr_uart));
#endif
		} else {
			ocp_free_dev(uart_drv);
			break;
		}
	}
	printk("OCP uart ver %s init complete\n", VER);
	return (curr_uart == -ENXIO) ? 0 : 1;
}

void __exit
ocp_uart_fini(void)
{
	struct ocp_dev *uart_dev;
	int i;
	for (i = 0; i < ocp_get_max(UART); i++) {
		uart_dev = ocp_get_dev(UART, i);
#ifdef CONFIG_PM
		mtdcr(DCRN_CPMFR, mfdcr(DCRN_CPMFR) | ocp_get_pm(UART, i));
#endif
		ocp_unregister(uart_dev);
	}
}

module_init(ocp_uart_init);
module_exit(ocp_uart_fini);
EXPORT_SYMBOL(early_uart_init);
