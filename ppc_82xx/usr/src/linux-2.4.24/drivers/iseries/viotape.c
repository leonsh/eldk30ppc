/* -*- linux-c -*-
 *  drivers/char/viotape.c
 *
 *  iSeries Virtual Tape
 ***************************************************************************
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
 ***************************************************************************
 * This routine provides access to tape drives owned and managed by an OS/400 
 * partition running on the same box as this Linux partition.
 *
 * All tape operations are performed by sending messages back and forth to 
 * the OS/400 partition.  The format of the messages is defined in
 * iSeries/vio.h
 * 
 */


#undef VIOT_DEBUG

#include <linux/config.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <asm/ioctls.h>
#include <linux/mtio.h>
#include <linux/pci.h>
#include <linux/devfs_fs.h>
#include <linux/devfs_fs_kernel.h>
#include <asm/uaccess.h>

#include "vio.h"
#include <asm/iSeries/HvLpEvent.h>
#include "asm/iSeries/HvCallEvent.h"
#include "asm/iSeries/HvLpConfig.h"
#include <asm/iSeries/iSeries_proc.h>

extern struct pci_dev * iSeries_vio_dev;

static int viotape_major = 230;
static int viotape_numdev = 0;

#define VIOTAPE_MAXREQ 1

/* version number for viotape driver */
static unsigned int version_major = 1;
static unsigned int version_minor = 0;

static u64 sndMsgSeq;
static u64 sndMsgAck;
static u64 rcvMsgSeq;
static u64 rcvMsgAck;

/***************************************************************************
 * The minor number follows the conventions of the SCSI tape drives.  The
 * rewind and mode are encoded in the minor #.  We use this struct to break
 * them out
 ***************************************************************************/
struct viot_devinfo_struct {
	int major;
	int minor;
	int devno;
	int mode;
	int rewind;
};

#define VIOTAPOP_RESET          0
#define VIOTAPOP_FSF	        1
#define VIOTAPOP_BSF	        2
#define VIOTAPOP_FSR	        3
#define VIOTAPOP_BSR	        4
#define VIOTAPOP_WEOF	        5
#define VIOTAPOP_REW	        6
#define VIOTAPOP_NOP	        7
#define VIOTAPOP_EOM	        8
#define VIOTAPOP_ERASE          9
#define VIOTAPOP_SETBLK        10
#define VIOTAPOP_SETDENSITY    11
#define VIOTAPOP_SETPOS	       12
#define VIOTAPOP_GETPOS	       13
#define VIOTAPOP_SETPART       14

struct viotapelpevent {
	struct HvLpEvent event;
	u32 mReserved1;
	u16 mVersion;
	u16 mSubTypeRc;
	u16 mTape;
	u16 mFlags;
	u32 mToken;
	u64 mLen;
	union {
		struct {
			u32 mTapeOp;
			u32 mCount;
		} tapeOp;
		struct {
			u32 mType;
			u32 mResid;
			u32 mDsreg;
			u32 mGstat;
			u32 mErreg;
			u32 mFileNo;
			u32 mBlkNo;
		} getStatus;
		struct {
			u32 mBlkNo;
		} getPos;
	} u;
};
enum viotapesubtype {
	viotapeopen = 0x0001,
	viotapeclose = 0x0002,
	viotaperead = 0x0003,
	viotapewrite = 0x0004,
	viotapegetinfo = 0x0005,
	viotapeop = 0x0006,
	viotapegetpos = 0x0007,
	viotapesetpos = 0x0008,
	viotapegetstatus = 0x0009
};

enum viotapeRc {
	viotape_InvalidRange = 0x0601,
	viotape_InvalidToken = 0x0602,
	viotape_DMAError = 0x0603,
	viotape_UseError = 0x0604,
	viotape_ReleaseError = 0x0605,
	viotape_InvalidTape = 0x0606,
	viotape_InvalidOp = 0x0607,
	viotape_TapeErr = 0x0608,

	viotape_AllocTimedOut = 0x0640,
	viotape_BOTEnc = 0x0641,
	viotape_BlankTape = 0x0642,
	viotape_BufferEmpty = 0x0643,
	viotape_CleanCartFound = 0x0644,
	viotape_CmdNotAllowed = 0x0645,
	viotape_CmdNotSupported = 0x0646,
	viotape_DataCheck = 0x0647,
	viotape_DecompressErr = 0x0648,
	viotape_DeviceTimeout = 0x0649,
	viotape_DeviceUnavail = 0x064a,
	viotape_DeviceBusy = 0x064b,
	viotape_EndOfMedia = 0x064c,
	viotape_EndOfTape = 0x064d,
	viotape_EquipCheck = 0x064e,
	viotape_InsufficientRs = 0x064f,
	viotape_InvalidLogBlk = 0x0650,
	viotape_LengthError = 0x0651,
	viotape_LibDoorOpen = 0x0652,
	viotape_LoadFailure = 0x0653,
	viotape_NotCapable = 0x0654,
	viotape_NotOperational = 0x0655,
	viotape_NotReady = 0x0656,
	viotape_OpCancelled = 0x0657,
	viotape_PhyLinkErr = 0x0658,
	viotape_RdyNotBOT = 0x0659,
	viotape_TapeMark = 0x065a,
	viotape_WriteProt = 0x065b
};

