/***********************************************************************
 *
 * (C) Copyright 2000-2004
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 *
 * This driver provides access to the V3021 RTC on
 * IP860 VMEBus systems.
 *
 ***********************************************************************/

/*
 * Standard in kernel modules
 */
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/rtc.h>
#include <linux/smp_lock.h>
#include <linux/version.h>
#include <asm/types.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/time.h>
#include <asm/machdep.h>		/* ppc_md */
#include <asm/mpc8xx.h>

#include "ip860_mem.h"

#include <platforms/ip860.h>

/***************************************************************************
 * Global stuff
 ***************************************************************************
 */
#if 0
#define	DEBUG
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


/*
 * Device Declarations
 */

/*
 * The name for our device, as it will appear in /proc/devices
 */
#define DEVICE_NAME	"rtc"
#define	RTC_VERSION	"0.1-wd"

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

/*
 * Prototypes
 */
int ip860_rtc_init (void);
static int ip860_rtc_open(struct inode *, struct file *);
static int ip860_rtc_release(struct inode *, struct file *);
static int ip860_rtc_ioctl(struct inode *, struct file *,
			   unsigned int, unsigned long);
unsigned long ip860_get_rtc_time(void);
int ip860_set_rtc_time(unsigned long now);
static unsigned int bcd2bin (unsigned int);
static unsigned int bin2bcd (unsigned int);
static unsigned char rtc_rd (unsigned char);
static void rtc_wr (unsigned char, unsigned char);
static void rtc_wr_addr (unsigned char);
static void rtc_init (void);

int init_module(void);
void cleanup_module(void);

/* avoid multiple access */
static char rtc_status = 0;

/* save/restore old machdep pointers */
int		(*save_set_rtc_time)(unsigned long);
unsigned long	(*save_get_rtc_time)(void);

/***************************************************************************
 * Driver interface
 ***************************************************************************
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)

static struct file_operations ip860_rtc_fops = {
	NULL,			/* lseek	*/
	NULL,			/* read		*/
	NULL,			/* write	*/
	NULL,			/* readdir	*/
	NULL,			/* select	*/
	ip860_rtc_ioctl,	/* ioctl	*/
	NULL,			/* mmap		*/
	ip860_rtc_open,		/* open		*/
	NULL,			/* flush	*/
	ip860_rtc_release	/* close	*/
};

#else	/* Linux kernel >= 2.3.x */

static struct file_operations ip860_rtc_fops = {
	owner:			THIS_MODULE,
	open:			ip860_rtc_open,
	release:		ip860_rtc_release,
	ioctl: 			ip860_rtc_ioctl,
};
#endif

/*
 * We sponge a minor off of the misc major. No need slurping
 * up another valuable major dev number for this.
 */
static struct miscdevice rtc_dev = {
	RTC_MINOR,
	DEVICE_NAME,
	&ip860_rtc_fops,
};


/***************************************************************************
 * Hardware related stuff
 ***************************************************************************
 */

/*
 * RTC V3021 register definitions
 */

#define V3021_STATUS0		0	/* Status 0 register		*/
#define V3021_STATUS1		1	/* Status 1 register		*/
#define V3021_SECONDS		2	/* Seconds register		*/
#define V3021_MINUTES		3	/* Minutes register		*/
#define V3021_HOURS		4	/* Hours register		*/
#define V3021_DAY		5	/* Day of the Month register	*/
#define V3021_MONTH		6	/* Month register		*/
#define V3021_YEAR		7	/* Years register		*/
#define V3021_WDAY		8	/* Day of the Week register	*/
#define V3021_WNUM		9	/* Week number register		*/
#define V3021_RAM2CLK		14	/* Copy RAM to Clock register	*/
#define V3021_CLK2RAM		15	/* Copy Clock to RAM register	*/

static volatile ip860_bcsr_t	*bcsr;

/*
 * STATUS 0  bit definitions
 */

#define V3021_ST0_FREQ		0x01	/* frequency measurement mode	*/
#define V3021_ST0_TEST0		0x04	/* Test Mode 0			*/
#define V3021_ST0_TEST1		0x08	/* Test Mode 1			*/
#define V3021_ST0_TSLOCK	0x10	/* Time Set/LOCK		*/

#define	V3021_OFFSET		0x16	/* Offset of RTC in BCSR area	*/

extern int vme_mem_init(void);

/***************************************************************************
 *
 * Initialize the driver - Register the character device
 */
