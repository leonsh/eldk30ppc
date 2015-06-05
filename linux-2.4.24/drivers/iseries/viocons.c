/* -*- linux-c -*-
 *  drivers/char/viocons.c
 *
 *  iSeries Virtual Terminal
 *
 *  Authors: Dave Boutcher <boutcher@us.ibm.com>
 *           Ryan Arnold <ryanarn@us.ibm.com>
 *           Colin Devilbiss <devilbis@us.ibm.com>
 *
 * (C) Copyright 2000 IBM Corporation
 * 
 * This program is free software;  you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) anyu later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.  
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
#include <linux/config.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/errno.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/console.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <asm/ioctls.h>
#include <linux/kd.h>

#include "vio.h"

#include <asm/iSeries/HvLpEvent.h>
#include "asm/iSeries/HvCallEvent.h"
#include "asm/iSeries/HvLpConfig.h"
#include "asm/iSeries/HvCall.h"
#include <asm/iSeries/iSeries_proc.h>

/* Check that the tty_driver_data actually points to our stuff
 */
#define VIOTTY_PARANOIA_CHECK 1
#define VIOTTY_MAGIC (0x0DCB)

static int debug;

static DECLARE_WAIT_QUEUE_HEAD(viocons_wait_queue);

#define viotty_major 229

#define VTTY_PORTS 10
#define VIOTTY_SERIAL_START 65

static u64 sndMsgSeq[VTTY_PORTS];
static u64 sndMsgAck[VTTY_PORTS];

static spinlock_t consolelock = SPIN_LOCK_UNLOCKED;

/* THe structure of the events that flow between us and OS/400.  You can't
 * mess with this unless the OS/400 side changes too
 */
struct viocharlpevent {
	struct HvLpEvent event;
	u32 mReserved1;
	u16 mVersion;
	u16 mSubTypeRc;
	u8 virtualDevice;
	u8 immediateDataLen;
	u8 immediateData[VIOCHAR_MAX_DATA];
};

#define viochar_window (10)
#define viochar_highwatermark (3)

enum viocharsubtype {
	viocharopen = 0x0001,
	viocharclose = 0x0002,
	viochardata = 0x0003,
	viocharack = 0x0004,
	viocharconfig = 0x0005
};

enum viochar_rc {
	viochar_rc_ebusy = 1
};

/* When we get writes faster than we can send it to the partition,
 * buffer the data here.  There is one set of buffers for each virtual
 * port.
 * Note that bufferUsed is a bit map of used buffers.
 * It had better have enough bits to hold NUM_BUF
 * the bitops assume it is a multiple of unsigned long
 */
#define NUM_BUF (8)
#define OVERFLOW_SIZE VIOCHAR_MAX_DATA

static struct overflowBuffers {
	unsigned long bufferUsed;
	u8 *buffer[NUM_BUF];
	int bufferBytes[NUM_BUF];
	int curbuf;
	int bufferOverflow;
	int overflowMessage;
} overflow[VTTY_PORTS];

static void initDataEvent(struct viocharlpevent *viochar, HvLpIndex lp);

static struct tty_driver viotty_driver;
static struct tty_driver viottyS_driver;
static int viotty_refcount;

static struct tty_struct *viotty_table[VTTY_PORTS];
static struct tty_struct *viottyS_table[VTTY_PORTS];
static struct termios *viotty_termios[VTTY_PORTS];
static struct termios *viottyS_termios[VTTY_PORTS];
static struct termios *viotty_termios_locked[VTTY_PORTS];
static struct termios *viottyS_termios_locked[VTTY_PORTS];

void hvlog(char *fmt, ...)
{
	int i;
	static char buf[256];
	va_list args;
	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);
	va_end(args);
	HvCall_writeLogBuffer(buf, i);
	HvCall_writeLogBuffer("\r", 1);

}

/* Our port information.  We store a pointer to one entry in the
 * tty_driver_data
 */
static struct port_info_tag {
	int magic;
	struct tty_struct *tty;
	HvLpIndex lp;
	u8 vcons;
	u8 port;
} port_info[VTTY_PORTS];

/* Make sure we're pointing to a valid port_info structure.  Shamelessly
 * plagerized from serial.c
 */
static inline int viotty_paranoia_check(struct port_info_tag *pi,
					kdev_t device, const char *routine)
{
#ifdef VIOTTY_PARANOIA_CHECK
	static const char *badmagic =
	    "%s Warning: bad magic number for port_info struct (%s) in %s\n";
	static const char *badinfo =
	    "%s Warning: null port_info for (%s) in %s\n";

	if (!pi) {
		printk(badinfo, KERN_WARNING_VIO, kdevname(device),
		       routine);
		return 1;
	}
	if (pi->magic != VIOTTY_MAGIC) {
		printk(badmagic, KERN_WARNING_VIO, kdevname(device),
		       routine);
		return 1;
	}
#endif
	return 0;
}

/*
 * Handle reads from the proc file system.  Right now we just dump the
 * state of the first TTY
 */
