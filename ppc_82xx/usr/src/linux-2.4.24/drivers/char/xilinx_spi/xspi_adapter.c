/*
 * xspi_adapter.c
 * 
 * Xilinx Adapter component to interface SPI component to Linux
 *
 * Only master mode is supported. One or more slaves can be served.
 *
 * Author: MontaVista Software, Inc.
 *         akonovalov@ru.mvista.com, or source@mvista.com
 *
 * 2004 (c) MontaVista, Software, Inc.  This file is licensed under the terms
 * of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>	/* wait_event_interruptible */
#include <linux/bitops.h>	/* ffs() */
#include <linux/slab.h>		/* kmalloc() etc. */
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/page.h>		/* PAGE_SIZE */

#include "xparameters.h"
#include "xspi.h"
#include "xspi_i.h"
#include "xspi_ioctl.h"

#include <linux/devfs_fs_kernel.h>

MODULE_AUTHOR("MontaVista Software, Inc. <source@mvista.com>");
MODULE_DESCRIPTION("Xilinx SPI driver");
MODULE_LICENSE("GPL");

/* Dynamically allocate a major number. */
static int xspi_major = 0;
MODULE_PARM(xspi_major, "i");

#define XSPI_NAME "xilinx_spi"

/*
 * Debugging macros
 */

#define DEBUG_FLOW   0x0001
#define DEBUG_STAT   0x0002

#define DEBUG_MASK   0x0000

#if (DEBUG_MASK != 0)
#define d_printk(str...)  printk(str)
#else
#define d_printk(str...)	/* nothing */
#endif

#if ((DEBUG_MASK & DEBUG_FLOW) != 0)
#define func_enter()      printk("xspi: enter %s\n", __FUNCTION__)
#define func_exit()       printk("xspi: exit  %s\n", __FUNCTION__)
#else
#define func_enter()
#define func_exit()
#endif

/* These options are always set by the driver. */
#define XSPI_DEFAULT_OPTIONS	(XSP_MASTER_OPTION | XSP_MANUAL_SSELECT_OPTION)
/* These options can be changed by the user. */
#define XSPI_CHANGEABLE_OPTIONS	(XSP_CLK_ACTIVE_LOW_OPTION | XSP_CLK_PHASE_1_OPTION \
				| XSP_LOOPBACK_OPTION)

/* Our private per interface data. */
struct xspi_instance {
	u32 save_BaseAddress;	/* Saved physical base address */
	devfs_handle_t devfs_handle;
	wait_queue_head_t waitq;	/* For those waiting until SPI is busy */
	struct semaphore sem;
	int use_count;

	/* The flag ISR uses to tell the transfer completion status
	 * (the values are defined in "xstatus.h"; set to 0 before the transfer) */
	int completion_status;
	/* The actual number of bytes transferred */
	int tx_count;

	/* The object used by Xilinx OS independent code */
	XSpi Spi;

	/* Device initialization status */
	unsigned int remapped:1;
};

static struct xspi_instance xspi_drv_data[XPAR_XSPI_NUM_INSTANCES];

/*
 * Xilinx configuration stuff
 */

/* SAATODO: This function will be moved into the Xilinx code. */
XSpi_Config *
XSpi_GetConfig(int Instance)
{
	if (Instance < 0 || Instance >= XPAR_XSPI_NUM_INSTANCES) {
		return NULL;
	}

	return &XSpi_ConfigTable[Instance];
}

/* As XSpi_GetConfig() can't tell us what IRQ is used by
 * each SPI instance, we have to refer xparameters.h directly.*/
static const int xspi_irq[XPAR_XSPI_NUM_INSTANCES] = {
	31 - XPAR_INTC_0_SPI_0_VEC_ID,
#ifdef XPAR_SPI_1_BASEADDR
	31 - XPAR_INTC_0_SPI_1_VEC_ID,
#ifdef XPAR_SPI_2_BASEADDR
	31 - XPAR_INTC_0_SPI_2_VEC_ID,
#ifdef XPAR_SPI_3_BASEADDR
	31 - XPAR_INTC_0_SPI_3_VEC_ID,
#ifdef XPAR_SPI_4_BASEADDR
#error Edit this file to add more devices.
#endif				/* 4 */
#endif				/* 3 */
#endif				/* 2 */
#endif				/* 1 */
};

