/* -*- linux-c -*-
 *  arch/ppc64/viopath.c
 *
 *  iSeries Virtual I/O Message Path code
 *
 *  Authors: Dave Boutcher <boutcher@us.ibm.com>
 *           Ryan Arnold <ryanarn@us.ibm.com>
 *           Colin Devilbiss <devilbis@us.ibm.com>
 *
 * (C) Copyright 2000 IBM Corporation
 * 
 * This code is used by the iSeries virtual disk, cd,
 * tape, and console to communicate with OS/400 in another
 * partition.
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
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/pci.h>
#include <linux/wait.h>

#include <asm/iSeries/HvLpEvent.h>
#include <asm/iSeries/HvLpConfig.h>
#include <asm/iSeries/HvCallCfg.h>
#include <asm/iSeries/mf.h>
#include <asm/iSeries/iSeries_proc.h>

#include "vio.h"

EXPORT_SYMBOL(viopath_hostLp);
EXPORT_SYMBOL(vio_set_hostlp);
EXPORT_SYMBOL(viopath_open);
EXPORT_SYMBOL(viopath_close);
EXPORT_SYMBOL(viopath_isactive);
EXPORT_SYMBOL(viopath_sourceinst);
EXPORT_SYMBOL(viopath_targetinst);
EXPORT_SYMBOL(vio_setHandler);
EXPORT_SYMBOL(vio_clearHandler);
EXPORT_SYMBOL(vio_get_event_buffer);
EXPORT_SYMBOL(vio_free_event_buffer);

extern struct pci_dev * iSeries_vio_dev;

/* Status of the path to each other partition in the system.
 * This is overkill, since we will only ever establish connections
 * to our hosting partition and the primary partition on the system.
 * But this allows for other support in the future.
 */
static struct viopathStatus {
	int isOpen:1;		/* Did we open the path?            */
	int isActive:1;		/* Do we have a mon msg outstanding */
	int users[VIO_MAX_SUBTYPES];
	HvLpInstanceId mSourceInst;
	HvLpInstanceId mTargetInst;
	int numberAllocated;
} viopathStatus[HVMAXARCHITECTEDLPS];

static spinlock_t statuslock = SPIN_LOCK_UNLOCKED;

/*
 * For each kind of event we allocate a buffer that is
 * guaranteed not to cross a page boundary
 */
static void *event_buffer[VIO_MAX_SUBTYPES];
static atomic_t event_buffer_available[VIO_MAX_SUBTYPES];

static void handleMonitorEvent(struct HvLpEvent *event);

/* We use this structure to handle asynchronous responses.  The caller
 * blocks on the semaphore and the handler posts the semaphore.
 */
struct doneAllocParms_t {
	struct semaphore *sem;
	int number;
};

/* Put a sequence number in each mon msg.  The value is not
 * important.  Start at something other than 0 just for
 * readability.  wrapping this is ok.
 */
static u8 viomonseq = 22;

/* Our hosting logical partition.  We get this at startup
 * time, and different modules access this variable directly.
 */
HvLpIndex viopath_hostLp = 0xff;	/* HvLpIndexInvalid */

/* For each kind of incoming event we set a pointer to a
 * routine to call.
 */
static vio_event_handler_t *vio_handler[VIO_MAX_SUBTYPES];

/* Handle reads from the proc file system
 */
static int proc_read(char *buf, char **start, off_t offset,
		     int blen, int *eof, void *data)
{
	HvLpEvent_Rc hvrc;
	DECLARE_MUTEX_LOCKED(Semaphore);
	dma_addr_t dmaa =
	    pci_map_single(iSeries_vio_dev, buf, PAGE_SIZE, PCI_DMA_FROMDEVICE);
	int len = PAGE_SIZE;

	if (len > blen)
		len = blen;

	memset(buf, 0x00, len);
	hvrc = HvCallEvent_signalLpEventFast(viopath_hostLp,
					     HvLpEvent_Type_VirtualIo,
					     viomajorsubtype_config |
					     vioconfigget,
					     HvLpEvent_AckInd_DoAck,
					     HvLpEvent_AckType_ImmediateAck,
					     viopath_sourceinst
					     (viopath_hostLp),
					     viopath_targetinst
					     (viopath_hostLp),
					     (u64) (unsigned long)
					     &Semaphore, VIOVERSION << 16,
					     ((u64) dmaa) << 32, len, 0,
					     0);
	if (hvrc != HvLpEvent_Rc_Good) {
		printk("viopath hv error on op %d\n", (int) hvrc);
	}

	down(&Semaphore);

	pci_unmap_single(iSeries_vio_dev, dmaa, PAGE_SIZE, PCI_DMA_FROMDEVICE);

	*eof = 1;
	return strlen(buf);
}

