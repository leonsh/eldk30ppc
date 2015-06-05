/* -*- linux-c -*-
 *  drivers/cdrom/viocd.c
 *
 ***************************************************************************
 *  iSeries Virtual CD Rom
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
 *************************************************************************** 
 * This routine provides access to CD ROM drives owned and managed by an 
 * OS/400 partition running on the same box as this Linux partition.
 *
 * All operations are performed by sending messages back and forth to 
 * the OS/400 partition.  
 *
 * 
 * This device driver can either use it's own major number, or it can
 * pretend to be an AZTECH drive. This is controlled with a 
 * CONFIG option.  You can either call this an elegant solution to the 
 * fact that a lot of software doesn't recognize a new CD major number...
 * or you can call this a really ugly hack.  Your choice.
 *
 */

#include <linux/major.h>
#include <linux/config.h>

/***************************************************************************
 * Decide if we are using our own major or pretending to be an AZTECH drive
 ***************************************************************************/
#ifdef CONFIG_VIOCD_AZTECH
#define MAJOR_NR AZTECH_CDROM_MAJOR
#define do_viocd_request do_aztcd_request
#else
#define MAJOR_NR VIOCD_MAJOR
#endif

#define VIOCD_VERS "1.04"

#include <linux/blk.h>
#include <linux/cdrom.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/proc_fs.h>
#include <linux/module.h>

#include <asm/iSeries/HvTypes.h>
#include <asm/iSeries/HvLpEvent.h>
#include "vio.h"
#include <asm/iSeries/iSeries_proc.h>

extern struct pci_dev * iSeries_vio_dev;

#define signalLpEvent HvCallEvent_signalLpEventFast

struct viocdlpevent {
	struct HvLpEvent event;
	u32 mReserved1;
	u16 mVersion;
	u16 mSubTypeRc;
	u16 mDisk;
	u16 mFlags;
	u32 mToken;
	u64 mOffset;		// On open, the max number of disks
	u64 mLen;		// On open, the size of the disk
	u32 mBlockSize;		// Only set on open
	u32 mMediaSize;		// Only set on open
};

enum viocdsubtype {
	viocdopen = 0x0001,
	viocdclose = 0x0002,
	viocdread = 0x0003,
	viocdwrite = 0x0004,
	viocdlockdoor = 0x0005,
	viocdgetinfo = 0x0006,
	viocdcheck = 0x0007
};

/* Should probably make this a module parameter....sigh
 */
#define VIOCD_MAX_CD 8
int viocd_blocksizes[VIOCD_MAX_CD];
static u64 viocd_size_in_bytes[VIOCD_MAX_CD];

/* This is the structure we use to exchange info between driver and interrupt
 * handler
 */
struct viocd_waitevent {
	struct semaphore *sem;
	int rc;
	int changed;
};

/* this is a lookup table for the true capabilities of a device */
struct capability_entry {
	char *type;
	int capability;
};

static struct capability_entry capability_table[] = {
	{ "6330", CDC_LOCK | CDC_DVD_RAM },
	{ "6321", CDC_LOCK },
	{ "632B", 0 },
	{ NULL  , CDC_LOCK },
};

struct block_device_operations viocd_fops =
{
	owner:			THIS_MODULE,
	open:			cdrom_open,
	release:		cdrom_release,
	ioctl:			cdrom_ioctl,
	check_media_change:	cdrom_media_changed,
};

/* These are our internal structures for keeping track of devices
 */
static int viocd_numdev;

struct cdrom_info {
	char rsrcname[10];
	char type[4];
	char model[3];
};
static struct cdrom_info *viocd_unitinfo = NULL;

struct disk_info{
	u32 useCount;
	u32 blocksize;
	u32 mediasize;
};
static struct disk_info viocd_diskinfo[VIOCD_MAX_CD];

static struct cdrom_device_info viocd_info[VIOCD_MAX_CD];

static spinlock_t viocd_lock = SPIN_LOCK_UNLOCKED;

#define MAX_CD_REQ 1
static LIST_HEAD(reqlist);

/* End a request
 */
static int viocd_end_request(struct request *req, int uptodate)
{
	if (end_that_request_first(req, uptodate, DEVICE_NAME))
		return 0;
	end_that_request_last(req);
	return 1;
}


/* Get info on CD devices from OS/400
 */
