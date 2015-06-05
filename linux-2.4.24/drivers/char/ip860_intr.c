/***********************************************************************
 *
 * (C) Copyright 2001-2004
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 *
 * This module provides support for the VMEBus (and other external)
 * interrupts on IP860 VMEBus systems.
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
#include <asm/8xx_immap.h>
#include <asm/types.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/time.h>
#include <asm/machdep.h>		/* ppc_md */
#include <asm/mpc8xx.h>

#include "ip860_mem.h"
#include "ip860_intr.h"

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
#define DEVICE_NAME		"ip860_intr"
#define	IP860_INTR_VERSION	"0.1-wd"

/***************************************************************************
 * Debug support
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

/***************************************************************************
 * Global stuff - export global symbols
 ***************************************************************************
 */
EXPORT_SYMBOL(request_ip860_irq);
EXPORT_SYMBOL(free_ip860_irq);
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
	char		enable;
	char		*name;
} onbd_intr_hdlr_t;

/*
 * IP860 VMEBus Interrupt Vector/Acknowledge registers
 */
typedef	volatile unsigned short vme_ivect_t[NR_VME_IVECT];

/*
 * Several of the SIU interrupts are shared by the VME Bus and
 * OnBoard interrupts; for instance, IP860_IRQ_VME_INTR (SIU_IRQ3) is
 * responsible for up to 256 VME Bus interrupts, IP860_IRQ_IPMOD_A
 * (SIU_IRQ4) is responsible for 2 IP Module A interrupts, etc.
 *
 * The following array is used as reference counter so we know when
 * to install or de-install a SIU interrupt. Please note that only
 * some of the slots are used, the others remain uninitialized (0).
 */
static unsigned int intr_cnt[NR_SIU_INTS];

/*
 * Prototypes
 */
static void  ip_interrupt_handler (int, void *, struct pt_regs *);
static void obd_interrupt_handler (int, void *, struct pt_regs *);
static void vme_interrupt_handler (int, void *, struct pt_regs *);
static int  vme_interrupt_en_dis  (unsigned int, unsigned int);

onbd_intr_hdlr_t onbd_intr_hdlr [] = {
	{	{ NULL, NULL, },	/* handler and argument uninitialized	*/
		IP860_IRQ_ABORT,
		IP860_ABORT_ENABLE,
		"Abort Key",
	},
	{	{ NULL, NULL, },	/* handler and argument uninitialized	*/
		IP860_IRQ_VME_MBSF,
		IP860_MBOX_IRQ_ENABLE,
		"VMEBus Mailbox / Sysfail",
	},
	{	{ NULL, NULL, },	/* handler and argument uninitialized	*/
		IP860_IRQ_IPMOD_B,
		IP860_IPB_IRQ1_ENABLE,
		"IP B IRQ1",
	},
	{	{ NULL, NULL, },	/* handler and argument uninitialized	*/
		IP860_IRQ_IPMOD_B,
		IP860_IPB_IRQ0_ENABLE,
		"IP B IRQ0",
	},
	{	{ NULL, NULL, },	/* handler and argument uninitialized	*/
		IP860_IRQ_IPMOD_A,
		IP860_IPA_IRQ1_ENABLE,
		"IP A IRQ1",
	},
	{	{ NULL, NULL, },	/* handler and argument uninitialized	*/
		IP860_IRQ_IPMOD_A,
		IP860_IPA_IRQ0_ENABLE,
		"IP A IRQ0",
	},
};
#define NR_ONBOARD_INTERRUPTS	(sizeof(onbd_intr_hdlr)/sizeof(onbd_intr_hdlr_t))


static volatile ip860_bcsr_t	*bcsr;
static volatile unsigned short	*ip_mod;

/***************************************************************************
 *
 * Initialize the module
 */
static int __init ip860_intr_init (void)
{
	vme_mem_t *p;

	printk (KERN_INFO
		"IP860 Extended Interrupt Support version "
		IP860_INTR_VERSION
		"\n"
	);

	if ((p = vme_mem_dev(VME_MEM_BCSR)) == NULL)
		return (-ENODEV);

	bcsr  = (ip860_bcsr_t *)(p->addr);

	if ((p = vme_mem_dev(VME_MEM_IPMOD)) == NULL)
		return (-ENODEV);

	ip_mod = (vu_short *)(p->addr);

	debugk ("IP860 Interrupt Support installed\n");

	return 0;
}

