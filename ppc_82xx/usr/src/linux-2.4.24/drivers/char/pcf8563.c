/***********************************************************************
 *
 * (C) Copyright 2000
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 *
 * This driver provides access to the Philips PCF8563 RTC
 * which is used for instance in LWMON systems.
 *
 * It uses the i2c-simple driver to access the device over the I2C bus.
 *
 ***********************************************************************/

/*
 * Standard in kernel modules
 */
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/rtc.h>
#include <linux/smp_lock.h>
#include <linux/version.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/init.h>
#include <asm/types.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/time.h>
#include <asm/machdep.h>		/* ppc_md */

#ifdef	CONFIG_8xx
#include <asm/mpc8xx.h>

#include "../i2c/i2c-simple.h"
#endif

#ifndef KERNEL_VERSION
# define KERNEL_VERSION(a,b,c)	(((a) << 16) + ((b) << 8) + (c))
#endif

#if (!defined(LINUX_VERSION_CODE) || LINUX_VERSION_CODE < KERNEL_VERSION(2,2,0))
#error "You need to use at least Linux kernel version 2.2.0"
#endif

/***************************************************************************
 * Global stuff
 ***************************************************************************
 */
 /*#define	DEBUG	1	//	*/

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


/*
 * Device Declarations
 */

/*
 * The name for our device, as it will appear in /proc/devices
 */
#define DEVICE_NAME	"rtc"
#define	RTC_VERSION	"$Revision: 1.3 $ wd@denx.de"

/***************************************************************************
 * Local stuff
 ***************************************************************************
 */

#ifdef	DEBUG
# define debugk(fmt,args...)	printk(fmt ,##args)
# define DEBUG_OPTARG(arg)	arg,
#else
# define debugk(fmt,args...)
# define DEBUG_OPTARG(arg)
#endif

static struct i2c_driver pcf8563_driver;

static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { 0x51, I2C_CLIENT_END };

static struct i2c_client_address_data addr_data = {
	normal_i2c:		normal_addr,
	normal_i2c_range:	ignore,
	probe:			ignore,
	probe_range:		ignore,
	ignore:			ignore,
	ignore_range:		ignore,
	force:			ignore,
};

/*
 * Prototypes
 */
static int		pcf8563_rtc_open (struct inode *, struct file *);
static int		pcf8563_rtc_release (struct inode *, struct file *);
static int		pcf8563_rtc_ioctl (struct inode *, struct file *,
				 unsigned int, unsigned long);
static unsigned int	bcd2bin (unsigned int);
static unsigned int	bin2bcd (unsigned int);
static unsigned char	rtc_rd (unsigned char);
static void		rtc_wr (unsigned char, unsigned char);
static unsigned long	pcf8563_get_rtc_time(void);
static int		pcf8563_set_rtc_time(unsigned long now);
static void		pcf8563_reset_rtc(void);

int init_module(void);
void cleanup_module(void);

/* avoid multiple access */
static char rtc_status = 0;

/* save/restore old machdep pointers */
int		(*save_set_rtc_time)(unsigned long);
unsigned long	(*save_get_rtc_time)(void);

/***************************************************************************
 * I2C stuff
 ***************************************************************************
 */
#define DAT(x) ((unsigned int)(x->data))

static struct i2c_client *clnt;

static int
pcf8563_attach(struct i2c_adapter *adap, int addr, unsigned short flags,
	       int kind)
{
#if 0
	unsigned char buf[1], ad[1] = { 0 };
	struct i2c_msg msgs[2] = {
		{ addr, 0,        1, ad  },
		{ addr, I2C_M_RD, 1, buf }
	};
#endif

	clnt = kmalloc(sizeof(*clnt), GFP_KERNEL);
	if (!clnt)
		return -ENOMEM;

	strcpy(clnt->name, "PCF8563");
	clnt->id	= pcf8563_driver.id;
	clnt->flags	= 0;
	clnt->addr	= addr;
	clnt->adapter	= adap;
	clnt->driver	= &pcf8563_driver;
	clnt->data	= NULL;

#if 0
	if (i2c_transfer(clnt->adapter, msgs, 2) == 2)
		DAT(clnt) = buf[0];
#endif

	return i2c_attach_client(clnt);
}

