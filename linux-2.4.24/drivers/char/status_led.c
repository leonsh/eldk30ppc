
/***********************************************************************
 *
 * (C) Copyright 1999, 2000, 2001
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 *
 ***********************************************************************/

/*
 * Standard in kernel modules
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <asm/types.h>
#include <asm/uaccess.h>
#include <asm/8xx_immap.h>
#include <asm/mpc8xx.h>

// hwang, 10/14/04
#include <asm-ppc/status_led.h>

#undef	DEBUG

#ifdef	DEBUG
# define debugk(fmt,args...)	printk(fmt ,##args)
#else
# define debugk(fmt,args...)
#endif

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

#if defined(CONFIG_IVMS8) || defined(CONFIG_IVML24)
static DECLARE_WAIT_QUEUE_HEAD(statusled_wait);
static char last_io_state;
static volatile char changed_io_state;
static spinlock_t statusled_lock;
static struct timer_list io_timer;
static void status_io_poll (unsigned long period);
static int  status_io_test (void);
#endif	/* CONFIG_IVMS8, CONFIG_IVML24 */

#define	LED_MAJOR	151	/* Reserverd for "Front panel LEDs" */

#define DRIVER_VERSION	"$Revision: 1.0 $"

static void statusled_blink (unsigned long minor);

/*
 * Device Declarations
 */

typedef struct {
	unsigned long		mask;
	int			state;
	int			period;
	int			flags;
	struct timer_list	timer;
} led_dev_t;

#define	SL_FLAG_OPEN	1

led_dev_t led_dev[] = {
    {	STATUS_LED_BIT,
	STATUS_LED_STATE,
	STATUS_LED_PERIOD,
	0,
	{ function: statusled_blink, data: 0UL /* minor 0 */ },
    },
#if defined(STATUS_LED_BIT1)
    {	STATUS_LED_BIT1,
	STATUS_LED_STATE1,
	STATUS_LED_PERIOD1,
	0,
	{ function: statusled_blink, data: 1UL /* minor 1 */ },
    },
#endif	/* STATUS_LED_BIT1 */
#if defined(STATUS_LED_BIT2)
    {	STATUS_LED_BIT2,
	STATUS_LED_STATE2,
	STATUS_LED_PERIOD2,
	0,
	{ function: statusled_blink, data: 2UL /* minor 2 */ },
    },
#endif	/* STATUS_LED_BIT2 */
};

#define	MAX_LED_DEV	(sizeof(led_dev)/sizeof(led_dev_t))

/*
 * On the IVM* we handle 2 additional devices with this driver:
 * MAX_LED_DEV+0 => "Interlock Switch" and "Device Reset Monitor":
 *			Both are input pins, which are polled periodically,
 *			and the user can wait with select() for a status change.
 * MAX_LED_DEV+1 => "Device Reset Enable", an output device we can set or reset.
 */
#if defined(CONFIG_IVMS8) || defined(CONFIG_IVML24)
# define MAX_IO_DEV	2
static int io_dev_flags[MAX_IO_DEV] = { 0, };	/* additional I/O devices */
#else
# define MAX_IO_DEV	0
#endif	/* CONFIG_IVMS8, CONFIG_IVML24 */


static volatile immap_t *immr = NULL;

/*
 * The name for our device, as it will appear in /proc/devices
 */
#define DEVICE_NAME "status_led"

/*
 * Prototypes
 */
static int statusled_init (void) __init;
static int statusled_open(struct inode *, struct file *);
static int statusled_release(struct inode *, struct file *);
# if defined(CONFIG_IVMS8) || defined(CONFIG_IVML24)
static ssize_t statusled_read(struct file *, char *, size_t, loff_t *);
static unsigned int statusled_poll (struct file *, poll_table *);
# endif	/* CONFIG_IVMS8, CONFIG_IVML24 */
int init_module(void);
void cleanup_module(void);

static ssize_t statusled_ioctl(struct inode *, struct file *,
			    unsigned int, unsigned long);