/* Handle writes to our proc file system
 */
static int proc_write(struct file *file, const char *buffer,
		      unsigned long count, void *data)
{
	/* Doesn't do anything today!!!
	 */
	return count;
}

/* setup our proc file system entries
 */
static void vio_proc_init(struct proc_dir_entry *iSeries_proc)
{
	struct proc_dir_entry *ent;
	ent = create_proc_entry("config", S_IFREG | S_IRUSR, iSeries_proc);
	if (!ent)
		return;
	ent->nlink = 1;
	ent->data = NULL;
	ent->read_proc = proc_read;
	ent->write_proc = proc_write;
}

/* See if a given LP is active.  Allow for invalid lps to be passed in
 * and just return invalid
 */
int viopath_isactive(HvLpIndex lp)
{
	if (lp == HvLpIndexInvalid)
		return 0;
	if (lp < HVMAXARCHITECTEDLPS)
		return viopathStatus[lp].isActive;
	else
		return 0;
}

/* We cache the source and target instance ids for each
 * partition.  
 */
HvLpInstanceId viopath_sourceinst(HvLpIndex lp)
{
	return viopathStatus[lp].mSourceInst;
}

HvLpInstanceId viopath_targetinst(HvLpIndex lp)
{
	return viopathStatus[lp].mTargetInst;
}

/* Send a monitor message.  This is a message with the acknowledge
 * bit on that the other side will NOT explicitly acknowledge.  When
 * the other side goes down, the hypervisor will acknowledge any
 * outstanding messages....so we will know when the other side dies.
 */
static void sendMonMsg(HvLpIndex remoteLp)
{
	HvLpEvent_Rc hvrc;

	viopathStatus[remoteLp].mSourceInst =
	    HvCallEvent_getSourceLpInstanceId(remoteLp,
					      HvLpEvent_Type_VirtualIo);
	viopathStatus[remoteLp].mTargetInst =
	    HvCallEvent_getTargetLpInstanceId(remoteLp,
					      HvLpEvent_Type_VirtualIo);

	/* Deliberately ignore the return code here.  if we call this
	 * more than once, we don't care.
	 */
	vio_setHandler(viomajorsubtype_monitor, handleMonitorEvent);

	hvrc = HvCallEvent_signalLpEventFast(remoteLp,
					     HvLpEvent_Type_VirtualIo,
					     viomajorsubtype_monitor,
					     HvLpEvent_AckInd_DoAck,
					     HvLpEvent_AckType_DeferredAck,
					     viopathStatus[remoteLp].
					     mSourceInst,
					     viopathStatus[remoteLp].
					     mTargetInst, viomonseq++,
					     0, 0, 0, 0, 0);

	if (hvrc == HvLpEvent_Rc_Good) {
		viopathStatus[remoteLp].isActive = 1;
	} else {
		printk(KERN_WARNING_VIO
		       "could not connect ot partition %d\n", remoteLp);
		viopathStatus[remoteLp].isActive = 0;
	}
}

