/*
 *  arch/ppc/mpc5xxx_io/psc.c
 *
 *  Serial driver for MPC5xxx processor psc ports
 *
 *  Author: Dale Farnsworth <dfarnsworth@mvista.com>
 *
 * 2003 (c) MontaVista, Software, Inc.  This file is licensed under the terms
 * of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */

#include <linux/init.h>
#include <linux/config.h>
#include <linux/tty.h>
#include <linux/major.h>
#include <linux/ptrace.h>
#include <linux/console.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <asm/mpc5xxx.h>
#include <asm/serial.h>
#include <linux/serialP.h>
#include <linux/generic_serial.h>
#ifdef CONFIG_UBOOT
#include <asm/ppcboot.h>
#endif

/* 
 * This driver can spew a whole lot of debugging output at you. If you
 * need maximum performance, you should disable the DEBUG define.
 */
#undef MPC5xxx_PSC_DEBUG 

#ifdef MPC5xxx_PSC_DEBUG
#define MPC5xxx_PSC_DEBUG_OPEN		0x00000001
#define MPC5xxx_PSC_DEBUG_SETTING	0x00000002
#define MPC5xxx_PSC_DEBUG_FLOW		0x00000004
#define MPC5xxx_PSC_DEBUG_MODEMSIGNALS	0x00000008
#define MPC5xxx_PSC_DEBUG_TERMIOS	0x00000010
#define MPC5xxx_PSC_DEBUG_TRANSMIT	0x00000020
#define MPC5xxx_PSC_DEBUG_RECEIVE	0x00000040
#define MPC5xxx_PSC_DEBUG_INTERRUPTS	0x00000080
#define MPC5xxx_PSC_DEBUG_PROBE		0x00000100
#define MPC5xxx_PSC_DEBUG_INIT		0x00000200
#define MPC5xxx_PSC_DEBUG_CLEANUP	0x00000400
#define MPC5xxx_PSC_DEBUG_CLOSE		0x00000800
#define MPC5xxx_PSC_DEBUG_FIRMWARE	0x00001000
#define MPC5xxx_PSC_DEBUG_MEMTEST	0x00002000
#define MPC5xxx_PSC_DEBUG_THROTTLE	0x00004000
#define MPC5xxx_PSC_DEBUG_ALL		0xffffffff

int rs_debug = MPC5xxx_PSC_DEBUG_ALL & ~MPC5xxx_PSC_DEBUG_TRANSMIT;

#define rs_dprintk(f, str...) if (rs_debug & f) printk(str)
#define func_enter() rs_dprintk(MPC5xxx_PSC_DEBUG_FLOW,	\
				"rs: enter " __FUNCTION__ "\n")
#define func_exit() rs_dprintk(MPC5xxx_PSC_DEBUG_FLOW,	\
				"rs: exit " __FUNCTION__ "\n")
#else
#define rs_dprintk(f, str...)
#define func_enter()
#define func_exit()
#endif	/* MPC5xxx_PSC_DEBUG */

/*
 * Number of serial ports
 */
#define MPC5xxx_PSC_NPORTS	3

#ifdef CONFIG_PPC_5xxx_PSC_CONSOLE
#ifndef CONFIG_PPC_5xxx_PSC_CONSOLE_PORT
#define CONFIG_PPC_5xxx_PSC_CONSOLE_PORT	(-1)
#endif

static int baud_table[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
	9600, 19200, 38400, 57600, 115200, 230400, 460800, 0
};
#endif

/*
 * Hardware specific serial port structure
 */
struct rs_port { 	
	struct gs_port		gs;		/* Must be first field! */

	struct mpc5xxx_psc	*psc;
	struct wait_queue	*shutdown_wait; 
	struct async_icount	icount;
	int			irq;
	int			x_char;		/* XON/XOFF character */
	u16			imr;		/* shadow of interrupt mask */
	u16			cflag;		/* current cflag setting*/
	int			baud;		/* current baud rate */
};

/*
 * Forward declarations for serial routines
 */
static void rs_disable_tx_interrupts(void * ptr);
static void rs_enable_tx_interrupts(void * ptr); 
static void rs_disable_rx_interrupts(void * ptr); 
static void rs_enable_rx_interrupts(void * ptr); 
static int rs_get_CD(void * ptr); 
static void rs_shutdown_port(void * ptr); 
static int rs_set_real_termios(void *ptr);
static int rs_chars_in_buffer(void * ptr); 
static void rs_hungup(void *ptr);
static void rs_close(void *ptr);

