/***********************************************************************
 *
 * (C) Copyright 2000-2004
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 *
 * This driver provides access to the VMEBus and IP Module memory of
 * IP860 VMEBus systems.
 *
 ***********************************************************************/

/*
 * Standard in kernel modules
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/version.h>
#include <linux/mm.h>			/* for mem_map		*/
#include <linux/wrapper.h>		/* for mem_map_reserve	*/
#include <asm/types.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/8xx_immap.h>
#include <asm/mpc8xx.h>

#include "ip860_mem.h"

#ifndef KERNEL_VERSION
# define KERNEL_VERSION(a,b,c)	(((a) << 16) + ((b) << 8) + (c))
#endif

#if (!defined(LINUX_VERSION_CODE) || LINUX_VERSION_CODE < KERNEL_VERSION(2,2,0))
#error "You need to use at least Linux kernel version 2.2.0"
#endif

#include <platforms/ip860.h>

/***************************************************************************
 * Global stuff
 ***************************************************************************
 */
#undef	DEBUG

/*
 * Deal with CONFIG_MODVERSIONS
 */
#if CONFIG_MODVERSIONS==1
/* # define MODVERSIONS */
# include <linux/modversions.h>
#endif

/*
 * For character devices
 */
#include <linux/fs.h>		/* character device definitions	*/
#include <linux/wrapper.h>	/* wrapper for compatibility with future versions */

#define	VME_MEM_MAJOR	221	/* reserved for VME -- see http://www.lanana.org/ */

/*
 * Device Declarations 
 */

/*
 * The name for our device, as it will appear in /proc/devices
 */
#define DEVICE_NAME	"ip860_mem"
#define	MEM_VERSION	"0.1-wd"

/***************************************************************************
 * Local stuff
 ***************************************************************************
 */
#define	XCAT(arg1,arg2)		arg1 ## arg2
#define	MEMC_BR(arg)		XCAT(memc_br, arg)
#define MEMC_OR(arg)		XCAT(memc_or, arg)

#ifndef	BR_BA_MSK
#define BR_BA_MSK	0xffff8000	/* Base Address Mask			*/
#define BR_AT_MSK	0x00007000	/* Address Type Mask			*/
#define BR_PS_MSK	0x00000c00	/* Port Size Mask			*/
#define BR_PS_32	0x00000000	/* 32 bit port size			*/
#define BR_PS_16	0x00000800	/* 16 bit port size			*/
#define BR_PS_8		0x00000400	/*  8 bit port size			*/
#endif

static int Major;

#ifdef	DEBUG
# define debugk(fmt,args...)	printk(fmt ,##args)
# define DEBUG_OPTARG(arg)	arg,
#else
# define debugk(fmt,args...)
# define DEBUG_OPTARG(arg)
#endif

/*
 * The following array is indexed using the device minor number.
 *
 * A "name" value of NULL indicates unused entries.
 */
#define	VME_MEM_NOT_USED	{ NULL, 0, 0, 0, 0, 0, 0, }

static vme_mem_t vme_memory[] = {
	/* Minor Number 0 - not used (yet) */
	VME_MEM_NOT_USED,

	/* Minor Number 1 - VME Standard Memory */
	{ "vme_std",	0, 0, 0, 0, 0,	IP860_VME_STD_CS,	},

	/* Minor Number 2 - VME Short Memory */
	{ "vme_short",	0, 0, 0, 0, 0,	IP860_VME_SHORT_CS,	},

	/* Minor Number 3 - Static RAM */
	{ "vme_sram",	0, 0, 0, 0, 0,  IP860_SRAM_CS,		},

	/* Minor Number 4 - Static Board Control & Status */
	{ "vme_bcsr",	0, 0, 0, 0, 0,  IP860_BCSR_CS,		},

	/* Minor Number 5, 6, and 7 - not used (yet) */
	VME_MEM_NOT_USED,
	VME_MEM_NOT_USED,
	VME_MEM_NOT_USED,

	/* Minor Number 8 - IP Module Memory */
	{ "vme_ipmod",	0, 0, 0, 0, 0,	IP860_IP_CS,		},
};

#define	VME_MAX_MINOR	(sizeof(vme_memory) / sizeof(vme_mem_t))

/*
 * Prototypes
 */
static int	ip860_mem_init (void) __init;
static void	init_mem_reg (volatile immap_t *immr, vme_mem_t *);
static int	ip860_mem_open(struct inode *, struct file *);
static ssize_t	ip860_mem_read (struct file *file,
				char *buf, size_t len, loff_t *off);
static ssize_t	ip860_mem_write(struct file *file,
				const char *buf, size_t len, loff_t *off);
