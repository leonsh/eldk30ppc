/* -*- linux-c -*-
 * viodasd.c
 *  Authors: Dave Boutcher <boutcher@us.ibm.com>
 *           Ryan Arnold <ryanarn@us.ibm.com>
 *           Colin Devilbiss <devilbis@us.ibm.com>
 *
 * (C) Copyright 2000 IBM Corporation
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA 
 ***************************************************************************
 * This routine provides access to disk space (termed "DASD" in historical
 * IBM terms) owned and managed by an OS/400 partition running on the
 * same box as this Linux partition.
 *
 * All disk operations are performed by sending messages back and forth to 
 * the OS/400 partition. 
 * 
 * This device driver can either use it's own major number, or it can
 * pretend to be an IDE drive (Major #3).  Currently it doesn't
 * emulate all the other IDE majors.  This is controlled with a 
 * CONFIG option.  You can either call this an elegant solution to the 
 * fact that a lot of software doesn't recognize a new disk major number...
 * or you can call this a really ugly hack.  Your choice.
 */

#include <linux/major.h>
#include <linux/config.h>

/* Decide if we are using our own major or pretending to be an IDE drive
 *
 * If we are using our own majors, we only support 3 partitions per physical
 * disk....so with minor numbers 0-255 we get a maximum of 64 disks.  If we
 * are emulating IDE, we get 16 partitions per disk, with a maximum of 16
 * disks
 */
#ifdef CONFIG_VIODASD_IDE
#define MAJOR_NR IDE0_MAJOR
#define PARTITION_SHIFT 6
#define do_viodasd_request do_hd_request
static int numdsk = 16;
static int viodasd_max_disk = 16;
#define VIOD_DEVICE_NAME "hd"
#define VIOD_GENHD_NAME "hd"
#else
#define MAJOR_NR VIODASD_MAJOR
#define PARTITION_SHIFT 3
static int numdsk = 32;
static int viodasd_max_disk = 32;
#define VIOD_DEVICE_NAME "viod"
#ifdef CONFIG_DEVFS_FS
#define VIOD_GENHD_NAME "viod"
#else
#define VIOD_GENHD_NAME "iSeries/vd"
#endif				/* CONFIG_DEVFS */
#endif				/* CONFIG_VIODASD_IDE */

#define VIODASD_VERS "1.02"
#define LOCAL_END_REQUEST

#include <linux/sched.h>
#include <linux/timer.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/blk.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/fd.h>
#include <linux/proc_fs.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/pci.h>

#include <asm/iSeries/HvTypes.h>
#include <asm/iSeries/HvLpEvent.h>
#include <asm/iSeries/HvLpConfig.h>
#include "vio.h"
#include <asm/iSeries/iSeries_proc.h>

MODULE_DESCRIPTION("iSeries Virtual DASD");
MODULE_AUTHOR("Dave Boutcher");

#define VIOMAXREQ 16
#define VIOMAXBLOCKDMA        12

extern struct pci_dev * iSeries_vio_dev;

struct vioblocklpevent {
	struct HvLpEvent event;
	u32 mReserved1;
	u16 mVersion;
	u16 mSubTypeRc;
	u16 mDisk;
	u16 mFlags;
	union {
		struct {	// Used during open
			u64 mDiskLen;
			u16 mMaxDisks;
			u16 mCylinders;
			u16 mTracks;
			u16 mSectors;
			u16 mBytesPerSector;
		} openData;
		struct {	// Used during rw
			u64 mOffset;
			struct {
				u32 mToken;
				u32 reserved;
				u64 mLen;
			} dmaInfo[VIOMAXBLOCKDMA];
		} rwData;

		struct {
			u64 changed;
		} check;
	} u;
};

#define vioblockflags_ro   0x0001

enum vioblocksubtype {
	vioblockopen = 0x0001,
	vioblockclose = 0x0002,
	vioblockread = 0x0003,
	vioblockwrite = 0x0004,
	vioblockflush = 0x0005,
	vioblockcheck = 0x0007
};

/* In a perfect world we will perform better if we get page-aligned I/O
 * requests, in multiples of pages.  At least peg our block size fo the
 * actual page size.
 */
static int blksize = HVPAGESIZE;	/* in bytes */

static DECLARE_WAIT_QUEUE_HEAD(viodasd_wait);
struct viodasd_waitevent {
	struct semaphore *sem;
	int rc;
	int changed;		/* Used only for check_change */
};

/* All our disk-related global structures
 */
static struct hd_struct *viodasd_partitions;
static int *viodasd_sizes;
static int *viodasd_sectsizes;
static int *viodasd_maxsectors;
extern struct gendisk viodasd_gendsk;

/* Figure out the biggest I/O request (in sectors) we can accept
 */
#define VIODASD_MAXSECTORS (4096 / 512 * VIOMAXBLOCKDMA)

/* Keep some statistics on what's happening for the PROC file system
 */
static struct {
	long tot;
	long nobh;
	long ntce[VIOMAXBLOCKDMA];
} viod_stats[64][2];

/* Number of disk I/O requests we've sent to OS/400
 */
static int numReqOut;

/* This is our internal structure for keeping track of disk devices
 */
struct viodasd_device {
	int useCount;
	u16 cylinders;
	u16 tracks;
	u16 sectors;
	u16 bytesPerSector;
	u64 size;
	int readOnly;
} *viodasd_devices;

/* When we get a disk I/O request we take it off the general request queue
 * and put it here.
 */
static LIST_HEAD(reqlist);

/* Handle reads from the proc file system
 */