static int proc_read(char *buf, char **start, off_t offset,
		     int blen, int *eof, void *data)
{
	int len = 0;
	struct tty_struct *tty = viotty_table[0];
	struct termios *termios;
	if (tty == NULL) {
		len += sprintf(buf + len, "no tty\n");
		*eof = 1;
		return len;
	}

	len +=
	    sprintf(buf + len,
		    "tty info: COOK_OUT %ld COOK_IN %ld, NO_WRITE_SPLIT %ld\n",
		    tty->flags & TTY_HW_COOK_OUT,
		    tty->flags & TTY_HW_COOK_IN,
		    tty->flags & TTY_NO_WRITE_SPLIT);

	termios = tty->termios;
	if (termios == NULL) {
		len += sprintf(buf + len, "no termios\n");
		*eof = 1;
		return len;
	}
	len += sprintf(buf + len, "INTR_CHAR     %2.2x\n", INTR_CHAR(tty));
	len += sprintf(buf + len, "QUIT_CHAR     %2.2x\n", QUIT_CHAR(tty));
	len +=
	    sprintf(buf + len, "ERASE_CHAR    %2.2x\n", ERASE_CHAR(tty));
	len += sprintf(buf + len, "KILL_CHAR     %2.2x\n", KILL_CHAR(tty));
	len += sprintf(buf + len, "EOF_CHAR      %2.2x\n", EOF_CHAR(tty));
	len += sprintf(buf + len, "TIME_CHAR     %2.2x\n", TIME_CHAR(tty));
	len += sprintf(buf + len, "MIN_CHAR      %2.2x\n", MIN_CHAR(tty));
	len += sprintf(buf + len, "SWTC_CHAR     %2.2x\n", SWTC_CHAR(tty));
	len +=
	    sprintf(buf + len, "START_CHAR    %2.2x\n", START_CHAR(tty));
	len += sprintf(buf + len, "STOP_CHAR     %2.2x\n", STOP_CHAR(tty));
	len += sprintf(buf + len, "SUSP_CHAR     %2.2x\n", SUSP_CHAR(tty));
	len += sprintf(buf + len, "EOL_CHAR      %2.2x\n", EOL_CHAR(tty));
	len +=
	    sprintf(buf + len, "REPRINT_CHAR  %2.2x\n", REPRINT_CHAR(tty));
	len +=
	    sprintf(buf + len, "DISCARD_CHAR  %2.2x\n", DISCARD_CHAR(tty));
	len +=
	    sprintf(buf + len, "WERASE_CHAR   %2.2x\n", WERASE_CHAR(tty));
	len +=
	    sprintf(buf + len, "LNEXT_CHAR    %2.2x\n", LNEXT_CHAR(tty));
	len += sprintf(buf + len, "EOL2_CHAR     %2.2x\n", EOL2_CHAR(tty));

	len += sprintf(buf + len, "I_IGNBRK      %4.4x\n", I_IGNBRK(tty));
	len += sprintf(buf + len, "I_BRKINT      %4.4x\n", I_BRKINT(tty));
	len += sprintf(buf + len, "I_IGNPAR      %4.4x\n", I_IGNPAR(tty));
	len += sprintf(buf + len, "I_PARMRK      %4.4x\n", I_PARMRK(tty));
	len += sprintf(buf + len, "I_INPCK       %4.4x\n", I_INPCK(tty));
	len += sprintf(buf + len, "I_ISTRIP      %4.4x\n", I_ISTRIP(tty));
	len += sprintf(buf + len, "I_INLCR       %4.4x\n", I_INLCR(tty));
	len += sprintf(buf + len, "I_IGNCR       %4.4x\n", I_IGNCR(tty));
	len += sprintf(buf + len, "I_ICRNL       %4.4x\n", I_ICRNL(tty));
	len += sprintf(buf + len, "I_IUCLC       %4.4x\n", I_IUCLC(tty));
	len += sprintf(buf + len, "I_IXON        %4.4x\n", I_IXON(tty));
	len += sprintf(buf + len, "I_IXANY       %4.4x\n", I_IXANY(tty));
	len += sprintf(buf + len, "I_IXOFF       %4.4x\n", I_IXOFF(tty));
	len += sprintf(buf + len, "I_IMAXBEL     %4.4x\n", I_IMAXBEL(tty));

	len += sprintf(buf + len, "O_OPOST       %4.4x\n", O_OPOST(tty));
	len += sprintf(buf + len, "O_OLCUC       %4.4x\n", O_OLCUC(tty));
	len += sprintf(buf + len, "O_ONLCR       %4.4x\n", O_ONLCR(tty));
	len += sprintf(buf + len, "O_OCRNL       %4.4x\n", O_OCRNL(tty));
	len += sprintf(buf + len, "O_ONOCR       %4.4x\n", O_ONOCR(tty));
	len += sprintf(buf + len, "O_ONLRET      %4.4x\n", O_ONLRET(tty));
	len += sprintf(buf + len, "O_OFILL       %4.4x\n", O_OFILL(tty));
	len += sprintf(buf + len, "O_OFDEL       %4.4x\n", O_OFDEL(tty));
	len += sprintf(buf + len, "O_NLDLY       %4.4x\n", O_NLDLY(tty));
	len += sprintf(buf + len, "O_CRDLY       %4.4x\n", O_CRDLY(tty));
	len += sprintf(buf + len, "O_TABDLY      %4.4x\n", O_TABDLY(tty));
	len += sprintf(buf + len, "O_BSDLY       %4.4x\n", O_BSDLY(tty));
	len += sprintf(buf + len, "O_VTDLY       %4.4x\n", O_VTDLY(tty));
	len += sprintf(buf + len, "O_FFDLY       %4.4x\n", O_FFDLY(tty));

	len += sprintf(buf + len, "C_BAUD        %4.4x\n", C_BAUD(tty));
	len += sprintf(buf + len, "C_CSIZE       %4.4x\n", C_CSIZE(tty));
	len += sprintf(buf + len, "C_CSTOPB      %4.4x\n", C_CSTOPB(tty));
	len += sprintf(buf + len, "C_CREAD       %4.4x\n", C_CREAD(tty));
	len += sprintf(buf + len, "C_PARENB      %4.4x\n", C_PARENB(tty));
	len += sprintf(buf + len, "C_PARODD      %4.4x\n", C_PARODD(tty));
	len += sprintf(buf + len, "C_HUPCL       %4.4x\n", C_HUPCL(tty));
	len += sprintf(buf + len, "C_CLOCAL      %4.4x\n", C_CLOCAL(tty));
	len += sprintf(buf + len, "C_CRTSCTS     %4.4x\n", C_CRTSCTS(tty));

	len += sprintf(buf + len, "L_ISIG        %4.4x\n", L_ISIG(tty));
	len += sprintf(buf + len, "L_ICANON      %4.4x\n", L_ICANON(tty));
	len += sprintf(buf + len, "L_XCASE       %4.4x\n", L_XCASE(tty));
	len += sprintf(buf + len, "L_ECHO        %4.4x\n", L_ECHO(tty));
	len += sprintf(buf + len, "L_ECHOE       %4.4x\n", L_ECHOE(tty));
	len += sprintf(buf + len, "L_ECHOK       %4.4x\n", L_ECHOK(tty));
	len += sprintf(buf + len, "L_ECHONL      %4.4x\n", L_ECHONL(tty));
	len += sprintf(buf + len, "L_NOFLSH      %4.4x\n", L_NOFLSH(tty));
	len += sprintf(buf + len, "L_TOSTOP      %4.4x\n", L_TOSTOP(tty));
	len += sprintf(buf + len, "L_ECHOCTL     %4.4x\n", L_ECHOCTL(tty));
	len += sprintf(buf + len, "L_ECHOPRT     %4.4x\n", L_ECHOPRT(tty));
	len += sprintf(buf + len, "L_ECHOKE      %4.4x\n", L_ECHOKE(tty));
	len += sprintf(buf + len, "L_FLUSHO      %4.4x\n", L_FLUSHO(tty));
	len += sprintf(buf + len, "L_PENDIN      %4.4x\n", L_PENDIN(tty));
	len += sprintf(buf + len, "L_IEXTEN      %4.4x\n", L_IEXTEN(tty));

	*eof = 1;
	return len;
}