static void get_viocd_info(void)
{
	dma_addr_t dmaaddr;
	HvLpEvent_Rc hvrc;
	int i;
	DECLARE_MUTEX_LOCKED(Semaphore);
	struct viocd_waitevent we;

	// If we don't have a host, bail out
	if (viopath_hostLp == HvLpIndexInvalid)
		return;

	if (viocd_unitinfo == NULL)
		viocd_unitinfo =
		    kmalloc(sizeof(struct cdrom_info) * VIOCD_MAX_CD,
			    GFP_KERNEL);

	memset(viocd_unitinfo, 0x00,
	       sizeof(struct cdrom_info) * VIOCD_MAX_CD);

	dmaaddr = pci_map_single(iSeries_vio_dev, viocd_unitinfo,
				 sizeof(struct cdrom_info) * VIOCD_MAX_CD,
				 PCI_DMA_FROMDEVICE);
	if (dmaaddr == 0xFFFFFFFF) {
		printk(KERN_WARNING_VIO "error allocating tce\n");
		return;
	}

	we.sem = &Semaphore;

	hvrc = signalLpEvent(viopath_hostLp,
			     HvLpEvent_Type_VirtualIo,
			     viomajorsubtype_cdio | viocdgetinfo,
			     HvLpEvent_AckInd_DoAck,
			     HvLpEvent_AckType_ImmediateAck,
			     viopath_sourceinst(viopath_hostLp),
			     viopath_targetinst(viopath_hostLp),
			     (u64) (unsigned long) &we,
			     VIOVERSION << 16,
			     dmaaddr,
			     0,
			     sizeof(struct cdrom_info) * VIOCD_MAX_CD,
			     0);
	if (hvrc != HvLpEvent_Rc_Good) {
		printk(KERN_WARNING_VIO "cdrom error sending event. rc %d\n", (int) hvrc);
		return;
	}

	down(&Semaphore);

	if (we.rc) {
		printk(KERN_WARNING_VIO "bad rc %d on getinfo\n", we.rc);
		return;
	}


	for (i = 0; (i < VIOCD_MAX_CD) && (viocd_unitinfo[i].rsrcname[0]); i++) {
		viocd_numdev++;
	}
}

/* Open a device
 */
static int viocd_open(struct cdrom_device_info *cdi, int purpose)
{
	DECLARE_MUTEX_LOCKED(Semaphore);
	int device_no = MINOR(cdi->dev);
	HvLpEvent_Rc hvrc;
	struct viocd_waitevent we;
	struct disk_info *diskinfo = &viocd_diskinfo[device_no];

	// If we don't have a host, bail out
	if (viopath_hostLp == HvLpIndexInvalid || device_no >= viocd_numdev)
		return -ENODEV;

	we.sem = &Semaphore;
	hvrc = signalLpEvent(viopath_hostLp,
			     HvLpEvent_Type_VirtualIo,
			     viomajorsubtype_cdio | viocdopen,
			     HvLpEvent_AckInd_DoAck,
			     HvLpEvent_AckType_ImmediateAck,
			     viopath_sourceinst(viopath_hostLp),
			     viopath_targetinst(viopath_hostLp),
			     (u64) (unsigned long) &we,
			     VIOVERSION << 16,
			     ((u64) device_no << 48),
			     0, 0, 0);
	if (hvrc != 0) {
		printk(KERN_WARNING_VIO "bad rc on signalLpEvent %d\n",
		       (int) hvrc);
		return -EIO;
	}

	down(&Semaphore);

	if (we.rc)
		return -EIO;

	if (diskinfo->useCount == 0) {
		if(diskinfo->blocksize > 0) {
			viocd_blocksizes[device_no] = diskinfo->blocksize;
			viocd_size_in_bytes[device_no] = diskinfo->blocksize * diskinfo->mediasize;
		} else {
			viocd_size_in_bytes[device_no] = 0xFFFFFFFFFFFFFFFF;
		}
	}
	MOD_INC_USE_COUNT;
	return 0;
}

/* Release a device
 */