static int proc_read(char *buf, char **start, off_t offset,
		     int blen, int *eof, void *data)
{
	int len = 0;
	int i;
	int j;

#if defined(MODULE)
	len +=
	    sprintf(buf + len,
		    "viod Module opened %d times.  Major number %d\n",
		    MOD_IN_USE, MAJOR_NR);
#endif
	len += sprintf(buf + len, "viod %d devices\n", numdsk);

	for (i = 0; i < 16; i++) {
		if (viod_stats[i][0].tot || viod_stats[i][1].tot) {
			len +=
			    sprintf(buf + len,
				    "DISK %2.2d: rd %-10.10ld wr %-10.10ld (no buffer list rd %-10.10ld wr %-10.10ld\n",
				    i, viod_stats[i][0].tot,
				    viod_stats[i][1].tot,
				    viod_stats[i][0].nobh,
				    viod_stats[i][1].nobh);

			len += sprintf(buf + len, "rd DMA: ");

			for (j = 0; j < VIOMAXBLOCKDMA; j++)
				len += sprintf(buf + len, " [%2.2d] %ld",
					       j,
					       viod_stats[i][0].ntce[j]);

			len += sprintf(buf + len, "\nwr DMA: ");

			for (j = 0; j < VIOMAXBLOCKDMA; j++)
				len += sprintf(buf + len, " [%2.2d] %ld",
					       j,
					       viod_stats[i][1].ntce[j]);
			len += sprintf(buf + len, "\n");
		}
	}

	*eof = 1;
	return len;
}

/* Handle writes to our proc file system
 */
static int proc_write(struct file *file, const char *buffer,
		      unsigned long count, void *data)
{
	return count;
}

/* setup our proc file system entries
 */
void viodasd_proc_init(struct proc_dir_entry *iSeries_proc)
{
	struct proc_dir_entry *ent;
	ent =
	    create_proc_entry("viodasd", S_IFREG | S_IRUSR, iSeries_proc);
	if (!ent)
		return;
	ent->nlink = 1;
	ent->data = NULL;
	ent->read_proc = proc_read;
	ent->write_proc = proc_write;
}

/* clean up our proc file system entries
 */
void viodasd_proc_delete(struct proc_dir_entry *iSeries_proc)
{
	remove_proc_entry("viodasd", iSeries_proc);
}

/* End a request
 */
static void viodasd_end_request(struct request *req, int uptodate)
{

	if (end_that_request_first(req, uptodate, VIOD_DEVICE_NAME))
		return;

	end_that_request_last(req);
}

/* This rebuilds the partition information for a single disk device
 */
static int viodasd_revalidate(kdev_t dev)
{
	int i;
	int device_no = DEVICE_NR(dev);
	int part0 = (device_no << PARTITION_SHIFT);
	int npart = (1 << PARTITION_SHIFT);
	int minor;
	kdev_t devp;
	struct super_block *sb;

	if (viodasd_devices[device_no].size == 0)
		return 0;

	for (i = npart - 1; i >= 0; i--) {
		minor = part0 + i;

		if (viodasd_partitions[minor].nr_sects != 0) {
			devp = MKDEV(MAJOR_NR, minor);
			fsync_dev(devp);

			sb = get_super(devp);
			if (sb)
				invalidate_inodes(sb);

			invalidate_buffers(devp);
		}

		viodasd_partitions[minor].start_sect = 0;
		viodasd_partitions[minor].nr_sects = 0;
	}

	grok_partitions(&viodasd_gendsk, device_no, npart,
			viodasd_devices[device_no].size >> 9);

	return 0;
}

/* This is the actual open code.  It gets called from the external
 * open entry point, as well as from the init code when we're figuring
 * out what disks we have
 */
static int internal_open(int device_no)
{
	int i;
	struct viodasd_waitevent we;

	HvLpEvent_Rc hvrc;
	/* This semaphore is raised in the interrupt handler                     */
	DECLARE_MUTEX_LOCKED(Semaphore);

	/* Check that we are dealing with a valid hosting partition              */
	if (viopath_hostLp == HvLpIndexInvalid) {
		printk(KERN_WARNING_VIO "Invalid hosting partition\n");
		return -EIO;
	}

	we.sem = &Semaphore;

	/* Send the open event to OS/400                                         */
	hvrc = HvCallEvent_signalLpEventFast(viopath_hostLp,
					     HvLpEvent_Type_VirtualIo,
					     viomajorsubtype_blockio |
					     vioblockopen,
					     HvLpEvent_AckInd_DoAck,
					     HvLpEvent_AckType_ImmediateAck,
					     viopath_sourceinst
					     (viopath_hostLp),
					     viopath_targetinst
					     (viopath_hostLp),
					     (u64) (unsigned long) &we,
					     VIOVERSION << 16,
					     ((u64) device_no << 48), 0, 0,
					     0);

	if (hvrc != 0) {
		printk(KERN_WARNING_VIO "bad rc on signalLpEvent %d\n", (int) hvrc);
		return -EIO;
	}

	/* Wait for the interrupt handler to get the response                    */
	down(&Semaphore);

	/* Check the return code                                                 */
	if (we.rc != 0) {
		printk(KERN_WARNING_VIO "bad rc opening disk: %d\n", (int) we.rc);
		return we.rc;
	}

	/* If this is the first open of this device, update the device information */
	/* If this is NOT the first open, assume that it isn't changing            */
	if (viodasd_devices[device_no].useCount == 0) {
		if (viodasd_devices[device_no].size > 0) {
			/* divide by 512 */
			u64 tmpint = viodasd_devices[device_no].size >> 9;
			viodasd_partitions[device_no << PARTITION_SHIFT].
			    nr_sects = tmpint;
			/* Now the value divided by 1024 */
			tmpint = tmpint >> 1;
			viodasd_sizes[device_no << PARTITION_SHIFT] =
			    tmpint;

			for (i = (device_no << PARTITION_SHIFT);
			     i < ((device_no + 1) << PARTITION_SHIFT); i++)
				viodasd_sectsizes[i] =
				    viodasd_devices[device_no].
				    bytesPerSector;

		}
	} else {
		/* If the size of the device changed, wierd things are happening!     */
		if (viodasd_sizes[device_no << PARTITION_SHIFT] !=
		    viodasd_devices[device_no].size >> 10) {
			printk(KERN_WARNING_VIO 
			       "disk size change (%dK to %dK) for device %d\n",
			       viodasd_sizes[device_no << PARTITION_SHIFT],
			       (int) viodasd_devices[device_no].size >> 10,
			       device_no);
		}
	}

	/* Bump the use count                                                      */
	viodasd_devices[device_no].useCount++;

	return 0;
}