static loff_t	ip860_mem_lseek(struct file *file, loff_t offset, int orig);
static int	ip860_mem_release(struct inode *, struct file *);
static int	ip860_mem_mmap(struct file *file, struct vm_area_struct *vma);
static int	ip860_mem_ioctl(struct inode *, struct file *,
				unsigned int, unsigned long);


/***************************************************************************
 * Driver interface
 ***************************************************************************
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)

static struct file_operations ip860_mem_fops = {
	ip860_mem_lseek,	/* lseek	*/
	ip860_mem_read,		/* read		*/
	ip860_mem_write,	/* write	*/
	NULL,			/* readdir	*/
	NULL,			/* select	*/
	ip860_mem_ioctl,	/* ioctl	*/
	ip860_mem_mmap,		/* mmap		*/
	ip860_mem_open,		/* open		*/
	NULL,			/* flush	*/
	ip860_mem_release	/* close	*/
};

#else	/* Linux kernel >= 2.3.x */

static struct file_operations ip860_mem_fops = {
	owner:			THIS_MODULE,
	open:			ip860_mem_open,
	release:		ip860_mem_release,
	read:			ip860_mem_read,
	write:			ip860_mem_write,
	llseek:			ip860_mem_lseek,
	ioctl: 			ip860_mem_ioctl,
	mmap:			ip860_mem_mmap,
};
#endif


/***************************************************************************
 *
 * Convert minor number to pointer to device info
 */
vme_mem_t *vme_mem_dev (int minor)
{
	vme_mem_t *mp;

	switch (minor) {
	case VME_MEM_STD:	/* VME Standard Memory			*/
	case VME_MEM_SHORT:	/* VME Short Memory			*/
	case VME_MEM_SRAM:	/* Static RAM				*/
	case VME_MEM_BCSR:	/* Static Board Control & Status	*/
	case VME_MEM_IPMOD:	/* IP Module Memory			*/
		mp = &vme_memory[minor];
		if (mp->name)
			return (mp);
		/* else: empty entry */
		return (NULL);
	default:
		return (NULL);
	}
}

/* Make visible to other drivers */

EXPORT_SYMBOL (vme_mem_dev);

/*
 * export init function so it can be called  by  ip860_rtc_init()  to
 * make sure it has access to the data structures it needs.
 */
int vme_mem_init(void);
EXPORT_SYMBOL (vme_mem_init);

/***************************************************************************
 *
 * Initialize the driver - Register the character device
 */
static int __init ip860_mem_init (void)
{
	return (vme_mem_init());
}

static int vme_mem_init_done;

int __init vme_mem_init(void)
{
	int i;
	volatile immap_t *immr;
	unsigned long val;

	if (vme_mem_init_done)
		return (0);

	printk (KERN_INFO "IP860 VMEBus Memory Driver version " MEM_VERSION "\n");

	/* init CPM ptr */
	asm( "mfspr %0,638": "=r"(val) : );
	val &= 0xFFFF0000;
	immr = (volatile immap_t *)val;

	/*
	 * Register the character device
	 */
	if ((i=register_chrdev(VME_MEM_MAJOR, DEVICE_NAME, &ip860_mem_fops)) < 0) {
		printk("Unable to get major %d for VME_MEM devices: rc=%d\n",
			VME_MEM_MAJOR, i);
		return (i);
	}

	Major = VME_MEM_MAJOR;

	debugk (DEVICE_NAME " registered: major = %d\n", Major);

	for (i=0; i<VME_MAX_MINOR; ++i) {
		debugk ("%s[%d]: init minor %d - \"%s\"\n",
			__FUNCTION__,__LINE__,i,
			vme_memory[i].name ? vme_memory[i].name : "NULL");
		init_mem_reg (immr, &vme_memory[i]);
	}

	return (0);
}

static void init_mem_reg (volatile immap_t *immr, vme_mem_t *mp)
{
	typedef struct mem_ctlr_regs_s {
		uint	br;
		uint	or;
	} mem_ctlr_regs_t;
	volatile mem_ctlr_regs_t *mem_ctlr_regs;
	uint br, or;

	if (mp->name == NULL)		/* unused entry */
		return;
	
	if (mp->cs > 7) {		/* max 8 chip selects on MPC8xx */
		printk ("IP860_MEM: invalid CS # %d for \"%s\" device\n",
			mp->cs, mp->name);
		return;
	}

	mem_ctlr_regs = (mem_ctlr_regs_t *)(&immr->im_memctl);
	br = mem_ctlr_regs[mp->cs].br;
	or = mem_ctlr_regs[mp->cs].or;

	debugk ("BR%d: %08x    OR%d: %08x\n", mp->cs, br, mp->cs, or);

	mp->flags= 0;
	mp->phys = br & BR_BA_MSK;
	mp->size = 1 + ~(or & BR_BA_MSK);
	mp->addr = (unsigned long)ioremap_nocache (mp->phys, mp->size);
	switch (br & BR_PS_MSK) {
	case BR_PS_32:	mp->width = 32;		break;
	case BR_PS_16:	mp->width = 16;		break;
	case BR_PS_8:	mp->width =  8;		break;
	default:	mp->width =  0;		break;
	}

	debugk ("phys %08lx    virt %08lx    "
		"size %6ld kB = %3ld MB   width = %2ld bit\n",
		mp->phys, mp->addr, mp->size>>10, mp->size>>20, mp->width);
}