/* Maximum # tapes we support
 */
#define VIOTAPE_MAX_TAPE 8
#define MAX_PARTITIONS 4

/* defines for current tape state */
#define VIOT_IDLE 0
#define VIOT_READING 1
#define VIOT_WRITING 2

/* Our info on the tapes
 */
struct tape_descr {
	char rsrcname[10];
	char type[4];
	char model[3];
};

static struct tape_descr *viotape_unitinfo = NULL;

static char *lasterr[VIOTAPE_MAX_TAPE];

static struct mtget viomtget[VIOTAPE_MAX_TAPE];

/* maintain the current state of each tape (and partition)
   so that we know when to write EOF marks.
*/
static struct {
	unsigned char cur_part;
	devfs_handle_t dev_handle;
	struct {
		unsigned char rwi;
	} part_stat[MAX_PARTITIONS];
} state[VIOTAPE_MAX_TAPE];

/* We single-thread
 */
static struct semaphore reqSem;

/* When we send a request, we use this struct to get the response back
 * from the interrupt handler
 */
struct opStruct {
	void *buffer;
	dma_addr_t dmaaddr;
	size_t count;
	int rc;
	struct semaphore *sem;
	struct opStruct *free;
};

static spinlock_t opStructListLock;
static struct opStruct *opStructList;

/* forward declaration to resolve interdependence */
static int chg_state(int index, unsigned char new_state,
		     struct file *file);

/* Decode the kdev_t into its parts
 */
void getDevInfo(kdev_t dev, struct viot_devinfo_struct *devi)
{
	devi->major = MAJOR(dev);
	devi->minor = MINOR(dev);
	devi->devno = devi->minor & 0x1F;
	devi->mode = (devi->minor & 0x60) >> 5;
	/* if bit is set in the minor, do _not_ rewind automatically */
	devi->rewind = !(devi->minor & 0x80);
}


/* Allocate an op structure from our pool
 */
static struct opStruct *getOpStruct(void)
{
	struct opStruct *newOpStruct;
	spin_lock(&opStructListLock);

	if (opStructList == NULL) {
		newOpStruct = kmalloc(sizeof(struct opStruct), GFP_KERNEL);
	} else {
		newOpStruct = opStructList;
		opStructList = opStructList->free;
	}

	if (newOpStruct)
		memset(newOpStruct, 0x00, sizeof(struct opStruct));

	spin_unlock(&opStructListLock);

	return newOpStruct;
}

/* Return an op structure to our pool
 */
static void freeOpStruct(struct opStruct *opStruct)
{
	spin_lock(&opStructListLock);
	opStruct->free = opStructList;
	opStructList = opStruct;
	spin_unlock(&opStructListLock);
}

/* Map our tape return codes to errno values
 */