/*
 * Handle writes to our proc file system.  Right now just turns on and off
 * our debug flag
 */
static int proc_write(struct file *file, const char *buffer,
		      unsigned long count, void *data)
{
	if (count) {
		if (buffer[0] == '1') {
			printk("viocons: debugging on\n");
			debug = 1;
		} else {
			printk("viocons: debugging off\n");
			debug = 0;
		}
	}
	return count;
}

/*
 * setup our proc file system entries
 */
void viocons_proc_init(struct proc_dir_entry *iSeries_proc)
{
	struct proc_dir_entry *ent;
	ent =
	    create_proc_entry("viocons", S_IFREG | S_IRUSR, iSeries_proc);
	if (!ent)
		return;
	ent->nlink = 1;
	ent->data = NULL;
	ent->read_proc = proc_read;
	ent->write_proc = proc_write;
}

/*
 * clean up our proc file system entries
 */
void viocons_proc_delete(struct proc_dir_entry *iSeries_proc)
{
	remove_proc_entry("viocons", iSeries_proc);
}

/*
 * Add data to our pending-send buffers.  
 *
 * NOTE: Don't use printk in here because it gets nastily recursive.  hvlog can be
 * used to log to the hypervisor buffer
 */
static int bufferAdd(u8 port, const char *buf, size_t len, int userFlag)
{
	size_t bleft = len;
	size_t curlen;
	char *cbuf = (char *) buf;
	int nextbuf;
	struct overflowBuffers *pov = &overflow[port];
	while (bleft > 0) {
		/* If there is no space left in the current buffer, we have
		 * filled everything up, so return.  If we filled the previous
		 * buffer we would already have moved to the next one.
		 */
		if (pov->bufferBytes[pov->curbuf] == OVERFLOW_SIZE) {
			hvlog("buffer %d full.  no more space\n",
			      pov->curbuf);
			pov->bufferOverflow++;
			pov->overflowMessage = 1;
			return len - bleft;
		}

		/* Turn on the "used" bit for this buffer.  If it's already on, that's
		 * fine.
		 */
		set_bit(pov->curbuf, &pov->bufferUsed);

		/* 
		 * See if this buffer has been allocated.  If not, allocate it
		 */
		if (pov->buffer[pov->curbuf] == NULL)
			pov->buffer[pov->curbuf] =
			    kmalloc(OVERFLOW_SIZE, GFP_ATOMIC);

		/*
		 * Figure out how much we can copy into this buffer
		 */
		if (bleft <
		    (OVERFLOW_SIZE - pov->bufferBytes[pov->curbuf]))
			curlen = bleft;
		else
			curlen =
			    OVERFLOW_SIZE - pov->bufferBytes[pov->curbuf];

		/*
		 * Copy the data into the buffer                      
		 */
		if (userFlag)
			copy_from_user(pov->buffer[pov->curbuf] +
				       pov->bufferBytes[pov->curbuf], cbuf,
				       curlen);
		else
			memcpy(pov->buffer[pov->curbuf] +
			       pov->bufferBytes[pov->curbuf], cbuf,
			       curlen);

		pov->bufferBytes[pov->curbuf] += curlen;
		cbuf += curlen;
		bleft -= curlen;

		/*
		 * Now see if we've filled this buffer
		 */
		if (pov->bufferBytes[pov->curbuf] == OVERFLOW_SIZE) {
			nextbuf = (pov->curbuf + 1) % NUM_BUF;

			/*
			 * Move to the next buffer if it hasn't been used yet
			 */
			if (test_bit(nextbuf, &pov->bufferUsed) == 0) {
				pov->curbuf = nextbuf;
			}
		}
	}
	return len;
}

/* Send pending data
 *
 * NOTE: Don't use printk in here because it gets nastily recursive.  hvlog can be
 * used to log to the hypervisor buffer
 */
