/***********************************************************************
 *
 * (C) Copyright 2002
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 *
 * This module provides support for the VMEBus (and other external)
 * interrupts on CU824 VMEBus systems.
 *
 ***********************************************************************/

/*
 * Standard in kernel modules
 */
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/rtc.h>
#include <linux/smp_lock.h>
#include <linux/version.h>
#include <asm/types.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/time.h>
#include <asm/machdep.h>		/* ppc_md */
#include <platforms/cu824.h>

#include "cu824_mem.h"
#include "cu824_intr.h"

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
#define DEVICE_NAME		"cu824_intr"
#define	CU824_INTR_VERSION	"0.1-wd"

/***************************************************************************
 * Debug support
 ***************************************************************************
 */
#if 0
#define	DEBUG
#endif

#ifdef	DEBUG
# define debugk(fmt,args...)	printk(fmt ,##args)
# define DEBUG_OPTARG(arg)	arg,
#else
# define debugk(fmt,args...)
# define DEBUG_OPTARG(arg)
#endif

/***************************************************************************
 * Global stuff - export global symbols
 ***************************************************************************
 */
EXPORT_SYMBOL(request_cu824_irq);
EXPORT_SYMBOL(free_cu824_irq);
EXPORT_SYMBOL(vme_interrupt_enable);
EXPORT_SYMBOL(vme_interrupt_disable);

/***************************************************************************
 * Local stuff
 ***************************************************************************
 */
typedef	struct intr_hdlr_s {
	void		(*handler)(int, void *, struct pt_regs *);
	void		*arg;
} intr_hdlr_t;

intr_hdlr_t	vme_intr_hdlr [NR_VME_INTERRUPTS];

typedef struct onbd_intr_hdlr_s {
	intr_hdlr_t	int_hand;
	char		irq;
	short		enable;
	char		*name;
} onbd_intr_hdlr_t;

/*
 * Several of the interrupts are shared by the VME Bus and
 * OnBoard interrupts; for instance, CU824_IRQ_VME_INTR (IRQ3)
 * is responsible for up to 7 VME Bus interrupts, etc.
 *
 * The following array is used as reference counter so we know when
 * to install or de-install an interrupt. Please note that only
 * some of the slots are used, the others remain uninitialized (0).
 */
static unsigned int intr_cnt[NUM_8259_INTERRUPTS];

/*
 * Prototypes
 */
static void obd_interrupt_handler (int, void *, struct pt_regs *);
static void vme_interrupt_handler (int, void *, struct pt_regs *);
static int vme_interrupt_en_dis (unsigned int, unsigned int);
static int onboard_interrupt_en_dis (unsigned int, unsigned int);

onbd_intr_hdlr_t onbd_intr_hdlr [] = {
	{	{ NULL, NULL, },	/* handler and argument uninitialized	*/
		CU824_IRQ_ABORT_AC,
		CU824_ONBOARD_ABORT_AC,
		"Abort Key / VMEbus ACFail",
	},
	{	{ NULL, NULL, },	/* handler and argument uninitialized	*/
		CU824_IRQ_PCI,
		CU824_ONBOARD_PCI0 | CU824_ONBOARD_PCI1 |
		CU824_ONBOARD_PCI2 | CU824_ONBOARD_PCI3,
		"PCI-IRQ(0-3)",
	},
	{	{ NULL, NULL, },	/* handler and argument uninitialized	*/
		CU824_IRQ_SIO,
		CU824_ONBOARD_SIO_A | CU824_ONBOARD_SIO_B,
		"SIO-A / SIO-B",
	},
	{	{ NULL, NULL, },	/* handler and argument uninitialized	*/
		CU824_IRQ_MAIL_LM_RTC,
		CU824_ONBOARD_MAILBOX | CU824_ONBOARD_LM | CU824_ONBOARD_RTC,
		"Mailbox / LM81 / RTC",
	},
};

#define NR_ONBOARD_INTERRUPTS	(sizeof(onbd_intr_hdlr)/sizeof(onbd_intr_hdlr_t))

static volatile cu824_bcsr_t	*bcsr;

/***************************************************************************
 *
 * Initialize the module
 */
static int __init cu824_intr_init (void)
{
	vme_mem_t *p;

	printk (KERN_INFO
		"CU824 Extended Interrupt Support version "
		CU824_INTR_VERSION
		"\n"
	);

	if ((p = vme_mem_dev(VME_MEM_BCSR)) == NULL)
		return (-ENODEV);

	bcsr  = (cu824_bcsr_t *)(p->addr);

	debugk ("CU824 Interrupt Support installed\n");
	return 0;
}

/***************************************************************************
 * Module Declarations
 ***************************************************************************
 */

module_init (cu824_intr_init);

#ifdef MODULE
/*
 * Cleanup - unregister the appropriate file from /proc
 */