/***************************************************************************
 * Module Declarations
 ***************************************************************************
 */

module_init (ip860_intr_init);

#ifdef MODULE
/*
 * Cleanup - unregister the appropriate file from /proc
 */
void ip860_intr_cleanup (void)
{
	int irq;

	debugk ("Cleanup IP860 Interrupt Support\n");

	/* make sure all interrupts are disabled */

	/* VME Interrupts first */
	for (irq=0; irq < NR_VME_INTERRUPTS; ++irq) {
		if (vme_intr_hdlr[irq].handler) {
			debugk ("Cleanup VME Interrupt handler vector 0x%02x\n",
				irq);
			free_ip860_irq (irq);
		}
	}

	/* the Onboard Interrupts */
	for ( ; irq < NR_VME_INTERRUPTS+NR_ONBOARD_INTERRUPTS; ++irq) {
		int index = irq - NR_VME_INTERRUPTS;
		if (onbd_intr_hdlr[index].int_hand.handler) {
			debugk ("Cleanup Onboard Interrupt handler vector 0x%02x\n",
				irq);
			free_ip860_irq (irq);
		}
	}

	debugk ("Cleanup completed\n");
}

module_exit (ip860_intr_cleanup);

#endif	/* MODULE */


/***************************************************************************
 * Exported API functions
 ***************************************************************************
 */

/*
 * Register a VMEBus or On-Board interrupt handler
 */
int request_ip860_irq (unsigned int irq,
		       void (*handler)(int, void *, struct pt_regs *),
	               void *argument)
{
	volatile immap_t *immr = (volatile immap_t *)IMAP_ADDR;
	unsigned int	 real_irq;

	debugk ("Install IP860 handler for IRQ %d\n", irq);

	real_irq = irq;

	if (real_irq < NR_VME_INTERRUPTS) {	/* VME Bus interrupt */
		intr_hdlr_t	*ih = &vme_intr_hdlr[real_irq];

		if (ih->handler) {
			printk ("Nested VMEBus interrupts not supported (yet)"
				" [IRQ: VME=%d SIU=%d]\n",
				real_irq, IP860_IRQ_VME_INTR);
			return (-EBUSY);
		}

		ih->handler = handler;
		ih->arg = argument;

		debugk ("VMEBus IRQ %d installed\n", real_irq);

		/*
		 * Enable VME Bus interrupt when the first handler gets installed
		 */
		if (intr_cnt[IP860_IRQ_VME_INTR] == 0) {

			debugk ("Enabling IP860 VME interrupt handler\n");

			if (request_8xxirq( IP860_IRQ_VME_INTR,
					    vme_interrupt_handler,
					    0, "IP860 VMEBus",
					    argument
					  ) != 0) {
				panic ("Can't allocate IP860 VME IRQ");
			}
		}
		++intr_cnt[IP860_IRQ_VME_INTR];	/* increment reference count */

		return (0);
	}

	real_irq -= NR_VME_INTERRUPTS;

	if (real_irq < NR_ONBOARD_INTERRUPTS) {	/* Onboard interrupt */
		onbd_intr_hdlr_t *ih = &onbd_intr_hdlr[real_irq];
		int siu_irq = ih->irq;

		if (ih->int_hand.handler) {
			printk ("Nested %s interrupts not supported (yet)"
				" [IRQ: OnBD=%d, SIU=%d]\n",
				ih->name, real_irq, siu_irq);
			return (-EBUSY);
		}

		ih->int_hand.handler = handler;
		ih->int_hand.arg = argument;

		debugk ("Enabling IP860 %s interrupt handler\n", ih->name);

		/*
		 * Enable interrupt when the first handler gets installed
		 *
		 * For the onboard interrupts this is not really required
		 * since these don't share the IRQ, but it's easier
		 * to use identical code.
		 */
		if (intr_cnt[siu_irq] == 0) {
		    int rc;

		    /*
                     * IP Module interrupts need special handling since
                     * several interrupts may share the same SIU IRQ
		     */
		    switch (irq) {
		    case IP860_ONBOARD_IRQ_IP_B_1:
		    case IP860_ONBOARD_IRQ_IP_B_0:
		    case IP860_ONBOARD_IRQ_IP_A_1:
		    case IP860_ONBOARD_IRQ_IP_A_0:
			rc = request_8xxirq(siu_irq, ip_interrupt_handler,
					    0, ih->name, (void *)ih);
			break;
		    default:
			rc = request_8xxirq(siu_irq, obd_interrupt_handler,
					    0, ih->name, (void *)ih);
			break;
		    }
		    if (rc != 0)
			panic ("Can't allocate IP860_INTR IRQ");

		    /* Make interrupt edge-triggered */
		    immr->im_siu_conf.sc_siel |= (0x80000000 >> siu_irq);
		}
		++intr_cnt[siu_irq];	/* increment reference count */

		/* enable interrupt */
		bcsr->onbd_imask |= ih->enable;

		return 0;
	}


	/* real_irq -= NR_ONBOARD_INTERRUPTS; */
	/* ...more types could be added here */


	debugk ("Invalid IP860 IRQ %d (0...%u only)\n",
		irq,
		NR_VME_INTERRUPTS + NR_ONBOARD_INTERRUPTS
	);
	return (-EINVAL);
}