int __init ip860_rtc_init (void)
{
	vme_mem_t *p;
	unsigned long now, flags;
	extern time_t last_rtc_update;
	extern rwlock_t xtime_lock;

	/* make sure data sructures are already initialized */
	if (vme_mem_init ())
		return (-ENODEV);

	printk (KERN_INFO "V3021 Real-Time Clock Driver version " RTC_VERSION "\n");

	misc_register(&rtc_dev);

	if ((p = vme_mem_dev(VME_MEM_BCSR)) == NULL)
		return (-ENODEV);

	if ((bcsr  = (ip860_bcsr_t *)(p->addr)) == NULL) {
		printk (KERN_ERR "ip860_rtc_init: bcsr == NULL\n");
		return (-ENODEV);
	}

	debugk ("ip860_rtc_init: p=%p, bcsr=%p\n", p, bcsr);

	rtc_init ();

	/* Switching kernel RTC pointers */
	debugk ("RTC switching kernel pointers\n");

	save_set_rtc_time   = ppc_md.set_rtc_time;
	save_get_rtc_time   = ppc_md.get_rtc_time;

	ppc_md.set_rtc_time = ip860_set_rtc_time;
	ppc_md.get_rtc_time = ip860_get_rtc_time;

	/*
	 * Set system time
	 * Code copied from arch/ppc/kernel/time.c
	 */
	write_lock_irqsave(&xtime_lock, flags);

	now = ip860_get_rtc_time();
	
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

	return 0;
}

/***************************************************************************
 *
 * called whenever a process attempts to open the device
 */
static int ip860_rtc_open (struct inode *inode, struct file *file)
{
	if (rtc_status)
		return -EBUSY;

	MOD_INC_USE_COUNT;
	rtc_status = 1;

	return 0;
}



/***************************************************************************
 *
 * Called when a process closes the device.
 */