void sendBuffers(u8 port, HvLpIndex lp)
{
	HvLpEvent_Rc hvrc;
	int nextbuf;
	struct viocharlpevent *viochar;
	unsigned long flags;
	struct overflowBuffers *pov = &overflow[port];

	spin_lock_irqsave(&consolelock, flags);

	viochar = (struct viocharlpevent *)
	    vio_get_event_buffer(viomajorsubtype_chario);

	/* Make sure we got a buffer
	 */
	if (viochar == NULL) {
		hvlog("Yikes...can't get viochar buffer");
		spin_unlock_irqrestore(&consolelock, flags);
		return;
	}

	if (pov->bufferUsed == 0) {
		hvlog("in sendbuffers, but no buffers used\n");
		vio_free_event_buffer(viomajorsubtype_chario, viochar);
		spin_unlock_irqrestore(&consolelock, flags);
		return;
	}

	/*
	 * curbuf points to the buffer we're filling.  We want to start sending AFTER
	 * this one.  
	 */
	nextbuf = (pov->curbuf + 1) % NUM_BUF;

	/*
	 * Loop until we find a buffer with the bufferUsed bit on
	 */
	while (test_bit(nextbuf, &pov->bufferUsed) == 0)
		nextbuf = (nextbuf + 1) % NUM_BUF;

	initDataEvent(viochar, lp);

	/*
	 * While we have buffers with data, and our send window is open, send them
	 */
	while ((test_bit(nextbuf, &pov->bufferUsed)) &&
	       ((sndMsgSeq[port] - sndMsgAck[port]) < viochar_window)) {
		viochar->immediateDataLen = pov->bufferBytes[nextbuf];
		viochar->event.xCorrelationToken = sndMsgSeq[port]++;
		viochar->event.xSizeMinus1 =
		    offsetof(struct viocharlpevent,
			     immediateData) + viochar->immediateDataLen;

		memcpy(viochar->immediateData, pov->buffer[nextbuf],
		       viochar->immediateDataLen);

		hvrc = HvCallEvent_signalLpEvent(&viochar->event);
		if (hvrc) {
			/*
			 * MUST unlock the spinlock before doing a printk
			 */
			vio_free_event_buffer(viomajorsubtype_chario,
					      viochar);
			spin_unlock_irqrestore(&consolelock, flags);

			printk(KERN_WARNING_VIO
			       "console error sending event! return code %d\n",
			       (int) hvrc);
			return;
		}

		/*
		 * clear the bufferUsed bit, zero the number of bytes in this buffer,
		 * and move to the next buffer
		 */
		clear_bit(nextbuf, &pov->bufferUsed);
		pov->bufferBytes[nextbuf] = 0;
		nextbuf = (nextbuf + 1) % NUM_BUF;
	}


	/*
	 * If we have emptied all the buffers, start at 0 again.
	 * this will re-use any allocated buffers
	 */
	if (pov->bufferUsed == 0) {
		pov->curbuf = 0;

		if (pov->overflowMessage)
			pov->overflowMessage = 0;

		if (port_info[port].tty) {
			if ((port_info[port].tty->
			     flags & (1 << TTY_DO_WRITE_WAKEUP))
			    && (port_info[port].tty->ldisc.write_wakeup))
				(port_info[port].tty->ldisc.
				 write_wakeup) (port_info[port].tty);
			wake_up_interruptible(&port_info[port].tty->
					      write_wait);
		}
	}

	vio_free_event_buffer(viomajorsubtype_chario, viochar);
	spin_unlock_irqrestore(&consolelock, flags);

}

/* Our internal writer.  Gets called both from the console device and
 * the tty device.  the tty pointer will be NULL if called from the console.
 *
 * NOTE: Don't use printk in here because it gets nastily recursive.  hvlog can be
 * used to log to the hypervisor buffer
 */
static int internal_write(struct tty_struct *tty, const char *buf,
			  size_t len, int userFlag)
{
	HvLpEvent_Rc hvrc;
	size_t bleft = len;
	size_t curlen;
	const char *curbuf = buf;
	struct viocharlpevent *viochar;
	unsigned long flags;
	struct port_info_tag *pi = NULL;
	HvLpIndex lp;
	u8 port;

	if (tty) {
		pi = (struct port_info_tag *) tty->driver_data;

		if (!pi
		    || viotty_paranoia_check(pi, tty->device,
					     "viotty_internal_write"))
			return -ENODEV;

		lp = pi->lp;
		port = pi->port;
	} else {
		/* If this is the console device, use the lp from the first port entry
		 */
		port = 0;
		lp = port_info[0].lp;
	}

	/* Always put console output in the hypervisor console log
	 */
	if (port == 0)
		HvCall_writeLogBuffer(buf, len);

	/* If the path to this LP is closed, don't bother doing anything more.
	 * just dump the data on the floor
	 */
	if (!viopath_isactive(lp))
		return len;

	/*
	 * If there is already data queued for this port, send it
	 */
	if (overflow[port].bufferUsed)
		sendBuffers(port, lp);

	spin_lock_irqsave(&consolelock, flags);

	viochar = (struct viocharlpevent *)
	    vio_get_event_buffer(viomajorsubtype_chario);
	/* Make sure we got a buffer
	 */
	if (viochar == NULL) {
		hvlog("Yikes...can't get viochar buffer");
		spin_unlock_irqrestore(&consolelock, flags);
		return -1;
	}

	initDataEvent(viochar, lp);

	/* Got the lock, don't cause console output */
	while ((bleft > 0) &&
	       (overflow[port].bufferUsed == 0) &&
	       ((sndMsgSeq[port] - sndMsgAck[port]) < viochar_window)) {
		if (bleft > VIOCHAR_MAX_DATA)
			curlen = VIOCHAR_MAX_DATA;
		else
			curlen = bleft;

		viochar->immediateDataLen = curlen;
		viochar->event.xCorrelationToken = sndMsgSeq[port]++;

		if (userFlag)
			copy_from_user(viochar->immediateData, curbuf,
				       curlen);
		else
			memcpy(viochar->immediateData, curbuf, curlen);

		viochar->event.xSizeMinus1 =
		    offsetof(struct viocharlpevent,
			     immediateData) + curlen;

		hvrc = HvCallEvent_signalLpEvent(&viochar->event);
		if (hvrc) {
			/*
			 * MUST unlock the spinlock before doing a printk
			 */
			vio_free_event_buffer(viomajorsubtype_chario,
					      viochar);
			spin_unlock_irqrestore(&consolelock, flags);

			hvlog("viocons: error sending event! %d\n",
			      (int) hvrc);
			return len - bleft;
		}

		curbuf += curlen;
		bleft -= curlen;
	}

	/*
	 * If we didn't send it all, buffer it
	 */
	if (bleft > 0) {
		bleft -= bufferAdd(port, curbuf, bleft, userFlag);
	}
	vio_free_event_buffer(viomajorsubtype_chario, viochar);
	spin_unlock_irqrestore(&consolelock, flags);

	return len - bleft;
}

