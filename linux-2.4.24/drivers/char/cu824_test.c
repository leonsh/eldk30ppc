/***********************************************************************
 *
 * (C) Copyright 2002
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 *
 * This is a test module for the VMEBus (and other external)
 * interrupt support on CU824 VMEBus systems.
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
#include <linux/init.h>
#include <asm/types.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/time.h>
#include <asm/machdep.h>		/* ppc_md */
#include <platforms/cu824.h>

#include "cu824_mem.h"
#include "cu824_intr.h"


/***************************************************************************
 * Debug stuff
 ***************************************************************************
 */
#define	DEBUG

#ifdef	DEBUG
# define debugk(fmt,args...)	printk(fmt ,##args)
# define DEBUG_OPTARG(arg)	arg,
#else
# define debugk(fmt,args...)
# define DEBUG_OPTARG(arg)
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
#define DEVICE_NAME		"CU824_test"
#define	CU824_INTR_VERSION	"0.1-wd"

/***************************************************************************
 * Local stuff
 ***************************************************************************
 */

/*
 * Prototypes
 */

int init_module(void);
void cleanup_module(void);

static void intr_handler_abort (int, void *, struct pt_regs *);

/***************************************************************************
 *
 * Initialize the module
 */
int __init cu824_test_init (void)
{
	printk (KERN_INFO
		"CU824 extended interrupt test module version "
		CU824_INTR_VERSION
		"\n"
	);

	request_cu824_irq (CU824_ONBOARD_IRQ_ABORT_AC,
			   intr_handler_abort,
			   (void *)2705);

	debugk ("CU824 Interrupt Test installed\n");

	return 0;
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
	debugk ("Initialize CU824 Interrupt Test\n");

	cu824_test_init ();

	return (0);
}

/*
 * Cleanup - unregister the appropriate file from /proc
 */
void cleanup_module (void)
{
	debugk ("Cleanup CU824 Interrupt Test\n");

	free_cu824_irq (CU824_ONBOARD_IRQ_ABORT_AC);

	debugk ("Cleanup completed\n");
}

#endif	/* MODULE */

/***************************************************************************
 * Utility functions
 ***************************************************************************
 */
void intr_handler_abort (int irq, void *arg, struct pt_regs *regs)
{
	printk ("## Abort Key interrupt handler called, IRQ=%d, arg=%d\n",
		irq, (int)arg);
}