void cu824_intr_cleanup (void)
{
	int irq;

	debugk ("Cleanup CU824 Interrupt Support\n");

	/* make sure all interrupts are disabled */

	/* VME Interrupts first */
	for (irq=0; irq < NR_VME_INTERRUPTS; ++irq) {
		if (vme_intr_hdlr[irq].handler) {
			debugk ("Cleanup VME Interrupt handler vector 0x%02x\n",
				irq);
			free_cu824_irq (irq);
		}
	}

	/* the Onboard Interrupts */
	for ( ; irq < NR_VME_INTERRUPTS+NR_ONBOARD_INTERRUPTS; ++irq) {
		int index = irq - NR_VME_INTERRUPTS;
		if (onbd_intr_hdlr[index].int_hand.handler) {
			debugk ("Cleanup Onboard Interrupt handler vector 0x%02x\n",
				irq);
			free_cu824_irq (irq);
		}
	}

	debugk ("Cleanup completed\n");
}

module_exit (cu824_intr_cleanup);

#endif	/* MODULE */


/***************************************************************************
 * Exported API functions
 ***************************************************************************
 */

/*
 * Register a VMEBus or On-Board interrupt handler
 */
int request_cu824_irq (unsigned int irq,
		       void (*handler)(int, void *, struct pt_regs *),
	               void *argument)
{
	unsigned int	 real_irq;

	debugk ("Install CU824 handler for IRQ %d\n", irq);

	real_irq = irq;

	if (real_irq < NR_VME_INTERRUPTS) {	/* VME Bus interrupt */
		intr_hdlr_t	*ih = &vme_intr_hdlr[real_irq];

		if (ih->handler) {
			printk ("Nested VMEBus interrupts not supported (yet)"
				" [IRQ: VME=%d IRQ%d]\n",
				real_irq, CU824_IRQ_VME_INTR);
			return (-EBUSY);
		}

		ih->handler = handler;
		ih->arg = argument;

		debugk ("VMEBus IRQ %d installed\n", real_irq);

		/*
		 * Enable VME Bus interrupt when the first handler gets installed
		 */
		if (intr_cnt[CU824_IRQ_VME_INTR] == 0) {

			debugk ("Enabling CU824 VME interrupt handler\n");

			if (request_irq( CU824_IRQ_VME_INTR,
					 vme_interrupt_handler,
					 0, "CU824 VMEBus",
					 vme_intr_hdlr
				       ) != 0) {
				panic ("Can't allocate CU824 VME IRQ");
			}
		}
		++intr_cnt[CU824_IRQ_VME_INTR];	/* increment reference count */

		return (0);
	}

	real_irq -= NR_VME_INTERRUPTS;

	if (real_irq < NR_ONBOARD_INTERRUPTS) {	/* Onboard interrupt */
		onbd_intr_hdlr_t *ih = &onbd_intr_hdlr[real_irq];
		int siu_irq = ih->irq;

		if (ih->int_hand.handler) {
			printk ("Nested %s interrupts not supported (yet)"
				" [IRQ: OnBD=%d, IRQ%d]\n",
				ih->name, real_irq, siu_irq);
			return (-EBUSY);
		}

		ih->int_hand.handler = handler;
		ih->int_hand.arg = argument;

		debugk ("Enabling CU824 %s interrupt handler\n", ih->name);

		/*
		 * Enable interrupt when the first handler gets installed
		 */
		if (intr_cnt[siu_irq] == 0) {
		    int rc;

			rc = request_irq(siu_irq, obd_interrupt_handler,
					 0, ih->name, (void *)ih);
			if (rc != 0)
				panic ("Can't allocate CU824_INTR IRQ");
		}
		++intr_cnt[siu_irq];	/* increment reference count */

		/* enable interrupt */
		onboard_interrupt_en_dis(ih->enable, 1);

		return 0;
	}


	/* real_irq -= NR_ONBOARD_INTERRUPTS; */
	/* ...more types could be added here */


	debugk ("Invalid CU824 IRQ %d (0...%u only)\n",
		irq,
		NR_VME_INTERRUPTS + NR_ONBOARD_INTERRUPTS
	);
	return (-EINVAL);
}


/*
 * De-register and free a VMEBus or On-Board interrupt handler
 */