static void viocd_release(struct cdrom_device_info *cdi)
{
	int device_no = MINOR(cdi->dev);
	HvLpEvent_Rc hvrc;

	/* If we don't have a host, bail out */
	if (viopath_hostLp == HvLpIndexInvalid
	    || device_no >= viocd_numdev)
		return;

	hvrc = signalLpEvent(viopath_hostLp,
			     HvLpEvent_Type_VirtualIo,
			     viomajorsubtype_cdio | viocdclose,
			     HvLpEvent_AckInd_NoAck,
			     HvLpEvent_AckType_ImmediateAck,
			     viopath_sourceinst(viopath_hostLp),
			     viopath_targetinst(viopath_hostLp),
			     0,
			     VIOVERSION << 16,
			     ((u64) device_no << 48),
			     0, 0, 0);
	if (hvrc != 0) {
		printk(KERN_WARNING_VIO "bad rc on signalLpEvent %d\n", (int) hvrc);
		return;
	}

	MOD_DEC_USE_COUNT;
}

/* Send a read or write request to OS/400
 */
static int send_request(struct request *req)
{
	HvLpEvent_Rc hvrc;
	dma_addr_t dmaaddr;
	int device_no = DEVICE_NR(req->rq_dev);
	u64 start = req->sector * 512,
	    len = req->current_nr_sectors * 512;
	char reading = req->cmd == READ;
	u16 command = reading ? viocdread : viocdwrite;


	if(start + len > viocd_size_in_bytes[device_no]) {
		printk(KERN_WARNING_VIO "viocd%d; access position %lx, past size %lx\n",
		       device_no, (unsigned long)(start + len), (unsigned long)viocd_size_in_bytes[device_no]);
		return -1;
	}
	
	dmaaddr = pci_map_single(iSeries_vio_dev, req->buffer, len,
				 reading ? PCI_DMA_FROMDEVICE : PCI_DMA_TODEVICE);
	if (dmaaddr == 0xFFFFFFFF) {
		printk(KERN_WARNING_VIO "error allocating tce for address %p len %ld\n",
			   req->buffer, (long)len);
		return -1;
	}

/* FIXME: experimental
// note the special use of mReserved1 to indicate that a response
//   is desired, but via NoAck...
//			     HvLpEvent_AckInd_NoAck,
//			     ((u64)0x00000001 << 32) | ((u64)VIOVERSION << 16),
*/

	hvrc = signalLpEvent(viopath_hostLp,
			     HvLpEvent_Type_VirtualIo,
			     viomajorsubtype_cdio | command,
			     HvLpEvent_AckInd_DoAck,
			     HvLpEvent_AckType_ImmediateAck,
			     viopath_sourceinst(viopath_hostLp),
			     viopath_targetinst(viopath_hostLp),
			     (u64) (unsigned long) req->buffer,
	                     VIOVERSION << 16,
			     ((u64) device_no << 48) | dmaaddr,
			     start, len, 0);
	if (hvrc != HvLpEvent_Rc_Good) {
		printk(KERN_WARNING_VIO "hv error on op %d\n", (int) hvrc);
		return -1;
	}

	return 0;
}


/* Do a request
 */
static int rwreq;
static void do_viocd_request(request_queue_t * q)
{
	for (;;) {
		struct request *req;
		char err_str[80] = "";
		int device_no;

		INIT_REQUEST;
		if (rwreq >= MAX_CD_REQ) {
			return;
		}

		device_no = CURRENT_DEV;

		/* remove the current request from the queue */
		req = CURRENT;
		blkdev_dequeue_request(req);

		/* check for any kind of error */
		if (device_no > viocd_numdev)
			sprintf(err_str, "Invalid device number %d", device_no);
		else if (send_request(req) < 0)
			strcpy(err_str, "unable to send message to OS/400!");

		/* if we had any sort of error, log it and cancel the request */
		if (*err_str) {
			printk(KERN_WARNING_VIO "%s\n", err_str);
			viocd_end_request(req, 0);
		} else {
			spin_lock(&viocd_lock);
			list_add_tail(&req->queue, &reqlist);
			++rwreq;
			spin_unlock(&viocd_lock);
		}
	}
}

/* Check if the CD changed
 */