static struct file_operations statusled_fops = {
	owner:		THIS_MODULE,
	open:		statusled_open,
	release:	statusled_release,
	ioctl: 		statusled_ioctl,
# if defined(CONFIG_IVMS8) || defined(CONFIG_IVML24)
	read:		statusled_read,
	poll:		statusled_poll,
# endif	/* CONFIG_IVMS8, CONFIG_IVML24 */
};

static int Major;

/*
 * Initialize the driver - Register the character device
 */
static int __init statusled_init (void)
{
	int i;

	if (!immr) {		/* init CPM ptr */
		unsigned long val;

		asm( "mfspr %0,638": "=r"(val) : );
		val &= 0xFFFF0000;
		immr = (volatile immap_t *)val;
	}

	/*
	 * Register the character device
	 */
	if ((i = register_chrdev(LED_MAJOR, DEVICE_NAME, &statusled_fops)) < 0) {
		immr = NULL;
		printk("Unable to get major %d for status LED driver: rc=%d\n",
			LED_MAJOR, i);
		return (i);
	}

	Major = LED_MAJOR;

	printk (KERN_INFO
		"Status LED driver " DRIVER_VERSION " initialized\n");

	for (i=0; i<MAX_LED_DEV; ++i) {
		immr->STATUS_LED_PAR &= ~(led_dev[i].mask);
#ifdef STATUS_LED_ODR
		immr->STATUS_LED_ODR &= ~(led_dev[i].mask);
#endif
		if (led_dev[i].state == STATUS_LED_ON) {
			#if (STATUS_LED_ACTIVE == 0)	/* start with LED on */
			immr->STATUS_LED_DAT &= ~(led_dev[i].mask);
			#else
			immr->STATUS_LED_DAT |=   led_dev[i].mask ;
			#endif
		} else {
			#if (STATUS_LED_ACTIVE == 0)	/* start with LED off */
			immr->STATUS_LED_DAT |=   led_dev[i].mask ;
			#else
			immr->STATUS_LED_DAT &= ~(led_dev[i].mask);
			#endif
		}
		immr->STATUS_LED_DIR |= led_dev[i].mask;

		statusled_blink(i);
	}
#if defined(CONFIG_IVML24)
	/*
	 * Configure interlock switch port for input
	 */
	immr->im_cpm.cp_pbpar &= ~(STATUS_ILOCK_SWITCH);
	immr->im_cpm.cp_pbodr &= ~(STATUS_ILOCK_SWITCH);
	immr->im_cpm.cp_pbdir &= ~(STATUS_ILOCK_SWITCH);
	
#endif	/* CONFIG_IVML24 */

#if defined(CONFIG_IVMS8) || defined(CONFIG_IVML24)
	/*
	 * Configure device reset monitor port for input
	 */
	immr->im_ioport.iop_pcpar &= ~(STATUS_RESET_MON);
	immr->im_ioport.iop_pcso  &= ~(STATUS_RESET_MON);
	immr->im_ioport.iop_pcdir &= ~(STATUS_RESET_MON);
	/*
	 * Configure device reset enable port for output
	 * Initialize with low (0) level
	 */
	immr->im_ioport.iop_pcpar &= ~(STATUS_RESET_ENA);
	immr->im_ioport.iop_pcso  &= ~(STATUS_RESET_ENA);
	immr->im_ioport.iop_pcdat &= ~(STATUS_RESET_ENA);	/* set 0  */
	immr->im_ioport.iop_pcdir |=   STATUS_RESET_ENA ;	/* output */
	/*
	 * We use the same function (originally for the ILOCK device only)
	 * to poll both the ILOCK and RESET_MON ports
	 */
	io_timer.function = status_io_poll;
	io_timer.data     = STATUS_ILOCK_PERIOD;

	last_io_state = (char)status_io_test();
	changed_io_state = 0;

	init_timer(&io_timer);
	io_timer.expires = jiffies + io_timer.data;
	add_timer(&io_timer);
#endif	/* CONFIG_IVMS8, CONFIG_IVML24 */

	return (0);
}

/*
 * called whenever a process attempts to open the device
 */