static int
convert_status(XStatus status)
{
	switch (status) {
	case XST_SUCCESS:
		return 0;
	case XST_DEVICE_NOT_FOUND:
		return -ENODEV;
	case XST_DEVICE_BUSY:
		return -EBUSY;
	default:
		return -EIO;
	}
}

/*
 * Simple function that hands an interrupt to the Xilinx code.
 * dev_id contains a pointer to proper XSpi instance.
 */
static void
xspi_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	XSpi_InterruptHandler((XSpi *) dev_id);
}

/*
 * This function is called back from the XSpi interrupt handler
 * when one of the following status events occures:
 * 	XST_SPI_TRANSFER_DONE - the requested data transfer is done,
 * 	XST_SPI_RECEIVE_OVERRUN - Rx FIFO overrun, transmission continues,
 * 	XST_SPI_MODE_FAULT - should not happen: the driver doesn't support multiple masters,
 * 	XST_SPI_TRANSMIT_UNDERRUN,
 * 	XST_SPI_SLAVE_MODE_FAULT - should not happen: the driver doesn't support slave mode.
 */
static void
xspi_status_handler(void *CallBackRef, u32 StatusEvent, unsigned int ByteCount)
{
	struct xspi_instance *dev = (struct xspi_instance *) CallBackRef;
	int index = dev - &xspi_drv_data[0];

	dev->completion_status = StatusEvent;

	if (StatusEvent == XST_SPI_TRANSFER_DONE) {
		dev->tx_count = (int) ByteCount;
		wake_up_interruptible(&dev->waitq);
	} else if (StatusEvent == XST_SPI_RECEIVE_OVERRUN) {
		/* As both Rx and Tx FIFO have the same sizes
		   this should not happen in master mode.
		   That is why we consider Rx overrun as severe error
		   and abort the transfer */
		dev->tx_count = (int) ByteCount;
		XSpi_Abort(&dev->Spi);
		wake_up_interruptible(&dev->waitq);
		printk(KERN_ERR XSPI_NAME "%d: Rx overrun!!!.\n", index);
	} else if (StatusEvent == XST_SPI_MODE_FAULT) {
		wake_up_interruptible(&dev->waitq);
	} else {
		printk(KERN_ERR XSPI_NAME "%d: Invalid status event %u.\n",
		       index, StatusEvent);
	}
}

/*
 * To be called from xspi_ioctl(), xspi_read(), and xspi_write().
 *
 * xspi_ioctl() uses both wr_buf and rd_buf.
 * xspi_read() doesn't care of what is sent, and sets wr_buf to NULL.
 * xspi_write() doesn't care of what it receives, and sets rd_buf to NULL.
 *
 * Set slave_ind to negative value if the currently selected SPI slave
 * device is to be used.
 * 
 * Returns the number of bytes transferred (0 or positive value)
 * or error code (negative value).
 */
static int
xspi_transfer(struct xspi_instance *dev, const char *wr_buf, char *rd_buf,
	      int count, int slave_ind)
{
	int retval;
	unsigned char *tmp_buf;

	if (count <= 0)
		return 0;

	/* Limit the count value to the small enough one.
	   This prevents a denial-of-service attack by using huge count values
	   thus making everything to be swapped out to free the space
	   for this huge buffer */
	if (count > 8192)
		count = 8192;

	/* Allocate buffer in the kernel space (it is first filled with
	   the data to send, then these data are overwritten with the
	   received data) */
	tmp_buf = kmalloc(count, GFP_KERNEL);
	if (tmp_buf == NULL)
		return -ENOMEM;

	/* Fill the buffer with data to send */
	if (wr_buf == NULL) {
		/* zero the buffer not to expose the kernel data */
		memset(tmp_buf, 0, count);
	} else {
		if (copy_from_user(tmp_buf, wr_buf, count) != 0) {
			kfree(tmp_buf);
			return -EFAULT;
		}
	}

	/* Lock the device */
	if (down_interruptible(&dev->sem)) {
		kfree(tmp_buf);
		return -ERESTARTSYS;
	}

	/* The while cycle below never loops - this is just a convenient
	   way to handle the errors */
	while (TRUE) {
		/* Select the proper slave if requested to do so */
		if (slave_ind >= 0) {
			retval =
			    convert_status(XSpi_SetSlaveSelect
					   (&dev->Spi,
					    0x00000001 << slave_ind));
			if (retval != 0)
				break;
		}

		/* Initiate transfer */
		dev->completion_status = 0;
		retval = convert_status(XSpi_Transfer(&dev->Spi, tmp_buf,
						      (rd_buf ==
						       NULL) ? NULL : tmp_buf,
						      count));
		if (retval != 0)
			break;

		/* Put the process to sleep */
		if (wait_event_interruptible(dev->waitq,
					     dev->completion_status != 0) !=
		    0) {
			/* ... woken up by the signal */
			retval = -ERESTARTSYS;
			break;
		}
		/* ... woken up by the transfer completed interrupt */
		if (dev->completion_status != XST_SPI_TRANSFER_DONE) {
			retval = -EIO;
			break;
		}

		/* Copy the received data to user if rd_buf != NULL */
		if (rd_buf != NULL &&
		    copy_to_user(rd_buf, tmp_buf, dev->tx_count) != 0) {
			retval = -EFAULT;
			break;
		}

		retval = dev->tx_count;
		break;
	}			/* while(TRUE) */

	/* Unlock the device, free the buffer and return */
	up(&dev->sem);
	kfree(tmp_buf);
	return retval;
}