static int viocd_media_changed(struct cdrom_device_info *cdi, int disc_nr)
{
	struct viocd_waitevent we;
	HvLpEvent_Rc hvrc;
	int device_no = MINOR(cdi->dev);

	/* This semaphore is raised in the interrupt handler                     */
	DECLARE_MUTEX_LOCKED(Semaphore);

	/* Check that we are dealing with a valid hosting partition              */
	if (viopath_hostLp == HvLpIndexInvalid) {
		printk(KERN_WARNING_VIO "Invalid hosting partition\n");
		return -EIO;
	}

	we.sem = &Semaphore;

	/* Send the open event to OS/400                                         */
	hvrc = signalLpEvent(viopath_hostLp,
			     HvLpEvent_Type_VirtualIo,
			     viomajorsubtype_cdio | viocdcheck,
			     HvLpEvent_AckInd_DoAck,
			     HvLpEvent_AckType_ImmediateAck,
			     viopath_sourceinst(viopath_hostLp),
			     viopath_targetinst(viopath_hostLp),
			     (u64) (unsigned long) &we,
			     VIOVERSION << 16,
			     ((u64) device_no << 48),
			     0, 0, 0);

	if (hvrc != 0) {
		printk(KERN_WARNING_VIO "bad rc on signalLpEvent %d\n", (int) hvrc);
		return -EIO;
	}

	/* Wait for the interrupt handler to get the response                    */
	down(&Semaphore);

	/* Check the return code.  If bad, assume no change                      */
	if (we.rc != 0) {
		printk(KERN_WARNING_VIO "bad rc on check_change. Assuming no change\n");
		return 0;
	}

	return we.changed;
}

static int viocd_lock_door(struct cdrom_device_info *cdi, int locking)
{
	HvLpEvent_Rc hvrc;
	u64 device_no = MINOR(cdi->dev);
	/* NOTE: flags is 1 or 0 so it won't overwrite the device_no             */
	u64 flags = !!locking;
	/* This semaphore is raised in the interrupt handler                     */
	DECLARE_MUTEX_LOCKED(Semaphore);
	struct viocd_waitevent we = { sem:&Semaphore };

	/* Check that we are dealing with a valid hosting partition              */
	if (viopath_hostLp == HvLpIndexInvalid) {
		printk(KERN_WARNING_VIO "Invalid hosting partition\n");
		return -EIO;
	}

	we.sem = &Semaphore;

	/* Send the lockdoor event to OS/400                                     */
	hvrc = signalLpEvent(viopath_hostLp,
			     HvLpEvent_Type_VirtualIo,
			     viomajorsubtype_cdio | viocdlockdoor,
			     HvLpEvent_AckInd_DoAck,
			     HvLpEvent_AckType_ImmediateAck,
			     viopath_sourceinst(viopath_hostLp),
			     viopath_targetinst(viopath_hostLp),
			     (u64) (unsigned long) &we,
			     VIOVERSION << 16,
			     (device_no << 48) | (flags << 32),
			     0, 0, 0);

	if (hvrc != 0) {
		printk(KERN_WARNING_VIO "bad rc on signalLpEvent %d\n", (int) hvrc);
		return -EIO;
	}

	/* Wait for the interrupt handler to get the response                    */
	down(&Semaphore);

	/* Check the return code.  If bad, assume no change                      */
	if (we.rc != 0) {
		return -EIO;
	}

	return 0;
}

/* This routine handles incoming CD LP events
 */