static int statusled_open (struct inode *inode, struct file *file)
{
	int minor  = MINOR(inode->i_rdev);

	debugk ("statusled_open(%p,%p): minor %d\n", inode, file, minor);

	/*
	 * Allow for MAX_LED_DEV status LEDs
	 * On IVMS8 and IVML24: allow for additional I/O devices
	 */
	if (minor >= MAX_LED_DEV + MAX_IO_DEV)
		return (-ENXIO);

	if (!immr)
		return (-ENODEV);

	/*
	 * exclusive open only
	 */
	if (minor < MAX_LED_DEV) {
		if (led_dev[minor].flags & SL_FLAG_OPEN)
			return -EBUSY;
		led_dev[minor].flags |= SL_FLAG_OPEN;
#if defined(CONFIG_IVMS8) || defined(CONFIG_IVML24)
	} else {
		int m = minor - MAX_LED_DEV;

		if (io_dev_flags[m] & SL_FLAG_OPEN)
			return -EBUSY;
		io_dev_flags[m] |= SL_FLAG_OPEN;
#endif	/* CONFIG_IVMS8, CONFIG_IVML24 */
	}

	/*
	 * Make sure that the module isn't removed while
	 * the file is open by incrementing the usage count
	 */
	MOD_INC_USE_COUNT;

	debugk ("LED_OPEN: minor %d  dir=0x%x  par=0x%x  odr=0x%x  dat=0x%x\n",
		minor,
		immr->STATUS_LED_DIR, immr->STATUS_LED_PAR,
		immr->STATUS_LED_ODR, immr->STATUS_LED_DAT);

	return 0;
}


/*
 * Called when a process closes the device.
 * Doesn't have a return value in version 2.0.x because it can't fail,
 * but in version 2.2.x it is allowed to fail
 */
static int statusled_release (struct inode *inode, struct file *file)
{
	int minor  = MINOR(inode->i_rdev);

	debugk ("statusled_release(%p,%p)\n", inode, file);

	if (!immr)
		return (-ENODEV);

	/* We're now ready for our next caller */
	if (minor < MAX_LED_DEV) {
		led_dev[minor].flags &= ~SL_FLAG_OPEN;
#if defined(CONFIG_IVMS8) || defined(CONFIG_IVML24)
	} else {
		int m = minor - MAX_LED_DEV;

		io_dev_flags[m] &= ~SL_FLAG_OPEN;
#endif	/* CONFIG_IVMS8, CONFIG_IVML24 */
	}

	MOD_DEC_USE_COUNT;

	debugk ("LED_CLOSE: dir=0x%x  par=0x%x  odr=0x%x  dat=0x%x\n",
		immr->STATUS_LED_DIR, immr->STATUS_LED_PAR,
		immr->STATUS_LED_ODR, immr->STATUS_LED_DAT);

	return 0;
}

#if defined(CONFIG_IVMS8) || defined(CONFIG_IVML24)
/*
 * read entry point:
 * Only supported for interlock switch (return ENXIO for LED devices).
 * For the ilock switch, block until the next status change happens;
 * then return exactly one byte containing the current state
 * (0x00 switch open, 0x01 switch closed).
 */
static ssize_t statusled_read (struct file *file,
			       char *buf, size_t count, loff_t *ppos)
{
	DECLARE_WAITQUEUE(wait, current);
	int minor = MINOR(file->f_dentry->d_inode->i_rdev);
	int rc = 0;
	char c;

	if (minor != MAX_LED_DEV)
		return (-ENXIO);

	add_wait_queue(&statusled_wait, &wait);

	current->state = TASK_INTERRUPTIBLE;

	for (;;) {
		if (changed_io_state)
			break;

		if (file->f_flags & O_NONBLOCK) {
			rc = -EAGAIN;
			goto OUT;
		}

		if (signal_pending(current)) {
			rc = -ERESTARTSYS;
			goto OUT;
		}

		schedule ();
	}

	spin_lock (&statusled_lock);
	c = (char)status_io_test ();
	changed_io_state = 0;
	spin_unlock (&statusled_lock);

	/* Copy out */
	if ((rc = verify_area(VERIFY_WRITE, buf, 1)) != 0) {
		goto OUT;
	}

        rc = 1; copy_to_user((void *)buf, (void*)(&c), rc);

OUT:
        current->state = TASK_RUNNING;
        remove_wait_queue(&statusled_wait, &wait);

	return (rc);
}