/*
 * De-register and free a VMEBus or On-Board interrupt handler
 */
int free_ip860_irq (unsigned int irq)
{
	volatile immap_t *immr = (volatile immap_t *)IMAP_ADDR;
	unsigned int	 real_irq;

	debugk ("Un-install handler for IP860 IRQ %d\n", irq);

	real_irq = irq;

	if (real_irq < NR_VME_INTERRUPTS) {	/* VME Bus interrupt */
		intr_hdlr_t	*ih = &vme_intr_hdlr[real_irq];

		if (ih->handler == NULL) {
			printk ("Can't uninstall NULL VMEBus interrupt %d\n",
				real_irq);
			return (-EINVAL);
		}

		if (intr_cnt[IP860_IRQ_VME_INTR] == 0) {
			printk ("Error: dead IP860 VMEBus interrupt %d\n",
				real_irq);
			return (0);
		}

		--intr_cnt[IP860_IRQ_VME_INTR];	/* decrement reference count */

		/*
		 * Disable VME Bus interrupt upon removal of last handler
		 */
		if (intr_cnt[IP860_IRQ_VME_INTR] == 0) {

			debugk ("Disabling IP860 VME interrupt handler\n");

			free_irq (IP860_IRQ_VME_INTR, NULL);
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
				" [IRQ: OnBD=%d, SIU=%d]\n",
				ih->name, real_irq, siu_irq);
			return (-EINVAL);
		}

		if (intr_cnt[siu_irq] == 0) {
			printk ("Error: dead IP860 %s interrupt"
				" [IRQ: OnBD=%d, SIU=%d]\n",
				ih->name, real_irq, siu_irq);
			return (-EINVAL);
		}

		/*
		 * disable onboard IRQ
		 */
		bcsr->onbd_imask &= ~(ih->enable);

		ih->int_hand.handler = NULL;
		ih->int_hand.arg = 0L;

		debugk ("IP860 %s interrupt handler uninstalled\n", ih->name);

		--intr_cnt[siu_irq];	/* decrement reference count */

		/*
		 * Disable interrupt upon removal of last handler
		 */
				
		if (intr_cnt[siu_irq] == 0) {
			immr->im_siu_conf.sc_siel &= ~(0x80000000 >> siu_irq);
			free_irq (siu_irq, (void *)ih);

			debugk ("IP860 SIU IRQ %d uninstalled\n", siu_irq);
		}

		return 0;
	}


	/* real_irq -= NR_ONBOARD_INTERRUPTS; */
	/* ...more types could be added here */


	debugk ("Invalid IP860 IRQ %d (0...%u only)\n",
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
 * The IP Module Interrupt handler.
 * Needed because these may share one SIU IRQ for IP IRQ 0+1
 * IP Module Interrupts must be ACKed by reading the IACK vector (p.47)
 */
static void ip_interrupt_handler (int irq, void *dev_id, struct pt_regs *regs)
{
	onbd_intr_hdlr_t *ih;
	unsigned int ix, ivect;
	unsigned short vector;
	unsigned char ipend;
	unsigned char c = 0;		/* error flag */

	ipend = bcsr->onbd_ipend;	/* read only once */

	switch (irq) {
	case IP860_IRQ_IPMOD_A:
		if (ipend & IP860_IPA_IRQ0_ENABLE) {
			ivect = IP860_ONBOARD_IRQ_IP_A_0;
			ix = IP860_IPA_IACK0_INDX;
			break;
		}
		if (ipend & IP860_IPA_IRQ1_ENABLE) {
			ivect = IP860_ONBOARD_IRQ_IP_A_1;
			ix = IP860_IPA_IACK1_INDX;
			break;
		}
		c = 'A';	/* remember bogus interrupt source */
		break;
	case IP860_IRQ_IPMOD_B:
		if (ipend & IP860_IPB_IRQ0_ENABLE) {
			ivect = IP860_ONBOARD_IRQ_IP_B_0;
			ix = IP860_IPB_IACK0_INDX;
			break;
		}
		if (ipend & IP860_IPB_IRQ1_ENABLE) {
			ivect = IP860_ONBOARD_IRQ_IP_B_1;
			ix = IP860_IPB_IACK1_INDX;
			break;
		}
		c = 'B';	/* remember bogus interrupt source */
		break;
	default:
		/* No IP Module interrupt */
		printk ("Bogus IP Module interrupt from IRQ %d\n", irq);
		return;
	}

	if (c) {
		printk ("IP Module %c IRQ, but no interrupt pending (IPEND %02x)\n",
			c, ipend);
		return;
	}

	ih = &onbd_intr_hdlr[ivect - NR_VME_INTERRUPTS];

	/* mask interrupt */
	bcsr->onbd_imask &= ~(ih->enable);

	/* run handler */
	if (ih->int_hand.handler)
		ih->int_hand.handler (ivect, ih->int_hand.arg, regs);
	else
		printk ("Bogus IP module interrupt vector 0x%02x\n", ivect);

	/* ACK interrupt */
	vector = ip_mod[ix];

	/* un-mask interrupt */
	bcsr->onbd_imask |=   ih->enable;
}

/*
 * A handler for the other (non-IP) onboard interrupts
 * needed to pass the "logical" IRQ instead of the SIU IRQ
 */
static void obd_interrupt_handler (int irq, void *dev_id, struct pt_regs *regs)
{
	onbd_intr_hdlr_t *ih = (onbd_intr_hdlr_t *)dev_id;
	unsigned int ivect;

	ivect = (ih - &onbd_intr_hdlr[0]) + NR_VME_INTERRUPTS;

	/* mask interrupt */
	bcsr->onbd_imask &= ~(ih->enable);

	/* run handler */
	if (ih->int_hand.handler)
		ih->int_hand.handler (ivect, ih->int_hand.arg, regs);
	else
		printk ("Bogus Onboard interrupt vector 0x%02x\n", ivect);
	
	/* un-mask interrupt */
	bcsr->onbd_imask |=   ih->enable;
}


/*
 * The VME Bus SIU interrupt handler
 */
static void vme_interrupt_handler (int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned char	imask, ipend, ivect;

	printk ("## VMEBus interrupt %d called\n", irq);

	/*
	 * Check for pending VMEBus interrupts
	 * Ignore SysFail - SF interrupt causes an onboard interrupt
	 */
	ipend = bcsr->vme_ipend & 0xFE;

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
		bcsr->vme_imask &= ~imask;

		ih = &vme_intr_hdlr[ivect];

		/* run handler */
		if (ih->handler)
			ih->handler (ivect, ih->arg, regs);
		else
			printk ("Bogus VME interrupt vector 0x%02x\n", ivect);

		/* re-enable interrupt */
		bcsr->vme_imask |= imask;
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
		bcsr->vme_imask |=  mask;
	} else {
		bcsr->vme_imask &= ~mask;
	}

	return (0);
}