/* Initialize the common fields in a charLpEvent
 */
static void initDataEvent(struct viocharlpevent *viochar, HvLpIndex lp)
{
	memset(viochar, 0x00, sizeof(struct viocharlpevent));

	viochar->event.xFlags.xValid = 1;
	viochar->event.xFlags.xFunction = HvLpEvent_Function_Int;
	viochar->event.xFlags.xAckInd = HvLpEvent_AckInd_NoAck;
	viochar->event.xFlags.xAckType = HvLpEvent_AckType_DeferredAck;
	viochar->event.xType = HvLpEvent_Type_VirtualIo;
	viochar->event.xSubtype = viomajorsubtype_chario | viochardata;
	viochar->event.xSourceLp = HvLpConfig_getLpIndex();
	viochar->event.xTargetLp = lp;
	viochar->event.xSizeMinus1 = sizeof(struct viocharlpevent);
	viochar->event.xSourceInstanceId = viopath_sourceinst(lp);
	viochar->event.xTargetInstanceId = viopath_targetinst(lp);
}


/* console device write
 */
static void viocons_write(struct console *co, const char *s,
			  unsigned count)
{
	/* This parser will ensure that all single instances of either \n or \r are
	 * matched into carriage return/line feed combinations.  It also allows for
	 * instances where there already exist \n\r combinations as well as the
	 * reverse, \r\n combinations.
	 */

	int index;
	char charptr[1];
	int foundcr;
	int slicebegin;
	int sliceend;

	foundcr = 0;
	slicebegin = 0;
	sliceend = 0;

	for (index = 0; index < count; index++) {
		if (!foundcr && s[index] == 0x0a) {
			if ((slicebegin - sliceend > 0)
			    && sliceend < count) {
				internal_write(NULL, &s[slicebegin],
					       sliceend - slicebegin, 0);
				slicebegin = sliceend;
			}
			charptr[0] = '\r';
			internal_write(NULL, charptr, 1, 0);
		}
		if (foundcr && s[index] != 0x0a) {
			if ((index - 2) >= 0) {
				if (s[index - 2] != 0x0a) {
					internal_write(NULL,
						       &s[slicebegin],
						       sliceend -
						       slicebegin, 0);
					slicebegin = sliceend;
					charptr[0] = '\n';
					internal_write(NULL, charptr, 1,
						       0);
				}
			}
		}
		sliceend++;

		if (s[index] == 0x0d)
			foundcr = 1;
		else
			foundcr = 0;
	}

	internal_write(NULL, &s[slicebegin], sliceend - slicebegin, 0);

	if (count > 1) {
		if (foundcr == 1 && s[count - 1] != 0x0a) {
			charptr[0] = '\n';
			internal_write(NULL, charptr, 1, 0);
		} else if (s[count - 1] == 0x0a && s[count - 2] != 0x0d) {

			charptr[0] = '\r';
			internal_write(NULL, charptr, 1, 0);
		}
	}
}

/* Work out a the device associate with this console
 */
static kdev_t viocons_device(struct console *c)
{
	return MKDEV(TTY_MAJOR, c->index + viotty_driver.minor_start);
}

/* console device read method
 */
static int viocons_read(struct console *co, const char *s, unsigned count)
{
	printk(KERN_DEBUG_VIO "viocons_read\n");
	// Implement me
	interruptible_sleep_on(&viocons_wait_queue);
	return 0;
}

/* console device wait until a key is pressed
 */
static int viocons_wait_key(struct console *co)
{
	printk(KERN_DEBUG_VIO "In viocons_wait_key\n");
	// Implement me
	interruptible_sleep_on(&viocons_wait_queue);
	return 0;
}

/* Do console device setup
 */
static int __init viocons_setup(struct console *co, char *options)
{
	return 0;
}

/* console device I/O methods
 */
static struct console viocons = {
	name:"ttyS",
	write:viocons_write,
	read:viocons_read,
	device:viocons_device,
	wait_key:viocons_wait_key,
	setup:viocons_setup,
	flags:CON_PRINTBUFFER,
};


/* TTY Open method
 */
static int viotty_open(struct tty_struct *tty, struct file *filp)
{
	int port;
	unsigned long flags;
	MOD_INC_USE_COUNT;
	port = MINOR(tty->device) - tty->driver.minor_start;

	if (port >= VIOTTY_SERIAL_START)
		port -= VIOTTY_SERIAL_START;

	if ((port < 0) || (port >= VTTY_PORTS)) {
		MOD_DEC_USE_COUNT;
		return -ENODEV;
	}

	spin_lock_irqsave(&consolelock, flags);

	/*
	 * If some other TTY is already connected here, reject the open
	 */
	if ((port_info[port].tty) && (port_info[port].tty != tty)) {
		spin_unlock_irqrestore(&consolelock, flags);
		MOD_DEC_USE_COUNT;
		printk(KERN_WARNING_VIO
		       "console attempt to open device twice from different ttys\n");
		return -EBUSY;
	}
	tty->driver_data = &port_info[port];
	port_info[port].tty = tty;
	spin_unlock_irqrestore(&consolelock, flags);

	return 0;
}

/* TTY Close method
 */
static void viotty_close(struct tty_struct *tty, struct file *filp)
{
	unsigned long flags;
	struct port_info_tag *pi =
	    (struct port_info_tag *) tty->driver_data;

	if (!pi || viotty_paranoia_check(pi, tty->device, "viotty_close"))
		return;

	spin_lock_irqsave(&consolelock, flags);
	if (tty->count == 1) {
		pi->tty = NULL;
	}

	spin_unlock_irqrestore(&consolelock, flags);

	MOD_DEC_USE_COUNT;
}

/* TTY Write method
 */
static int viotty_write(struct tty_struct *tty, int from_user,
			const unsigned char *buf, int count)
{
	return internal_write(tty, buf, count, from_user);
}