static int
pcf8563_probe(struct i2c_adapter *adap)
{
	/*
         * Probing seems to confuse the RTC on PM826 and CPU86.
	 * Not sure if it's true for other boards though - thus the
         * conditional.
	 */
#if defined(CONFIG_PM826) || defined(CONFIG_CPU86)
	pcf8563_attach(adap, 0x51, 0, 0);
	return 0;
#endif	/* CONFIG_PM826 */
	return i2c_probe(adap, &addr_data, pcf8563_attach);
}

static int
pcf8563_detach(struct i2c_client *client)
{
	i2c_detach_client(client);
	return 0;
}

static struct i2c_driver pcf8563_driver = {
	name:		"PCF8563",
	id:		I2C_DRIVERID_PCF8563,
	flags:		I2C_DF_NOTIFY,
	attach_adapter:	pcf8563_probe,
	detach_client:	pcf8563_detach,
	command:	NULL
};

/***************************************************************************
 * Driver interface
 ***************************************************************************
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)

static struct file_operations pcf8563_rtc_fops = {
	NULL,			/* lseek	*/
	NULL,			/* read		*/
	NULL,			/* write	*/
	NULL,			/* readdir	*/
	NULL,			/* select	*/
	pcf8563_rtc_ioctl,	/* ioctl	*/
	NULL,			/* mmap		*/
	pcf8563_rtc_open,		/* open		*/
	NULL,			/* flush	*/
	pcf8563_rtc_release	/* close	*/
};

#else	/* Linux kernel >= 2.3.x */

static struct file_operations pcf8563_rtc_fops = {
	owner:			THIS_MODULE,
	open:			pcf8563_rtc_open,
	release:		pcf8563_rtc_release,
	ioctl: 			pcf8563_rtc_ioctl,
};
#endif

/*
 * We sponge a minor off of the misc major. No need slurping
 * up another valuable major dev number for this.
 */
static struct miscdevice rtc_dev = {
	RTC_MINOR,
	DEVICE_NAME,
	&pcf8563_rtc_fops,
};


/***************************************************************************
 *
 * Initialize the driver - Register the character device
 */
static int __init pcf8563_rtc_init (void)
{
	unsigned long now, flags;
	extern time_t last_rtc_update;
	extern rwlock_t xtime_lock;

	printk (KERN_INFO "PCF8563 Real-Time Clock Driver " RTC_VERSION "\n");

	misc_register(&rtc_dev);

	i2c_add_driver(&pcf8563_driver);

	/* Switching kernel RTC pointers */
	debugk ("RTC switching kernel pointers\n");

	save_set_rtc_time   = ppc_md.set_rtc_time;
	save_get_rtc_time   = ppc_md.get_rtc_time;

	ppc_md.set_rtc_time = pcf8563_set_rtc_time;
	ppc_md.get_rtc_time = pcf8563_get_rtc_time;

	/*
	 * Set system time
	 * Code copied from arch/ppc/kernel/time.c
	 */
	write_lock_irqsave(&xtime_lock, flags);

	now = pcf8563_get_rtc_time();

	debugk ("Set System Time from RTC Time: %lu\n", now);

	xtime.tv_usec = 0;
	xtime.tv_sec  = now;

	last_rtc_update = now - 658;

	time_adjust   = 0;		/* stop active adjtime() */
	time_status  |= STA_UNSYNC;
	time_state    = TIME_ERROR;
	time_maxerror = NTP_PHASE_LIMIT;
	time_esterror = NTP_PHASE_LIMIT;

	write_unlock_irqrestore(&xtime_lock, flags);

	debugk ("RTC installed, minor %d\n", RTC_MINOR);

	/*
	 * Check for low voltage
	 */
	if (rtc_rd (0x02) & 0x80) {
		printk (KERN_CRIT
			"PCF8563 RTC Low Voltage - date/time not reliable\n");
	}

	/*
	 * Reset any error conditions, alarms, etc.
	 */
	pcf8563_reset_rtc ();

	return 0;
}

/***************************************************************************
 *
 * called whenever a process attempts to open the device
 */