/***************************************************************************
 *
 * called whenever a process attempts to open the device
 */
static int ip860_mem_open (struct inode *inode, struct file *file)
{
	int minor;
	vme_mem_t *mp;

	minor = MINOR(inode->i_rdev);

	debugk ("%s: minor %d\n", __FUNCTION__, minor);

	if ((mp = vme_mem_dev(minor)) == NULL) {
		return -ENXIO;
	}

	/*
	 * exclusive open only
	 */
	if (mp->flags & IP860_FLAGS_BUSY) {
		return -EBUSY;
	}

	mp->flags |= IP860_FLAGS_BUSY;

	/*
	 * Make sure that the module isn't removed while
	 * the file is open by incrementing the usage count
	 */
	MOD_INC_USE_COUNT;

	debugk ("%s: \"%s\" done\n", __FUNCTION__, mp->name);

	return 0;
}



/***************************************************************************
 * 
 * Called when a process closes the device.
 */
static int ip860_mem_release (struct inode *inode, struct file *file)
{
	int minor;
	vme_mem_t *mp;

	minor = MINOR(inode->i_rdev);

	debugk ("%s: minor %d\n", __FUNCTION__, minor);

	if ((mp = vme_mem_dev(minor)) == NULL) {
		return -ENXIO;
	}

	/* We're now ready for our next caller */
	mp->flags &= ~IP860_FLAGS_BUSY;

	MOD_DEC_USE_COUNT;

	return 0;
}


/***************************************************************************
 * 
 */
static loff_t ip860_mem_lseek(struct file *file, loff_t offset, int orig)
{
	int minor;
	vme_mem_t *mp;

	minor = MINOR(file->f_dentry->d_inode->i_rdev);

	debugk ("%s: minor %d\n", __FUNCTION__, minor);

	if ((mp = vme_mem_dev(minor)) == NULL) {
		return -ENXIO;
	}

	switch (orig) {
	case 0:
		if (offset > mp->size)
			return -EINVAL;
		file->f_pos = offset;
		return file->f_pos;
	case 1:
		if (file->f_pos + offset > mp->size)
			return -EINVAL;
		file->f_pos += offset;
		return file->f_pos;
	default:
		return -EINVAL;
	}
}


/***************************************************************************
 * 
 */
static ssize_t
ip860_mem_read(struct file *file, char *buf, size_t len, loff_t *off)
{
	int minor, error;
	ulong offset = *off;
	uint count;
	vme_mem_t *mp;

	minor = MINOR(file->f_dentry->d_inode->i_rdev);

	debugk ("%s: minor %d len %d off %#x\n",
		__FUNCTION__, minor, len, (unsigned int)*off);

	/* POSIX: mtime/ctime may not change for 0 count */
	if (!len)
		return 0;

	if ((mp = vme_mem_dev(minor)) == NULL) {
		return -ENXIO;
	}

	/*
	 * Check that the file was opened for reading ?
	 * Seems to be done in sys_read.
	 */
	if (!(file->f_mode & FMODE_READ)) {
		debugk ("%s: minor %d (%s) not readable\n",
			__FUNCTION__, minor, mp->name);
		return -EBADF;
	}
	
	/*
	 * make sure that the user isn't trying to read past end of memory
	 */
	if (offset > mp->size) {
		debugk ("%s: minor %d (%s) off %08lx > size %08lx\n",
			__FUNCTION__, minor, mp->name, offset, mp->size);
		return -EINVAL;
	}

	count = mp->size - (unsigned int)offset;
	if (count > len)
		count = (unsigned int)len;
	
	/* Copy out */
	if ((error = verify_area(VERIFY_WRITE, buf, count)) != 0) {
		printk("%s: (%s) verify_area returned %d\n",
			__FUNCTION__, mp->name, error);
		return error;
	}

	copy_to_user((void *)buf, (void*) (mp->addr + offset),  count);
	file->f_pos += count;

	return count;
}