static spinlock_t mpc5xxx_serial_lock = SPIN_LOCK_UNLOCKED;

/*
 * Used by generic serial driver to access hardware
 */
static struct real_driver rs_real_driver = { 
	.disable_tx_interrupts	= rs_disable_tx_interrupts, 
	.enable_tx_interrupts	= rs_enable_tx_interrupts, 
	.disable_rx_interrupts	= rs_disable_rx_interrupts, 
	.enable_rx_interrupts	= rs_enable_rx_interrupts, 
	.get_CD			= rs_get_CD, 
	.shutdown_port		= rs_shutdown_port,
	.set_real_termios	= rs_set_real_termios,
	.chars_in_buffer	= rs_chars_in_buffer, 
	.close			= rs_close, 
	.hungup			= rs_hungup,
}; 

static struct serial_state rs_table[RS_TABLE_SIZE] = {
	SERIAL_PORT_DFNS	/* Defined in serial.h */
};

/*
 * Structures and such for TTY sessions and usage counts
 */
static struct tty_driver rs_driver, rs_callout_driver;
static struct tty_struct *rs_tty[MPC5xxx_PSC_NPORTS];
static struct rs_port rs_ports[MPC5xxx_PSC_NPORTS];
static struct termios **rs_termios;
static struct termios **rs_termios_locked;
static int baud_index = B115200;
int rs_refcount;
int rs_initialized = 0;

/*
 * ----------------------------------------------------------------------
 *
 * Here starts the interrupt handling routines.  All of the following
 * subroutines are declared as inline and are folded into
 * rs_interrupt().  They were separated out for readability's sake.
 *
 * Note: rs_interrupt() is a "fast" interrupt, which means that it
 * runs with interrupts turned off.  People who may want to modify
 * rs_interrupt() should try to keep the interrupt handler as fast as
 * possible.  After you are done making modifications, it is not a bad
 * idea to do:
 * 
 * gcc -S -DKERNEL -Wall -Wstrict-prototypes -O6 -fomit-frame-pointer serial.c
 *
 * and look at the resulting assemble code in serial.s.
 *
 * 				- Ted Ts'o (tytso@mit.edu), 7-Mar-93
 * -----------------------------------------------------------------------
 */
static inline void receive_char(struct rs_port *port)
{
	struct tty_struct *tty = port->gs.tty;
	struct mpc5xxx_psc *psc = port->psc;
	unsigned int status;
	unsigned char ch;
	int count = 256;

	/* While there are characters, get them ... */
	while (--count >= 0) {
		status = in_be16(&psc->mpc5xxx_psc_status);
		if (!(status & MPC5xxx_PSC_SR_RXRDY))
			break;

		ch = in_8(&psc->mpc5xxx_psc_buffer_8);

		if (tty->flip.count >= TTY_FLIPBUF_SIZE)
			continue;

		*tty->flip.char_buf_ptr = ch;
		*tty->flip.flag_buf_ptr = 0;
		port->icount.rx++;

		if (status  &  (MPC5xxx_PSC_SR_OE |
				MPC5xxx_PSC_SR_PE |
				MPC5xxx_PSC_SR_FE |
				MPC5xxx_PSC_SR_RB)) {
			if (status & MPC5xxx_PSC_SR_RB)
				*tty->flip.flag_buf_ptr = TTY_BREAK;
			else if (status & MPC5xxx_PSC_SR_PE)
				*tty->flip.flag_buf_ptr = TTY_PARITY;
			else if (status & MPC5xxx_PSC_SR_FE)
				*tty->flip.flag_buf_ptr = TTY_FRAME;
			if (status & MPC5xxx_PSC_SR_OE) {
				/*
				 * Overrun is special, since it's
				 * reported immediately, and doesn't
				 * affect the current character
				 */
				if (tty->flip.count < (TTY_FLIPBUF_SIZE-1)) {
					tty->flip.flag_buf_ptr++;
					tty->flip.char_buf_ptr++;
					tty->flip.count++;
				}
				*tty->flip.flag_buf_ptr = TTY_OVERRUN;
			}
		}

		tty->flip.char_buf_ptr++;
		tty->flip.flag_buf_ptr++;
		tty->flip.count++;
	}

	tty_flip_buffer_push(tty);
}