static unsigned int statusled_poll (struct file *file, poll_table *wait)
{
	poll_wait(file, &statusled_wait, wait);

	if (changed_io_state)
		return POLLIN | POLLRDNORM;

	return 0;
}

#endif	/* CONFIG_IVMS8, CONFIG_IVML24 */

/*
 * ioctl entry point:
 */

static ssize_t statusled_ioctl (
	struct inode *inode,
	struct file *file,
	unsigned int cmd,
	unsigned long arg)
{
	int minor  = MINOR(inode->i_rdev);
	int n;

	if (!immr)
		return (-ENODEV);

	debugk ("IOCTL: minor=%d, cmd=%s, arg=%ld\n",
		minor,
		(cmd == STATUSLED_GET   ) ? "STATUSLED_GET"    :
		(cmd == STATUSLED_SET   ) ? "STATUSLED_SET"    :
		(cmd == STATUSLED_PERIOD) ? "STATUSLED_PERIOD" :
		"<unknown>",
		arg);

	if (minor < MAX_LED_DEV) {
		switch (cmd) {
		case STATUSLED_GET:
			switch (led_dev[minor].state) {
			case STATUS_LED_OFF:	n =  0;
						break;
			case STATUS_LED_ON:	n = -1;
						break;
			default:		n = led_dev[minor].period;
						break;
			}
			return (copy_to_user((int *)arg, &n, sizeof (int)));
		case STATUSLED_SET:
			switch (arg) {
			case STATUS_LED_OFF:
				debugk ("IOCTL: was %d, set %ld=OFF\n",
					led_dev[minor].state, arg);
				if (led_dev[minor].state == STATUS_LED_BLINKING)
					del_timer(&led_dev[minor].timer);
				led_dev[minor].state = arg;
#if (STATUS_LED_ACTIVE == 0)
				immr->STATUS_LED_DAT |=   led_dev[minor].mask ;
#else
				immr->STATUS_LED_DAT &= ~(led_dev[minor].mask);
#endif
				return (0);
			case STATUS_LED_ON:
				debugk ("IOCTL: was %d, set %ld=ON\n",
					led_dev[minor].state, arg);
				if (led_dev[minor].state == STATUS_LED_BLINKING)
					del_timer(&led_dev[minor].timer);
				led_dev[minor].state = arg;
#if (STATUS_LED_ACTIVE == 0)
				immr->STATUS_LED_DAT &= ~(led_dev[minor].mask);
#else
				immr->STATUS_LED_DAT |=   led_dev[minor].mask ;
#endif
				return (0);
			case STATUS_LED_BLINKING:
				debugk ("IOCTL: was %d, set %ld=BLINKING\n",
					led_dev[minor].state, arg);
				if (led_dev[minor].state == STATUS_LED_BLINKING)
					return (0);
				led_dev[minor].state = arg; /* must come first! */
				/* start blinking */
				statusled_blink (minor);
				return (0);
			}
			return (-EINVAL);
		case STATUSLED_PERIOD:
			led_dev[minor].period = arg;
			debugk ("IOCTL: set PERIOD=%d\n", arg);
			return (0);
		}
	}
#if defined(CONFIG_IVMS8) || defined(CONFIG_IVML24)
	else {
		int m = minor - MAX_LED_DEV;

		switch (m) {
		case 0:		/* Interlock switch and device reset monitor */
		    if (cmd == STATUS_SWITCH_GET) {
			n = status_io_test ();
			return (copy_to_user((int *)arg, &n, sizeof (int)));
		    }
		    break;
		case 1:		/* Device reset enable output */
		    if (cmd == STATUS_RESET_GET) {
			if (immr->im_ioport.iop_pcdat & STATUS_RESET_ENA) {
				n = 1;
			} else {
				n = 0;
			}
			return (copy_to_user((int *)arg, &n, sizeof (int)));
		    }
		    if (cmd == STATUS_RESET_SET) {
			if (arg == 0) {
				immr->im_ioport.iop_pcdat &= ~STATUS_RESET_ENA;
			} else {
				immr->im_ioport.iop_pcdat |=  STATUS_RESET_ENA;
			}
			return (0);
		    }
		    break;
		default:
		    break;
		}
	}
#endif
	return (-EINVAL);
}

