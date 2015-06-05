/*
 * arch/ppc/boot/simple/mpc5xxx_tty.c
 * 
 * Minimal serial functions needed to send messages out a MPC5xxx
 * Programmable Serial Controller (PSC).
 *
 * Author: Dale Farnsworth <dfarnsworth@mvista.com>
 *
 * 2003 (c) MontaVista, Software, Inc.  This file is licensed under the terms
 * of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */

#include <linux/config.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <asm/mpc5xxx.h>
#include <asm/serial.h>
#ifdef CONFIG_UBOOT
#include <asm/ppcboot.h>
#endif

#if CONFIG_PPC_5xxx_PSC_CONSOLE_PORT == 0
#define MPC5xxx_CONSOLE		MPC5xxx_PSC1
#define MPC5xxx_PSC_CONFIG	(MPC5xxx_GPIO_PSC_CONFIG_UART_WITHOUT_CD << 0)
#elif CONFIG_PPC_5xxx_PSC_CONSOLE_PORT == 1
#define MPC5xxx_CONSOLE		MPC5xxx_PSC2
#define MPC5xxx_PSC_CONFIG	(MPC5xxx_GPIO_PSC_CONFIG_UART_WITHOUT_CD << 4)
#elif CONFIG_PPC_5xxx_PSC_CONSOLE_PORT == 2
#define MPC5xxx_CONSOLE		MPC5xxx_PSC3
#define MPC5xxx_PSC_CONFIG	(MPC5xxx_GPIO_PSC_CONFIG_UART_WITHOUT_CD << 8)
#else
#error "mpc5xxx PSC for console not selected"
#endif

static struct mpc5xxx_psc *psc = (struct mpc5xxx_psc *)MPC5xxx_CONSOLE;

unsigned long
serial_init(int ignored, void *ignored2)
{
	struct mpc5xxx_gpio *gpio = (struct mpc5xxx_gpio *)MPC5xxx_GPIO;
	int divisor;
	int mode1;
	int mode2;
	u32 val32;
#ifdef CONFIG_UBOOT
	extern unsigned char __res[];
	bd_t *bd = (bd_t *)__res;
#define MPC5xxx_IPBFREQ bd->bi_ipbfreq
#else
#define MPC5xxx_IPBFREQ CONFIG_PPC_5xxx_IPBFREQ
#endif

	val32 = in_be32(&gpio->port_config);
	val32 |= MPC5xxx_GPIO_PSC_CONFIG_UART_WITHOUT_CD;
	out_be32(&gpio->port_config, val32);

	out_8(&psc->command, MPC5xxx_PSC_RST_TX
			| MPC5xxx_PSC_RX_DISABLE | MPC5xxx_PSC_TX_ENABLE);
	out_8(&psc->command, MPC5xxx_PSC_RST_RX);

	out_be32(&psc->sicr, 0x0);
	out_be16(&psc->mpc5xxx_psc_clock_select, 0xdd00);
	out_be16(&psc->tfalarm, 0xf8);

	out_8(&psc->command, MPC5xxx_PSC_SEL_MODE_REG_1
			| MPC5xxx_PSC_RX_ENABLE
			| MPC5xxx_PSC_TX_ENABLE);

	divisor = ((MPC5xxx_IPBFREQ
			/ (CONFIG_PPC_5xxx_PSC_CONSOLE_BAUD * 16)) + 1) >> 1;

	mode1 = MPC5xxx_PSC_MODE_8_BITS | MPC5xxx_PSC_MODE_PARNONE
			| MPC5xxx_PSC_MODE_ERR;
	mode2 = MPC5xxx_PSC_MODE_ONE_STOP;

	out_8(&psc->ctur, divisor>>8);
	out_8(&psc->ctlr, divisor);
	out_8(&psc->command, MPC5xxx_PSC_SEL_MODE_REG_1);
	out_8(&psc->mode, mode1);
	out_8(&psc->mode, mode2);

	return 0;	/* ignored */
}

void
serial_putc(void *ignored, const char c)
{
	while (!(in_be16(&psc->mpc5xxx_psc_status) & MPC5xxx_PSC_SR_TXEMP)) ;
	out_8(&psc->mpc5xxx_psc_buffer_8, c);
	while (!(in_be16(&psc->mpc5xxx_psc_status) & MPC5xxx_PSC_SR_TXEMP)) ;
}

char
serial_getc(void *ignored)
{
	while (!(in_be16(&psc->mpc5xxx_psc_status) & MPC5xxx_PSC_SR_RXRDY)) ;

	return in_8(&psc->mpc5xxx_psc_buffer_8);
}

int
serial_tstc(void *ignored)
{
	return (in_be16(&psc->mpc5xxx_psc_status) & MPC5xxx_PSC_SR_RXRDY) != 0;
}