static void vioHandleCDEvent(struct HvLpEvent *event)
{
	struct viocdlpevent *bevent = (struct viocdlpevent *) event;
	struct viocd_waitevent *pwe;

	if (event == NULL) {
		/* Notification that a partition went away! */
		return;
	}
	/* First, we should NEVER get an int here...only acks */
	if (event->xFlags.xFunction == HvLpEvent_Function_Int) {
		printk(KERN_WARNING_VIO "Yikes! got an int in viocd event handler!\n");
		if (event->xFlags.xAckInd == HvLpEvent_AckInd_DoAck) {
			event->xRc = HvLpEvent_Rc_InvalidSubtype;
			HvCallEvent_ackLpEvent(event);
		}
	}

	switch (event->xSubtype & VIOMINOR_SUBTYPE_MASK) {
	case viocdopen:
		viocd_diskinfo[bevent->mDisk].blocksize = bevent->mBlockSize;
		viocd_diskinfo[bevent->mDisk].mediasize = bevent->mMediaSize;
		/* FALLTHROUGH !! */
	case viocdgetinfo:
	case viocdlockdoor:
		pwe = (struct viocd_waitevent *) (unsigned long) event->xCorrelationToken;
		pwe->rc = event->xRc;
		up(pwe->sem);
		break;

	case viocdclose:
		break;

	case viocdwrite:
	case viocdread:{
		unsigned long flags;
		int reading = ((event->xSubtype & VIOMINOR_SUBTYPE_MASK) == viocdread);
		struct request *req = blkdev_entry_to_request(reqlist.next);
		/* Since this is running in interrupt mode, we need to make sure we're not
		 * stepping on any global I/O operations
		 */
		spin_lock_irqsave(&io_request_lock, flags);

		pci_unmap_single(iSeries_vio_dev,
				 bevent->mToken,
				 bevent->mLen,
				 reading ? PCI_DMA_FROMDEVICE : PCI_DMA_TODEVICE);

		/* find the event to which this is a response */
		while ((&req->queue != &reqlist) &&
		       ((u64) (unsigned long) req->buffer != bevent->event.xCorrelationToken))
			req = blkdev_entry_to_request(req->queue.next);

		/* if the event was not there, then what are we responding to?? */
		if (&req->queue == &reqlist) {
			printk(KERN_WARNING_VIO "Yikes! we didn't ever enqueue this guy!\n");
			spin_unlock_irqrestore(&io_request_lock,
					       flags);
			break;
		}

		/* we don't need to keep it around anymore... */
		spin_lock(&viocd_lock);
		list_del(&req->queue);
		--rwreq;
		spin_unlock(&viocd_lock);
		{
			char stat = event->xRc == HvLpEvent_Rc_Good;
			int nsect = bevent->mLen >> 9;

			if (!stat)
			printk(KERN_WARNING_VIO
			       "request %p failed with rc %d:0x%08x\n",
			       req->buffer, event->xRc, bevent->mSubTypeRc);
			while ((nsect > 0) && (req->bh)) {
				nsect -= req->current_nr_sectors;
				viocd_end_request(req, stat);
			}
			/* we weren't done yet */
			if (req->bh) {
				if (send_request(req) < 0) {
					printk(KERN_WARNING_VIO
					    "couldn't re-submit req %p\n", req->buffer);
					viocd_end_request(req, 0);
				} else {
					spin_lock(&viocd_lock);
					list_add_tail(&req->queue, &reqlist);
					++rwreq;
					spin_unlock(&viocd_lock);
				}
			}
		}

		/* restart handling of incoming requests */
		do_viocd_request(NULL);
		spin_unlock_irqrestore(&io_request_lock, flags);
		break;
	}
	case viocdcheck:
		pwe = (struct viocd_waitevent *) (unsigned long) event->xCorrelationToken;
		pwe->rc = event->xRc;
		pwe->changed = bevent->mFlags;
		up(pwe->sem);
		break;

	default:
		printk(KERN_WARNING_VIO "invalid subtype!");
		if (event->xFlags.xAckInd == HvLpEvent_AckInd_DoAck) {
			event->xRc = HvLpEvent_Rc_InvalidSubtype;
			HvCallEvent_ackLpEvent(event);
		}
	}
}

/* Our file operations table
 */
static struct cdrom_device_ops viocd_dops = {
	open:viocd_open,
	release:viocd_release,
	media_changed:viocd_media_changed,
	lock_door:viocd_lock_door,
	capability:CDC_CLOSE_TRAY | CDC_OPEN_TRAY | CDC_LOCK | CDC_SELECT_SPEED | CDC_SELECT_DISC | CDC_MULTI_SESSION | CDC_MCN | CDC_MEDIA_CHANGED | CDC_PLAY_AUDIO | CDC_RESET | CDC_IOCTLS | CDC_DRIVE_STATUS | CDC_GENERIC_PACKET | CDC_CD_R | CDC_CD_RW | CDC_DVD | CDC_DVD_R | CDC_DVD_RAM
};

/* Handle reads from the proc file system
 */
static int proc_read(char *buf, char **start, off_t offset,
		     int blen, int *eof, void *data)
{
	int len = 0;
	int i;

	for (i = 0; i < viocd_numdev; i++) {
		len +=
		    sprintf(buf + len,
			    "viocd device %d is iSeries resource %10.10s type %4.4s, model %3.3s\n",
			    i, viocd_unitinfo[i].rsrcname,
			    viocd_unitinfo[i].type,
			    viocd_unitinfo[i].model);
	}
	*eof = 1;
	return len;
}


/* setup our proc file system entries
 */
void viocd_proc_init(struct proc_dir_entry *iSeries_proc)
{
	struct proc_dir_entry *ent;
	ent = create_proc_entry("viocd", S_IFREG | S_IRUSR, iSeries_proc);
	if (!ent)
		return;
	ent->nlink = 1;
	ent->data = NULL;
	ent->read_proc = proc_read;
}

/* clean up our proc file system entries
 */