/* TTY put_char method
 */
static void viotty_put_char(struct tty_struct *tty, unsigned char ch)
{
	internal_write(tty, &ch, 1, 0);
}

/* TTY flush_chars method
 */
static void viotty_flush_chars(struct tty_struct *tty)
{
}

/* TTY write_room method
 */
static int viotty_write_room(struct tty_struct *tty)
{
	int i;
	int room = 0;
	struct port_info_tag *pi =
	    (struct port_info_tag *) tty->driver_data;

	if (!pi
	    || viotty_paranoia_check(pi, tty->device,
				     "viotty_sendbuffers"))
		return 0;

	// If no buffers are used, return the max size
	if (overflow[pi->port].bufferUsed == 0)
		return VIOCHAR_MAX_DATA * NUM_BUF;

	for (i = 0; ((i < NUM_BUF) && (room < VIOCHAR_MAX_DATA)); i++) {
		room +=
		    (OVERFLOW_SIZE - overflow[pi->port].bufferBytes[i]);
	}

	if (room > VIOCHAR_MAX_DATA)
		return VIOCHAR_MAX_DATA;
	else
		return room;
}

/* TTY chars_in_buffer_room method
 */
static int viotty_chars_in_buffer(struct tty_struct *tty)
{
	return 0;
}

static void viotty_flush_buffer(struct tty_struct *tty)
{
}

static int viotty_ioctl(struct tty_struct *tty, struct file *file,
			unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
		/* the ioctls below read/set the flags usually shown in the leds */
		/* don't use them - they will go away without warning */
	case KDGETLED:
	case KDGKBLED:
		return put_user(0, (char *) arg);

	case KDSKBLED:
		return 0;
	}

	return n_tty_ioctl(tty, file, cmd, arg);
}

static void viotty_throttle(struct tty_struct *tty)
{
}

static void viotty_unthrottle(struct tty_struct *tty)
{
}

static void viotty_set_termios(struct tty_struct *tty,
			       struct termios *old_termios)
{
}

static void viotty_stop(struct tty_struct *tty)
{
}

static void viotty_start(struct tty_struct *tty)
{
}

static void viotty_hangup(struct tty_struct *tty)
{
}

static void viotty_break(struct tty_struct *tty, int break_state)
{
}

static void viotty_send_xchar(struct tty_struct *tty, char ch)
{
}

static void viotty_wait_until_sent(struct tty_struct *tty, int timeout)
{
}

/* Handle an open charLpEvent.  Could be either interrupt or ack
 */
static void vioHandleOpenEvent(struct HvLpEvent *event)
{
	unsigned long flags;
	u8 eventRc;
	u16 eventSubtypeRc;
	struct viocharlpevent *cevent = (struct viocharlpevent *) event;
	u8 port = cevent->virtualDevice;

	if (event->xFlags.xFunction == HvLpEvent_Function_Ack) {
		if (port >= VTTY_PORTS)
			return;

		spin_lock_irqsave(&consolelock, flags);
		/* Got the lock, don't cause console output */

		if (event->xRc == HvLpEvent_Rc_Good) {
			sndMsgSeq[port] = sndMsgAck[port] = 0;
		}

		port_info[port].lp = event->xTargetLp;

		spin_unlock_irqrestore(&consolelock, flags);

		if (event->xCorrelationToken != 0) {
			unsigned long semptr = event->xCorrelationToken;
			up((struct semaphore *) semptr);
		} else
			printk(KERN_WARNING_VIO
			       "console: wierd...got open ack without semaphore\n");
	} else {
		/* This had better require an ack, otherwise complain
		 */
		if (event->xFlags.xAckInd != HvLpEvent_AckInd_DoAck) {
			printk(KERN_WARNING_VIO
			       "console: viocharopen without ack bit!\n");
			return;
		}

		spin_lock_irqsave(&consolelock, flags);
		/* Got the lock, don't cause console output */

		/* Make sure this is a good virtual tty */
		if (port >= VTTY_PORTS) {
			eventRc = HvLpEvent_Rc_SubtypeError;
			eventSubtypeRc = viorc_openRejected;
		}

		/* If this is tty is already connected to a different
		   partition, fail */
		else if ((port_info[port].lp != HvLpIndexInvalid) &&
			 (port_info[port].lp != event->xSourceLp)) {
			eventRc = HvLpEvent_Rc_SubtypeError;
			eventSubtypeRc = viorc_openRejected;
		} else {
			port_info[port].lp = event->xSourceLp;
			eventRc = HvLpEvent_Rc_Good;
			eventSubtypeRc = viorc_good;
			sndMsgSeq[port] = sndMsgAck[port] = 0;
		}

		spin_unlock_irqrestore(&consolelock, flags);

		/* Return the acknowledgement */
		HvCallEvent_ackLpEvent(event);
	}
}

/* Handle a close open charLpEvent.  Could be either interrupt or ack
 */
static void vioHandleCloseEvent(struct HvLpEvent *event)
{
	unsigned long flags;
	struct viocharlpevent *cevent = (struct viocharlpevent *) event;
	u8 port = cevent->virtualDevice;

	if (event->xFlags.xFunction == HvLpEvent_Function_Int) {
		if (port >= VTTY_PORTS)
			return;

		/* For closes, just mark the console partition invalid */
		spin_lock_irqsave(&consolelock, flags);
		/* Got the lock, don't cause console output */

		if (port_info[port].lp == event->xSourceLp)
			port_info[port].lp = HvLpIndexInvalid;

		spin_unlock_irqrestore(&consolelock, flags);
		printk(KERN_INFO_VIO
		       "console close from %d\n", event->xSourceLp);
	} else {
		printk(KERN_WARNING_VIO
		       "console got unexpected close acknowlegement\n");
	}
}

/* Handle a config charLpEvent.  Could be either interrupt or ack
 */