static int pcf8563_rtc_open (struct inode *inode, struct file *file)
{
	lock_kernel();
	if (rtc_status) {
		unlock_kernel();
		return -EBUSY;
	}

	rtc_status = 1;
	unlock_kernel();

	MOD_INC_USE_COUNT;

	return 0;
}



/***************************************************************************
 *
 * Called when a process closes the device.
 */
static int pcf8563_rtc_release (struct inode *inode, struct file *file)
{
	lock_kernel();
	rtc_status = 0;
	unlock_kernel();

	MOD_DEC_USE_COUNT;

	return 0;
}


/***************************************************************************
 *
 * ioctl entry point:
 */
static int pcf8563_rtc_ioctl (struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	struct rtc_time tm;
	int leap_yr;
	static const unsigned char days_in_month[] =
		{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	switch (cmd) {
	case RTC_RD_TIME:	/* Read the time/date from RTC	*/

		to_tm (pcf8563_get_rtc_time(), &tm);

		/* convert from rtc_time to struct tm conventions */
		tm.tm_year -= 1900;
		tm.tm_mon  -= 1;

		debugk ("RTC_RD_TIME: %4d-%02d-%02d-%02d:%02d:%02d\n",
			tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);

		return (copy_to_user((void *)arg, &tm, sizeof(tm))) ? -EFAULT : 0;

	case RTC_SET_TIME:	/* Set the time/date in the RTC	*/

		if (!capable(CAP_SYS_TIME))
			return -EACCES;

		if (copy_from_user(&tm, (struct rtc_time *)arg, sizeof(tm)))
			return -EFAULT;

		/* convert from struct tm to rtc_time conventions */
		tm.tm_year += 1900;
		tm.tm_mon  += 1;

		/* simple but OK for all dates in time_t range */
		leap_yr = ((tm.tm_year % 4) == 0);

		if ((tm.tm_year < 1970) ||
		    (tm.tm_mon  > 12)	||
		    (tm.tm_mday == 0)	||
		    (tm.tm_mday >
		    	(days_in_month[tm.tm_mon] + ((tm.tm_mon==2)&&leap_yr)))	||
		    (tm.tm_hour >= 24)	||
		    (tm.tm_min  >= 60)	||
		    (tm.tm_sec  >= 60)	) {

			return (-EINVAL);
		}

		debugk ("RTC_SET_TIME: %4d-%02d-%02d-%02d:%02d:%02d\n",
			tm.tm_year, tm.tm_mon, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);

		pcf8563_set_rtc_time (mktime(tm.tm_year, tm.tm_mon, tm.tm_mday,
					   tm.tm_hour, tm.tm_min, tm.tm_sec ) );

		return (0);

	default:
		return (-EINVAL);
	}
}


/***************************************************************************
 * Hardware related stuff
 ***************************************************************************
 */


/***************************************************************************
 *
 * get RTC time:
 */
static unsigned long pcf8563_get_rtc_time(void)
{
	unsigned char year, mon_cent, wday, mday, hour, min, sec;
	int yr;

	sec	= rtc_rd (0x02);
	min	= rtc_rd (0x03);
	hour	= rtc_rd (0x04);
	mday	= rtc_rd (0x05);
	wday	= rtc_rd (0x06);
	mon_cent= rtc_rd (0x07);
	year	= rtc_rd (0x08);

	debugk ("Get RTC year: %02x mon/cent: %02x mday: %02x wday: %02x "
		"hr: %02x min: %02x sec: %02x\n",
		year, mon_cent, mday, wday,
		hour, min, sec );
	debugk ("Alarms: wday: %02x day: %02x hour: %02x min: %02x\n",
		rtc_rd(0x0C), rtc_rd(0x0B), rtc_rd(0x0A), rtc_rd(0x09) );

	if (sec & 0x80) {
		printk (KERN_CRIT
			"PCF8563 RTC Low Voltage - date/time not reliable\n");
	}

	yr = bcd2bin (year) + ((mon_cent & 0x80) ? 2000 : 1900);

	debugk ("get rtc [dec] year=%d mon=%d day=%d hour=%d min=%d sec=%d\n",
		yr,
		bcd2bin(mon_cent & 0x1F),
		bcd2bin(mday & 0x3F),
		bcd2bin(hour & 0x3F),
		bcd2bin(min  & 0x7F),
		bcd2bin(sec  & 0x7F) );

	return (mktime (yr,
			bcd2bin(mon_cent & 0x1F),
			bcd2bin(mday & 0x3F),
			bcd2bin(hour & 0x3F),
			bcd2bin(min  & 0x7F),
			bcd2bin(sec  & 0x7F)
		) );
}

/***************************************************************************
 *
 * set RTC time:
 */
static int pcf8563_set_rtc_time(unsigned long now)
{
	struct rtc_time tm;
	unsigned char century, year, mon, wday, mday, hour, min, sec;

	to_tm (now, &tm);

	debugk ("Set RTC [dec] year=%d mon=%d day=%d hour=%d min=%d sec=%d\n",
		tm.tm_year, tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	century = (tm.tm_year >= 2000) ? 0x80 : 0;
	year = bin2bcd (tm.tm_year % 100);
	mon  = bin2bcd (tm.tm_mon) | century;
	wday = bin2bcd (tm.tm_wday);
	mday = bin2bcd (tm.tm_mday);
	hour = bin2bcd (tm.tm_hour);
	min  = bin2bcd (tm.tm_min);
	sec  = bin2bcd (tm.tm_sec);

	debugk ("Set RTC [bcd] year=%X mon=%X day=%X "
		"hour=%X min=%X sec=%X wday=%X\n",
		year, mon, mday, hour, min, sec, wday);

	rtc_wr (0x08, year);
	rtc_wr (0x07, mon );
	rtc_wr (0x06, wday);
	rtc_wr (0x05, mday);
	rtc_wr (0x04, hour);
	rtc_wr (0x03, min );
	rtc_wr (0x02, sec );

	return (0);
}

/***************************************************************************
 *
 * set RTC time:
 */
static void pcf8563_reset_rtc(void)
{
	/* clear all control & status registers */
	rtc_wr (0x00, 0x00);
	rtc_wr (0x01, 0x00);
	rtc_wr (0x0D, 0x00);

	/* clear Voltage Low bit */
	rtc_wr (0x02, rtc_rd (0x02) & 0x7F);

	/* reset all alarms */
	rtc_wr (0x09, 0x00);
	rtc_wr (0x0A, 0x00);
	rtc_wr (0x0B, 0x00);
	rtc_wr (0x0C, 0x00);
}

/***************************************************************************
 * Module Declarations
 ***************************************************************************
 */
module_init(pcf8563_rtc_init);

#ifdef MODULE
/*
 * Cleanup - unregister the appropriate file from /proc
 */
static void pcf8563_rtc_cleanup (void)
{
	ppc_md.set_rtc_time = save_set_rtc_time;
	ppc_md.get_rtc_time = save_get_rtc_time;

	misc_deregister(&rtc_dev);

	debugk ("PCF8563 Real-Time Clock Driver de-registered\n");
}

module_exit(pcf8563_rtc_cleanup);
#endif	/* MODULE */


/***************************************************************************
 *
 * Utility functions
 */

static unsigned int bcd2bin (unsigned int n)
{
	return ((((n >> 4) & 0x0F) * 10) + (n & 0x0F));
}

static unsigned int bin2bcd (unsigned int n)
{
	return (((n / 10) << 4) | (n % 10));
}

static void rtc_wr (unsigned char reg, unsigned char val)
{
	unsigned char buf[2];

	buf[0] = reg;
	buf[1] = val;

	if (i2c_master_send(clnt, (char *)buf, 2) != 2)
	{
	    printk(__FUNCTION__ "(): i2c transfer error\n");
	}
}

static unsigned char rtc_rd  (unsigned char reg)
{
	unsigned char buf[1], addr[1];
	struct i2c_msg msgs[2] = {
		{ clnt->addr, 0,        1, addr },
		{ clnt->addr, I2C_M_RD, 1, buf  }
	};

	buf[0] = 0;
	addr[0] = reg;

	if (i2c_transfer(clnt->adapter, msgs, 2) != 2)
	{
	    printk(__FUNCTION__ "(): i2c transfer error\n");
	    return -1;
	}

	return buf[0];
}