void viocd_proc_delete(struct proc_dir_entry *iSeries_proc)
{
	remove_proc_entry("viocd", iSeries_proc);
}

static int find_capability(const char *type)
{
	struct capability_entry *entry;
	for(entry = capability_table; entry->type; ++entry)
		if(!strncmp(entry->type, type, 4))
			break;
	return entry->capability;
}

/* Initialize the whole device driver.  Handle module and non-module
 * versions
 */
__init int viocd_init(void)
{
	int i, rc;

	if (viopath_hostLp == HvLpIndexInvalid)
		vio_set_hostlp();

	/* If we don't have a host, bail out */
	if (viopath_hostLp == HvLpIndexInvalid)
		return -ENODEV;

	rc = viopath_open(viopath_hostLp, viomajorsubtype_cdio, MAX_CD_REQ+2);
	if (rc) {
		printk(KERN_WARNING_VIO "error opening path to host partition %d\n",
			   viopath_hostLp);
		return rc;
	}

	/* Initialize our request handler
	 */
	rwreq = 0;
	vio_setHandler(viomajorsubtype_cdio, vioHandleCDEvent);

	memset(&viocd_diskinfo, 0x00, sizeof(viocd_diskinfo));

	get_viocd_info();

	if (viocd_numdev == 0) {
		vio_clearHandler(viomajorsubtype_cdio);
		viopath_close(viopath_hostLp, viomajorsubtype_cdio, MAX_CD_REQ+2);
		return 0;
	}

	printk(KERN_INFO_VIO
	       "%s: iSeries Virtual CD vers %s, major %d, max disks %d, hosting partition %d\n",
	       DEVICE_NAME, VIOCD_VERS, MAJOR_NR, VIOCD_MAX_CD, viopath_hostLp);

	if (devfs_register_blkdev(MAJOR_NR, "viocd", &viocd_fops) != 0) {
		printk(KERN_WARNING_VIO "Unable to get major %d for viocd CD-ROM\n", MAJOR_NR);
		return -EIO;
	}

	blksize_size[MAJOR_NR] = viocd_blocksizes;
	blk_init_queue(BLK_DEFAULT_QUEUE(MAJOR_NR), DEVICE_REQUEST);
	read_ahead[MAJOR_NR] = 4;

	memset(&viocd_info, 0x00, sizeof(viocd_info));
	for (i = 0; i < viocd_numdev; i++) {
		viocd_info[i].dev = MKDEV(MAJOR_NR, i);
		viocd_info[i].ops = &viocd_dops;
		viocd_info[i].speed = 4;
		viocd_info[i].capacity = 1;
		viocd_info[i].mask = ~find_capability(viocd_unitinfo[i].type);
		sprintf(viocd_info[i].name, "viocd%d", i);
		if (register_cdrom(&viocd_info[i]) != 0) {
			printk(KERN_WARNING_VIO "Cannot register viocd CD-ROM %s!\n", viocd_info[i].name);
		} else {
			printk(KERN_INFO_VIO 
			       "cd %s is iSeries resource %10.10s type %4.4s, model %3.3s\n",
			       viocd_info[i].name,
			       viocd_unitinfo[i].rsrcname,
			       viocd_unitinfo[i].type,
			       viocd_unitinfo[i].model);
		}
	}

	/* 
	 * Create the proc entry
	 */
	iSeries_proc_callback(&viocd_proc_init);

	return 0;
}

#ifdef MODULE
void viocd_exit(void)
{
	int i;
	for (i = 0; i < viocd_numdev; i++) {
		if (unregister_cdrom(&viocd_info[i]) != 0) {
			printk(KERN_WARNING_VIO "Cannot unregister viocd CD-ROM %s!\n", viocd_info[i].name);
		}
	}
	if ((devfs_unregister_blkdev(MAJOR_NR, "viocd") == -EINVAL)) {
		printk(KERN_WARNING_VIO "can't unregister viocd\n");
		return;
	}
	blk_cleanup_queue(BLK_DEFAULT_QUEUE(MAJOR_NR));
	if (viocd_unitinfo)
		kfree(viocd_unitinfo);

	iSeries_proc_callback(&viocd_proc_delete);

	viopath_close(viopath_hostLp, viomajorsubtype_cdio, MAX_CD_REQ+2);
	vio_clearHandler(viomajorsubtype_cdio);
}
#endif

#ifdef MODULE
module_init(viocd_init);
module_exit(viocd_exit);
#endif