int tapeRcToErrno(int tapeRc, char *operation, int tapeno)
{
	int terrno;
	char *tmsg;

	switch (tapeRc) {
	case 0:
		return 0;
	case viotape_InvalidRange:
		terrno = EIO;
		tmsg = "Internal error";
		break;
	case viotape_InvalidToken:
		terrno = EIO;
		tmsg = "Internal error";
		break;
	case viotape_DMAError:
		terrno = EIO;
		tmsg = "DMA error";
		break;
	case viotape_UseError:
		terrno = EIO;
		tmsg = "Internal error";
		break;
	case viotape_ReleaseError:
		terrno = EIO;
		tmsg = "Internal error";
		break;
	case viotape_InvalidTape:
		terrno = EIO;
		tmsg = "Invalid tape device";
		break;
	case viotape_InvalidOp:
		terrno = EIO;
		tmsg = "Invalid operation";
		break;
	case viotape_TapeErr:
		terrno = EIO;
		tmsg = "Tape error";
		break;

	case viotape_AllocTimedOut:
		terrno = EBUSY;
		tmsg = "Allocate timed out";
		break;
	case viotape_BOTEnc:
		terrno = EIO;
		tmsg = "Beginning of tape encountered";
		break;
	case viotape_BlankTape:
		terrno = EIO;
		tmsg = "Blank tape";
		break;
	case viotape_BufferEmpty:
		terrno = EIO;
		tmsg = "Buffer empty";
		break;
	case viotape_CleanCartFound:
		terrno = ENOMEDIUM;
		tmsg = "Cleaning cartridge found";
		break;
	case viotape_CmdNotAllowed:
		terrno = EIO;
		tmsg = "Command not allowed";
		break;
	case viotape_CmdNotSupported:
		terrno = EIO;
		tmsg = "Command not supported";
		break;
	case viotape_DataCheck:
		terrno = EIO;
		tmsg = "Data check";
		break;
	case viotape_DecompressErr:
		terrno = EIO;
		tmsg = "Decompression error";
		break;
	case viotape_DeviceTimeout:
		terrno = EBUSY;
		tmsg = "Device timeout";
		break;
	case viotape_DeviceUnavail:
		terrno = EIO;
		tmsg = "Device unavailable";
		break;
	case viotape_DeviceBusy:
		terrno = EBUSY;
		tmsg = "Device busy";
		break;
	case viotape_EndOfMedia:
		terrno = ENOSPC;
		tmsg = "End of media";
		break;
	case viotape_EndOfTape:
		terrno = ENOSPC;
		tmsg = "End of tape";
		break;
	case viotape_EquipCheck:
		terrno = EIO;
		tmsg = "Equipment check";
		break;
	case viotape_InsufficientRs:
		terrno = EOVERFLOW;
		tmsg = "Insufficient tape resources";
		break;
	case viotape_InvalidLogBlk:
		terrno = EIO;
		tmsg = "Invalid logical block location";
		break;
	case viotape_LengthError:
		terrno = EOVERFLOW;
		tmsg = "Length error";
		break;
	case viotape_LibDoorOpen:
		terrno = EBUSY;
		tmsg = "Door open";
		break;
	case viotape_LoadFailure:
		terrno = ENOMEDIUM;
		tmsg = "Load failure";
		break;
	case viotape_NotCapable:
		terrno = EIO;
		tmsg = "Not capable";
		break;
	case viotape_NotOperational:
		terrno = EIO;
		tmsg = "Not operational";
		break;
	case viotape_NotReady:
		terrno = EIO;
		tmsg = "Not ready";
		break;
	case viotape_OpCancelled:
		terrno = EIO;
		tmsg = "Operation cancelled";
		break;
	case viotape_PhyLinkErr:
		terrno = EIO;
		tmsg = "Physical link error";
		break;
	case viotape_RdyNotBOT:
		terrno = EIO;
		tmsg = "Ready but not beginning of tape";
		break;
	case viotape_TapeMark:
		terrno = EIO;
		tmsg = "Tape mark";
		break;
	case viotape_WriteProt:
		terrno = EROFS;
		tmsg = "Write protection error";
		break;
	default:
		terrno = EIO;
		tmsg = "I/O error";
	}

	printk(KERN_WARNING_VIO "tape error on Device %d (%10.10s): %s\n",
	       tapeno, viotape_unitinfo[tapeno].rsrcname, tmsg);

	lasterr[tapeno] = tmsg;

	return -terrno;
}

/* Handle reads from the proc file system.  
 */
static int proc_read(char *buf, char **start, off_t offset,
		     int blen, int *eof, void *data)
{
	int len = 0;
	int i;

	len += sprintf(buf + len, "viotape driver version %d.%d\n",
		       version_major, version_minor);

	for (i = 0; i < viotape_numdev; i++) {

		len +=
		    sprintf(buf + len,
			    "viotape device %d is iSeries resource %10.10s type %4.4s, model %3.3s\n",
			    i, viotape_unitinfo[i].rsrcname,
			    viotape_unitinfo[i].type,
			    viotape_unitinfo[i].model);
		if (lasterr[i])
			len +=
			    sprintf(buf + len, "   last error: %s\n",
				    lasterr[i]);
	}

	*eof = 1;
	return len;
}

/* setup our proc file system entries
 */
void viotape_proc_init(struct proc_dir_entry *iSeries_proc)
{
	struct proc_dir_entry *ent;
	ent =
	    create_proc_entry("viotape", S_IFREG | S_IRUSR, iSeries_proc);
	if (!ent)
		return;
	ent->nlink = 1;
	ent->data = NULL;
	ent->read_proc = proc_read;
}

/* clean up our proc file system entries
 */
void viotape_proc_delete(struct proc_dir_entry *iSeries_proc)
{
	remove_proc_entry("viotape", iSeries_proc);
}


/* Get info on all tapes from OS/400
 */