int free_cu824_irq (unsigned int irq)
{
	unsigned int	 real_irq;

	debugk ("Un-install handler for CU824 IRQ %d\n", irq);

	real_irq = irq;

	if (real_irq < NR_VME_INTERRUPTS) {	/* VME Bus interrupt */
		intr_hdlr_t	*ih = &vme_intr_hdlr[real_irq];

		if (ih->handler == NULL) {
			printk ("Can't uninstall NULL VMEBus interrupt %d\n",
				real_irq);
			return (-EINVAL);
		}

		if (intr_cnt[CU824_IRQ_VME_INTR] == 0) {
			printk ("Error: dead CU824 VMEBus interrupt %d\n",
				real_irq);
			return (0);
		}

		--intr_cnt[CU824_IRQ_VME_INTR];	/* decrement reference count */

		/*
		 * Disable VME Bus interrupt upon removal of last handler
		 */
		if (intr_cnt[CU824_IRQ_VME_INTR] == 0) {

			debugk ("Disabling CU824 VME interrupt handler\n");

			free_irq (CU824_IRQ_VME_INTR, vme_intr_hdlr);
		}

		ih->handler = NULL;
		ih->arg = 0L;

		debugk ("VMEBus IRQ %d uninstalled\n", real_irq);

		return (0);
	}

	real_irq -= NR_VME_INTERRUPTS;

	if (real_irq < NR_ONBOARD_INTERRUPTS) {	/* Onboard interrupt */
		onbd_intr_hdlr_t *ih = &onbd_intr_hdlr[real_irq];
		int siu_irq = ih->irq;

		if (ih->int_hand.handler == NULL) {
			printk ("Can't uninstall NULL %s interrupt"
				" [IRQ: OnBD=%d, IRQ%d]\n",
				ih->name, real_irq, siu_irq);
			return (-EINVAL);
		}

		if (intr_cnt[siu_irq] == 0) {
			printk ("Error: dead CU824 %s interrupt"
				" [IRQ: OnBD=%d, IRQ%d]\n",
				ih->name, real_irq, siu_irq);
			return (-EINVAL);
		}

		/*
		 * disable onboard IRQ
		 */
		onboard_interrupt_en_dis(ih->enable, 0);

		ih->int_hand.handler = NULL;
		ih->int_hand.arg = 0L;

		debugk ("CU824 %s interrupt handler uninstalled\n", ih->name);

		--intr_cnt[siu_irq];	/* decrement reference count */

		/*
		 * Disable interrupt upon removal of last handler
		 */
				
		if (intr_cnt[siu_irq] == 0) {
			free_irq (siu_irq, (void *)ih);

			debugk ("CU824 IRQ%d uninstalled\n", siu_irq);
		}

		return 0;
	}


	/* real_irq -= NR_ONBOARD_INTERRUPTS; */
	/* ...more types could be added here */


	debugk ("Invalid CU824 IRQ %d (0...%u only)\n",
		irq,
		NR_VME_INTERRUPTS + NR_ONBOARD_INTERRUPTS
	);
	return (-EINVAL);
}


int vme_interrupt_enable  (unsigned int level)
{
	return vme_interrupt_en_dis (level, 1);
}


int vme_interrupt_disable (unsigned int level)
{
	return vme_interrupt_en_dis (level, 0);
}

/***************************************************************************
 * Utility functions
 ***************************************************************************
 */

/*
 * The handler for onboard interrupts needed to pass the "logical" IRQ
 */
static void obd_interrupt_handler (int irq, void *dev_id, struct pt_regs *regs)
{
	onbd_intr_hdlr_t *ih = (onbd_intr_hdlr_t *)dev_id;
	unsigned int ivect;

	ivect = (ih - &onbd_intr_hdlr[0]) + NR_VME_INTERRUPTS;

	/* run handler */
	if (ih->int_hand.handler)
		ih->int_hand.handler (ivect, ih->int_hand.arg, regs);
	else
		printk ("Bogus Onboard interrupt vector 0x%02x\n", ivect);
}


/*
 * The VME Bus interrupt handler
 */
static void vme_interrupt_handler (int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned char	imask, ipend, ivect;

	printk ("## VMEBus interrupt %d called\n", irq);

	/*
	 * Check for pending VMEBus interrupts
	 * Ignore SysFail - SF interrupt causes an onboard interrupt
	 */
	ipend = bcsr->vis & 0xFE;

	if (ipend == 0) {
		debugk ("Spurious VMEBus interrupt\n");
		return;
	}

	/*
	 * Checking IRQ; start at VME-IRQ7 which has highest priority
	 */
	for (imask=0x80; imask; imask >>= 1) {
		intr_hdlr_t	*ih;

		if ((ipend & imask) == 0)	/* interrupt pending?	*/
			continue;

		/*
		 * VME Interrupt handling requires:
		 * mask interrupt, read vector, [run handler,] unmask interrupt
		 */
		bcsr->vie &= ~imask;

		ih = &vme_intr_hdlr[ivect];

		/* run handler */
		if (ih->handler)
			ih->handler (ivect, ih->arg, regs);
		else
			printk ("Bogus VME interrupt vector 0x%02x\n", ivect);

		/* re-enable interrupt */
		bcsr->vie |= imask;
	}
}


/*
 * Enable or disable a VME Bus interrupt level
 */
static int vme_interrupt_en_dis (unsigned int level, unsigned int enable)
{
	unsigned char mask;

	/* allow for VME interrupt levels 1 ... 7 */
	if ((level < 1) || (level > 7)) {
		return (-1);
	}

	mask = 1 << level;

	if (enable) {
		bcsr->vie |=  mask;
	} else {
		bcsr->vie &= ~mask;
	}

	return (0);
}

/*
 * Enable or disable an onboard interrupts
 */
static int onboard_interrupt_en_dis (unsigned int mask, unsigned int enable)
{
	unsigned char lie_mask = mask & 0xFF;
	unsigned char bcr_mask = mask >> 8;

	if (enable) {
		bcsr->lie |=  lie_mask;
		bcsr->bcr |=  bcr_mask;
	} else {
		bcsr->lie &= ~lie_mask;
		bcsr->bcr &= ~bcr_mask;
	}

	return (0);
}