static inline void transmit_char(struct rs_port *port)
{
	struct mpc5xxx_psc *psc = port->psc;
	int count = 256;
	int status;

	/* While I'm able to transmit ... */
	while (--count >= 0) {
		status = in_be16(&psc->mpc5xxx_psc_status);
		/*
		 * XXX -df
		 * check_modem_status(status);
		*/
		if (!(status & MPC5xxx_PSC_SR_TXRDY))
			break;

		if (port->x_char) {
			out_8(&psc->mpc5xxx_psc_buffer_8, port->x_char);
			port->icount.tx++;
			port->x_char = 0;
		}
		else if (port->gs.xmit_cnt <= 0 || port->gs.tty->stopped ||
				port->gs.tty->hw_stopped) {
			break;
		}
		else {
			out_8(&psc->mpc5xxx_psc_buffer_8,
				port->gs.xmit_buf[port->gs.xmit_tail++]);
			port->icount.tx++;
			port->gs.xmit_tail &= SERIAL_XMIT_SIZE-1;
			if (--port->gs.xmit_cnt <= 0) {
				break;
			}
		}
	}

	if (port->gs.xmit_cnt <= 0 || port->gs.tty->stopped ||
			 port->gs.tty->hw_stopped) {
		rs_disable_tx_interrupts(port);
	}
	
	if (port->gs.xmit_cnt <= port->gs.wakeup_chars) {
		if ((port->gs.tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
				port->gs.tty->ldisc.write_wakeup)
			(port->gs.tty->ldisc.write_wakeup)(port->gs.tty);
		rs_dprintk(MPC5xxx_PSC_DEBUG_TRANSMIT,
				"Waking up.... ldisc (%d)....\n",
				port->gs.wakeup_chars); 
		wake_up_interruptible(&port->gs.tty->write_wait);
	}	
}



static inline void check_modem_status(struct rs_port *port)
{
	/* need to add code to handle modem status change XXX -df */
	wake_up_interruptible(&port->gs.open_wait);
}

int count = 0;

/*
 * This is the serial driver's interrupt routine.
 */

static void rs_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{
	struct rs_port * port;
	unsigned long flags;

	spin_lock_irqsave(&mpc5xxx_serial_lock, flags);

	port = (struct rs_port *)dev_id;
	rs_dprintk(MPC5xxx_PSC_DEBUG_INTERRUPTS,
			"rs_interrupt (port %p)...", port);

	receive_char(port);

	transmit_char(port);

	spin_unlock_irqrestore(&mpc5xxx_serial_lock, flags);

	rs_dprintk(MPC5xxx_PSC_DEBUG_INTERRUPTS, "end.\n");
}

/*
 ************************************************************************
 *		Here are the routines that actually			*
 *		interface with the generic_serial driver		*
 ************************************************************************
 */
static void rs_disable_tx_interrupts(void * ptr) 
{
	struct rs_port *port = ptr; 
	struct mpc5xxx_psc *psc = port->psc;
	unsigned long flags;

	spin_lock_irqsave(&mpc5xxx_serial_lock, flags);
	port->gs.flags &= ~GS_TX_INTEN;

	port->imr &= ~MPC5xxx_PSC_IMR_TXRDY;
	out_be16(&psc->mpc5xxx_psc_imr, port->imr);

	spin_unlock_irqrestore(&mpc5xxx_serial_lock, flags);
}

static void rs_enable_tx_interrupts(void * ptr) 
{
	struct rs_port *port = ptr; 
	struct mpc5xxx_psc *psc = port->psc;
	unsigned long flags;

	spin_lock_irqsave(&mpc5xxx_serial_lock, flags);

	port->imr |= MPC5xxx_PSC_IMR_TXRDY;
	out_be16(&psc->mpc5xxx_psc_imr, port->imr);

	/* Send a char to start TX interrupts happening */
	transmit_char(port);

	spin_unlock_irqrestore(&mpc5xxx_serial_lock, flags);
}

static void rs_disable_rx_interrupts(void * ptr) 
{
	struct rs_port *port = ptr;
	struct mpc5xxx_psc *psc = port->psc;
	unsigned long flags;

	spin_lock_irqsave(&mpc5xxx_serial_lock, flags);

	port->imr &= ~MPC5xxx_PSC_IMR_RXRDY;
	out_be16(&psc->mpc5xxx_psc_imr, port->imr);

	spin_unlock_irqrestore(&mpc5xxx_serial_lock, flags);
}

static void rs_enable_rx_interrupts(void * ptr) 
{
	struct rs_port *port = ptr;
	struct mpc5xxx_psc *psc = port->psc;
	unsigned long flags;

	spin_lock_irqsave(&mpc5xxx_serial_lock, flags);

	port->imr |= MPC5xxx_PSC_IMR_RXRDY;
	out_be16(&psc->mpc5xxx_psc_imr, port->imr);

	/* Empty the input buffer - apparently this is *vital* */
	/*
	 * XXXX add this back in if needed, -df
	 *
	 * while (in_be16(&psc->mpc5xxx_psc_status) & MPC5xxx_PSC_SR_RXRDY)
	 *	in_8(&psc->mpc5xxx_psc_buffer_8);
	 */

	spin_unlock_irqrestore(&mpc5xxx_serial_lock, flags);

}

static int rs_get_CD(void * ptr) 
{
	struct rs_port *port = ptr;

	func_enter();

	func_exit();
	return (in_8(&port->psc->mpc5xxx_psc_ipcr) & MPC5xxx_PSC_DCD) != 0;
}

static void rs_shutdown_port(void * ptr) 
{
	struct rs_port *port = ptr; 

	func_enter();

	port->gs.flags &= ~GS_ACTIVE;

	func_exit();
}

static int rs_set_real_termios(void *ptr)
{
	struct rs_port *port = ptr;
	struct mpc5xxx_psc *psc = port->psc;
	int divisor;
	int bits;
	int parity;
	int stop;
	int mode1;
	int mode2;

#define CFLAG port->gs.tty->termios->c_cflag

	if (port->gs.baud == port->baud && CFLAG == port->cflag)
		return 0;

	port->baud = port->gs.baud;
	port->cflag = CFLAG;

	divisor = ((port->gs.baud_base / (port->gs.baud * 16)) + 1) >> 1;
	if (port->gs.baud < 50 || divisor == 0) {
		printk(KERN_NOTICE "MPC5xxx PSC: Bad speed requested, %d\n",
				port->gs.baud);
		return 0;
	}
	rs_dprintk(MPC5xxx_PSC_DEBUG_TERMIOS, "calculated divisor: %d\n",
			divisor);
	out_8(&psc->ctur, divisor>>8);
	out_8(&psc->ctlr, divisor);

	/* Program hardware for parity, data bits, stop bits */
	if ((CFLAG & CSIZE)==CS5)
		stop = MPC5xxx_PSC_MODE_ONE_STOP_5_BITS;
	else
		stop = MPC5xxx_PSC_MODE_ONE_STOP;

	if (C_PARENB(port->gs.tty)) {
		if (C_PARODD(port->gs.tty))
			parity = MPC5xxx_PSC_MODE_PARODD;
		else
			parity = MPC5xxx_PSC_MODE_PAREVEN;
	}
	else
		parity = MPC5xxx_PSC_MODE_PARNONE;

	if ((CFLAG & CSIZE)==CS6)
		bits = MPC5xxx_PSC_MODE_6_BITS;
	if ((CFLAG & CSIZE)==CS6)
		bits = MPC5xxx_PSC_MODE_6_BITS;
	if ((CFLAG & CSIZE)==CS7)
		bits = MPC5xxx_PSC_MODE_7_BITS;
	if ((CFLAG & CSIZE)==CS8)
		bits = MPC5xxx_PSC_MODE_8_BITS;
	if (C_CSTOPB(port->gs.tty))
		stop = MPC5xxx_PSC_MODE_TWO_STOP;

	mode1 = bits | parity | MPC5xxx_PSC_MODE_ERR;
	mode2 = stop;

	rs_dprintk(MPC5xxx_PSC_DEBUG_TERMIOS,
			"requested baud: %d, bits: %d, parity: %08x\n",
			port->gs.baud, bits, parity);
	rs_dprintk(MPC5xxx_PSC_DEBUG_TERMIOS,
			"baud_base: %d\n", port->gs.baud_base);
#ifdef MPC5xxx_PSC_DEBUG
	if (rs_debug & MPC5xxx_PSC_DEBUG_TERMIOS)
		out_8(&psc->command, MPC5xxx_PSC_SEL_MODE_REG_1);
#endif
	rs_dprintk(MPC5xxx_PSC_DEBUG_TERMIOS,
			"mode 1 register was: %08x, now: %08x\n",
			in_8(&psc->mode), mode1);
	rs_dprintk(MPC5xxx_PSC_DEBUG_TERMIOS,
			"mode 2 register was: %08x, now: %08x\n",
			in_8(&psc->mode), mode2);

	out_8(&psc->command, MPC5xxx_PSC_SEL_MODE_REG_1);
	out_8(&psc->mode, mode1);
	out_8(&psc->mode, mode2);

	func_exit();
	return 0;
}

static int rs_chars_in_buffer(void * ptr) 
{
	struct rs_port *port = ptr;

	return in_be16(&port->psc->rfnum) & MPC5xxx_PSC_RFNUM_MASK;
}

/* **********************************************************************
 *		Here are the routines that actually			*
 *		interface with the rest of the system			*
 * **********************************************************************
 */
static int rs_open(struct tty_struct * tty, struct file * filp)
{
	struct rs_port *port;
	int retval, line;

	func_enter();

	if (!rs_initialized) {
		return -EIO;
	}

	line = minor(tty->device) - tty->driver.minor_start;
	rs_dprintk(MPC5xxx_PSC_DEBUG_OPEN,
			"%d: opening line %d. tty=%p ctty=%p)\n", 
			(int) current->pid, line, tty, current->tty);

	if ((line < 0) || (line >= RS_TABLE_SIZE))
		return -ENODEV;

	/* Pre-initialized already */
	port = &rs_ports[line];

	rs_dprintk(MPC5xxx_PSC_DEBUG_OPEN, "port = %p\n", port);

	tty->driver_data = port;
	port->gs.tty = tty;
	port->gs.count++;

	rs_dprintk(MPC5xxx_PSC_DEBUG_OPEN, "starting port\n");

	/*
	 * Start up serial port
	 */
	retval = gs_init_port(&port->gs);
	rs_dprintk(MPC5xxx_PSC_DEBUG_OPEN, "done gs_init\n");
	if (retval) {
		port->gs.count--;
		return retval;
	}

	port->gs.flags |= GS_ACTIVE;

	rs_dprintk(MPC5xxx_PSC_DEBUG_OPEN,
			"before inc_use_count (count=%d.\n",
			port->gs.count);
	if (port->gs.count == 1) {
		MOD_INC_USE_COUNT;
	}
	rs_dprintk(MPC5xxx_PSC_DEBUG_OPEN, "after inc_use_count\n");

	rs_enable_rx_interrupts(port); 

	retval = gs_block_til_ready(&port->gs, filp);
	rs_dprintk(MPC5xxx_PSC_DEBUG_OPEN,
			"Block til ready returned %d. Count=%d\n",
			retval, port->gs.count);

	if (retval) {
		MOD_DEC_USE_COUNT;
		port->gs.count--;
		return retval;
	}
	/* tty->low_latency = 1; */

	if ((port->gs.count == 1) && (port->gs.flags & ASYNC_SPLIT_TERMIOS)) {
		if (tty->driver.subtype == SERIAL_TYPE_NORMAL)
			*tty->termios = port->gs.normal_termios;
		else 
			*tty->termios = port->gs.callout_termios;
		rs_set_real_termios(port);
	}

	port->gs.session = current->session;
	port->gs.pgrp = current->pgrp;
	func_exit();

	return 0;

}

static void rs_close(void *ptr)
{
	func_enter();

	MOD_DEC_USE_COUNT;
	func_exit();
}


/* I haven't the foggiest why the decrement use count has to happen
   here. The whole linux serial drivers stuff needs to be redesigned.
   My guess is that this is a hack to minimize the impact of a bug
   elsewhere. Thinking about it some more. (try it sometime) Try
   running minicom on a serial port that is driven by a modularized
   driver. Have the modem hangup. Then remove the driver module. Then
   exit minicom.  I expect an "oops".  -- REW */
static void rs_hungup(void *ptr)
{
	func_enter();
	MOD_DEC_USE_COUNT;
	func_exit();
}

static int rs_ioctl(struct tty_struct * tty, struct file * filp, 
		unsigned int cmd, unsigned long arg)
{
	int rc;
	struct rs_port *port = tty->driver_data;
	int ival;

	rc = 0;
	switch (cmd) {
	case TIOCGSOFTCAR:
		rc = put_user(((tty->termios->c_cflag & CLOCAL) ? 1 : 0),
				(unsigned int *) arg);
		break;
	case TIOCSSOFTCAR:
		if ((rc = verify_area(VERIFY_READ, (void *) arg,
				sizeof(int))) == 0) {
			get_user(ival, (unsigned int *) arg);
			tty->termios->c_cflag =
				(tty->termios->c_cflag & ~CLOCAL) |
				(ival ? CLOCAL : 0);
		}
		break;
	case TIOCGSERIAL:
		if ((rc = verify_area(VERIFY_WRITE, (void *) arg,
				sizeof(struct serial_struct))) == 0)
			rc = gs_getserial(&port->gs,
					  (struct serial_struct *) arg);
		break;
	case TIOCSSERIAL:
		if ((rc = verify_area(VERIFY_READ, (void *) arg,
				sizeof(struct serial_struct))) == 0)
			rc = gs_setserial(&port->gs,
					  (struct serial_struct *) arg);
		break;
	default:
		rc = -ENOIOCTLCMD;
		break;
	}

	/* func_exit(); */
	return rc;
}


/*
 * This function is used to send a high-priority XON/XOFF character to
 * the device
 */
static void rs_send_xchar(struct tty_struct * tty, char ch)
{
	struct rs_port *port = (struct rs_port *)tty->driver_data;
	func_enter();
	
	port->x_char = ch;
	if (ch) {
		/* Make sure transmit interrupts are on */
		rs_enable_tx_interrupts(tty);
	}

	func_exit();
}


/*
 * ------------------------------------------------------------
 * rs_throttle()
 * 
 * This routine is called by the upper-layer tty layer to signal that
 * incoming characters should be throttled.
 * ------------------------------------------------------------
 */
static void rs_throttle(struct tty_struct * tty)
{
#ifdef MPC5xxx_PSC_DEBUG_THROTTLE
	char	buf[64];
	
	printk("throttle %s: %d....\n", tty_name(tty, buf),
			tty->ldisc.chars_in_buffer(tty));
#endif

	func_enter();
	
	if (I_IXOFF(tty))
		rs_send_xchar(tty, STOP_CHAR(tty));

	func_exit();
}

static void rs_unthrottle(struct tty_struct * tty)
{
	struct rs_port *port = (struct rs_port *)tty->driver_data;
#ifdef MPC5xxx_PSC_DEBUG_THROTTLE
	char	buf[64];
	
	printk("unthrottle %s: %d....\n", tty_name(tty, buf),
			tty->ldisc.chars_in_buffer(tty));
#endif

	func_enter();
	
	if (I_IXOFF(tty)) {
		if (port->x_char)
			port->x_char = 0;
		else
			rs_send_xchar(tty, START_CHAR(tty));
	}

	func_exit();
}





/* **********************************************************************
 *		Here are the initialization routines.			*
 * **********************************************************************
 */

void * ckmalloc(int size)
{
	void *p;

	p = kmalloc(size, GFP_KERNEL);
	if (p) 
		memset(p, 0, size);
	return p;
}



static int rs_init_portstructs(void)
{
	struct rs_port *port;
	int i;
#ifdef CONFIG_UBOOT
	extern unsigned char __res[];
	bd_t *bd = (bd_t *)__res;
#endif

	/* Debugging */
	func_enter();

	rs_termios = ckmalloc(MPC5xxx_PSC_NPORTS * sizeof (struct termios *));
	if (!rs_termios) {
		return -ENOMEM;
	}

	rs_termios_locked = ckmalloc(MPC5xxx_PSC_NPORTS *
						sizeof (struct termios *));
	if (!rs_termios_locked) {
		kfree(rs_termios);
		return -ENOMEM;
	}

	/* Adjust the values in the "driver" */
	rs_driver.termios = rs_termios;
	rs_driver.termios_locked = rs_termios_locked;

	port = rs_ports;
	for (i=0; i < MPC5xxx_PSC_NPORTS;i++) {
		rs_dprintk(MPC5xxx_PSC_DEBUG_INIT, "initing port %d\n", i);
		port->gs.callout_termios = tty_std_termios;
		port->gs.normal_termios	= tty_std_termios;
		port->gs.magic = SERIAL_MAGIC;
		port->gs.close_delay = HZ/2;
		port->gs.closing_wait = 30 * HZ;
		port->gs.rd = &rs_real_driver;
#ifdef NEW_WRITE_LOCKING
		port->gs.port_write_sem = MUTEX;
#endif
#ifdef DECLARE_WAITQUEUE
		init_waitqueue_head(&port->gs.open_wait);
		init_waitqueue_head(&port->gs.close_wait);
#endif
		port->psc = (struct mpc5xxx_psc *)rs_table[i].iomem_base;
		port->irq = rs_table[i].irq;
#ifdef CONFIG_UBOOT
		port->gs.baud_base = bd->bi_ipbfreq;
#else
		port->gs.baud_base = rs_table[i].baud_base;
#endif
		rs_dprintk(MPC5xxx_PSC_DEBUG_INIT, "psc base 0x%08lx\n",
				(unsigned long)port->psc);
		port++;
	}

	func_exit();
	return 0;
}

static int rs_init_drivers(void)
{
	int error;

	func_enter();

	memset(&rs_driver, 0, sizeof(rs_driver));
	rs_driver.magic = TTY_DRIVER_MAGIC;
	rs_driver.driver_name = "serial";
	rs_driver.name = "ttyS";
	rs_driver.major = TTY_MAJOR;
	rs_driver.minor_start = 64;
	rs_driver.num = MPC5xxx_PSC_NPORTS;
	rs_driver.type = TTY_DRIVER_TYPE_SERIAL;
	rs_driver.subtype = SERIAL_TYPE_NORMAL;
	rs_driver.init_termios = tty_std_termios;
	rs_driver.init_termios.c_cflag =
		baud_index | CS8 | CREAD | HUPCL | CLOCAL;
	rs_driver.refcount = &rs_refcount;
	rs_driver.table = rs_tty;
	rs_driver.termios = rs_termios;
	rs_driver.termios_locked = rs_termios_locked;

	rs_driver.open	= rs_open;
	rs_driver.close = gs_close;
	rs_driver.write = gs_write;
	rs_driver.put_char = gs_put_char; 
	rs_driver.flush_chars = gs_flush_chars;
	rs_driver.write_room = gs_write_room;
	rs_driver.chars_in_buffer = gs_chars_in_buffer;
	rs_driver.flush_buffer = gs_flush_buffer;
	rs_driver.ioctl = rs_ioctl;
	rs_driver.throttle = rs_throttle;
	rs_driver.unthrottle = rs_unthrottle;
	rs_driver.set_termios = gs_set_termios;
	rs_driver.stop = gs_stop;
	rs_driver.start = gs_start;
	rs_driver.hangup = gs_hangup;

	rs_callout_driver = rs_driver;
	rs_callout_driver.name = "cua";
	rs_callout_driver.major = TTYAUX_MAJOR;
	rs_callout_driver.subtype = SERIAL_TYPE_CALLOUT;

	if ((error = tty_register_driver(&rs_driver))) {
		printk(KERN_ERR
			"Couldn't register serial driver, error = %d\n",
			error);
		return 1;
	}
	if ((error = tty_register_driver(&rs_callout_driver))) {
		tty_unregister_driver(&rs_driver);
		printk(KERN_ERR
			"Couldn't register callout driver, error = %d\n",
			error);
		return 1;
	}

	func_exit();
	return 0;
}


void __init mpc5xxx_psc_rs_init(void)
{
	int rc;
	int i;
	struct rs_port *port;

	func_enter();
	rs_dprintk(MPC5xxx_PSC_DEBUG_INIT,
			"Initializing serial... (rs_debug=%d)\n", rs_debug);

	rc = rs_init_portstructs();
	rs_init_drivers();
	port = rs_ports;
	for (i=0; i < MPC5xxx_PSC_NPORTS;i++) {
		rs_disable_rx_interrupts(port); 
		rs_disable_tx_interrupts(port); 
		if (request_irq(port->irq, rs_interrupt,
			SA_SHIRQ | SA_INTERRUPT, "serial", port)) {
			printk(KERN_ERR
				"rs: Cannot allocate irq for PSC%d.\n", i+1);
			rc = 0;
			continue;
		}
		rs_dprintk(MPC5xxx_PSC_DEBUG_INIT,
				"requested irq for port[%d] = %08x\n",
				i, (u32)port);
		port++;
	}

/*
 * XXX
 * Need to set up uart registers, baud rate, etc., ifndef
 * CONFIG_PPC_5xxx_PSC_CONSOLE?
 *
 */

	if (rc >= 0) 
		rs_initialized++;

	func_exit();
}

/*
 * Begin serial console routines
 */
#ifdef CONFIG_PPC_5xxx_PSC_CONSOLE

static void serial_outc(struct rs_port *port, unsigned char c)
{
	struct mpc5xxx_psc *psc = port->psc;
	int i;
#define BUSY_WAIT 10000

	/*
	 * Turn PSC interrupts off
	 */
	out_be16(&psc->mpc5xxx_psc_imr, 0);

	/*
	 * Wait for the Tx register to become empty
	 */
	i = BUSY_WAIT;
	while (!(in_be16(&psc->mpc5xxx_psc_status) & MPC5xxx_PSC_SR_TXEMP) &&
			i--)
		udelay(1);
	out_8(&psc->mpc5xxx_psc_buffer_8, c);
	i = BUSY_WAIT;
	while (!(in_be16(&psc->mpc5xxx_psc_status) & MPC5xxx_PSC_SR_TXEMP) &&
			i--)
		udelay(1);

	/*
	 * Turn PSC interrupts back on
	 */
	out_be16(&psc->mpc5xxx_psc_imr, port->imr);
}

static void serial_console_write(struct console *co, const char *s,
		unsigned count)
{
	unsigned int i;
	struct rs_port *port;

	port = (struct rs_port *)&rs_ports[co->index];

	for (i = 0; i < count; i++) {
		if (*s == '\n')
			serial_outc(port, '\r');
		serial_outc(port, *s++);
	}
}

static kdev_t serial_console_device(struct console *c)
{
	return mk_kdev(TTY_MAJOR, 64 + c->index);
}

static __init int serial_console_setup(struct console *co, char *options)
{
	struct serial_state *ser = &rs_table[co->index];
	struct mpc5xxx_psc *psc = (struct mpc5xxx_psc *)ser->iomem_base;
	int	baud = CONFIG_PPC_5xxx_PSC_CONSOLE_BAUD;
	int	bits = '8';
	int	parity = 'n';
	char	*s;
	int	divisor;
	int	mode1;
	int	mode2;
	int	i;
#ifdef CONFIG_UBOOT
	extern unsigned char __res[];
	bd_t *bd = (bd_t *)__res;
#endif

	rs_ports[co->index].psc = psc;

	if (options) {
		baud = simple_strtoul(options, NULL, 10);
		if (baud == 0)
			baud = CONFIG_PPC_5xxx_PSC_CONSOLE_BAUD;
		s = options;
		while(*s >= '0' && *s <= '9')
			s++;
		if (*s) parity = *s++;
		if (*s) bits   = *s;
	}

	for (i=0; i<(sizeof(baud_table) / sizeof(baud_table[0])); i++) {
		if (baud_table[i] == baud) {
			baud_index = i;
			break;
		}
	}

	switch (bits) {
	case '5':
		bits = MPC5xxx_PSC_MODE_5_BITS;
		break;
	case '6':
		bits = MPC5xxx_PSC_MODE_6_BITS;
		break;
	case '7':
		bits = MPC5xxx_PSC_MODE_7_BITS;
		break;
	case '8':
		bits = MPC5xxx_PSC_MODE_8_BITS;
		break;
	default:
		printk("console bits/char out of range: '%c'\n", bits);
	}

	switch (parity) {
	case 'n':
		parity = MPC5xxx_PSC_MODE_PARNONE;
		break;
	case 'o': case 'O':
		parity = MPC5xxx_PSC_MODE_PARODD;
		break;
	case 'e': case 'E':
		parity = MPC5xxx_PSC_MODE_PAREVEN;
		break;
	default:
		printk("bad parity requested: '%c'\n", parity);
		break;
	}

	out_8(&psc->command, MPC5xxx_PSC_RST_TX
			| MPC5xxx_PSC_RX_DISABLE | MPC5xxx_PSC_TX_ENABLE);
	out_8(&psc->command, MPC5xxx_PSC_RST_RX);

	out_be32(&psc->sicr, 0x0);
	out_be16(&psc->mpc5xxx_psc_clock_select, 0xdd00);
	out_be16(&psc->tfalarm, 0xf8);

	out_8(&psc->command, MPC5xxx_PSC_SEL_MODE_REG_1
			| MPC5xxx_PSC_RX_ENABLE
			| MPC5xxx_PSC_TX_ENABLE);

#ifdef CONFIG_UBOOT
	divisor = ((bd->bi_ipbfreq / (baud * 16)) + 1) >> 1;	/* round up */
#else
	divisor = ((ser->baud_base / (baud * 16)) + 1) >> 1;	/* round up */
#endif

	mode1 = bits | parity | MPC5xxx_PSC_MODE_ERR;
	mode2 = MPC5xxx_PSC_MODE_ONE_STOP;

	out_8(&psc->ctur, divisor>>8);
	out_8(&psc->ctlr, divisor);
	out_8(&psc->command, MPC5xxx_PSC_SEL_MODE_REG_1);
	out_8(&psc->mode, mode1);
	out_8(&psc->mode, mode2);

	return 0;
}

static struct console sercons = {
	.name	= "ttyS",
	.write	= serial_console_write,
	.device	= serial_console_device,
	.setup	= serial_console_setup,
	.flags	= CON_PRINTBUFFER,
	.index	= CONFIG_PPC_5xxx_PSC_CONSOLE_PORT
};

void __init mpc5xxx_psc_console_init(void)
{
	register_console(&sercons);
}

#endif