static int
xspi_ioctl(struct inode *inode, struct file *filp,
	   unsigned int cmd, unsigned long arg)
{
	struct xspi_instance *dev = filp->private_data;

	/* paranoia check */
	if (dev == NULL || dev->remapped == 0) {
		return -ENODEV;
	}

	switch (cmd) {
	case XSPI_IOC_GETSLAVESELECT:
		{
			int i;

			i = ffs(XSpi_GetSlaveSelect(&dev->Spi)) - 1;
			return put_user(i, (int *) arg);	/* -1 means nothing selected */
		}
		break;
	case XSPI_IOC_SETSLAVESELECT:
		{
			int i;
			int retval;

			if (get_user(i, (int *) arg) != 0)
				return -EFAULT;

			if (i < -1 || i > 31)
				return -EINVAL;

			/* Lock the device. */
			if (down_interruptible(&dev->sem))
				return -ERESTARTSYS;

			if (i == -1)
				retval =
				    convert_status(XSpi_SetSlaveSelect
						   (&dev->Spi, 0));
			else
				retval =
				    convert_status(XSpi_SetSlaveSelect
						   (&dev->Spi, (u32) 1 << i));

			/* Unlock the device. */
			up(&dev->sem);

			return retval;
		}
		break;
	case XSPI_IOC_GETOPTS:
		{
			int index = dev - &xspi_drv_data[0];
			struct xspi_ioc_options xspi_opts;
			u32 xspi_options;
			XSpi_Config *cfg;

			xspi_options = XSpi_GetOptions(&dev->Spi);
			cfg = XSpi_GetConfig(index);

			memset(&xspi_opts, 0, sizeof (xspi_opts));
			if (cfg->HasFifos)
				xspi_opts.has_fifo = 1;
			if (xspi_options & XSP_CLK_ACTIVE_LOW_OPTION)
				xspi_opts.clk_level = 1;
			if (xspi_options & XSP_CLK_PHASE_1_OPTION)
				xspi_opts.clk_phase = 1;
			if (xspi_options & XSP_LOOPBACK_OPTION)
				xspi_opts.loopback = 1;
			xspi_opts.slave_selects = cfg->NumSlaveBits;

			return put_user(xspi_opts,
					(struct xspi_ioc_options *) arg);
		}
		break;
	case XSPI_IOC_SETOPTS:
		{
			struct xspi_ioc_options xspi_opts;
			u32 xspi_options;
			int retval;

			if (copy_from_user(&xspi_opts,
					   (struct xspi_ioc_options *) arg,
					   sizeof (struct xspi_ioc_options)) !=
			    0)
				return -EFAULT;

			/* Lock the device. */
			if (down_interruptible(&dev->sem))
				return -ERESTARTSYS;

			/* Read current settings and set the changeable ones. */
			xspi_options = XSpi_GetOptions(&dev->Spi)
			    & ~XSPI_CHANGEABLE_OPTIONS;
			if (xspi_opts.clk_level != 0)
				xspi_options |= XSP_CLK_ACTIVE_LOW_OPTION;
			if (xspi_opts.clk_phase != 0)
				xspi_options |= XSP_CLK_PHASE_1_OPTION;
			if (xspi_opts.loopback != 0)
				xspi_options |= XSP_LOOPBACK_OPTION;

			retval =
			    convert_status(XSpi_SetOptions
					   (&dev->Spi, xspi_options));

			/* Unlock the device. */
			up(&dev->sem);

			return retval;
		}
		break;
	case XSPI_IOC_TRANSFER:
		{
			struct xspi_ioc_transfer_data trans_data;
			int retval;

			if (copy_from_user(&trans_data,
					   (struct xspi_ioc_transfer_data *)
					   arg,
					   sizeof (struct
						   xspi_ioc_transfer_data)) !=
			    0)
				return -EFAULT;

			/* Transfer the data. */
			retval = xspi_transfer(dev, trans_data.write_buf,
					       trans_data.read_buf,
					       trans_data.count,
					       trans_data.slave_index);
			if (retval > 0)
				return 0;
			else
				return retval;
		}
		break;
	default:
		return -ENOTTY;	/* redundant */
	}			/* switch(cmd) */

	return -ENOTTY;
}