static void vioHandleConfig(struct HvLpEvent *event)
{
	struct viocharlpevent *cevent = (struct viocharlpevent *) event;
	int len;

	len = cevent->immediateDataLen;
	HvCall_writeLogBuffer(cevent->immediateData,
			      cevent->immediateDataLen);

	if (cevent->immediateData[0] == 0x01) {
		printk(KERN_INFO_VIO
		       "console window resized to %d: %d: %d: %d\n",
		       cevent->immediateData[1],
		       cevent->immediateData[2],
		       cevent->immediateData[3], cevent->immediateData[4]);
	} else {
		printk(KERN_WARNING_VIO "console unknown config event\n");
	}
	return;
}

/* Handle a data charLpEvent. 
 */
static void vioHandleData(struct HvLpEvent *event)
{
	struct tty_struct *tty;
	struct viocharlpevent *cevent = (struct viocharlpevent *) event;
	struct port_info_tag *pi;
	int len;
	u8 port = cevent->virtualDevice;

	if (port >= VTTY_PORTS) {
		printk(KERN_WARNING_VIO
		       "console data on invalid virtual device %d\n",
		       port);
		return;
	}

	tty = port_info[port].tty;

	if (tty == NULL) {
		printk(KERN_WARNING_VIO
		       "no tty for virtual device %d\n", port);
		return;
	}

	if (tty->magic != TTY_MAGIC) {
		printk(KERN_WARNING_VIO "tty bad magic\n");
		return;
	}

	/*
	 * Just to be paranoid, make sure the tty points back to this port
	 */
	pi = (struct port_info_tag *) tty->driver_data;

	if (!pi || viotty_paranoia_check(pi, tty->device, "vioHandleData"))
		return;

	len = cevent->immediateDataLen;

	if (len == 0)
		return;

	/*
	 * Log port 0 data to the hypervisor log
	 */
	if (port == 0)
		HvCall_writeLogBuffer(cevent->immediateData,
				      cevent->immediateDataLen);

	/* Don't copy more bytes than there is room for in the buffer */
	if (tty->flip.count + len > TTY_FLIPBUF_SIZE) {
		len = TTY_FLIPBUF_SIZE - tty->flip.count;
		printk(KERN_WARNING_VIO
		       "console input buffer overflow!\n");
	}

	memcpy(tty->flip.char_buf_ptr, cevent->immediateData, len);
	memset(tty->flip.flag_buf_ptr, TTY_NORMAL, len);

	/* Update the kernel buffer end */
	tty->flip.count += len;
	tty->flip.char_buf_ptr += len;

	tty->flip.flag_buf_ptr += len;

	tty_flip_buffer_push(tty);
}

/* Handle an ack charLpEvent. 
 */
static void vioHandleAck(struct HvLpEvent *event)
{
	struct viocharlpevent *cevent = (struct viocharlpevent *) event;
	unsigned long flags;
	u8 port = cevent->virtualDevice;

	if (port >= VTTY_PORTS) {
		printk(KERN_WARNING_VIO
		       "viocons: data on invalid virtual device\n");
		return;
	}

	spin_lock_irqsave(&consolelock, flags);
	sndMsgAck[port] = event->xCorrelationToken;
	spin_unlock_irqrestore(&consolelock, flags);

	if (overflow[port].bufferUsed)
		sendBuffers(port, port_info[port].lp);
}

/* Handle charLpEvents and route to the appropriate routine
 */
static void vioHandleCharEvent(struct HvLpEvent *event)
{
	int charminor;

	if (event == NULL) {
		return;
	}
	charminor = event->xSubtype & VIOMINOR_SUBTYPE_MASK;
	switch (charminor) {
	case viocharopen:
		vioHandleOpenEvent(event);
		break;
	case viocharclose:
		vioHandleCloseEvent(event);
		break;
	case viochardata:
		vioHandleData(event);
		break;
	case viocharack:
		vioHandleAck(event);
		break;
	case viocharconfig:
		vioHandleConfig(event);
		break;
	default:
		if ((event->xFlags.xFunction == HvLpEvent_Function_Int) &&
		    (event->xFlags.xAckInd == HvLpEvent_AckInd_DoAck)) {
			event->xRc = HvLpEvent_Rc_InvalidSubtype;
			HvCallEvent_ackLpEvent(event);
		}
	}
}

/* Send an open event
 */
static int viocons_sendOpen(HvLpIndex remoteLp, u8 port, void *sem)
{
	return HvCallEvent_signalLpEventFast(remoteLp,
					     HvLpEvent_Type_VirtualIo,
					     viomajorsubtype_chario
					     | viocharopen,
					     HvLpEvent_AckInd_DoAck,
					     HvLpEvent_AckType_ImmediateAck,
					     viopath_sourceinst
					     (remoteLp),
					     viopath_targetinst
					     (remoteLp),
					     (u64) (unsigned long)
					     sem, VIOVERSION << 16,
					     ((u64) port << 48), 0, 0, 0);

}