static int ip860_rtc_release (struct inode *inode, struct file *file)
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
static int ip860_rtc_ioctl (struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	struct rtc_time tm;
	int leap_yr;
	static const unsigned char days_in_month[] =
		{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	switch (cmd) {
	case RTC_RD_TIME:	/* Read the time/date from RTC	*/

		to_tm (ip860_get_rtc_time(), &tm);

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

		ip860_set_rtc_time (mktime(tm.tm_year, tm.tm_mon, tm.tm_mday,
					   tm.tm_hour, tm.tm_min, tm.tm_sec ) );

		return (0);

	default:
		return (-EINVAL);
	}
}

/***************************************************************************
 *
 * get RTC time:
 */
unsigned long ip860_get_rtc_time(void)
{
	unsigned char year, mon, day, hour, min, sec;
	int yr;

	rtc_wr_addr (V3021_CLK2RAM);	/* copy date to RTC RAM */

	sec  = rtc_rd (V3021_SECONDS);
	min  = rtc_rd (V3021_MINUTES);
	hour = rtc_rd (V3021_HOURS);
	day  = rtc_rd (V3021_DAY);
	mon  = rtc_rd (V3021_MONTH);
	year = rtc_rd (V3021_YEAR);

	debugk ("get rtc [bcd] year=%X mon=%X day=%X hour=%X min=%X sec=%X\n",
		year, mon, day, hour, min, sec);

	yr = bcd2bin (year) + 1900;

	if (yr < 1970)
		yr += 100;

	debugk ("get rtc [dec] year=%d mon=%d day=%d hour=%d min=%d sec=%d\n",
		yr, bcd2bin(mon), bcd2bin(day),
		bcd2bin(hour), bcd2bin(min),  bcd2bin(sec));

	return (mktime (yr,
			bcd2bin(mon),
			bcd2bin(day),
			bcd2bin(hour),
			bcd2bin(min),
			bcd2bin(sec)
		) );
}

/***************************************************************************
 *
 * set RTC time:
 */
int ip860_set_rtc_time(unsigned long now)
{
	struct rtc_time tm;
	unsigned char year, mon, day, hour, min, sec, wday;

	to_tm (now, &tm);

	debugk ("Set RTC [dec] year=%d mon=%d day=%d hour=%d min=%d sec=%d\n",
		tm.tm_year, tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	year = bin2bcd ((tm.tm_year - 1900) % 100);
	mon  = bin2bcd (tm.tm_mon);
	day  = bin2bcd (tm.tm_mday);
	hour = bin2bcd (tm.tm_hour);
	min  = bin2bcd (tm.tm_min);
	sec  = bin2bcd (tm.tm_sec);
	wday = bin2bcd (tm.tm_wday);

	debugk ("Set RTC [bcd] year=%X mon=%X day=%X "
		"hour=%X min=%X sec=%X wday=%X\n",
		year, mon, day, hour, min, sec, wday);

	rtc_wr (V3021_STATUS0, 0x00);		/* clear time set lock bit */
	rtc_wr (V3021_SECONDS, sec );
	rtc_wr (V3021_MINUTES, min );
	rtc_wr (V3021_HOURS,   hour);
	rtc_wr (V3021_DAY,     day );
	rtc_wr (V3021_MONTH,   mon );
	rtc_wr (V3021_YEAR,    year);
	rtc_wr (V3021_WDAY,    wday);
	rtc_wr (V3021_WNUM,    0   );

	udelay (1);
	rtc_wr_addr (V3021_RAM2CLK);		/* copy ram to clock */
	udelay (1);
	rtc_wr (V3021_STATUS0, V3021_ST0_TSLOCK);	/* lock time */

	return (0);
}

/***************************************************************************
 * Module Declarations
 ***************************************************************************
 */
#ifdef MODULE
/*
 * Initialize the module
 */
int init_module (void)
{
	ip860_rtc_init();

	return (0);
}

/*
 * Cleanup - unregister the appropriate file from /proc
 */
void cleanup_module (void)
{
	ppc_md.set_rtc_time = save_set_rtc_time;
	ppc_md.get_rtc_time = save_get_rtc_time;

	misc_deregister(&rtc_dev);

	debugk ("V3021 Real-Time Clock Driver de-registered\n");
}

#endif	/* MODULE */


/***************************************************************************
 *
 * Utility functions
 */

#define	V3021_CS		0x08
#define V3021_RD		0x04
#define V3021_WR		0x02
#define V3021_DATA	0x01

static unsigned int bcd2bin (unsigned int n)
{
	return ((((n >> 4) & 0x0F) * 10) + (n & 0x0F));
}

static unsigned int bin2bcd (unsigned int n)
{
	return (((n / 10) << 4) | (n % 10));
}

static void rtc_init (void)
{
	int i;

	bcsr->rtc = 0;				/* all control lines H */

	for (i=0; i<8; ++i) {
		udelay (1);
		bcsr->rtc = V3021_CS | V3021_RD;/* CS, RD = L for read cycles */
		udelay (1);
		bcsr->rtc = 0;			/* release RD */
	}

	rtc_wr (V3021_STATUS0, 0);		/* initialize status register */
	udelay (5);
	rtc_wr (V3021_STATUS0, V3021_ST0_TSLOCK);	/* lock time */
}

static void rtc_wr_addr (unsigned char addr)
{
	int i;

	bcsr->rtc = 0;		/* force all control lines H */

	for (i=0; i<4; ++i) {
		unsigned char bit = addr & 1;

		udelay (1);
		bcsr->rtc = bit;
		udelay (1);
		bcsr->rtc = bit | V3021_WR | V3021_CS;	/* CS, WR */
		udelay (1);
		bcsr->rtc = bit | V3021_WR;		/* hold WR */
		udelay (1);
		bcsr->rtc = 0;				/* release WR */

		addr >>= 1;
	}
}

static void rtc_wr (unsigned char addr, unsigned char data)
{
	int i;

	rtc_wr_addr (addr);

	for (i=0; i<8; ++i) {
		unsigned char bit = data & 1;

		udelay (1);
		bcsr->rtc = bit;
		udelay (1);
		bcsr->rtc = bit | V3021_WR | V3021_CS;
		udelay (1);
		bcsr->rtc = bit | V3021_WR;
		udelay (1);
		bcsr->rtc = 0;

		data >>= 1;
	}
}

static unsigned char rtc_rd  (unsigned char addr)
{
	int i;
	unsigned char data = 0;

	rtc_wr_addr (addr);

	for (i=0; i<8; ++i) {
		unsigned char val;

		udelay (1);
		bcsr->rtc = V3021_RD | V3021_CS;  /* CS, RD */
		udelay (1);
		bcsr->rtc = V3021_RD | V3021_CS;  /* CS, RD : clock in read data */
		udelay (1);
		val = bcsr->rtc;		/* read data (1 bit only) */
		udelay (1);
		bcsr->rtc = 0;			/* release RD */

		data = (data >> 1) | ((val & 1) ? 0x80 : 0);

	}

	return (data);
}