static ssize_t
xspi_read(struct file *filp, char *buf, size_t count, loff_t * not_used)
{
	struct xspi_instance *dev = filp->private_data;

	/* Set the 2nd arg to NULL to indicate we don't care what to send;
	   set the last arg to -1 to talk to the currently selected SPI
	   slave */
	return xspi_transfer(dev, NULL, buf, count, -1);
}

static ssize_t
xspi_write(struct file *filp, const char *buf, size_t count, loff_t * not_used)
{
	struct xspi_instance *dev = filp->private_data;

	/* Set the 3d arg to NULL to indicate we are not interested in
	   the data read; set the last arg to -1 to talk to the currently
	   selected SPI slave */
	return xspi_transfer(dev, buf, NULL, count, -1);
}

static int
xspi_open(struct inode *inode, struct file *filp)
{
	int retval = 0;
	struct xspi_instance *dev = filp->private_data;
	int index;

	func_enter();

	/* If dev is NULL, devfs must not be being used; find the right dev. */
	if (dev == NULL) {
		int minor;

		minor = MINOR(inode->i_rdev);
		if (minor < 0 || minor >= XPAR_XSPI_NUM_INSTANCES) {
			return -ENODEV;
		}
		dev = &xspi_drv_data[minor];
		filp->private_data = dev;
	}

	if (dev->remapped == 0)
		return -ENODEV;

	index = dev - &xspi_drv_data[0];

	if (down_interruptible(&dev->sem))
		return -EINTR;

	while (dev->use_count++ == 0) {
		/*
		 * This was the first opener; we need  to get the IRQ,
		 * and to setup the device as master.
		 */
		retval = request_irq(xspi_irq[index], xspi_isr, 0, XSPI_NAME,
				     &dev->Spi);
		if (retval != 0) {
			printk(KERN_ERR XSPI_NAME
			       "%d: Could not allocate interrupt %d.\n",
			       index, xspi_irq[index]);
			break;
		}

		if (XSpi_SetOptions(&dev->Spi, XSPI_DEFAULT_OPTIONS) !=
		    XST_SUCCESS) {
			printk(KERN_ERR XSPI_NAME
			       "%d: Could not set device options.\n", index);
			free_irq(xspi_irq[index], &dev->Spi);
			retval = -EIO;
			break;
		}

		if (XSpi_Start(&dev->Spi) != XST_SUCCESS) {
			printk(KERN_ERR XSPI_NAME
			       "%d: Could not start the device.\n", index);
			free_irq(xspi_irq[index], &dev->Spi);
			retval = -EIO;
			break;
		}

		break;
	}

	if (retval != 0)
		--dev->use_count;
	else
		MOD_INC_USE_COUNT;

	up(&dev->sem);
	return retval;
}

static int
xspi_release(struct inode *inode, struct file *filp)
{
	struct xspi_instance *dev = filp->private_data;
	int index = dev - &xspi_drv_data[0];

	func_enter();

	if (down_interruptible(&dev->sem))
		return -EINTR;

	if (--dev->use_count == 0) {
		/* This was the last closer: stop the device and free the IRQ */
		if (wait_event_interruptible(dev->waitq,
					     XSpi_Stop(&dev->Spi) !=
					     XST_DEVICE_BUSY) != 0) {
			/* Abort transfer by brute force */
			XSpi_Abort(&dev->Spi);
		}
		disable_irq(xspi_irq[index]);
		free_irq(xspi_irq[index], &dev->Spi);
	}

	MOD_DEC_USE_COUNT;

	up(&dev->sem);
	return 0;
}