static void handleMonitorEvent(struct HvLpEvent *event)
{
	HvLpIndex remoteLp;
	int i;

	/* First see if this is just a normal monitor message from the
	 * other partition
	 */
	if (event->xFlags.xFunction == HvLpEvent_Function_Int) {
		remoteLp = event->xSourceLp;
		if (!viopathStatus[remoteLp].isActive)
			sendMonMsg(remoteLp);
		return;
	}

	/* This path is for an acknowledgement; the other partition
	 * died
	 */
	remoteLp = event->xTargetLp;
	if ((event->xSourceInstanceId !=
	     viopathStatus[remoteLp].mSourceInst)
	    || (event->xTargetInstanceId !=
		viopathStatus[remoteLp].mTargetInst)) {
		printk(KERN_WARNING_VIO
		       "ignoring ack....mismatched instances\n");
		return;
	}

	printk(KERN_WARNING_VIO "partition %d ended\n", remoteLp);

	viopathStatus[remoteLp].isActive = 0;

	/* For each active handler, pass them a NULL
	 * message to indicate that the other partition
	 * died
	 */
	for (i = 0; i < VIO_MAX_SUBTYPES; i++) {
		if (vio_handler[i] != NULL)
			(*vio_handler[i]) (NULL);
	}
}

int vio_setHandler(int subtype, vio_event_handler_t * beh)
{
	subtype = subtype >> VIOMAJOR_SUBTYPE_SHIFT;

	if ((subtype < 0) || (subtype >= VIO_MAX_SUBTYPES))
		return -EINVAL;

	if (vio_handler[subtype] != NULL)
		return -EBUSY;

	vio_handler[subtype] = beh;
	return 0;
}

int vio_clearHandler(int subtype)
{
	subtype = subtype >> VIOMAJOR_SUBTYPE_SHIFT;

	if ((subtype < 0) || (subtype >= VIO_MAX_SUBTYPES))
		return -EINVAL;

	if (vio_handler[subtype] == NULL)
		return -EAGAIN;

	vio_handler[subtype] = NULL;
	return 0;
}

static void handleConfig(struct HvLpEvent *event)
{
	if (event->xFlags.xFunction == HvLpEvent_Function_Int) {
		printk(KERN_WARNING_VIO
		       "unexpected config request from partition %d",
		       event->xSourceLp);

		if ((event->xFlags.xFunction == HvLpEvent_Function_Int) &&
		    (event->xFlags.xAckInd == HvLpEvent_AckInd_DoAck)) {
			event->xRc = HvLpEvent_Rc_InvalidSubtype;
			HvCallEvent_ackLpEvent(event);
		}
		return;
	}

	up((struct semaphore *)(unsigned long) event->xCorrelationToken);
}

/* Initialization of the hosting partition
 */
void vio_set_hostlp(void)
{
	/* If this has already been set then we DON'T want to either change
	 * it or re-register the proc file system
	 */
	if (viopath_hostLp != HvLpIndexInvalid)
		return;

	/* Figure out our hosting partition.  This isn't allowed to change
	 * while we're active
	 */
	viopath_hostLp =
	    HvCallCfg_getHostingLpIndex(HvLpConfig_getLpIndex());

	/* If we have a valid hosting LP, create a proc file system entry
	 * for config information
	 */
	if (viopath_hostLp != HvLpIndexInvalid) {
		iSeries_proc_callback(&vio_proc_init);
		vio_setHandler(viomajorsubtype_config, handleConfig);
	}
}