/* This is the actual release code.  It gets called from the external
 * release entry point, as well as from the init code when we're figuring
 * out what disks we have
 */
static int internal_release(int device_no)
{
	/* Send the event to OS/400.  We DON'T expect a response                 */
	HvLpEvent_Rc hvrc = HvCallEvent_signalLpEventFast(viopath_hostLp,
							  HvLpEvent_Type_VirtualIo,
							  viomajorsubtype_blockio
							  | vioblockclose,
							  HvLpEvent_AckInd_NoAck,
							  HvLpEvent_AckType_ImmediateAck,
							  viopath_sourceinst
							  (viopath_hostLp),
							  viopath_targetinst
							  (viopath_hostLp),
							  0,
							  VIOVERSION << 16,
							  ((u64) device_no
							   << 48),
							  0, 0, 0);

	viodasd_devices[device_no].useCount--;

	if (hvrc != 0) {
		printk(KERN_WARNING_VIO "bad rc sending event to OS/400 %d\n", (int) hvrc);
		return -EIO;
	}
	return 0;
}

/* External open entry point.
 */
static int viodasd_open(struct inode *ino, struct file *fil)
{
	int device_no;

	/* Do a bunch of sanity checks                                           */
	if (!ino) {
		printk(KERN_WARNING_VIO "no inode provided in open\n");
		return -ENODEV;
	}

	if (MAJOR(ino->i_rdev) != MAJOR_NR) {
		printk(KERN_WARNING_VIO "Wierd error...wrong major number on open\n");
		return -ENODEV;
	}

	device_no = DEVICE_NR(ino->i_rdev);
	if (device_no > numdsk) {
		printk(KERN_WARNING_VIO "Invalid minor device number %d in open\n",
			   device_no);
		return -ENODEV;
	}

	/* Call the actual open code                                             */
	if (internal_open(device_no) == 0) {
		if (fil && fil->f_mode) {
			if (fil->f_mode & 2) {
				if (viodasd_devices[device_no].readOnly) {
					internal_release(device_no);
					return -EROFS;
				}
			}
		}
		MOD_INC_USE_COUNT;
		return 0;
	} else {
		return -EIO;
	}
}

/* External release entry point.
 */
static int viodasd_release(struct inode *ino, struct file *fil)
{
	int device_no;

	/* Do a bunch of sanity checks                                           */
	if (!ino) {
		printk(KERN_WARNING_VIO "no inode provided in release\n");
		return -ENODEV;
	}

	if (MAJOR(ino->i_rdev) != MAJOR_NR) {
		printk(KERN_WARNING_VIO 
		       "Wierd error...wrong major number on release\n");
		return -ENODEV;
	}

	device_no = DEVICE_NR(ino->i_rdev);
	if (device_no > numdsk) {
		return -ENODEV;
	}

	/* Just to be paranoid, sync the device                                  */
	fsync_dev(ino->i_rdev);

	/* Call the actual release code                                          */
	internal_release(device_no);

	MOD_DEC_USE_COUNT;
	return 0;
}

/* External ioctl entry point.
 */