static void get_viotape_info(void)
{
	dma_addr_t dmaaddr;
	HvLpEvent_Rc hvrc;
	int i;
	struct opStruct *op = getOpStruct();
	DECLARE_MUTEX_LOCKED(Semaphore);
	if (op == NULL)
		return;

	if (viotape_unitinfo == NULL) {
		viotape_unitinfo =
		    kmalloc(sizeof(struct tape_descr) * VIOTAPE_MAX_TAPE,
			    GFP_KERNEL);
	}
	memset(viotape_unitinfo, 0x00,
	       sizeof(struct tape_descr) * VIOTAPE_MAX_TAPE);
	memset(lasterr, 0x00, sizeof(lasterr));

	op->sem = &Semaphore;

	dmaaddr = pci_map_single(iSeries_vio_dev, viotape_unitinfo,
				 sizeof(struct tape_descr) *
				 VIOTAPE_MAX_TAPE, PCI_DMA_FROMDEVICE);
	if (dmaaddr == 0xFFFFFFFF) {
		printk(KERN_WARNING_VIO "viotape error allocating tce\n");
		return;
	}

	hvrc = HvCallEvent_signalLpEventFast(viopath_hostLp,
					     HvLpEvent_Type_VirtualIo,
					     viomajorsubtype_tape |
					     viotapegetinfo,
					     HvLpEvent_AckInd_DoAck,
					     HvLpEvent_AckType_ImmediateAck,
					     viopath_sourceinst
					     (viopath_hostLp),
					     viopath_targetinst
					     (viopath_hostLp),
					     (u64) (unsigned long) op,
					     VIOVERSION << 16, dmaaddr,
					     sizeof(struct tape_descr) *
					     VIOTAPE_MAX_TAPE, 0, 0);
	if (hvrc != HvLpEvent_Rc_Good) {
		printk("viotape hv error on op %d\n", (int) hvrc);
	}

	down(&Semaphore);

	freeOpStruct(op);


	for (i = 0;
	     ((i < VIOTAPE_MAX_TAPE) && (viotape_unitinfo[i].rsrcname[0]));
	     i++) {
		printk("found a tape %10.10s\n",
		       viotape_unitinfo[i].rsrcname);
		viotape_numdev++;
	}
}


/* Write
 */
static ssize_t viotap_write(struct file *file, const char *buf,
			    size_t count, loff_t * ppos)
{
	HvLpEvent_Rc hvrc;
	kdev_t dev = file->f_dentry->d_inode->i_rdev;
	unsigned short flags = file->f_flags;
	struct opStruct *op = getOpStruct();
	int noblock = ((flags & O_NONBLOCK) != 0);
	int err;
	struct viot_devinfo_struct devi;
	DECLARE_MUTEX_LOCKED(Semaphore);

	if (op == NULL)
		return -ENOMEM;

	getDevInfo(dev, &devi);

	/* We need to make sure we can send a request.  We use
	 * a semaphore to keep track of # requests in use.  If
	 * we are non-blocking, make sure we don't block on the 
	 * semaphore
	 */
	if (noblock) {
		if (down_trylock(&reqSem)) {
			freeOpStruct(op);
			return -EWOULDBLOCK;
		}
	} else {
		down(&reqSem);
	}

	/* Allocate a DMA buffer */
	op->buffer = pci_alloc_consistent(iSeries_vio_dev, count, &op->dmaaddr);

	if ((op->dmaaddr == 0xFFFFFFFF) || (op->buffer == NULL)) {
		printk(KERN_WARNING_VIO 
		       "tape error allocating dma buffer for len %ld\n",
		       (long)count);
		freeOpStruct(op);
		up(&reqSem);
		return -EFAULT;
	}

	op->count = count;

	/* Copy the data into the buffer */
	err = copy_from_user(op->buffer, (const void *) buf, count);
	if (err) {
		printk(KERN_WARNING_VIO 
		       "tape: error on copy from user\n");
		pci_free_consistent(iSeries_vio_dev, count, op->buffer, op->dmaaddr);
		freeOpStruct(op);
		up(&reqSem);
		return -EFAULT;
	}

	if (noblock) {
		op->sem = NULL;
	} else {
		op->sem = &Semaphore;
	}

	hvrc = HvCallEvent_signalLpEventFast(viopath_hostLp,
					     HvLpEvent_Type_VirtualIo,
					     viomajorsubtype_tape |
					     viotapewrite,
					     HvLpEvent_AckInd_DoAck,
					     HvLpEvent_AckType_ImmediateAck,
					     viopath_sourceinst
					     (viopath_hostLp),
					     viopath_targetinst
					     (viopath_hostLp),
					     (u64) (unsigned long) op,
					     VIOVERSION << 16,
					     ((u64) devi.
					      devno << 48) | op->dmaaddr,
					     count, 0, 0);
	if (hvrc != HvLpEvent_Rc_Good) {
		printk("viotape hv error on op %d\n", (int) hvrc);
		pci_free_consistent(iSeries_vio_dev, count, op->buffer, op->dmaaddr);
		freeOpStruct(op);
		up(&reqSem);
		return -EIO;
	}

	if (noblock)
		return count;

	down(&Semaphore);

	err = op->rc;

	/* Free the buffer */
	pci_free_consistent(iSeries_vio_dev, count, op->buffer, op->dmaaddr);

	count = op->count;

	freeOpStruct(op);
	up(&reqSem);
	if (err)
		return tapeRcToErrno(err, "write", devi.devno);
	else {
		chg_state(devi.devno, VIOT_WRITING, file);
		return count;
	}
}

