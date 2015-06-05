/*
 * arch/ppc/kernel/gt64260_dbg.c
 * 
 * KGDB and progress routines for the Marvell/Galileo GT64260 (Discovery).
 *
 * Author: Mark A. Greer <mgreer@mvista.com>
 *
 * 2003 (c) MontaVista Software, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

/*
 *****************************************************************************
 *
 *	Low-level MPSC/UART I/O routines
 *
 *****************************************************************************
 */


#include <linux/config.h>
#include <linux/irq.h>
#include <asm/gt64260.h>


#if defined(CONFIG_SERIAL_TEXT_DEBUG)
void
gt64260_progress_init(void)
{
	if (ppc_md.early_serial_map) {
		ppc_md.early_serial_map();
	}
	return;
}

void
gt64260_mpsc_progress(char *s, unsigned short hex)
{
	volatile char	c;

	gt_polled_putc(0, '\r');

	while ((c = *s++) != 0){
		gt_polled_putc(0, c);
	}

	gt_polled_putc(0, '\n');
	gt_polled_putc(0, '\r');

	return;
}
#endif	/* CONFIG_SERIAL_TEXT_DEBUG */


#if defined(CONFIG_KGDB)

#if defined(CONFIG_KGDB_TTYS0)
#define KGDB_PORT 0
#elif defined(CONFIG_KGDB_TTYS1)
#define KGDB_PORT 1
#else
#error "Invalid kgdb_tty port"
#endif

void
putDebugChar(unsigned char c)
{
	gt_polled_putc(KGDB_PORT, (char)c);
}

int
getDebugChar(void)
{
	unsigned char	c;

	while (!gt_polled_getc(KGDB_PORT, &c));
	return (int)c;
}

void
putDebugString(char* str)
{
	while (*str != '\0') {
		putDebugChar(*str);
		str++;
	}
	putDebugChar('\r');
	return;
}

void
kgdb_interruptible(int enable)
{
}

void
kgdb_map_scc(void)
{
	if (ppc_md.early_serial_map) {
		ppc_md.early_serial_map();
	}
}
#endif	/* CONFIG_KGDB */