static int viodasd_ioctl(struct inode *ino, struct file *fil,
			 unsigned int cmd, unsigned long arg)
{
	int device_no;
	int err;
	HvLpEvent_Rc hvrc;
	DECLARE_MUTEX_LOCKED(Semaphore);

	/* Sanity checks                                                        */
	if (!ino) {
		printk(KERN_WARNING_VIO "no inode provided in ioctl\n");
		return -ENODEV;
	}

	if (MAJOR(ino->i_rdev) != MAJOR_NR) {
		printk(KERN_WARNING_VIO "Wierd error...wrong major number on ioctl\n");
		return -ENODEV;
	}

	device_no = DEVICE_NR(ino->i_rdev);
	if (device_no > numdsk) {
		printk(KERN_WARNING_VIO "Invalid minor device number %d in ioctl\n",
			   device_no);
		return -ENODEV;
	}

	switch (cmd) {
	case BLKGETSIZE:
		/* return the device size in sectors */
		if (!arg)
			return -EINVAL;
		err =
		    verify_area(VERIFY_WRITE, (long *) arg, sizeof(long));
		if (err)
			return err;

		put_user(viodasd_partitions[MINOR(ino->i_rdev)].nr_sects,
			 (long *) arg);
		return 0;

	case FDFLUSH:
	case BLKFLSBUF:
		if (!suser())
			return -EACCES;
		fsync_dev(ino->i_rdev);
		invalidate_buffers(ino->i_rdev);
		hvrc = HvCallEvent_signalLpEventFast(viopath_hostLp,
						     HvLpEvent_Type_VirtualIo,
						     viomajorsubtype_blockio
						     | vioblockflush,
						     HvLpEvent_AckInd_DoAck,
						     HvLpEvent_AckType_ImmediateAck,
						     viopath_sourceinst
						     (viopath_hostLp),
						     viopath_targetinst
						     (viopath_hostLp),
						     (u64) (unsigned long)
						     &Semaphore,
						     VIOVERSION << 16,
						     ((u64) device_no <<
						      48), 0, 0, 0);


		if (hvrc != 0) {
			printk(KERN_WARNING_VIO "bad rc on sync signalLpEvent %d\n",
				   (int) hvrc);
			return -EIO;
		}

		down(&Semaphore);

		return 0;

	case BLKRAGET:
		if (!arg)
			return -EINVAL;
		err =
		    verify_area(VERIFY_WRITE, (long *) arg, sizeof(long));
		if (err)
			return err;
		put_user(read_ahead[MAJOR_NR], (long *) arg);
		return 0;

	case BLKRASET:
		if (!suser())
			return -EACCES;
		if (arg > 0x00ff)
			return -EINVAL;
		read_ahead[MAJOR_NR] = arg;
		return 0;

	case BLKRRPART:
		viodasd_revalidate(ino->i_rdev);
		return 0;

	case HDIO_GETGEO:
		{
			unsigned char sectors;
			unsigned char heads;
			unsigned short cylinders;

			struct hd_geometry *geo =
			    (struct hd_geometry *) arg;
			if (geo == NULL)
				return -EINVAL;

			err = verify_area(VERIFY_WRITE, geo, sizeof(*geo));
			if (err)
				return err;

			sectors = viodasd_devices[device_no].sectors;
			if (sectors == 0)
				sectors = 32;

			heads = viodasd_devices[device_no].tracks;
			if (heads == 0)
				heads = 64;

			cylinders = viodasd_devices[device_no].cylinders;
			if (cylinders == 0)
				cylinders =
				    viodasd_partitions[MINOR(ino->i_rdev)].
				    nr_sects / (sectors * heads);

			put_user(sectors, &geo->sectors);
			put_user(heads, &geo->heads);
			put_user(cylinders, &geo->cylinders);

			put_user(viodasd_partitions[MINOR(ino->i_rdev)].
				 start_sect, (long *) &geo->start);

			return 0;
		}

#define PRTIOC(x) case x: printk(KERN_WARNING_VIO "got unsupported FD ioctl " #x "\n"); \
                          return -EINVAL;

		PRTIOC(FDCLRPRM);
		PRTIOC(FDSETPRM);
		PRTIOC(FDDEFPRM);
		PRTIOC(FDGETPRM);
		PRTIOC(FDMSGON);
		PRTIOC(FDMSGOFF);
		PRTIOC(FDFMTBEG);
		PRTIOC(FDFMTTRK);
		PRTIOC(FDFMTEND);
		PRTIOC(FDSETEMSGTRESH);
		PRTIOC(FDSETMAXERRS);
		PRTIOC(FDGETMAXERRS);
		PRTIOC(FDGETDRVTYP);
		PRTIOC(FDSETDRVPRM);
		PRTIOC(FDGETDRVPRM);
		PRTIOC(FDGETDRVSTAT);
		PRTIOC(FDPOLLDRVSTAT);
		PRTIOC(FDRESET);
		PRTIOC(FDGETFDCSTAT);
		PRTIOC(FDWERRORCLR);
		PRTIOC(FDWERRORGET);
		PRTIOC(FDRAWCMD);
		PRTIOC(FDEJECT);
		PRTIOC(FDTWADDLE);

	}

	return -EINVAL;
}

/* Send an actual I/O request to OS/400
 */