/* read
 */
static ssize_t viotap_read(struct file *file, char *buf, size_t count,
			   loff_t * ptr)
{
	HvLpEvent_Rc hvrc;
	kdev_t dev = file->f_dentry->d_inode->i_rdev;
	unsigned short flags = file->f_flags;
	struct opStruct *op = getOpStruct();
	int noblock = ((flags & O_NONBLOCK) != 0);
	int err;
	struct viot_devinfo_struct devi;
	DECLARE_MUTEX_LOCKED(Semaphore);

	if (op == NULL)
		return -ENOMEM;

	getDevInfo(dev, &devi);

	/* We need to make sure we can send a request.  We use
	 * a semaphore to keep track of # requests in use.  If
	 * we are non-blocking, make sure we don't block on the 
	 * semaphore
	 */
	if (noblock) {
		if (down_trylock(&reqSem)) {
			freeOpStruct(op);
			return -EWOULDBLOCK;
		}
	} else {
		down(&reqSem);
	}

	chg_state(devi.devno, VIOT_READING, file);

	/* Allocate a DMA buffer */
	op->buffer = pci_alloc_consistent(iSeries_vio_dev, count, &op->dmaaddr);

	if ((op->dmaaddr == 0xFFFFFFFF) || (op->buffer == NULL)) {
		freeOpStruct(op);
		up(&reqSem);
		return -EFAULT;
	}

	op->count = count;

	op->sem = &Semaphore;

	hvrc = HvCallEvent_signalLpEventFast(viopath_hostLp,
					     HvLpEvent_Type_VirtualIo,
					     viomajorsubtype_tape |
					     viotaperead,
					     HvLpEvent_AckInd_DoAck,
					     HvLpEvent_AckType_ImmediateAck,
					     viopath_sourceinst
					     (viopath_hostLp),
					     viopath_targetinst
					     (viopath_hostLp),
					     (u64) (unsigned long) op,
					     VIOVERSION << 16,
					     ((u64) devi.
					      devno << 48) | op->dmaaddr,
					     count, 0, 0);
	if (hvrc != HvLpEvent_Rc_Good) {
		printk(KERN_WARNING_VIO 
		       "tape hv error on op %d\n", (int) hvrc);
		pci_free_consistent(iSeries_vio_dev, count, op->buffer, op->dmaaddr);
		freeOpStruct(op);
		up(&reqSem);
		return -EIO;
	}

	down(&Semaphore);

	if (op->rc == 0) {
		/* If we got data back        */
		if (op->count) {
			/* Copy the data into the buffer */
			err = copy_to_user(buf, op->buffer, count);
			if (err) {
				printk("error on copy_to_user\n");
				pci_free_consistent(iSeries_vio_dev, count,
						    op->buffer,
						    op->dmaaddr);
				freeOpStruct(op);
				up(&reqSem);
				return -EFAULT;
			}
		}
	}

	err = op->rc;

	/* Free the buffer */
	pci_free_consistent(iSeries_vio_dev, count, op->buffer, op->dmaaddr);
	count = op->count;

	freeOpStruct(op);
	up(&reqSem);
	if (err)
		return tapeRcToErrno(err, "read", devi.devno);
	else
		return count;
}

/* read
 */