static void vio_handleEvent(struct HvLpEvent *event, struct pt_regs *regs)
{
	HvLpIndex remoteLp;
	int subtype =
	    (event->
	     xSubtype & VIOMAJOR_SUBTYPE_MASK) >> VIOMAJOR_SUBTYPE_SHIFT;

	if (event->xFlags.xFunction == HvLpEvent_Function_Int) {
		remoteLp = event->xSourceLp;
		if (event->xSourceInstanceId !=
		    viopathStatus[remoteLp].mTargetInst) {
			printk(KERN_WARNING_VIO
			       "message from invalid partition. "
			       "int msg rcvd, source inst (%d) doesnt match (%d)\n",
			       viopathStatus[remoteLp].mTargetInst,
			       event->xSourceInstanceId);
			return;
		}

		if (event->xTargetInstanceId !=
		    viopathStatus[remoteLp].mSourceInst) {
			printk(KERN_WARNING_VIO
			       "message from invalid partition. "
			       "int msg rcvd, target inst (%d) doesnt match (%d)\n",
			       viopathStatus[remoteLp].mSourceInst,
			       event->xTargetInstanceId);
			return;
		}
	} else {
		remoteLp = event->xTargetLp;
		if (event->xSourceInstanceId !=
		    viopathStatus[remoteLp].mSourceInst) {
			printk(KERN_WARNING_VIO
			       "message from invalid partition. "
			       "ack msg rcvd, source inst (%d) doesnt match (%d)\n",
			       viopathStatus[remoteLp].mSourceInst,
			       event->xSourceInstanceId);
			return;
		}

		if (event->xTargetInstanceId !=
		    viopathStatus[remoteLp].mTargetInst) {
			printk(KERN_WARNING_VIO
			       "message from invalid partition. "
			       "viopath: ack msg rcvd, target inst (%d) doesnt match (%d)\n",
			       viopathStatus[remoteLp].mTargetInst,
			       event->xTargetInstanceId);
			return;
		}
	}

	if (vio_handler[subtype] == NULL) {
		printk(KERN_WARNING_VIO
		       "unexpected virtual io event subtype %d from partition %d\n",
		       event->xSubtype, remoteLp);
		/* No handler.  Ack if necessary
		 */
		if ((event->xFlags.xFunction == HvLpEvent_Function_Int) &&
		    (event->xFlags.xAckInd == HvLpEvent_AckInd_DoAck)) {
			event->xRc = HvLpEvent_Rc_InvalidSubtype;
			HvCallEvent_ackLpEvent(event);
		}
		return;
	}

	/* This inocuous little line is where all the real work happens
	 */
	(*vio_handler[subtype]) (event);
}

static void viopath_donealloc(void *parm, int number)
{
	struct doneAllocParms_t *doneAllocParmsp =
	    (struct doneAllocParms_t *) parm;
	doneAllocParmsp->number = number;
	up(doneAllocParmsp->sem);
}

static int allocateEvents(HvLpIndex remoteLp, int numEvents)
{
	struct doneAllocParms_t doneAllocParms;
	DECLARE_MUTEX_LOCKED(Semaphore);
	doneAllocParms.sem = &Semaphore;

	mf_allocateLpEvents(remoteLp, HvLpEvent_Type_VirtualIo, 250,	/* It would be nice to put a real number here! */
			    numEvents,
			    &viopath_donealloc, &doneAllocParms);

	down(&Semaphore);

	return doneAllocParms.number;
}

int viopath_open(HvLpIndex remoteLp, int subtype, int numReq)
{
	int i;
	unsigned long flags;

	if ((remoteLp >= HvMaxArchitectedLps)
	    || (remoteLp == HvLpIndexInvalid))
		return -EINVAL;

	subtype = subtype >> VIOMAJOR_SUBTYPE_SHIFT;
	if ((subtype < 0) || (subtype >= VIO_MAX_SUBTYPES))
		return -EINVAL;

	spin_lock_irqsave(&statuslock, flags);

	/* OK...we can fit 4 maximum-sized events (256 bytes) in
	 * each page (4096).  Get a new page every 4
	 */
	if (event_buffer[0] == NULL) {
		for (i = 0; i < VIO_MAX_SUBTYPES; i++) {
			if ((i % 4) == 0) {
				event_buffer[i] =
				    (void *) get_free_page(GFP_KERNEL);
				if (event_buffer[i] == NULL) {
					spin_unlock_irqrestore(&statuslock, flags);
					return -ENOMEM;
				}
			} else {
				event_buffer[i] =
				    event_buffer[i - 1] + 256;
			}
			atomic_set(&event_buffer_available[i], 1);
		}
	}

	viopathStatus[remoteLp].users[subtype]++;

	if (!viopathStatus[remoteLp].isOpen) {
		HvCallEvent_openLpEventPath(remoteLp,
					    HvLpEvent_Type_VirtualIo);

		viopathStatus[remoteLp].numberAllocated +=
		    allocateEvents(remoteLp, 1);

		if (viopathStatus[remoteLp].numberAllocated == 0) {
			HvCallEvent_closeLpEventPath(remoteLp,
						     HvLpEvent_Type_VirtualIo);
			
			spin_unlock_irqrestore(&statuslock, flags);
			return -ENOMEM;
		}

		viopathStatus[remoteLp].mSourceInst =
		    HvCallEvent_getSourceLpInstanceId(remoteLp,
						      HvLpEvent_Type_VirtualIo);
		viopathStatus[remoteLp].mTargetInst =
		    HvCallEvent_getTargetLpInstanceId(remoteLp,
						      HvLpEvent_Type_VirtualIo);

		HvLpEvent_registerHandler(HvLpEvent_Type_VirtualIo,
					  &vio_handleEvent);

		viopathStatus[remoteLp].isOpen = 1;

		sendMonMsg(remoteLp);

		printk(KERN_INFO_VIO
		       "Opening connection to partition %d, setting sinst %d, tinst %d\n",
		       remoteLp,
		       viopathStatus[remoteLp].mSourceInst,
		       viopathStatus[remoteLp].mTargetInst);
	}

	viopathStatus[remoteLp].numberAllocated +=
	    allocateEvents(remoteLp, numReq);
	spin_unlock_irqrestore(&statuslock, flags);

	return 0;
}