static int send_request(struct request *req)
{
	u64 sect_size;
	u64 start;
	u64 len;
	int direction;
	int nsg;
	u16 viocmd;
	HvLpEvent_Rc hvrc;
	struct vioblocklpevent *bevent;
	struct scatterlist sg[VIOMAXBLOCKDMA];
	struct buffer_head *bh;
	int sgindex;
	int device_no = DEVICE_NR(req->rq_dev);
	int statindex;

	/* Note that this SHOULD always be 512...but lets be architecturally correct */
	sect_size = hardsect_size[MAJOR_NR][device_no];

	/* Figure out teh starting sector and length                                 */
	start =
	    (req->sector +
	     viodasd_partitions[MINOR(req->rq_dev)].start_sect) *
	    sect_size;
	len = req->nr_sectors * sect_size;

	/* More paranoia checks                                                      */
	if ((req->sector + req->nr_sectors) >
	    (viodasd_partitions[MINOR(req->rq_dev)].start_sect +
	     viodasd_partitions[MINOR(req->rq_dev)].nr_sects)) {
		printk(KERN_WARNING_VIO "Invalid request offset & length\n");
		printk(KERN_WARNING_VIO "req->sector: %ld, req->nr_sectors: %ld\n",
			   req->sector, req->nr_sectors);
		printk(KERN_WARNING_VIO "RQ_DEV: %d, minor: %d\n", req->rq_dev,
			   MINOR(req->rq_dev));
		return -1;
	}

	if (req->cmd == READ) {
		direction = PCI_DMA_FROMDEVICE;
		viocmd = viomajorsubtype_blockio | vioblockread;
		statindex = 0;
	} else {
		direction = PCI_DMA_TODEVICE;
		viocmd = viomajorsubtype_blockio | vioblockwrite;
		statindex = 1;
	}

	/* Update totals */
	viod_stats[device_no][statindex].tot++;

	/* Now build the scatter-gather list                                        */
	memset(&sg, 0x00, sizeof(sg));
	sgindex = 0;

	/* See if this is a swap I/O (without a bh pointer) or a regular I/O        */
	if (req->bh) {
		/* OK...this loop takes buffers from the request and adds them to the SG
		   until we're done, or until we hit a maximum.  If we hit a maximum we'll
		   just finish this request later                                       */
		bh = req->bh;
		while ((bh) && (sgindex < VIOMAXBLOCKDMA)) {
			sg[sgindex].address = bh->b_data;
			sg[sgindex].length = bh->b_size;

			sgindex++;
			bh = bh->b_reqnext;
		}
		nsg = pci_map_sg(iSeries_vio_dev, sg, sgindex, direction);
		if ((nsg == 0) || (sg_dma_len(sg) == 0)
		    || (sg_dma_address(sg) == 0xFFFFFFFF)) {
			printk(KERN_WARNING_VIO "error getting sg tces\n");
			return -1;
		}

	} else {
		/* Update stats */
		viod_stats[device_no][statindex].nobh++;

		sg_dma_address(sg) = pci_map_single(iSeries_vio_dev, req->buffer,
						    len, direction);
		if (sg_dma_address(sg) == 0xFFFFFFFF) {
			printk(KERN_WARNING_VIO 
			       "error allocating tce for address %p len %ld\n",
			     req->buffer, (long) len);
			return -1;
		}
		sg_dma_len(sg) = len;
		nsg = 1;
	}

	/* Update stats */
	viod_stats[device_no][statindex].ntce[sgindex]++;

	/* This optimization handles a single DMA block                          */
	if (sgindex == 1) {
		/* Send the open event to OS/400                                         */
		hvrc = HvCallEvent_signalLpEventFast(viopath_hostLp,
						     HvLpEvent_Type_VirtualIo,
						     viomajorsubtype_blockio
						     | viocmd,
						     HvLpEvent_AckInd_DoAck,
						     HvLpEvent_AckType_ImmediateAck,
						     viopath_sourceinst
						     (viopath_hostLp),
						     viopath_targetinst
						     (viopath_hostLp),
						     (u64) (unsigned long)
						     req->buffer,
						     VIOVERSION << 16,
						     ((u64) device_no <<
						      48), start,
						     ((u64) sg[0].
						      dma_address) << 32,
						     sg_dma_len(sg));
	} else {
		bevent = (struct vioblocklpevent *) vio_get_event_buffer(viomajorsubtype_blockio);
		if (bevent == NULL) {
			printk(KERN_WARNING_VIO 
			       "error allocating disk event buffer\n");
			return -1;
		}
			
		/* Now build up the actual request.  Note that we store the pointer      */
		/* to the request buffer in the correlation token so we can match        */
		/* this response up later                                                */
		memset(bevent, 0x00, sizeof(struct vioblocklpevent));
		bevent->event.xFlags.xValid = 1;
		bevent->event.xFlags.xFunction = HvLpEvent_Function_Int;
		bevent->event.xFlags.xAckInd = HvLpEvent_AckInd_DoAck;
		bevent->event.xFlags.xAckType =
		    HvLpEvent_AckType_ImmediateAck;
		bevent->event.xType = HvLpEvent_Type_VirtualIo;
		bevent->event.xSubtype = viocmd;
		bevent->event.xSourceLp = HvLpConfig_getLpIndex();
		bevent->event.xTargetLp = viopath_hostLp;
		bevent->event.xSizeMinus1 =
		    offsetof(struct vioblocklpevent,
			     u.rwData.dmaInfo) +
		    (sizeof(bevent->u.rwData.dmaInfo[0]) * (sgindex)) - 1;
		bevent->event.xSizeMinus1 =
		    sizeof(struct vioblocklpevent) - 1;
		bevent->event.xSourceInstanceId =
		    viopath_sourceinst(viopath_hostLp);
		bevent->event.xTargetInstanceId =
		    viopath_targetinst(viopath_hostLp);
		bevent->event.xCorrelationToken =
		    (u64) (unsigned long) req->buffer;
		bevent->mVersion = VIOVERSION;
		bevent->mDisk = device_no;
		bevent->u.rwData.mOffset = start;

		/* Copy just the dma information from the sg list into the request */
		for (sgindex = 0; sgindex < nsg; sgindex++) {
			bevent->u.rwData.dmaInfo[sgindex].mToken =
			    sg_dma_address(&sg[sgindex]);
			bevent->u.rwData.dmaInfo[sgindex].mLen =
			    sg_dma_len(&sg[sgindex]);
		}

		/* Send the request                                               */
		hvrc = HvCallEvent_signalLpEvent(&bevent->event);
		vio_free_event_buffer(viomajorsubtype_blockio, bevent);
	}

	if (hvrc != HvLpEvent_Rc_Good) {
		printk(KERN_WARNING_VIO "error sending disk event to OS/400 (rcp %d)\n", (int) hvrc);
		return -1;
	} else {
		/* If the request was successful, bump the number of outstanding */
		numReqOut++;
	}
	return 0;
}

/* This is the external request processing routine
 */
static void do_viodasd_request(request_queue_t * q)
{
	int device_no;
	struct request *req;
	for (;;) {

		INIT_REQUEST;

		device_no = CURRENT_DEV;
		if (device_no > numdsk) {
			printk(KERN_WARNING_VIO "Invalid device # %d\n", CURRENT_DEV);
			viodasd_end_request(CURRENT, 0);
			continue;
		}

		if (viodasd_gendsk.sizes == NULL) {
			printk(KERN_WARNING_VIO 
			       "Ouch!  viodasd_gendsk.sizes is NULL\n");
			viodasd_end_request(CURRENT, 0);
			continue;
		}

		/* If the queue is plugged, don't dequeue anything right now */
		if ((q) && (q->plugged)) {
			return;
		}

		/* If we already have the maximum number of requests outstanding to OS/400
		   just bail out. We'll come back later                              */
		if (numReqOut >= VIOMAXREQ)
			return;

		/* get the current request, then dequeue it from the queue           */
		req = CURRENT;
		blkdev_dequeue_request(req);

		/* Try sending the request                                           */
		if (send_request(req) == 0) {
			list_add_tail(&req->queue, &reqlist);
		} else {
			viodasd_end_request(req, 0);
		}
	}
}