static int viotap_ioctl(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long arg)
{
	HvLpEvent_Rc hvrc;
	int err;
	DECLARE_MUTEX_LOCKED(Semaphore);
	kdev_t dev = file->f_dentry->d_inode->i_rdev;
	struct opStruct *op = getOpStruct();
	struct viot_devinfo_struct devi;
	if (op == NULL)
		return -ENOMEM;

	getDevInfo(dev, &devi);

	down(&reqSem);

	switch (cmd) {
	case MTIOCTOP:{
			struct mtop mtc;
			u32 myOp;

			/* inode is null if and only if we (the kernel) made the request */
			if (inode == NULL)
				memcpy(&mtc, (void *) arg,
				       sizeof(struct mtop));
			else if (copy_from_user
				 ((char *) &mtc, (char *) arg,
				  sizeof(struct mtop))) {
				freeOpStruct(op);
				up(&reqSem);
				return -EFAULT;
			}

			switch (mtc.mt_op) {
			case MTRESET:
				myOp = VIOTAPOP_RESET;
				break;
			case MTFSF:
				myOp = VIOTAPOP_FSF;
				break;
			case MTBSF:
				myOp = VIOTAPOP_BSF;
				break;
			case MTFSR:
				myOp = VIOTAPOP_FSR;
				break;
			case MTBSR:
				myOp = VIOTAPOP_BSR;
				break;
			case MTWEOF:
				myOp = VIOTAPOP_WEOF;
				break;
			case MTREW:
				myOp = VIOTAPOP_REW;
				break;
			case MTNOP:
				myOp = VIOTAPOP_NOP;
				break;
			case MTEOM:
				myOp = VIOTAPOP_EOM;
				break;
			case MTERASE:
				myOp = VIOTAPOP_ERASE;
				break;
			case MTSETBLK:
				myOp = VIOTAPOP_SETBLK;
				break;
			case MTSETDENSITY:
				myOp = VIOTAPOP_SETDENSITY;
				break;
			case MTTELL:
				myOp = VIOTAPOP_GETPOS;
				break;
			case MTSEEK:
				myOp = VIOTAPOP_SETPOS;
				break;
			case MTSETPART:
				myOp = VIOTAPOP_SETPART;
				break;
			default:
				return -EIO;
			}

/* if we moved the head, we are no longer reading or writing */
			switch (mtc.mt_op) {
			case MTFSF:
			case MTBSF:
			case MTFSR:
			case MTBSR:
			case MTTELL:
			case MTSEEK:
			case MTREW:
				chg_state(devi.devno, VIOT_IDLE, file);
			}

			op->sem = &Semaphore;
			hvrc =
			    HvCallEvent_signalLpEventFast(viopath_hostLp,
							  HvLpEvent_Type_VirtualIo,
							  viomajorsubtype_tape
							  | viotapeop,
							  HvLpEvent_AckInd_DoAck,
							  HvLpEvent_AckType_ImmediateAck,
							  viopath_sourceinst
							  (viopath_hostLp),
							  viopath_targetinst
							  (viopath_hostLp),
							  (u64) (unsigned
								 long) op,
							  VIOVERSION << 16,
							  ((u64) devi.
							   devno << 48), 0,
							  (((u64) myOp) <<
							   32) | mtc.
							  mt_count, 0);
			if (hvrc != HvLpEvent_Rc_Good) {
				printk("viotape hv error on op %d\n",
				       (int) hvrc);
				freeOpStruct(op);
				up(&reqSem);
				return -EIO;
			}
			down(&Semaphore);
			if (op->rc) {
				freeOpStruct(op);
				up(&reqSem);
				return tapeRcToErrno(op->rc,
						     "tape operation",
						     devi.devno);
			} else {
				freeOpStruct(op);
				up(&reqSem);
				return 0;
			}
			break;
		}

	case MTIOCGET:
		op->sem = &Semaphore;
		hvrc = HvCallEvent_signalLpEventFast(viopath_hostLp,
						     HvLpEvent_Type_VirtualIo,
						     viomajorsubtype_tape |
						     viotapegetstatus,
						     HvLpEvent_AckInd_DoAck,
						     HvLpEvent_AckType_ImmediateAck,
						     viopath_sourceinst
						     (viopath_hostLp),
						     viopath_targetinst
						     (viopath_hostLp),
						     (u64) (unsigned long)
						     op, VIOVERSION << 16,
						     ((u64) devi.
						      devno << 48), 0, 0,
						     0);
		if (hvrc != HvLpEvent_Rc_Good) {
			printk("viotape hv error on op %d\n", (int) hvrc);
			freeOpStruct(op);
			up(&reqSem);
			return -EIO;
		}
		down(&Semaphore);
		up(&reqSem);
		if (op->rc) {
			freeOpStruct(op);
			return tapeRcToErrno(op->rc, "get status",
					     devi.devno);
		} else {
			freeOpStruct(op);
			err =
			    copy_to_user((void *) arg, &viomtget[dev],
					 sizeof(viomtget[0]));
			if (err) {
				freeOpStruct(op);
				return -EFAULT;
			}
			return 0;
		}
		break;
	case MTIOCPOS:
		printk("Got an MTIOCPOS\n");
	default:
		return -ENOSYS;
	}
	return 0;
}

/* Open
 */