int viopath_close(HvLpIndex remoteLp, int subtype, int numReq)
{
	unsigned long flags;
	int i;
	int numOpen;
	struct doneAllocParms_t doneAllocParms;
	DECLARE_MUTEX_LOCKED(Semaphore);
	doneAllocParms.sem = &Semaphore;

	if ((remoteLp >= HvMaxArchitectedLps)
	    || (remoteLp == HvLpIndexInvalid))
		return -EINVAL;

	subtype = subtype >> VIOMAJOR_SUBTYPE_SHIFT;
	if ((subtype < 0) || (subtype >= VIO_MAX_SUBTYPES))
		return -EINVAL;

	spin_lock_irqsave(&statuslock, flags);

	viopathStatus[remoteLp].users[subtype]--;
	
	mf_deallocateLpEvents( remoteLp,HvLpEvent_Type_VirtualIo,
			       numReq,
			       &viopath_donealloc,
			       &doneAllocParms );
	down(&Semaphore);

	for (i = 0, numOpen = 0; i < VIO_MAX_SUBTYPES; i++) {
		numOpen += viopathStatus[remoteLp].users[i];
	}
	
	if ((viopathStatus[remoteLp].isOpen) && (numOpen == 0)) {
		printk(KERN_INFO_VIO
		       "Closing connection to partition %d", remoteLp);

		HvCallEvent_closeLpEventPath(remoteLp,
					     HvLpEvent_Type_VirtualIo);
		viopathStatus[remoteLp].isOpen = 0;
		viopathStatus[remoteLp].isActive = 0;

		for (i = 0; i < VIO_MAX_SUBTYPES; i++) {
			atomic_set(&event_buffer_available[i], 0);
			
			for (i = 0; i < VIO_MAX_SUBTYPES; i += 4) {
				free_page((unsigned long) event_buffer[i]);
			}
		}

	}
	spin_unlock_irqrestore(&statuslock, flags);
	return 0;
}

void *vio_get_event_buffer(int subtype)
{
	subtype = subtype >> VIOMAJOR_SUBTYPE_SHIFT;
	if ((subtype < 0) || (subtype >= VIO_MAX_SUBTYPES))
		return NULL;

	if (atomic_dec_if_positive(&event_buffer_available[subtype]) == 0)
		return event_buffer[subtype];
	else
		return NULL;
}

void vio_free_event_buffer(int subtype, void *buffer)
{
	subtype = subtype >> VIOMAJOR_SUBTYPE_SHIFT;
	if ((subtype < 0) || (subtype >= VIO_MAX_SUBTYPES)) {
		printk(KERN_WARNING_VIO
		       "unexpected subtype %d freeing event buffer\n",
		       subtype);
		return;
	}

	if (atomic_read(&event_buffer_available[subtype]) != 0) {
		printk(KERN_WARNING_VIO
		       "freeing unallocated event buffer, subtype %d\n",
		       subtype);
		return;
	}

	if (buffer != event_buffer[subtype]) {
		printk(KERN_WARNING_VIO
		       "freeing invalid event buffer, subtype %d\n",
		       subtype);
	}

	atomic_set(&event_buffer_available[subtype], 1);
}