int __init viocons_init2(void)
{
	DECLARE_MUTEX_LOCKED(Semaphore);
	int rc;

	/*
	 * Now open to the primary LP
	 */
	printk(KERN_INFO_VIO "console open path to primary\n");
	rc = viopath_open(HvLpConfig_getPrimaryLpIndex(), viomajorsubtype_chario, viochar_window + 2);	/* +2 for fudge */
	if (rc) {
		printk(KERN_WARNING_VIO
		       "console error opening to primary %d\n", rc);
	}

	if (viopath_hostLp == HvLpIndexInvalid) {
		vio_set_hostlp();
	}

	/*
	 * And if the primary is not the same as the hosting LP, open to the 
	 * hosting lp
	 */
	if ((viopath_hostLp != HvLpIndexInvalid) &&
	    (viopath_hostLp != HvLpConfig_getPrimaryLpIndex())) {
		printk(KERN_INFO_VIO
		       "console open path to hosting (%d)\n",
		       viopath_hostLp);
		rc = viopath_open(viopath_hostLp, viomajorsubtype_chario, viochar_window + 2);	/* +2 for fudge */
		if (rc) {
			printk(KERN_WARNING_VIO
			       "console error opening to partition %d: %d\n",
			       viopath_hostLp, rc);
		}
	}

	if (vio_setHandler(viomajorsubtype_chario, vioHandleCharEvent) < 0) {
		printk(KERN_WARNING_VIO
		       "Error seting handler for console events!\n");
	}

	printk(KERN_INFO_VIO "console major number is %d\n", TTY_MAJOR);

	/* First, try to open the console to the hosting lp.
	 * Wait on a semaphore for the response.
	 */
	if ((viopath_isactive(viopath_hostLp)) &&
	    (viocons_sendOpen(viopath_hostLp, 0, &Semaphore) == 0)) {
		printk(KERN_INFO_VIO
		       "opening console to hosting partition %d\n",
		       viopath_hostLp);
		down(&Semaphore);
	}

	/*
	 * If we don't have an active console, try the primary
	 */
	if ((!viopath_isactive(port_info[0].lp)) &&
	    (viopath_isactive(HvLpConfig_getPrimaryLpIndex())) &&
	    (viocons_sendOpen
	     (HvLpConfig_getPrimaryLpIndex(), 0, &Semaphore) == 0)) {
		printk(KERN_INFO_VIO
		       "opening console to primary partition\n");
		down(&Semaphore);
	}

	/* Initialize the tty_driver structure */
	memset(&viotty_driver, 0, sizeof(struct tty_driver));
	viotty_driver.magic = TTY_DRIVER_MAGIC;
	viotty_driver.driver_name = "vioconsole";
#if defined(CONFIG_DEVFS_FS)
	viotty_driver.name = "tty%d";
#else
	viotty_driver.name = "tty";
#endif
	viotty_driver.major = TTY_MAJOR;
	viotty_driver.minor_start = 1;
	viotty_driver.name_base = 1;
	viotty_driver.num = VTTY_PORTS;
	viotty_driver.type = TTY_DRIVER_TYPE_CONSOLE;
	viotty_driver.subtype = 1;
	viotty_driver.init_termios = tty_std_termios;
	viotty_driver.flags =
	    TTY_DRIVER_REAL_RAW | TTY_DRIVER_RESET_TERMIOS;
	viotty_driver.refcount = &viotty_refcount;
	viotty_driver.table = viotty_table;
	viotty_driver.termios = viotty_termios;
	viotty_driver.termios_locked = viotty_termios_locked;

	viotty_driver.open = viotty_open;
	viotty_driver.close = viotty_close;
	viotty_driver.write = viotty_write;
	viotty_driver.put_char = viotty_put_char;
	viotty_driver.flush_chars = viotty_flush_chars;
	viotty_driver.write_room = viotty_write_room;
	viotty_driver.chars_in_buffer = viotty_chars_in_buffer;
	viotty_driver.flush_buffer = viotty_flush_buffer;
	viotty_driver.ioctl = viotty_ioctl;
	viotty_driver.throttle = viotty_throttle;
	viotty_driver.unthrottle = viotty_unthrottle;
	viotty_driver.set_termios = viotty_set_termios;
	viotty_driver.stop = viotty_stop;
	viotty_driver.start = viotty_start;
	viotty_driver.hangup = viotty_hangup;
	viotty_driver.break_ctl = viotty_break;
	viotty_driver.send_xchar = viotty_send_xchar;
	viotty_driver.wait_until_sent = viotty_wait_until_sent;

	viottyS_driver = viotty_driver;
#if defined(CONFIG_DEVFS_FS)
	viottyS_driver.name = "ttyS%d";
#else
	viottyS_driver.name = "ttyS";
#endif
	viottyS_driver.major = TTY_MAJOR;
	viottyS_driver.minor_start = VIOTTY_SERIAL_START;
	viottyS_driver.type = TTY_DRIVER_TYPE_SERIAL;
	viottyS_driver.table = viottyS_table;
	viottyS_driver.termios = viottyS_termios;
	viottyS_driver.termios_locked = viottyS_termios_locked;

	if (tty_register_driver(&viotty_driver)) {
		printk(KERN_WARNING_VIO
		       "Couldn't register console driver\n");
	}

	if (tty_register_driver(&viottyS_driver)) {
		printk(KERN_WARNING_VIO
		       "Couldn't register console S driver\n");
	}
	/* Now create the vcs and vcsa devfs entries so mingetty works */
#if defined(CONFIG_DEVFS_FS)
	{
		struct tty_driver temp_driver = viotty_driver;
		int i;

		temp_driver.name = "vcs%d";
		for (i = 0; i < VTTY_PORTS; i++)
			tty_register_devfs(&temp_driver,
					   0, i + temp_driver.minor_start);

		temp_driver.name = "vcsa%d";
		for (i = 0; i < VTTY_PORTS; i++)
			tty_register_devfs(&temp_driver,
					   0, i + temp_driver.minor_start);

		// For compatibility with some earlier code only!
		// This will go away!!!
		temp_driver.name = "viocons/%d";
		temp_driver.name_base = 0;
		for (i = 0; i < VTTY_PORTS; i++)
			tty_register_devfs(&temp_driver,
					   0, i + temp_driver.minor_start);
	}
#endif

	/* 
	 * Create the proc entry
	 */
	iSeries_proc_callback(&viocons_proc_init);

	return 0;
}

void __init viocons_init(void)
{
	int i;
	printk(KERN_INFO_VIO "registering console\n");

	memset(&port_info, 0x00, sizeof(port_info));
	for (i = 0; i < VTTY_PORTS; i++) {
		sndMsgSeq[i] = sndMsgAck[i] = 0;
		port_info[i].port = i;
		port_info[i].lp = HvLpIndexInvalid;
		port_info[i].magic = VIOTTY_MAGIC;
	}

	register_console(&viocons);
	memset(overflow, 0x00, sizeof(overflow));
	debug = 0;

	HvCall_setLogBufferFormatAndCodepage(HvCall_LogBuffer_ASCII, 437);
}