static int viotap_open(struct inode *inode, struct file *file)
{
	DECLARE_MUTEX_LOCKED(Semaphore);
	kdev_t dev = file->f_dentry->d_inode->i_rdev;
	HvLpEvent_Rc hvrc;
	struct opStruct *op = getOpStruct();
	struct viot_devinfo_struct devi;
	if (op == NULL)
		return -ENOMEM;

	getDevInfo(dev, &devi);

// Note: We currently only support one mode!
	if ((devi.devno >= viotape_numdev) || (devi.mode)) {
		freeOpStruct(op);
		return -ENODEV;
	}

	op->sem = &Semaphore;

	hvrc = HvCallEvent_signalLpEventFast(viopath_hostLp,
					     HvLpEvent_Type_VirtualIo,
					     viomajorsubtype_tape |
					     viotapeopen,
					     HvLpEvent_AckInd_DoAck,
					     HvLpEvent_AckType_ImmediateAck,
					     viopath_sourceinst
					     (viopath_hostLp),
					     viopath_targetinst
					     (viopath_hostLp),
					     (u64) (unsigned long) op,
					     VIOVERSION << 16,
					     ((u64) devi.devno << 48), 0,
					     0, 0);


	if (hvrc != 0) {
		printk("viotape bad rc on signalLpEvent %d\n", (int) hvrc);
		freeOpStruct(op);
		return -EIO;
	}

	down(&Semaphore);

	if (op->rc) {
		freeOpStruct(op);
		return tapeRcToErrno(op->rc, "open", devi.devno);
	} else {
		freeOpStruct(op);
		MOD_INC_USE_COUNT;
		return 0;
	}
}


/* Release
 */
static int viotap_release(struct inode *inode, struct file *file)
{
	DECLARE_MUTEX_LOCKED(Semaphore);
	kdev_t dev = file->f_dentry->d_inode->i_rdev;
	HvLpEvent_Rc hvrc;
	struct viot_devinfo_struct devi;
	struct opStruct *op = getOpStruct();

	if (op == NULL)
		return -ENOMEM;
	op->sem = &Semaphore;

	getDevInfo(dev, &devi);

	if (devi.devno >= viotape_numdev) {
		freeOpStruct(op);
		return -ENODEV;
	}

	chg_state(devi.devno, VIOT_IDLE, file);

	if (devi.rewind) {
		hvrc = HvCallEvent_signalLpEventFast(viopath_hostLp,
						     HvLpEvent_Type_VirtualIo,
						     viomajorsubtype_tape |
						     viotapeop,
						     HvLpEvent_AckInd_DoAck,
						     HvLpEvent_AckType_ImmediateAck,
						     viopath_sourceinst
						     (viopath_hostLp),
						     viopath_targetinst
						     (viopath_hostLp),
						     (u64) (unsigned long)
						     op, VIOVERSION << 16,
						     ((u64) devi.
						      devno << 48), 0,
						     ((u64) VIOTAPOP_REW)
						     << 32, 0);
		down(&Semaphore);

		if (op->rc) {
			tapeRcToErrno(op->rc, "rewind", devi.devno);
		}
	}

	hvrc = HvCallEvent_signalLpEventFast(viopath_hostLp,
					     HvLpEvent_Type_VirtualIo,
					     viomajorsubtype_tape |
					     viotapeclose,
					     HvLpEvent_AckInd_DoAck,
					     HvLpEvent_AckType_ImmediateAck,
					     viopath_sourceinst
					     (viopath_hostLp),
					     viopath_targetinst
					     (viopath_hostLp),
					     (u64) (unsigned long) op,
					     VIOVERSION << 16,
					     ((u64) devi.devno << 48), 0,
					     0, 0);


	if (hvrc != 0) {
		printk("viotape: bad rc on signalLpEvent %d\n",
		       (int) hvrc);
		return -EIO;
	}

	down(&Semaphore);

	if (op->rc) {
		printk("viotape: close failed\n");
	}
	MOD_DEC_USE_COUNT;
	return 0;
}

struct file_operations viotap_fops = {
	owner:THIS_MODULE,
	read:viotap_read,
	write:viotap_write,
	ioctl:viotap_ioctl,
	open:viotap_open,
	release:viotap_release,
};

/* Handle interrupt events for tape
 */