struct file_operations xspi_fops = {
      open:xspi_open,
      release:xspi_release,
      read:xspi_read,
      write:xspi_write,
      ioctl:xspi_ioctl
};

devfs_handle_t devfs_dir;

static int __init
check_spi_config(XSpi_Config * cfg)
{
	if (cfg->SlaveOnly || cfg->NumSlaveBits == 0)
		return -1;
	else
		return 0;	/* the configuration is supported by this driver */
}

static void
cleanup_instance(int i)
{
	if (xspi_drv_data[i].devfs_handle) {
		devfs_unregister(xspi_drv_data[i].devfs_handle);
		xspi_drv_data[i].devfs_handle = NULL;
	}

	if (xspi_drv_data[i].remapped == 1) {
		XSpi_Config *cfg;

		xspi_drv_data[i].remapped = 0;
		cfg = XSpi_GetConfig(i);
		iounmap((void *) cfg->BaseAddress);
		cfg->BaseAddress = xspi_drv_data[i].save_BaseAddress;
	}
}

static void __exit
xilinx_spi_cleanup(void)
{
	int i;

	/* xilinx_spi_cleanup is never called if registering failed */
	devfs_unregister_chrdev(xspi_major, XSPI_NAME);

	for (i = 0; i < XPAR_XSPI_NUM_INSTANCES; i++) {
		cleanup_instance(i);
	}

	if (devfs_dir) {
		devfs_unregister(devfs_dir);
		devfs_dir = NULL;
	}
}

static int __init
xilinx_spi_init(void)
{
	int i;
	int retval;
	char name[8];

	/* If we have devfs, create /dev/xilinx_spi to put files in there */
	devfs_dir = devfs_mk_dir(NULL, XSPI_NAME, NULL);

	/* Register the major, and accept a dynamic number */
	retval = devfs_register_chrdev(xspi_major, XSPI_NAME, &xspi_fops);
	if (retval < 0) {
		printk(KERN_ERR XSPI_NAME ": failed to get major %d\n",
		       xspi_major);
		return retval;
	}
	if (xspi_major == 0)
		xspi_major = retval;
	printk(XSPI_NAME ": got major number %d\n", xspi_major);

	/* initialize all the SPIs present */
	for (i = 0; i < XPAR_XSPI_NUM_INSTANCES; i++) {
		XSpi_Config *cfg;

		/* Find the config for our device. */
		cfg = XSpi_GetConfig(i);
		if (cfg == NULL || check_spi_config(cfg) != 0) {
			break;
		}

		/* Initialize driver's data. */
		init_waitqueue_head(&xspi_drv_data[i].waitq);
		sema_init(&xspi_drv_data[i].sem, 1);

		/*
		 * Change the addresses to be virtual;
		 * save the old ones to restore.
		 */
		xspi_drv_data[i].save_BaseAddress = cfg->BaseAddress;
		cfg->BaseAddress =
		    (u32) ioremap(xspi_drv_data[i].save_BaseAddress,
				  XPAR_SPI_0_HIGHADDR + 1 -
				  XPAR_SPI_0_BASEADDR);
		xspi_drv_data[i].remapped = 1;

		/* Initialize SPI */
		if (XSpi_Initialize(&xspi_drv_data[i].Spi, i) != XST_SUCCESS) {
			printk(KERN_ERR XSPI_NAME
			       "%d: Could not initialize device.\n", i);
			cleanup_instance(i);
			break;
		}

		/* Set interrupt callback */
		XSpi_SetStatusHandler(&xspi_drv_data[i].Spi, &xspi_drv_data[i],
				      xspi_status_handler);
		/* IRQ is assigned in open() */

		sprintf(name, "%d", i);
		xspi_drv_data[i].devfs_handle = devfs_register(devfs_dir, name,
							       DEVFS_FL_DEFAULT,
							       xspi_major, i,
							       S_IFCHR | S_IRUGO
							       | S_IWUGO,
							       &xspi_fops,
							       &xspi_drv_data
							       [i]);

		printk(KERN_INFO XSPI_NAME
		       "%d at 0x%08X mapped to 0x%08X, irq=%d\n", i,
		       xspi_drv_data[i].save_BaseAddress, cfg->BaseAddress,
		       xspi_irq[i]);
	}

	if (i == 0) {
		xilinx_spi_cleanup();
		return -ENODEV;
	}

	return 0;
}

EXPORT_NO_SYMBOLS;

module_init(xilinx_spi_init);
module_exit(xilinx_spi_cleanup);