/***************************************************************************
 * 
 */
static ssize_t
ip860_mem_write(struct file *file, const char *buf, size_t len, loff_t *off)
{
	int minor, error;
	ulong offset = *off;
	uint count;
	vme_mem_t *mp;
	minor = MINOR(file->f_dentry->d_inode->i_rdev);

	debugk ("%s: minor %d len %d off %lx\n", 
		__FUNCTION__, minor, len, offset);

	/* POSIX: mtime/ctime may not change for 0 count */
	if (!len)
		return 0;

	if ((mp = vme_mem_dev(minor)) == NULL) {
		return -ENXIO;
	}

	/*
	 * Check that the file was opened for writing ?
	 * Seems to be done in sys_write.
	 */
	if (!(file->f_mode & FMODE_WRITE)) {
		debugk ("%s: minor %d (%s) not writeable\n",
			__FUNCTION__, minor, mp->name);
		return -EBADF;
	}
	/*
	 * make sure that the user isn't trying to write past end of memory
	 */
	if (offset >= mp->size) {
		debugk ("ip860_mem_write: minor %d off %08lx > size %08lx\n",
			minor, offset, mp->size);
		return (offset == mp->size) ? -ENOSPC : -EINVAL;
	}

	count = mp->size - (unsigned int)offset;
	if (count > len)
		count = (unsigned int)len;

	/* Copy in */
	if ((error = verify_area(VERIFY_READ, (const void *) buf, count)) != 0) {
		printk("%s: (%s) verify_area returned %d\n",
			__FUNCTION__, mp->name, error);
		return error;
	}

	copy_from_user((void *)(mp->addr + offset), (const void*) buf, count);
	file->f_pos += count;

	return count;
}

/***************************************************************************
 * 
 * ioctl entry point:
 */
static int ip860_mem_ioctl (struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	int minor;

	minor = MINOR(file->f_dentry->d_inode->i_rdev);

	debugk ("IOCTL: minor %d, cmd=%x, arg=%ld\n", minor, cmd, arg);

	return (-EINVAL);
}


/***************************************************************************
 * mmap entry point
 ***************************************************************************
 */
static int ip860_mem_mmap(struct file *file, struct vm_area_struct *vma)
{
	int minor, size;
	unsigned long error;
	unsigned long offset;
	vme_mem_t *mp;

	minor = MINOR(file->f_dentry->d_inode->i_rdev);

	debugk ("%s: minor %d\n",__FUNCTION__,minor);

	if ((mp = vme_mem_dev(minor)) == NULL) {
		return -ENXIO;
	}

	size   = vma->vm_end - vma->vm_start;
	offset = vma->vm_pgoff << PAGE_SHIFT;

	if (size > mp->size) {
		debugk ("%s: minor %d (%s) bad size %d > %ld\n",
			__FUNCTION__, minor, mp->name, size, mp->size);
		return -EFAULT;
	}

	pgprot_val(vma->vm_page_prot) |= _PAGE_NO_CACHE|_PAGE_GUARDED;

	/* remapping */
	if ((error = io_remap_page_range(vma->vm_start, mp->phys+offset, size,
				      vma->vm_page_prot)) != 0) {
		printk ("%s: minor %d (%s) remap_page_range() error %ld\n",
			__FUNCTION__, minor, mp->name, error);
		return error;
	}

	return 0;
}


/***************************************************************************
 * Module Declarations
 ***************************************************************************
 */

module_init (ip860_mem_init);

#ifdef MODULE
/*
 * Cleanup - unregister the appropriate file from /proc
 */
void ip860_mem_cleanup (void)
{
	int i, rc;

	for (i=0; i<VME_MAX_MINOR; ++i) {
		debugk ("%s[%d]: cleanup minor %d - \"%s\"\n",
			__FUNCTION__,__LINE__,i,
			vme_memory[i].name ? vme_memory[i].name : "NULL");
		if (vme_memory[i].name == NULL)
			continue;
		iounmap ((void *)vme_memory[i].addr);
		vme_memory[i].phys  = 0;
		vme_memory[i].addr  = 0;
		vme_memory[i].size  = 0;
		vme_memory[i].width = 0;
		vme_memory[i].flags = 0;
	}

	/*
	 * Unregister the device
	 */
	rc = unregister_chrdev (Major, DEVICE_NAME);

	/*
	 * If there's an error, report it
	 */
	if (rc < 0) {
		printk (DEVICE_NAME "unregister_chrdev: error %d\n", rc);
	}
}

module_exit (ip860_mem_cleanup);

#endif	/* MODULE */