static void vioHandleTapeEvent(struct HvLpEvent *event)
{
	int tapeminor;
	struct opStruct *op;
	struct viotapelpevent *tevent = (struct viotapelpevent *) event;

	if (event == NULL) {
	  /* Notification that a partition went away! */
	  if (!viopath_isactive(viopath_hostLp)) {
	    /* TODO! Clean up */
	  }
	  return;
	}

	tapeminor = event->xSubtype & VIOMINOR_SUBTYPE_MASK;
	switch (tapeminor) {
	case viotapegetinfo:
	case viotapeopen:
	case viotapeclose:
		op = (struct opStruct *) (unsigned long) event->
		    xCorrelationToken;
		op->rc = tevent->mSubTypeRc;
		up(op->sem);
		break;
	case viotaperead:
	case viotapewrite:
		op = (struct opStruct *) (unsigned long) event->
		    xCorrelationToken;
		op->rc = tevent->mSubTypeRc;;
		op->count = tevent->mLen;

		if (op->sem) {
			up(op->sem);
		} else {
			freeOpStruct(op);
			up(&reqSem);
		}
		break;
	case viotapeop:
	case viotapegetpos:
	case viotapesetpos:
	case viotapegetstatus:
		op = (struct opStruct *) (unsigned long) event->
		    xCorrelationToken;
		if (op) {
			op->count = tevent->u.tapeOp.mCount;
			op->rc = tevent->mSubTypeRc;;

			if (op->sem) {
				up(op->sem);
			}
		}
		break;
	default:
		printk("viotape: wierd ack\n");
	}
}


/* Do initialization
 */
int __init viotap_init(void)
{
	DECLARE_MUTEX_LOCKED(Semaphore);
	int rc;
	char tapename[32];
	int i;

	printk("viotape driver version %d.%d\n", version_major,
	       version_minor);

	sndMsgSeq = sndMsgAck = 0;
	rcvMsgSeq = rcvMsgAck = 0;
	opStructList = NULL;
	spin_lock_init(&opStructListLock);

	sema_init(&reqSem, VIOTAPE_MAXREQ);

	if (viopath_hostLp == HvLpIndexInvalid)
		vio_set_hostlp();

	/*
	 * Open to our hosting lp
	 */
	if (viopath_hostLp == HvLpIndexInvalid)
		return -1;

	printk("viotape: init - open path to hosting (%d)\n",
	       viopath_hostLp);

	rc = viopath_open(viopath_hostLp, viomajorsubtype_tape, VIOTAPE_MAXREQ + 2);
	if (rc) {
		printk("viotape: error on viopath_open to hostlp %d\n",
		       rc);
	}

	vio_setHandler(viomajorsubtype_tape, vioHandleTapeEvent);

	printk("viotape major is %d\n", viotape_major);

	get_viotape_info();

	if (devfs_register_chrdev(viotape_major, "viotape", &viotap_fops)) {
		printk("Error registering viotape device\n");
		return -1;
	}

	for (i = 0; i < viotape_numdev; i++) {
		int j;
		state[i].cur_part = 0;
		for (j = 0; j < MAX_PARTITIONS; ++j)
			state[i].part_stat[j].rwi = VIOT_IDLE;
		sprintf(tapename, "viotape%d", i);
		state[i].dev_handle =
		    devfs_register(NULL, tapename, DEVFS_FL_DEFAULT,
				   viotape_major, i,
				   S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP |
				   S_IWGRP, &viotap_fops, NULL);
		printk
		    ("viotape device %s is iSeries resource %10.10s type %4.4s, model %3.3s\n",
		     tapename, viotape_unitinfo[i].rsrcname,
		     viotape_unitinfo[i].type, viotape_unitinfo[i].model);
	}

	/* 
	 * Create the proc entry
	 */
	iSeries_proc_callback(&viotape_proc_init);

	return 0;
}

/* Give a new state to the tape object
 */
static int chg_state(int index, unsigned char new_state, struct file *file)
{
	unsigned char *cur_state =
	    &state[index].part_stat[state[index].cur_part].rwi;
	int rc = 0;

	/* if the same state, don't bother */
	if (*cur_state == new_state)
		return 0;

	/* write an EOF if changing from writing to some other state */
	if (*cur_state == VIOT_WRITING) {
		struct mtop write_eof = { MTWEOF, 1 };
		rc = viotap_ioctl(NULL, file, MTIOCTOP,
				  (unsigned long) &write_eof);
	}
	*cur_state = new_state;
	return rc;
}

/* Cleanup
 */
static void __exit viotap_exit(void)
{
	int i, ret;
	for (i = 0; i < viotape_numdev; ++i)
		devfs_unregister(state[i].dev_handle);
	ret = devfs_unregister_chrdev(viotape_major, "viotape");
	if (ret < 0)
		printk("Error unregistering device: %d\n", ret);
	iSeries_proc_callback(&viotape_proc_delete);
	if (viotape_unitinfo != NULL) {
		kfree(viotape_unitinfo);
		viotape_unitinfo = NULL;
	}
	viopath_close(viopath_hostLp, viomajorsubtype_tape, VIOTAPE_MAXREQ + 2);
	vio_clearHandler(viomajorsubtype_tape);
}

module_init(viotap_init);
module_exit(viotap_exit);