#if defined(CONFIG_IVMS8) || defined(CONFIG_IVML24)
/*
 * Test status of interlock switch
 */
static int  status_io_test (void)
{
	int n = 0;

#if defined(CONFIG_IVMS8)
	spin_lock (&statusled_lock);
	if ((immr->im_siu_conf.sc_sipend & STATUS_ILOCK_SWITCH) != 0)
		n |= STATUS_ILOCK_BIT;
	immr->im_siu_conf.sc_sipend = STATUS_ILOCK_SWITCH;
	spin_unlock (&statusled_lock);
#elif defined(CONFIG_IVML24)
	if ((immr->im_cpm.cp_pbdat & STATUS_ILOCK_SWITCH) != 0)
		n |= STATUS_ILOCK_BIT;
#endif
	if ((immr->im_ioport.iop_pcdat & STATUS_RESET_MON) != 0)
		n |= STATUS_RESET_MON_BIT;
	return (n);
}

static void status_io_poll (unsigned long period)
{
	char c;

	spin_lock (&statusled_lock);
	c = (char)status_io_test ();
	if (c != last_io_state) {
		last_io_state = c;
		changed_io_state = 1;
	}
	spin_unlock (&statusled_lock);

	if (changed_io_state)
		wake_up_interruptible(&statusled_wait);

	init_timer(&io_timer);
	io_timer.expires = jiffies + io_timer.data;
	add_timer(&io_timer);
}
#endif	/* CONFIG_IVMS8, CONFIG_IVML24 */


/*
 * Timer controlled blink entry point.
 *
 * We delete the timer when the status is changed to non-blinking
 * or when the module is unloaded.
 */
static void statusled_blink (unsigned long minor)
{
	unsigned long flags;

	if (!immr)
		return;

        if (led_dev[minor].state != STATUS_LED_BLINKING) {
		/* don't change any more */
		return;
	}

	immr->STATUS_LED_DAT ^= led_dev[minor].mask;

	save_flags(flags);
	cli();

	init_timer(&led_dev[minor].timer);
	led_dev[minor].timer.expires = jiffies + led_dev[minor].period;
	add_timer(&led_dev[minor].timer);

	restore_flags(flags);
}


/******************************
 **** Module Declarations *****
 **************************** */

module_init (statusled_init);

#ifdef MODULE
/*
 * Cleanup - unregister the driver
 */
void statusled_cleanup (void)
{
	int minor, ret;

	/*
	 * Cleanup timer
	 */
	for (minor=0; minor<MAX_LED_DEV; ++minor) {
		if (led_dev[minor].state == STATUS_LED_BLINKING)
			del_timer(&led_dev[minor].timer);
#if (STATUS_LED_ACTIVE == 0)
		immr->STATUS_LED_DAT |=   led_dev[minor].mask ;
#else
		immr->STATUS_LED_DAT &= ~(led_dev[minor].mask);
#endif
		led_dev[minor].state = STATUS_LED_OFF;
	}
#if defined(CONFIG_IVMS8) || defined(CONFIG_IVML24)
	del_timer(&io_timer);
#endif	/* CONFIG_IVMS8, CONFIG_IVML24 */

	/*
	 * Unregister the device
	 */
	ret = unregister_chrdev (Major, DEVICE_NAME);

	/*
	 * If there's an error, report it
	 */
	if (ret < 0) {
		printk ("unregister_chrdev: error %d\n", ret);
	}
}

module_exit (statusled_cleanup);

#endif	/* MODULE */