/* Check for changed disks
 */
static int viodasd_check_change(kdev_t dev)
{
	struct viodasd_waitevent we;
	HvLpEvent_Rc hvrc;
	int device_no = DEVICE_NR(dev);

	/* This semaphore is raised in the interrupt handler                     */
	DECLARE_MUTEX_LOCKED(Semaphore);

	/* Check that we are dealing with a valid hosting partition              */
	if (viopath_hostLp == HvLpIndexInvalid) {
		printk(KERN_WARNING_VIO "Invalid hosting partition\n");
		return -EIO;
	}

	we.sem = &Semaphore;

	/* Send the open event to OS/400                                         */
	hvrc = HvCallEvent_signalLpEventFast(viopath_hostLp,
					     HvLpEvent_Type_VirtualIo,
					     viomajorsubtype_blockio |
					     vioblockcheck,
					     HvLpEvent_AckInd_DoAck,
					     HvLpEvent_AckType_ImmediateAck,
					     viopath_sourceinst
					     (viopath_hostLp),
					     viopath_targetinst
					     (viopath_hostLp),
					     (u64) (unsigned long) &we,
					     VIOVERSION << 16,
					     ((u64) device_no << 48), 0, 0,
					     0);

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

/* Our file operations table
 */
static struct block_device_operations viodasd_fops = {
	open:viodasd_open,
	release:viodasd_release,
	ioctl:viodasd_ioctl,
	check_media_change:viodasd_check_change,
	revalidate:viodasd_revalidate
};

/* Our gendisk table
 */
struct gendisk viodasd_gendsk = {
	0,			/* major - fill in later           */
	"viodasd",
	PARTITION_SHIFT,
	1 << PARTITION_SHIFT,
	NULL,			/* partition array - fill in later */
	NULL,			/* block sizes - fill in later     */
	0,			/* # units                         */
	NULL,			/* "real device" pointer           */
	NULL,			/* next                            */
	&viodasd_fops		/* operations                      */
};

/* This routine handles incoming block LP events
 */
static void vioHandleBlockEvent(struct HvLpEvent *event)
{
	struct scatterlist sg[VIOMAXBLOCKDMA];
	struct vioblocklpevent *bevent = (struct vioblocklpevent *) event;
	int nsect;
	struct request *req;
	int i;
	struct viodasd_waitevent *pwe;
	unsigned long flags;
	int maxsg;

	if (event == NULL) {
		/* Notification that a partition went away! */
		return;
	}
	// First, we should NEVER get an int here...only acks
	if (event->xFlags.xFunction == HvLpEvent_Function_Int) {
		printk(KERN_WARNING_VIO 
		       "Yikes! got an int in viodasd event handler!\n");
		if (event->xFlags.xAckInd == HvLpEvent_AckInd_DoAck) {
			event->xRc = HvLpEvent_Rc_InvalidSubtype;
			HvCallEvent_ackLpEvent(event);
		}
	}

	switch (event->xSubtype & VIOMINOR_SUBTYPE_MASK) {

		/* Handle a response to an open request.  We get all the disk information
		 * in the response, so update it.  The correlation token contains a pointer to
		 * a waitevent structure that has a semaphore in it.  update the return code
		 * in the waitevent structure and post the semaphore to wake up the guy who
		 * sent the request */
	case vioblockopen:
		pwe =
		    (struct viodasd_waitevent *) (unsigned long) event->
		    xCorrelationToken;
		pwe->rc = event->xRc;
		if (event->xRc == HvLpEvent_Rc_Good) {
			viodasd_devices[bevent->mDisk].size =
			    bevent->u.openData.mDiskLen;
			viodasd_devices[bevent->mDisk].cylinders =
			    bevent->u.openData.mCylinders;
			viodasd_devices[bevent->mDisk].tracks =
			    bevent->u.openData.mTracks;
			viodasd_devices[bevent->mDisk].sectors =
			    bevent->u.openData.mSectors;
			viodasd_devices[bevent->mDisk].bytesPerSector =
			    bevent->u.openData.mBytesPerSector;
			viodasd_devices[bevent->mDisk].readOnly =
			    bevent->mFlags & vioblockflags_ro;

			if (viodasd_max_disk !=
			    bevent->u.openData.mMaxDisks) {
				viodasd_max_disk =
				    bevent->u.openData.mMaxDisks;
			}
		}
		up(pwe->sem);
		break;

	case vioblockclose:
		break;

		/* For read and write requests, decrement the number of outstanding requests,
		 * Free the DMA buffers we allocated, and find the matching request by
		 * using the buffer pointer we stored in the correlation token.
		 */
	case vioblockread:
	case vioblockwrite:

		/* Free the DMA buffers                                                      */
		i = 0;
		nsect = 0;
		memset(sg, 0x00, sizeof(sg));

		maxsg = (((bevent->event.xSizeMinus1 + 1) -
			  offsetof(struct vioblocklpevent,
				   u.rwData.dmaInfo)) /
			 sizeof(bevent->u.rwData.dmaInfo[0]));


		while ((i < maxsg) &&
		       (bevent->u.rwData.dmaInfo[i].mLen > 0) &&
		       (i < VIOMAXBLOCKDMA)) {
			sg_dma_address(&sg[i]) =
			    bevent->u.rwData.dmaInfo[i].mToken;
			sg_dma_len(&sg[i]) =
			    bevent->u.rwData.dmaInfo[i].mLen;
			nsect += bevent->u.rwData.dmaInfo[i].mLen;
			i++;
		}

		pci_unmap_sg(iSeries_vio_dev,
			     sg,
			     i,
			     (bevent->event.xSubtype ==
			      (viomajorsubtype_blockio | vioblockread)) ?
			     PCI_DMA_FROMDEVICE : PCI_DMA_TODEVICE);


		/* Since this is running in interrupt mode, we need to make sure we're not
		 * stepping on any global I/O operations
		 */
		spin_lock_irqsave(&io_request_lock, flags);

		/* Decrement the number of outstanding requests                              */
		numReqOut--;

		/* Now find the matching request in OUR list (remember we moved the request
		 * from the global list to our list when we got it)
		 */
		req = blkdev_entry_to_request(reqlist.next);
		while ((&req->queue != &reqlist) &&
		       ((u64) (unsigned long) req->buffer !=
			bevent->event.xCorrelationToken))
			req = blkdev_entry_to_request(req->queue.next);

		if (&req->queue == &reqlist) {
			printk(KERN_WARNING_VIO 
			       "Yikes! Could not find matching buffer %p in reqlist\n",
			       req->buffer);
			break;
		}

		/* Remove the request from our list                                    */
		list_del(&req->queue);

		/* Calculate the number of sectors from the length in bytes            */
		nsect = nsect >> 9;
		if (!req->bh) {
			if (event->xRc != HvLpEvent_Rc_Good) {
				printk(KERN_WARNING_VIO "read/wrute error %d:%d\n", event->xRc,
					   bevent->mSubTypeRc);
				viodasd_end_request(req, 0);
			} else {
				if (nsect != req->current_nr_sectors) {
					printk(KERN_WARNING_VIO 
					       "Yikes...non bh i/o # sect doesn't match!!!\n");
				}
				viodasd_end_request(req, 1);
			}
		} else {
			while ((nsect > 0) && (req->bh)) {
				nsect -= req->current_nr_sectors;
				viodasd_end_request(req, 1);
			}
			if (nsect) {
				printk(KERN_WARNING_VIO 
				       "Yikes...sectors left over on a request!!!\n");
			}

			/* If the original request could not handle all the buffers, re-send
			 * the request 
			 */
			if (req->bh) {
				if (send_request(req) == 0) {
					list_add_tail(&req->queue,
						      &reqlist);
				} else {
					viodasd_end_request(req, 0);
				}
			}

		}

		/* Finally, send more requests                                */
		do_viodasd_request(NULL);

		spin_unlock_irqrestore(&io_request_lock, flags);
		break;

	case vioblockflush:
		up((void *) (unsigned long) event->xCorrelationToken);
		break;

	case vioblockcheck:
		pwe =
		    (struct viodasd_waitevent *) (unsigned long) event->
		    xCorrelationToken;
		pwe->rc = event->xRc;
		pwe->changed = bevent->u.check.changed;
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

/* This routine tries to clean up anything we allocated/registered
 */
static void cleanup2(void)
{
	int i;

#define CLEANIT(x) if (x) {kfree(x); x=NULL;}

	for (i = 0; i < numdsk; i++)
		fsync_dev(MKDEV(MAJOR_NR, i));

	read_ahead[MAJOR_NR] = 0;

	CLEANIT(viodasd_devices);
	CLEANIT(blk_size[MAJOR_NR]);
	CLEANIT(blksize_size[MAJOR_NR]);
	CLEANIT(hardsect_size[MAJOR_NR]);
	CLEANIT(max_sectors[MAJOR_NR]);
	CLEANIT(viodasd_gendsk.part);
	blk_size[MAJOR_NR] = NULL;
	blksize_size[MAJOR_NR] = NULL;

	devfs_unregister_blkdev(MAJOR_NR, VIOD_DEVICE_NAME);
}

/* Initialize the whole device driver.  Handle module and non-module
 * versions
 */
__init int viodasd_init(void)
{
	int i, j;
	int rc;
	int *viodasd_blksizes;
	int numpart = numdsk << PARTITION_SHIFT;

	/* Try to open to our host lp
	 */
	if (viopath_hostLp == HvLpIndexInvalid) {
		vio_set_hostlp();
	}

	if (viopath_hostLp == HvLpIndexInvalid) {
		printk(KERN_WARNING_VIO "%s: invalid hosting partition\n",
			   VIOD_DEVICE_NAME);
		return -1;
	}

	/*
	 * Do the devfs_register.  This works even if devfs is not
	 * configured
	 */
	if (devfs_register_blkdev
	    (MAJOR_NR, VIOD_DEVICE_NAME, &viodasd_fops)) {
		printk(KERN_WARNING_VIO "%s: unable to get major number %d\n",
			   VIOD_DEVICE_NAME, MAJOR_NR);
		return -1;
	}

	printk(KERN_INFO_VIO
	       "%s: Disk vers %s, major %d, max disks %d, hosting partition %d\n",
	     VIOD_DEVICE_NAME, VIODASD_VERS, MAJOR_NR, numdsk,
	     viopath_hostLp);

	if (ROOT_DEV == NODEV) {
		ROOT_DEV = MKDEV(MAJOR_NR,1);

		printk(KERN_INFO_VIO
		       "Claiming root file system as first partition of first virtual disk");
	}

	/* Do the blk device initialization                          */
	blk_init_queue(BLK_DEFAULT_QUEUE(MAJOR_NR), DEVICE_REQUEST);

	read_ahead[MAJOR_NR] = 8;	/* 8 sector (4kB) read ahead */

	/* Start filling in gendsk structure              */
	viodasd_gendsk.major = MAJOR_NR;
	viodasd_gendsk.major_name = VIOD_GENHD_NAME;
	viodasd_gendsk.nr_real = numdsk;
	add_gendisk(&viodasd_gendsk);

	/* Actually open the path to the hosting partition           */
	rc = viopath_open(viopath_hostLp, viomajorsubtype_blockio, VIOMAXREQ+2);
	if (rc) {
		printk(KERN_WARNING_VIO "error opening path to host partition %d\n",
			   viopath_hostLp);
		blk_cleanup_queue(BLK_DEFAULT_QUEUE(MAJOR_NR));
		return -1;
	} else {
		printk("%s: opened path to hosting partition %d\n",
		       VIOD_DEVICE_NAME, viopath_hostLp);
	}

	/*
	 * Initialize our request handler
	 */
	vio_setHandler(viomajorsubtype_blockio, vioHandleBlockEvent);

	/*
	 * Now fill in all the device driver info     
	 */
	viodasd_devices =
	    kmalloc(numdsk * sizeof(struct viodasd_device), GFP_KERNEL);
	if (!viodasd_devices) {
		cleanup2();
		return -ENOMEM;
	}
	memset(viodasd_devices, 0x00,
	       numdsk * sizeof(struct viodasd_device));

	viodasd_sizes = kmalloc(numpart * sizeof(int), GFP_KERNEL);
	if (!viodasd_sizes) {
		cleanup2();
		return -ENOMEM;
	}
	memset(viodasd_sizes, 0x00, numpart * sizeof(int));
	blk_size[MAJOR_NR] = viodasd_gendsk.sizes = viodasd_sizes;

	viodasd_partitions =
	    kmalloc(numpart * sizeof(struct hd_struct), GFP_KERNEL);
	if (!viodasd_partitions) {
		cleanup2();
		return -ENOMEM;
	}
	memset(viodasd_partitions, 0x00,
	       numpart * sizeof(struct hd_struct));
	viodasd_gendsk.part = viodasd_partitions;

	viodasd_blksizes = kmalloc(numpart * sizeof(int), GFP_KERNEL);
	if (!viodasd_blksizes) {
		cleanup2();
		return -ENOMEM;
	}
	for (i = 0; i < numpart; i++)
		viodasd_blksizes[i] = blksize;
	blksize_size[MAJOR_NR] = viodasd_blksizes;

	viodasd_sectsizes = kmalloc(numpart * sizeof(int), GFP_KERNEL);
	if (!viodasd_sectsizes) {
		cleanup2();
		return -ENOMEM;
	}
	for (i = 0; i < numpart; i++)
		viodasd_sectsizes[i] = 0;
	hardsect_size[MAJOR_NR] = viodasd_sectsizes;

	viodasd_maxsectors = kmalloc(numpart * sizeof(int), GFP_KERNEL);
	if (!viodasd_maxsectors) {
		cleanup2();
		return -ENOMEM;
	}
	for (i = 0; i < numpart; i++)
		viodasd_maxsectors[i] = VIODASD_MAXSECTORS;
	max_sectors[MAJOR_NR] = viodasd_maxsectors;

	viodasd_max_disk = numdsk;
	for (i = 0; i <= viodasd_max_disk; i++) {
		// Note that internal_open has two side effects:
		//  a) it updates the size of the disk
		//  b) it updates viodasd_max_disk
		if (internal_open(i) == 0) {
			if (i == 0)
				printk(KERN_INFO_VIO
				       "%s: Currently %d disks connected\n",
				       VIOD_DEVICE_NAME,
				       (int) viodasd_max_disk + 1);

			register_disk(&viodasd_gendsk,
				      MKDEV(MAJOR_NR,
					    i << PARTITION_SHIFT),
				      1 << PARTITION_SHIFT, &viodasd_fops,
				      viodasd_partitions[i <<
							 PARTITION_SHIFT].
				      nr_sects);

			printk(KERN_INFO_VIO
			       "%s: Disk %2.2d size %dM, sectors %d, heads %d, cylinders %d, sectsize %d\n",
			       VIOD_DEVICE_NAME, i,
			       (int) (viodasd_devices[i].size /
				      (1024 * 1024)),
			       (int) viodasd_devices[i].sectors,
			       (int) viodasd_devices[i].tracks,
			       (int) viodasd_devices[i].cylinders,
			       (int) viodasd_sectsizes[i <<
						      PARTITION_SHIFT]);

			for (j = (i << PARTITION_SHIFT) + 1;
			     j < ((i + 1) << PARTITION_SHIFT); j++) {
				if (viodasd_gendsk.part[j].nr_sects)
					printk(KERN_INFO_VIO
					       "%s: Disk %2.2d partition %2.2d start sector %ld, # sector %ld\n",
					       VIOD_DEVICE_NAME, i,
					       j - (i << PARTITION_SHIFT),
					       viodasd_gendsk.part[j].
					       start_sect,
					       viodasd_gendsk.part[j].
					       nr_sects);
			}

			internal_release(i);
		}
	}

	/* 
	 * Create the proc entry
	 */
	iSeries_proc_callback(&viodasd_proc_init);

	return 0;
}

#ifdef MODULE
void viodasd_exit(void)
{
	int i;
	for (i = 0; i < numdsk << PARTITION_SHIFT; i++)
		fsync_dev(MKDEV(MAJOR_NR, i));

	blk_cleanup_queue(BLK_DEFAULT_QUEUE(MAJOR_NR));

	iSeries_proc_callback(&viodasd_proc_delete);

	cleanup2();
}
#endif

#ifdef MODULE
module_init(viodasd_init);
module_exit(viodasd_exit);
#endif
