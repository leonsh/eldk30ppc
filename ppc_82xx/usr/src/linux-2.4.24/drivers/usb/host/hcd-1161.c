/*
 * Philips 1362 HCD (Host Controller Driver) for USB.
 * Copyright (C) 2003 DENX Software Engineering, Wolfgang Denk, wd@denx.de
 *
 * The driver is based on
 *
 * Philips 1161 HCD (Host Controller Driver) for USB.
 *
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000 David Brownell <david-b@pacbell.net>
 * 
 *  Modified for Philips 1161 HCD by Srinivas Yarra <Srinivas.Yarra@philips.com>
 *
 *  ARM support and various cleanups by Abraham van der Merwe <abraham@2d3d.co.za>
 * 
 * History:
 * 
 */
 
#include <linux/config.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>  /* for in_interrupt() */
#include <linux/usb.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#ifdef CONFIG_NSCU
#include <asm/8xx_immap.h>
#endif

#define MARKER() do {} while(0)
//#define MARKER() printk (KERN_DEBUG "%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__)

#define ep_print_int_eds(a,b)
#define urb_print(a,b,c)

//#define DEBUG
//#define __TRACE_CTL_LEVEL__

#include "../hcd.h"
#include "hcd-1161.h"

typedef	__u32		ULONG;
typedef	__u8		UCHAR;
typedef	void		VOID;

static      td_tree_addr_t   		tstAtlBridge[MAX_GTD+1];
static      td_t             		*pstDoneHead_hcd ;
static		isotd_map_buffer_t		aIsoTdMapBuffer[2];			/* One for each ITL */
static 		LIST_HEAD (ohci_1161_hcd_list);
static 		spinlock_t 				usb_ed_lock = SPIN_LOCK_UNLOCKED;

#define		ATL_DONE_TOUT						10
#define		ATL_DONE_DELAY					10
#define		MAX_BULK_TD_BUFF_SIZE		ATL_BLOCK_SZ
#define		MAX_CNTL_TD_BUFF_SIZE		ATL_BLOCK_SZ
#define		hc_1161_name	"1362-OHCI-HC"	
#define		OHCI_UNLINK_TIMEOUT			(HZ / 10)

void print_int_ed_list(ohci_t	*ohci);
/* 1161 HC accessing functions decleration */
void 	fnvIsp1161HcorWrite	(ohci_t 	*ohci,ULONG 	uReg, ULONG 	uRegData);
void 	fnvIsp1161HcorRead	(ohci_t 	*ohci,ULONG 	uReg, ULONG 	*puRegData);
void 	fnvIsp1161HcRead	(ULONG	uReg, ULONG 	*puRegData);
void 	fnvIsp1161HcWrite	(ULONG 	uReg, ULONG 	uRegData);
void 	fnvIsp1161AtlRead	(__u8* 	pbyChar, __u32 	uTotalByte);
void 	fnvIsp1161AtlWrite	(__u8* 	pbyChar, __u32 	uTotalByte);
void 	fnvIsp1161ItlRead	(__u8* 	pbyChar, __u32 	uTotalByte);
void 	fnvIsp1161ItlWrite	(__u8* 	pbyChar, __u32 	uTotalByte);

/* 1161 HC Initialization functions decleration */
ULONG 	fnvIsp1161HostReset	(ohci_t 	*ohci);
ULONG 	fnuIsp1161HostDetect	(void);
void 	fnvHcHardwareConfig	(ohci_t	*ohci, ULONG	uIntLevel);
void 	fnvHcIntEnable	(ohci_t 	*ohci);
void 	fnvHcControlInit	(ohci_t 	*ohci);
void 	fnvHcInterruptInit	(ohci_t 	*ohci);
void 	fnvHcFmIntervalInit	(ohci_t 	*ohci);
void 	fnvHcRhPower	(ohci_t 	*ohci);
int	 	fnuHci1161HostInit	(ohci_t 	*ohci);

/* 1161 HC Interrupt Functions */
static 	void 	fnvHci1161IrqHandler (int 	irq, void 	*__ohci, struct 	pt_regs *r);
void 	fnvProcessSofItlInt	(ohci_t *ohci);

/* TD functions decleration */
static 	void 	td_fill (unsigned int info, void * data, int len, struct urb * urb, int index);
static 	void 	td_submit_urb (struct urb * urb);

/* EP handling functions */
static int ep_int_ballance (ohci_t * ohci, int interval, int load);
static int ep_2_n_interval (int inter);
static int ep_rev (int num_bits, int word);
static int ep_link (ohci_t * ohci, ed_t * edi);
static int ep_unlink (ohci_t * ohci, ed_t * ed);
static void ep_rm_ed (struct usb_device * usb_dev, ed_t * ed);
static ed_t * ep_add_ed (struct usb_device * usb_dev, unsigned int pipe, int interval, int load);

/* Done List handling functions */
static void dl_transfer_length(td_t * td);
static void dl_del_urb (struct urb * urb);
static void dl_del_list (ohci_t  * ohci, unsigned int frame);
static td_t * dl_reverse_done_list (ohci_t * ohci, td_t *td_list);
static void dl_done_list (ohci_t * ohci, td_t * td_list);

/* Root Hub function declerations */
static int rh_send_irq (ohci_t * ohci, void * rh_data, int rh_len);
static void rh_int_timer_do (unsigned long ptr);
static int rh_init_int_timer (struct urb * urb);
static int rh_submit_urb (struct urb * urb);
static int rh_unlink_urb (struct urb * urb);

/* URB support functions */
static void urb_free_priv( struct ohci	*hc, urb_priv_t	*urb_priv);
static void urb_rm_priv_locked (struct urb	*urb);
static void urb_rm_priv (struct urb	*urb);

/* SOHCI functions */
static int s1161_alloc_dev (struct usb_device *usb_dev);
static int s1161_free_dev (struct usb_device *usb_dev);
static int s1161_get_current_frame_number (struct usb_device *usb_dev);
static int s1161_return_urb (struct urb * urb);
static int s1161_submit_urb (struct urb	*urb);
static int s1161_unlink_urb (struct urb	*urb);

/* OHCI functions */
static void hc_release_1161 (ohci_t * ohci);
static ohci_t * __devinit hc_alloc_1161 (struct pci_dev *dev);
static int __devinit hc_found_1161 ( void) ;


#ifndef __PPC__

#define  outw_hcd(x,y)	outw_p(y,x)
#define  inw_hcd	inw_p

#else /* __PPC__ */

#define	outw_hcd(port,data) { \
			__asm__ __volatile__ ("eieio;sync;isync"); \
			(*(volatile u16 *) (port)) = (data); \
			__asm__ __volatile__ ("eieio;sync;isync"); \
		}

#define	inw_hcd(port) ({ \
			u16 __v; \
			__asm__ __volatile__ ("eieio;sync;isync"); \
			__v = (*(volatile u16 *) (port)); \
			__asm__ __volatile__ ("eieio;sync;isync"); \
			__v; \
		})

#endif /* __PPC__ */

#define	 sti_hcd	sti
#define	 cli_hcd	cli
#define	 iodelay_hcd()	udelay(1)



/*--------------------------------------------------------------*
 * 1161 Host Controller operational registers write
 *--------------------------------------------------------------*/
void fnvIsp1161HcorWrite(ohci_t *ohci,ULONG uReg, ULONG uRegData)
{

	ULONG	uData;

	/* Check if the registers are controlled by software */
	if ((uReg == uHcHcdControl ) || (uReg == (uHcHcdControl | 0x80))) {
		ohci->uHcHcdControl_hcd = uRegData;
		return;
	}

	if ((uReg == uHcHcdCommandStatus) || (uReg == (uHcHcdCommandStatus | 0x80))) {
		ohci->uHcHcdCommandStatus_hcd = uRegData;
		return;
	}

	/* Write the register index to the command register */
	outw_hcd(HC_COM, uReg | 0x80);

	/* Write the data to the data register */
	uData = uRegData & 0x0000FFFF;				/* Write lower 16-bit first */
	outw_hcd(HC_DATA, uData);

	uData = (uRegData & 0xFFFF0000) >> 16;		/* Then the higher 16-bit */
	outw_hcd(HC_DATA, uData);


} /* fnvIsp1161HcorWrite() */


/*--------------------------------------------------------------*
 * 1161 Host Controller operational registers read
 *--------------------------------------------------------------*/
void fnvIsp1161HcorRead(ohci_t *ohci,ULONG uReg, ULONG *puRegData)
{
	ULONG	uData;

	/* Service the HCD HC transfer control registers first */
	if (uReg == uHcHcdControl ) {
		*puRegData = ohci->uHcHcdControl_hcd;
		return;
	}

	if (uReg == uHcHcdCommandStatus) {
		*puRegData = ohci->uHcHcdCommandStatus_hcd;
		return;
	}


	/* Write the register index to the command register */
	outw_hcd(HC_COM, uReg);

	/* Read the data from the data register */
	uData = inw_hcd(HC_DATA);

	*puRegData = uData & 0x0000FFFF;			/* Save the lower 16-bit value */

	uData = inw_hcd(HC_DATA);
	*puRegData |= (uData & 0x0000FFFF) << 16;		/* Take the higher 16-bit */

} /* fnvIsp1161HcorRead() */


/*--------------------------------------------------------------*
 * 1161 Host Controller registers read
 *--------------------------------------------------------------*/
void fnvIsp1161HcRead(ULONG uReg, ULONG *puRegData)
{
	ULONG	uData;

	/* Write the register index to the command register */
	outw_hcd(HC_COM, uReg);

	/* Read the data from the data register */
	uData = inw_hcd(HC_DATA);
	*puRegData = uData;



} /* fnvIsp1161HcRead() */


/*--------------------------------------------------------------*
 * 1161 Host Controller registers write
 *--------------------------------------------------------------*/
void fnvIsp1161HcWrite(ULONG uReg, ULONG uRegData)
{

	/* Write the register index to the command register */
	outw_hcd(HC_COM, uReg | 0x80);

	/* Write the data to the data register */
	outw_hcd(HC_DATA, uRegData);

} /* fnvIsp1161HcWrite() */


/*--------------------------------------------------------------*
 * 1161 Host Controller Atl Buffer Reading 
 *--------------------------------------------------------------*/
void fnvIsp1161AtlRead(__u8* pbyChar, __u32 uTotalByte)
{
	__u32           uTotalDoubleWord;
	__u32*          puLong;
	__u32           uIndex;
	__u32           uData1;
	__u32           uData2;

#ifdef __TRACE_MID_LEVEL__
	printk("fnvIsp1161AtlRead( buff = 0x%p, bytes = %d)\n",pbyChar,uTotalByte);
#endif /* __TRACE_MID_LEVEL__ */

	if ((__u32)pbyChar & 3)
	{
		printk ("fnvIsp1161AtlRead: FATAL ERROR: pbyChar must be double-word aligned\n");
		return;
	}

	/* Program the transfer counter */
	fnvIsp1161HcWrite(REG_XFER_CNTR, uTotalByte);

	/* 2. Pass HCD ATL to ISP1161 internal ATL through the 32-bit ATLBuffer register */
	/* Use PIO for the time being. Will use DMA */
	uTotalDoubleWord = uTotalByte >> 2;             /* Number of double words to move from ISP1161 ATL */

	puLong = (__u32*) pbyChar;      /* Convert the HCD byte buffer to double-word buffer */

	cli_hcd();

	/* Send the command */
	outw_hcd(HC_COM,REG_ATL_BUFF_IO);

	iodelay_hcd();
	iodelay_hcd();
	iodelay_hcd();

	/* Read data from data port */
	for (uIndex = 0; uIndex < uTotalDoubleWord; uIndex ++) {
		uData1 = inw_hcd(HC_DATA);              /* Read lower 16-bit first */
		uData2 = inw_hcd(HC_DATA);              /* then higher 16-bit */

		/* Combine to 32-bit double word */
		puLong[uIndex] = cpu_to_le32((uData1 & 0x0000FFFF) | ((uData2 & 0x0000FFFF) << 16));         /* Take the higher 16-bit */

		iodelay_hcd();
	} /* for */

	sti_hcd();
} /* End of fnvIsp1161AtlRead() */

/*--------------------------------------------------------------*
 * 1161 Host Controller Atl Buffer Reading 
 *--------------------------------------------------------------*/
void fnvIsp1161AtlWrite(__u8* pbyChar, __u32 uTotalByte)
{
	__u32           uTotalDoubleWord;
	__u32*          puLong;
	__u32           uIndex;
	__u32           uData1;
	__u32           uData2;

#ifdef __TRACE_MID_LEVEL__
	printk("fnvIsp1161AtlWrite( buff = 0x%p, bytes = %d)\n",pbyChar,uTotalByte);
#endif /* __TRACE_MID_LEVEL__ */

	if ((__u32)pbyChar & 3)
	{
		printk ("fnvIsp1161AtlWrite: FATAL ERROR: pbyChar must be double-word aligned\n");
		return;
	}

	/* Program the transfer counter */
	fnvIsp1161HcWrite(REG_XFER_CNTR, uTotalByte);

	/* 2. Pass HCD ATL to ISP1161 internal ATL through the 32-bit ATLBuffer register */
	/* Use PIO for the time being. Will use DMA */
	uTotalDoubleWord = uTotalByte >> 2;			/* Number of double words to move from ISP1161 ATL */
	puLong = (__u32*) pbyChar;					/* Convert the HCD byte buffer to double-word buffer */


	/* Send the command */
	outw_hcd(HC_COM,(REG_ATL_BUFF_IO | 0x80));		/* Don't forget to set bit 7 for write*/

	iodelay_hcd();
	iodelay_hcd();
	iodelay_hcd();

	cli_hcd();
	/* Write data to the data port */
	for (uIndex = 0; uIndex < uTotalDoubleWord; uIndex ++) {
		uData1 = le32_to_cpu(puLong[uIndex]) & 0x0000FFFF;		/* Take the lower 16-bit of the double word */
		uData2 = (le32_to_cpu(puLong[uIndex]) & 0xFFFF0000) >> 16;	/* Take the higher 16-bit of the double word */

		/* Write them to ATL */
		outw_hcd(HC_DATA,uData1);				/* Write lower 16-bit first */
		outw_hcd(HC_DATA,uData2);				/* Write higher 16-bit */

		iodelay_hcd();
	} /* for */

	sti_hcd();

} /* End of fnvIsp1161AtlWrite() */


/*--------------------------------------------------------------*
 * 1161 Host Controller Itl Buffer Reading 
 *--------------------------------------------------------------*/
void fnvIsp1161ItlRead(__u8* pbyChar, __u32 uTotalByte)
{
	__u32           uTotalDoubleWord;
	__u32*          puLong;
	__u32           uIndex;
	__u32           uData1;
	__u32           uData2;

#ifdef __TRACE_MID_LEVEL__
	printk("fnvIsp1161ItlRead( buff = 0x%p, bytes = %d)\n",pbyChar,uTotalByte);
#endif /* __TRACE_MID_LEVEL__ */

	if ((__u32)pbyChar & 3)
	{
		printk ("fnvIsp1161ItlRead: FATAL ERROR: pbyChar must be double-word aligned\n");
		return;
	}

#if 0
	/* Program the transfer counter */
	fnvIsp1161HcWrite(REG_XFER_CNTR, uTotalByte);
#endif

	outw_hcd(HC_COM, REG_XFER_CNTR | 0x80);
	outw_hcd(HC_DATA, uTotalByte);

	/* 2. Pass HCD ITL to ISP1161 internal ITL through the 32-bit ITLBuffer register */
	/* Use PIO for the time being. Will use DMA */
	uTotalDoubleWord = uTotalByte >> 2;             /* Number of double words to move from ISP1161 ATL */
	puLong = (__u32*) pbyChar;      /* Convert the HCD byte buffer to double-word buffer */

	cli_hcd();

	/* Send the command */
	outw_hcd(HC_COM,REG_ITL_BUFF_IO);

#if 0
	iodelay_hcd();
#endif

	/* Read data from data port */
	for (uIndex = 0; uIndex < uTotalDoubleWord; uIndex ++) {
		uData1 = inw_hcd(HC_DATA);              /* Read lower 16-bit first */
		uData2 = inw_hcd(HC_DATA);              /* then higher 16-bit */

		/* Combine to 32-bit double word */
		puLong[uIndex] = cpu_to_le32((uData1 & 0x0000FFFF) | ((uData2 & 0x0000FFFF) << 16));         /* Take the higher 16-bit */

#if 0
		iodelay_hcd();
#endif
	} /* for */

	sti_hcd();
} /* End of fnvIsp1161ItlRead() */

/*--------------------------------------------------------------*
 * 1161 Host Controller Itl Buffer Reading 
 *--------------------------------------------------------------*/
void fnvIsp1161ItlWrite(__u8* pbyChar, __u32 uTotalByte)
{
	__u32           uTotalDoubleWord;
	__u32*          puLong;
	__u32           uIndex;
	__u32           uData1;
	__u32           uData2;

#ifdef __TRACE_MID_LEVEL__
	printk("fnvIsp1161ItlWrite( buff = 0x%p, bytes = %d)\n",pbyChar,uTotalByte);
#endif /* __TRACE_MID_LEVEL__ */

	if ((__u32)pbyChar & 3)
	{
		printk ("fnvIsp1161ItlRead: FATAL ERROR: pbyChar must be double-word aligned\n");
		return;
	}

#if 0
	/* Program the transfer counter */
	fnvIsp1161HcWrite(REG_XFER_CNTR, uTotalByte);
#endif

	outw_hcd(HC_COM, REG_XFER_CNTR | 0x80);
	outw_hcd(HC_DATA, uTotalByte);

	/* 2. Pass HCD ITL to ISP1161 internal ITL through the 32-bit ITLBuffer register */
	/* Use PIO for the time being. Will use DMA */
	uTotalDoubleWord = uTotalByte >> 2;			/* Number of double words to move from ISP1161 ATL */
	puLong = (__u32*) pbyChar;					/* Convert the HCD byte buffer to double-word buffer */


	/* Send the command */
	outw_hcd(HC_COM,(REG_ITL_BUFF_IO | 0x80));		/* Don't forget to set bit 7 for write*/

#if 0
	iodelay_hcd();
#endif

	cli_hcd();
	/* Write data to the data port */
	for (uIndex = 0; uIndex < uTotalDoubleWord; uIndex ++) {
		uData1 = le32_to_cpu(puLong[uIndex]) & 0x0000FFFF; /* Take the lower 16-bit of the double word */
		uData2 = (le32_to_cpu(puLong[uIndex]) & 0xFFFF0000) >> 16;	/* Take the higher 16-bit of the double word */

		/* Write them to ITL */
		outw_hcd(HC_DATA,uData1);				/* Write lower 16-bit first */
		outw_hcd(HC_DATA,uData2);				/* Write higher 16-bit */

#if 0
		iodelay_hcd();
#endif
	} /* for */

	sti_hcd();

} /* End of fnvIsp1161ItlWrite() */


/*--------------------------------------------------------------*
 * 1161 Initialization functions 
 *--------------------------------------------------------------*/
/*--------------------------------------------------------------*
 * 1161 Host Controller Reset
 *--------------------------------------------------------------*/
ULONG fnvIsp1161HostReset(ohci_t *ohci)
{
ULONG uData;
ULONG uI;
ULONG uRetVal;


	/* Set the HostController Reset bit in command status register */
 	fnvIsp1161HcorRead(ohci,uHcCommandStatus,&uData);

 	uData = HC_COMMAND_STATUS_HCR;

 	fnvIsp1161HcorWrite(ohci,uHcCommandStatus, uData);

	/* wait some time for 1161 to be reset */
 	for(uI=0;uI<5000;uI++) {
  		fnvIsp1161HcorRead(ohci,uHcCommandStatus,&uData);
  		if((uData & HC_COMMAND_STATUS_HCR) == 0) break;
 	}

 	uRetVal = uData & HC_COMMAND_STATUS_HCR;

#ifdef CONFIG_SA1100_FRODO
	frodo_cpld_clear (FRODO_CPLD_USB,FRODO_USB_HWAKEUP | FRODO_USB_DWAKEUP);
	frodo_cpld_set (FRODO_CPLD_USB,FRODO_USB_NDPSEL);
#endif

 return uRetVal;
} /* End of fnvIsp1161HostReset */

/*--------------------------------------------------------------*
 * 1161 Host Controller detection on ISA bus
 *--------------------------------------------------------------*/
ULONG fnuIsp1161HostDetect(void)
{
 	ULONG	uDataWrite;
	ULONG	uDataRead;

#ifndef CONFIG_SA1100_FRODO
	/* Check whether the IO Region is free to use for 1161 Host Controller or not */
	if( check_region(HC_IO_BASE, HC_IO_SIZE) < 0 ) {
		return -EBUSY;
	}
#endif

	/* Reset the host controller assuming that it will work */
	fnvIsp1161HcWrite(REG_RESET_DEV, 0xF6);
	
	/* ISP1161 host controller detection */
	uDataWrite = 0x55aa;							/* Test data */

	fnvIsp1161HcWrite(REG_SCRATCH, uDataWrite);		/* Write a value to the scratch pad register */
	fnvIsp1161HcRead(REG_SCRATCH, &uDataRead);		/* Read it back. They should be equal */

	if (uDataWrite == uDataRead)					/* Is the host controller there? */
	    return 0;
	else
	    return -ENODEV;

} /* End of fnuIsp1161Host() */


/*--------------------------------------------------------------*
 * 1161 Host Controller hardware registers initialization
 *--------------------------------------------------------------*/
void fnvHcHardwareConfig(ohci_t	*ohci, ULONG uIntLevel)
{
	ULONG		uData;

#if !defined(CONFIG_SA1100_FRODO) && !defined(CONFIG_NSCU)
	UCHAR		PicMaskBit[] = {1, 2, 4, 8, 16, 32, 64, 128};
	ULONG		uIntPort;

	/* Disable IRQ for ISP1161 */

	/* If IRQ is connected to master PIC: IRQ0..IRQ7 */
	if (uIntLevel < 8) {
		uData = (ULONG) inb(PIC1_OCW1);
		uData |= PicMaskBit[uIntLevel];
		outb(PIC1_OCW1, uData);

	} /* if */
	/* if IRQ is connected to slave PIC: IRQ8..IRQ15*/
	else {
		uData = (ULONG) inb(PIC2_OCW1);
		uData |= PicMaskBit[uIntLevel - 8];
		outb(PIC2_OCW1, uData);

	} /* else */

	fnvIsp1161HcRead(REG_HW_MODE, &uData);
	uData |= (INT_PIN_ENABLE | INT_OUTPUT_POLARITY);		/* Global interrupt */
#else
	uData = SUSPEND_CLK_NOT_STOP |
		INT_PIN_ENABLE |
		INT_PIN_TRIGGER |
		DATA_BUS_WIDTH_16 |
		ANALOG_OC_ENABLE;
#endif

	fnvIsp1161HcWrite(REG_HW_MODE, uData);

#if !defined(CONFIG_SA1100_FRODO) && !defined(CONFIG_NSCU)
	if (uIntLevel < 8)					/* IRQ0..IRQ7 */
		uIntPort = 0x4d0;
	else {
		uIntPort = 0x4d1;
		uIntLevel -= 8;
	}

	uData = (ULONG) inb(uIntPort);

	uData |= PicMaskBit[uIntLevel];
	outb(uIntPort, uData);
#endif

#ifdef CONFIG_NSCU
	/* Configure IRQ3 as edge-triggered interrupt */
	((immap_t *)IMAP_ADDR)->im_siu_conf.sc_siel |= 0x02000000;
#endif
} /* End of fnvHcHardwareConfig() */

/*--------------------------------------------------------------*
 * 1161 Host Controller hardware Interrupt Initialization
 *--------------------------------------------------------------*/
void fnvHcIntEnable(ohci_t *ohci)
{
	ULONG		uData;

	/* Clear all pending int. source */
	uData = 0xFFFFFFFF;
	fnvIsp1161HcWrite(REG_IRQ, uData);

	/* Enable int. according settings in host_conf.h */
	uData = 0;			/* All int. are initially disabled */

	/* Should SOF_ITL_INT be enabled? */
	uData |= SOF_ITL_INT;

	/* Should OPR_INT be enabled? */
	uData |= OPR_INT;

	/* Should ATL_INT be enabled? */
	if (0)
		uData |= ATL_INT;

	/* Should EOT_INT be enabled? */
	if (0)
		uData |= EOT_INT;

	/* Should HC_SUSPEND_INT be enabled? */
	if (0)
		uData |= HC_SUSPEND_INT;

	/* Should HC_RESUME_INT be enabled? */
	if (0)
		uData |= HC_RESUME_INT;


	fnvIsp1161HcWrite(REG_IRQ_MASK, uData);

} /* End of fnvHcIntEnable() */

/*--------------------------------------------------------------*
 * 1161 Host Controller Control Register Initialization
 *--------------------------------------------------------------*/
void fnvHcControlInit(ohci_t *ohci)
{
	ULONG uData;
	ULONG uHostConfigData;

	uHostConfigData = 0;

	/* Fill the control register of 1161 */
	/* (1) Set the sate to operational */
	uData = HC_STATE;
	uHostConfigData &= (~HC_CONTROL_HCFS);
	uHostConfigData |= (uData << 6);

	/* (2)  Enable remote wakeup connection if opted */
	uData = REMOTE_WAKEUP_CONN;
	uHostConfigData &= (~HC_CONTROL_RWC);
	if (uData == YES)
		uHostConfigData |= HC_CONTROL_RWC;

	/* (3)  Enable remote wakeup if opted */
	uData = REMOTE_WAKEUP_ENABLE;
	uHostConfigData &= (~HC_CONTROL_RWE);
	if (uData == YES)
		uHostConfigData |= HC_CONTROL_RWE;

	fnvIsp1161HcorWrite(ohci,uHcControl, uHostConfigData);


	/* Configure the HCD transfer control registers uHcHcdControl and uHcHcdCommandStatus */
	uHostConfigData = 0;			
	uData = PERIODIC_LIST_ENABLE;				/* Periodic list Enable ? */
	if (uData == YES)
		uHostConfigData |= HC_CONTROL_PLE;

	uData = ISO_ENABLE;							/* Isochronous transfer enabled  ? */
	if (uData == YES)
		uHostConfigData |= HC_CONTROL_IE;

	uData = CONTROL_LIST_ENABLE;				/* Control trasnfer enabled ? */
	if (uData == YES)
		uHostConfigData |= HC_CONTROL_CLE;

	uData = BULK_LIST_ENABLE;					/* Bulk Transfer Enabled ? */
	if (uData == YES)
		uHostConfigData |= HC_CONTROL_BLE;

	/* Note: HC_CONTROL_TIP (Transfer In Progress) is alreday set to 0 */
	ohci->hc_control = (OHCI_CTRL_CBSR & 0x3) | OHCI_CTRL_PLE | OHCI_USB_OPER ;

	fnvIsp1161HcorWrite(ohci,uHcHcdControl, uHostConfigData);

} /* End of fnvHcControlInit() */

/*--------------------------------------------------------------*
 * 1161 Host Controller Interrupt Register Initialization
 *--------------------------------------------------------------*/
void fnvHcInterruptInit(ohci_t *ohci)
{
//	ULONG uData;
	ULONG uHostConfigData;

	/* First of all, disable all interrupts */
	uHostConfigData = HC_INTERRUPT_ALL;
	fnvIsp1161HcorWrite(ohci,uHcInterruptDisable, uHostConfigData);

	uHostConfigData = 0;

        #if 0
	/* 1. Enable scheduling overrun interrupt */
	uData = SCH_OVR_INT;
	if ((uData == YES) && (stHostCallback.HcdSchOvHandler != NULL))
		uHostConfigData |= HC_INTERRUPT_SO;

	/* 2. Enable ALT list done interrupt */
	uData = ALT_LIST_DONE_INT;
	if (uData == YES)
		uHostConfigData |= HC_INTERRUPT_ATD;
        #endif

	/* 3. Enable Start of Frame interrupt */
	/*uData = SOF_INT;
	if (uData == YES)*/
		uHostConfigData |= HC_INTERRUPT_SF;

        #if 0
	/* 4. Enable Resume Detect interrupt */
	uData = RESUME_DETECT_INT;
	if ((uData == YES) && (stHostCallback.HcdRsmHandler != NULL))
		uHostConfigData |= HC_INTERRUPT_RD;

	/* 5. Enable Unrecoverable error interrupt */
	uData = UNRECOV_ERROR_INT;
	if ((uData == YES) && (stHostCallback.HcdUnrcvErrHandler != NULL))
		uHostConfigData |= HC_INTERRUPT_UE;

	/* 6. Enable Frame Number Overflow interrupt */
	uData = FN_OVR_INT;
	if ((uData == YES) && (stHostCallback.HcdFnOvHandler != NULL))
		uHostConfigData |= HC_INTERRUPT_FNO;

	/* 7. Enable Root Hub Status Change interrupt */
	uData = RH_STATUS_CHG_INT;
	if ((uData == YES) && (stHostCallback.HcdRhStsChgHandler != NULL))
		uHostConfigData |= HC_INTERRUPT_RHSC;
        #endif
 
	/* 8. Master Interrupt Enable */
	/*uData = MASTER_INT_ENABLE;
	if (uData == YES)*/
		uHostConfigData |= HC_INTERRUPT_MIE;

	/* Write the configuration to uHcInterruptEnable register */
	fnvIsp1161HcorWrite(ohci,uHcInterruptEnable, uHostConfigData);


} /* End of fnvHcInterruptInit() */

/*--------------------------------------------------------------*
 * 1161 Host Controller Frame Number interval Initialization
 *--------------------------------------------------------------*/
void fnvHcFmIntervalInit(ohci_t *ohci)
{
	ULONG uHostConfigData;

	/* Determine the fram interval and the largest data size */
	uHostConfigData = FRAME_INTERVAL | (FS_LARGEST_DATA << 16);

	/* Write the configuration to uHcFmInterval register */
	fnvIsp1161HcorWrite(ohci,uHcFmInterval, uHostConfigData);

} /* End of fnvHcFmIntervalInit() */


/*--------------------------------------------------------------*
 * 1161 Host Controller Roothub registers Initialization
 *--------------------------------------------------------------*/
void fnvHcRhPower(ohci_t *ohci)
{
	ULONG uData;
	ULONG uHostConfigData;

	/* Initialize the configuration data */
	uHostConfigData = 0;

	/* Enable or disable power switching */
	uData = PORT_POWER_SWITCHING;

	if (uData == NO)	/* No power switching; ports are always powered on when the HC is powered on */
		uHostConfigData |= HC_RH_DESCRIPTORA_NPS;

	else {

		/********************************************************/
		/* Power switching is enabled                           */
		/* Program root hub in per-port switching mode          */
		/********************************************************/
		uHostConfigData |= HC_RH_DESCRIPTORA_PSM;

	} /* else */

	/* Port over current protection */
	uData = OVER_CURRENT_PROTECTION;
	if (uData == NO)					/* No over current protection */
		uHostConfigData |= HC_RH_DESCRIPTORA_NOCP;

	else
		uData = PER_PORT_OVER_CURRENT_REPORT;
		if (uData == YES)
			uHostConfigData |= HC_RH_DESCRIPTORA_OCPM;

	/* Set the power on to power good time */
	uHostConfigData |= ((POWER_ON_TO_POWER_GOOD_TIME / 2) << 24);			/* Divided by 2, then move it into position */

	fnvIsp1161HcorWrite(ohci,uHcRhDescriptorA, uHostConfigData);

	/* Set the global power to all the ports */
        uHostConfigData = RH_HS_LPSC;
        fnvIsp1161HcorWrite(ohci,uHcRhStatus, uHostConfigData);

	/* Wait till the POWER ON TO POWER GOOD time */
        mdelay(POWER_ON_TO_POWER_GOOD_TIME);


	/* Set the HcRhDescriptorB register (13h) */
	uHostConfigData = DEVICE_REMOVABLE;

	if (PORT_POWER_SWITCHING == YES)
		uHostConfigData |= 0xFFFF0000;		/* Set PPCM bits 31..16 to disable gang-mode power switching */

	fnvIsp1161HcorWrite(ohci,uHcRhDescriptorB, uHostConfigData);

} /* End of fnvHcRhPower() */


/*--------------------------------------------------------------*
 * 1161 Host Controller Initialization
 *--------------------------------------------------------------*/
int	 fnuHci1161HostInit(ohci_t	*ohci)
{
	struct usb_device 	*usb_dev;

	/* 1. ISP1161 Host Controller Extended Registers Initialization */
	fnvHcHardwareConfig(ohci, ohci->irq);
	fnvHcIntEnable(ohci);

	/* ISO transfer registers */
	fnvIsp1161HcWrite(REG_ITL_BUFLEN, ITL_BUFF_LENGTH);
	fnvIsp1161HcWrite(REG_ITL_TGRATE, 0);

	/* Interrupt transfer registers */
	fnvIsp1161HcWrite(REG_INTL_BUFLEN, INTL_BUFF_LENGTH);
	fnvIsp1161HcWrite(REG_INTL_BLOCK_SZ, INTL_BLOCK_SZ);
	fnvIsp1161HcorWrite(ohci, REG_INTL_SKIP, 0xFFFFFFFF);
	fnvIsp1161HcorWrite(ohci, REG_INTL_LAST, 0);

	/* Aperiodic transfer registers */
	fnvIsp1161HcWrite(REG_ATL_BUFLEN, ATL_BUFF_LENGTH);
	fnvIsp1161HcWrite(REG_ATL_BLOCK_SZ, ATL_BLOCK_SZ);
	fnvIsp1161HcorWrite(ohci, REG_ATL_SKIP, 0xFFFFFFFF);
	fnvIsp1161HcorWrite(ohci, REG_ATL_LAST, 1 << ((ATL_BUFF_LENGTH / (8 + ATL_BLOCK_SZ)) - 1));
	fnvIsp1161HcWrite(REG_ATL_THRESH_CNT, 1);
	fnvIsp1161HcWrite(REG_ATL_THRESH_TOUT, 0);

	/* 2. ISP1161 Host Controller Operational Registers Initialization */

	pstDoneHead_hcd  = 0;								/* Done head */

	aIsoTdMapBuffer[0].byStatus = 0;
	aIsoTdMapBuffer[1].byStatus = 0;
	aIsoTdMapBuffer[0].uFrameNumber = INVALID_FRAME_NUMBER;
	aIsoTdMapBuffer[1].uFrameNumber = INVALID_FRAME_NUMBER;

	fnvHcControlInit(ohci);						 /* Initialize HcControl register */
	
	ohci->disabled = 0;

	fnvHcInterruptInit(ohci/*, stHostCallback*/);	/* Initialize HcInterruptEnable/Disable registers */

	fnvHcFmIntervalInit(ohci);				  /* Initialize HcFmInterval Register */

	fnvHcRhPower(ohci);						/* Root hub port power switching mode */

	ohci->rh.devnum = 0;					/* No root hub yet */

	ohci->p_ed_controlhead = NULL;			/* Initialize control & bulk list heads */
	ohci->p_ed_bulkhead = NULL;

	/* Allocate data structure for root hub */
	usb_dev = usb_alloc_dev ( NULL, ohci->bus) ;
	
	if( !usb_dev ) {
		ohci->disabled = 1;
		return -ENOMEM;
	}

	ohci->bus->root_hub = usb_dev;

	/* Connect the virtual hub to the usb bus, host stack will do all the enumeration,
	   configuration etc.. */
	usb_connect (usb_dev); 

	if(usb_new_device(usb_dev) != 0) {
		usb_free_dev(usb_dev);
		ohci->disabled = 1;
		return -ENODEV;
	}

	return 0;
} /* End of fnuHci1161HostInit() */

/*--------------------------------------------------------------*
 * 1161 HC Interrupt Functions									* 
 *--------------------------------------------------------------*/
/*--------------------------------------------------------------*
 * 1161 Host Controller Interrupt function						*
 *--------------------------------------------------------------*/
static void fnvHci1161IrqHandler (int irq, void * __ohci, struct pt_regs * r)
{

	ohci_t	*ohci = (ohci_t*)__ohci;
	ULONG uIntStatus;				/* Int. status of the HCOR */
	ULONG uEnabledHcorIntStatus;	/* HcInterruptStatus HCOR anded with HcInterruptEnable */
	ULONG uIntExtStatus;			/* Int. status of the ISP1161 ext. int register */
	ULONG uData1, uData2;

#if 0
	/* Check the int. status in the ISP1161 extended register */
	fnvIsp1161HcRead(REG_IRQ, &uData1);				/* Read the int. register */
	fnvIsp1161HcRead(REG_IRQ_MASK, &uData2);		/* Read the int. enable register */
#endif
	outw_hcd(HC_COM,REG_IRQ);
	uData1 = inw_hcd(HC_DATA);

	outw_hcd(HC_COM,REG_IRQ_MASK);
	uData2 = inw_hcd(HC_DATA);
	uIntExtStatus = uData1 & uData2;				/* Get the true int. status */

	/* uIntExtStatus contains enabled interrupt status bits read from the */
	/* ISP1161 HC ext. register REG_IRQ (0x24) */

	/* If the IRQ is not from ISP1161 HC, don't do anything. */
	if (uIntExtStatus == 0)				/* ISP1161 int. status in HC extended register */
		return;

	/* Get the enabled HcInterruptStatus HCOR */
	fnvIsp1161HcorRead(ohci,uHcInterruptStatus, &uData1);	/* Read the HCOR int. status */
	fnvIsp1161HcorRead(ohci,uHcInterruptEnable, &uData2);	/* Read the HCOR int. enable status */
	uIntStatus = uData1 & uData2;						/* Get the true int. status */
	uEnabledHcorIntStatus = uData1 & uData2;

	/*******************************/
	/* Process SOFITLInt IRQ       */
	/* SOF and ITL interrupts      */
	/*******************************/
	if (uIntExtStatus & SOF_ITL_INT) {

		if (uEnabledHcorIntStatus & HC_INTERRUPT_SF) {
			/* SOFITL Int. service */
            fnvProcessSofItlInt(ohci);
		}

		/* Clear the same bit of HC_INTERRUPT_SF in uEnabledHcorIntStatus */
		uEnabledHcorIntStatus &= ~HC_INTERRUPT_SF;

		/* Clear SOFITLInt IRQ of REG_IRQ register of ISP1161 ext regsiter */
		uData1 = SOF_ITL_INT;
		fnvIsp1161HcWrite(REG_IRQ, uData1);

	} /* if SOF_ITL_INT */

	/*********************************************/
	/* Process ATL_INT                           */
	/* ALT Done                                  */
	/*********************************************/
	if (uIntExtStatus & ATL_INT) {


		/* fnvProcessOprInt(pstHostNode); */

		/* Clear ATL_INT IRQ */
		uData1 = ATL_INT;
		fnvIsp1161HcWrite(REG_IRQ, uData1);
	} /* if ATL_INT */


	/*********************************************/
	/* Process EOT_INT                           */
	/* End of Transfer                           */
	/*********************************************/
	if (uIntExtStatus & EOT_INT) {


		/* fnvProcessOprInt(pstHostNode); */

		/* Clear EOT_INT IRQ */
		uData1 = EOT_INT;
		fnvIsp1161HcWrite(REG_IRQ, uData1);
	} /* if EOT_INT */



	/*********************************************/
	/* Process OPR_INT                           */
	/* HC operational register HcInterruptStatus */
	/*********************************************/
	if (uIntExtStatus & OPR_INT) {


		/*fnvProcessOprInt(pstHostNode, uEnabledHcorIntStatus);*/

		/* Clear OPR_INT IRQ */
		uData1 = OPR_INT;
		fnvIsp1161HcWrite(REG_IRQ, uData1);

	} /* if OPR_INT */


	/*********************************************/
	/* Process HC_SUSPEND_INT                    */
	/* HC Suspended                              */
	/*********************************************/
	if (uIntExtStatus & HC_SUSPEND_INT) {


		/* fnvProcessOprInt(pstHostNode); */

		/* Clear HC_SUSPEND_INT IRQ */
		uData1 = HC_SUSPEND_INT;
		fnvIsp1161HcWrite(REG_IRQ, uData1);
	} /* if HC_SUSPEND_INT */


	/*********************************************/
	/* Process HC_RESUME_INT                     */
	/* HC Resume int.                            */
	/*********************************************/
	if (uIntExtStatus & HC_RESUME_INT) {

		/* fnvProcessOprInt(pstHostNode); */

		/* Clear HC_RESUME_INT IRQ */
		uData1 = HC_RESUME_INT;
		fnvIsp1161HcWrite(REG_IRQ, uData1);
	} /* if HC_RESUME_INT */

#if 0
	/* Read the current frame number from HCOR uHcRmNumber */
	fnvIsp1161HcorRead(ohci,uHcFmRemaining, &uData1);
	printk("Frame Remaining = %d    ",uData1&0x00003FFF);
	fnvIsp1161HcorRead(ohci,uHcFmNumber, &uData2);
	printk("Frame = %d\n",uData2&0x0000FFFF);

	fnvIsp1161HcRead(REG_IRQ, &uData1);
	outw_hcd(HC_COM,REG_IRQ);
	uData1 = inw_hcd(HC_DATA);

	/* Enable HC interrupt */
	uData1 = HC_INTERRUPT_MIE;							/* Main interrupt */
	fnvIsp1161HcorWrite(ohci,uHcInterruptEnable, uData1);	/* Write to HCOR HcInterruptEnable */
#endif

} /* End of fnvHci1161IrqHandler() */

/*--------------------------------------------------------------*
 * 1161 Host Controller SOF/ITL Interrupt function				*
 *--------------------------------------------------------------*/
void fnvProcessSofItlInt(ohci_t * ohci)
{
	__u32   		uData1;
	__u8  		 	byData;
	__u32			uFrameNumber;			/* Frame number of 1161 */
	__u8			* pbyHcdAtlBuff;        /* HCD ATL buffer address */
	ed_t			* pstCurrentHcdEd;      /* Pointer to the current HCD ED entry in the HCD ED pool */
	td_t			* pstCurrentHcdGtd;     /* Pointer to the current HCD GTD entry in the HCD GTD pool */

	__u32   		uHcdEdIndex;			/* Ed index in Interrupt ed table */
	__u32   		uAtlIndex;				/* ATL buffer td-ptd index */
	__u32   		uPipeHandle;			/* URB pipe handle */
	__u32   		uAtlByteCount;			/* Atl buffer byte counter */
	__u32				uAtlDoneCur;		/* Last value from HcATLPTDDoneMap register */
	__u32   		uLastPtd;				/* number of PTD's in the buffer or last ptd */
	__u32   		uTotal;					/* General total bytes variable */
	__u32   		uIndex;					/* General count index */
	__u8			* pbyChar;
	__u32   		uStartPTDByteCount;
	__u8			* pbyData;
	__u32   		uPhysical;
	__u32   		uPtdCompletionCode;		/* PTD completion Code */
	__u32   		uPhysicalHead;			/* TD Head pointer of ED */
	__u32   		uPhysicalTail;			/* TD Tail pointer of ED */
	__u32			frame ;					/* Even or Odd frame */

	__u8                * pbyHcdItlBuff;
	__u32               uItlByteCount;		/* ITL buffer byte counter */
	isotd_map_buffer_t  * pstItdMapBuffer;	/* ITL map buffer pointer */
	__u32               uTdFrameNumber;		/* TD frmae number */
	__u8                uIsoTdMapBuffSelector = 0;		/* ITL0/1 buffer selector */
	__u8                uItlIndex = 0;		/* ITL td-ptd index */
	td_tree_addr_t      * pstItlTdMapBuffer;	/* ITL td-ptd map buffer pointer */
	__u32   			uData2;
	td_t				* pstTd;			/* td pointer */

	__u16				bytes;				/* General no of bytes */
	__u8			iso_in_flag = 0;
#ifdef __TRACE_CTL_LEVEL__
	__u32			count;					/* General count */
#endif
	__u32 uData32;

	static __u32 uAtlActive = 0;
	static __u32 uAtlDone;
	static __u32 uAtlLastStartMs;

	static __u32 uAtlLastDoneMs = 0;


	/* Read the current frame number from HCOR uHcRmNumber */
	fnvIsp1161HcorRead(ohci,uHcFmNumber, &uFrameNumber);


	frame = uFrameNumber & 0x1 ;


	pbyHcdAtlBuff = ohci->p_atl_buffer ;

	outw_hcd(HC_COM,REG_IRQ);
	uData1 = inw_hcd(HC_DATA);

	outw_hcd(HC_COM,REG_BUFF_STS);
	uData2 = inw_hcd(HC_DATA);

	if( uData1 & SOF_ITL_INT ) {
		uAtlLastStartMs++;
		uAtlLastDoneMs++;

		pbyHcdItlBuff = ohci->p_itl_buffer;

		uIsoTdMapBuffSelector = 0;

Iso_Done_Queue_Processing:

		pstItdMapBuffer = &(aIsoTdMapBuffer[uIsoTdMapBuffSelector]);

		pstItlTdMapBuffer = pstItdMapBuffer->aItlTdTreeBridge;
		


		if(pstItdMapBuffer->byStatus) {
			
			if(uFrameNumber == ((pstItdMapBuffer->uFrameNumber + 1)%0x10000)) {
				/* Read buffer, process & go to done */
				if(pstItdMapBuffer->byStatus & 0x02) {		/* ISO In ptd's exist */
					uItlByteCount = pstItdMapBuffer->uByteCount ;
//					if( (uData2 & ISOA_BUFF_DONE) || (uData2 & ISOB_BUFF_DONE) ) 
					{
						fnvIsp1161ItlRead(pbyHcdItlBuff, uItlByteCount);	/* Read ITL */
			

						uLastPtd = pstItdMapBuffer->byActiveTds;			/* # of ISO TD's under progress */
						pbyChar	= pbyHcdItlBuff;
		
						for( uIndex = 0; uIndex < uLastPtd; uIndex++) {
			
							pstCurrentHcdGtd = (td_t*)(pstItlTdMapBuffer[uIndex].pstHcdTd);	/* Td in process */
							pstCurrentHcdEd = pstCurrentHcdGtd->ed ;

							if( pstCurrentHcdEd->hwINFO & 0x1000) {		/* ISOC in PTD */
//								printk("Done PTD 3:: pstCurrentHcdGtd= %p, fn# %x\n", pstCurrentHcdGtd, uFrameNumber);
								/* process completion code */
								byData = pbyChar[1];

								if( (byData & PTD_ACTIVE) == 0 ) {
									/* Copy Compeltion code from PTD */
									uPtdCompletionCode = (__u32)( (byData & PTD_COMPLETION_CODE) >> 4);
//									printk("uPtdCompletionCode= %d",uPtdCompletionCode);

									/* TODO do the necessary things needed for the ed/td bits */
		
									pstCurrentHcdGtd->hwINFO &= ~HC_GTD_CC; /* Clear the original CC */
									pstCurrentHcdGtd->hwINFO |= uPtdCompletionCode << 28; /* Move CC into position */
					
									/* Forget about data toggle bit, as data toggle bit is not important for ISOC trnasfers */

									/* Write the CC part of psw for the td */
							
									/* Move data returned from device to the buffer for IN transfer */
									/* Take out the PID token from PTD byte 5 */

									if( uPtdCompletionCode == TD_CC_NOERROR || uPtdCompletionCode == TD_DATAUNDERRUN ) {
										pbyData = (__u8*) (pstCurrentHcdGtd->hwCBP+(pstCurrentHcdGtd->hwPSW[0] & 0x0FFF)) ;
										/* get the actual number of bytes transfered */
										uTotal = ((__u32) (pbyChar[1] & PTD_ACTUAL_BYTES98) ) << 8; /* Bit 9..8 */
										uTotal |= (__u32)(pbyChar[0]); /* Bit 7..0 */

										/* Write the actual amount of data transfered for ISO IN trnasfers,
										for out it is zero */	
										pstCurrentHcdGtd->hwPSW[0] = uTotal & 0x07FF;	
//										for(count=0; count< uTotal;count++) { if(count%32==0) printk("\n"); printk("%x  ", *(pbyChar+8+count));}
//										printk("** %p\n",pbyData);
										memcpy(pbyData,pbyChar+8,uTotal);
										pstCurrentHcdGtd->hwCBP = 0;
									} /* (uPtdCompletionCode == 0 || == 9) */ else
									{	
										pstCurrentHcdGtd->hwPSW[0] = 0; 
									}
									pstCurrentHcdGtd->hwPSW[0] |= (uPtdCompletionCode << 12);	

								}	/* (byData & PTD_ACTIVE) == 0 */
							} /* pstCurrentHcdEd->hwINFO & 0x1000 */ 

							pbyChar += pstItlTdMapBuffer[uIndex].uAtlNodeLength;
				
						} /* for( uIndex = 0; uIndex < uLastPtd; uIndex++) */
					} /* if( (uData2 & ISOA_BUFF_DONE) || (uData2 & ISOB_BUFF_DONE) ) */

					/* move pstInDone queue to the DoneQueue */
					pstTd = pstItdMapBuffer->pstIsoInDoneHead;
					if(pstTd) {
						while(pstTd->hwNextTD & HC_GTD_NEXTTD) pstTd = (td_t*)(pstTd->hwNextTD & HC_GTD_NEXTTD); 
						pstTd->hwNextTD = (__u32)(pstDoneHead_hcd);
						pstDoneHead_hcd  = pstItdMapBuffer->pstIsoInDoneHead;
					}
				} /* if(pstItdMapBuffer->byStatus & 0x02) */
				pstItdMapBuffer->byStatus = 0;
				pstItdMapBuffer->uFrameNumber = INVALID_FRAME_NUMBER;
				pstItdMapBuffer->pstIsoInDoneHead = 0;
				pstItdMapBuffer->byActiveTds = 0;
			} else if(uFrameNumber != pstItdMapBuffer->uFrameNumber ) {
		
				/* This is a missed frame processing, move pstInDone queue to the DoneQueue */
				pstTd = pstItdMapBuffer->pstIsoInDoneHead;
				if(pstTd) {
					while(pstTd->hwNextTD & HC_GTD_NEXTTD) {
						pstTd = (td_t*)(pstTd->hwNextTD & HC_GTD_NEXTTD); 
					}
//					printk("Done PTD missing 2:: pstTd= %p, fn# %x\n", pstTd, uFrameNumber);
					pstTd->hwNextTD = (__u32)(pstDoneHead_hcd);
					pstDoneHead_hcd  = pstItdMapBuffer->pstIsoInDoneHead;
				}
				pstItdMapBuffer->byStatus = 0;
				pstItdMapBuffer->uFrameNumber = INVALID_FRAME_NUMBER;
				pstItdMapBuffer->pstIsoInDoneHead = 0;
				pstItdMapBuffer->byActiveTds = 0;
			}
		}
		uIsoTdMapBuffSelector++;
		if(uIsoTdMapBuffSelector == 1) goto Iso_Done_Queue_Processing;
	}

	/* If ATL is complete, process the done-queue list */
	if (uAtlActive) {

		fnvIsp1161HcorRead(ohci, REG_ATL_DONE, &uAtlDoneCur);
		
#ifdef __TRACE_CTL_LEVEL__
		if (uAtlDoneCur)
		{
			fnvIsp1161HcorRead(ohci,uHcFmNumber, &uData32);
			printk("[FM-%d] HcATLPTDDoneMap: %08x |= %08X\n", uData32, uAtlDone, uAtlDoneCur);
		}
#endif

		uAtlDone |= uAtlDoneCur;

		if ((uAtlDone & uAtlActive) == uAtlActive || uAtlLastStartMs > ATL_DONE_TOUT) {
			
#ifdef __TRACE_CTL_LEVEL__
			fnvIsp1161HcorRead(ohci,uHcFmNumber, &uData32);
			printk("[FM-%d] ----------------------- READ: %08x / %08X\n", uData32, uAtlDone, uAtlActive);
#endif

			uAtlActive = 0;

			fnvIsp1161HcRead(REG_BUFF_STS, &uData32);
			uData32 &= ~ATL_ACTIVE;
			fnvIsp1161HcWrite(REG_BUFF_STS, uData32);

			fnvIsp1161HcorWrite(ohci, REG_ATL_SKIP, 0xFFFFFFFF);

			/* Copy ATL from HC to HCD */
			/* Get the number of total bytes in the ISP1161 ATL */

			uAtlByteCount = tstAtlBridge[0].uAtlNodeLength; /* uAtlNodeLength field contains total number of bytes */
			fnvIsp1161AtlRead(pbyHcdAtlBuff, uAtlByteCount);
			
			uAtlLastDoneMs = 0;

			/* Now, pbyHcdAtlBuff[] contains the contents read from ISP1161 internal ATL buffer */
			if (tstAtlBridge[0].pstHcdTd > 0) {
				/* tstAtlBridge[0].pstHcdTd is a reserved entry which contains index to */
				/* the last PTD in the tstAtlBridge[] */
				/* This is the number of Transfer Descriptors in the ATL buffer */

				uLastPtd = tstAtlBridge[0].pstHcdTd; /* Last index number of HCD GTD in PTD */
				pbyChar = pbyHcdAtlBuff;                        /* Address of the first HCD PTD buffer */

				/* Process PTDs one by one */
				for (uAtlIndex = 1; uAtlIndex <= uLastPtd; uAtlIndex++) {

#ifdef __TRACE_CTL_LEVEL__
					printk ("PTD#%d: %02X %02X %02X %02X %02X %02X %02X %02X - "
									"%02X %02X %02X %02X %02X %02X %02X %02X ...\n", uAtlIndex,
									pbyChar[0], pbyChar[1], pbyChar[2], pbyChar[3],
									pbyChar[4],	pbyChar[5], pbyChar[6], pbyChar[7],
									pbyChar[8], pbyChar[9], pbyChar[10], pbyChar[11],
									pbyChar[12], pbyChar[13], pbyChar[14], pbyChar[15]);
#endif

					/* Get the address of the ED associated with the current PTD  */
					pstCurrentHcdGtd = (td_t*)(tstAtlBridge[uAtlIndex].pstHcdTd);
					pstCurrentHcdEd = pstCurrentHcdGtd->ed ;

					/* Process completion code, and toggle bit */
					byData = pbyChar[1];    /* Byte 1 of PTD contains the above bits */
					if ((byData & PTD_ACTIVE) == 0) {       /* Test if the PTD is still active */

						/* Copy completion code to GTD */
						uPtdCompletionCode = (__u32) ((byData & PTD_COMPLETION_CODE) >> 4);

						bytes = (pbyChar[0]+((pbyChar[1]&3)<<8));	/* Get the actual bytes */

//						printk("Completion Code(0) = 0x%d, bytes = %d, OUT:%d\n",uPtdCompletionCode,bytes,(((pbyChar[5] & PTD_DIR) >> 2) == OHCI_OUT));

						/* MOCK OHCI IN SOFTWARE */
						
						/* Check if there is any transmission error (pid check fail, data toggle mismatch, crc fail, bitstuffing or dev not responding)
						and number of retries is not zero then process the PTD else go to transmission error processing */
						if(!(--(pstCurrentHcdGtd->retry_count)) || (uPtdCompletionCode != TD_PIDCHECKFAIL && 
							uPtdCompletionCode != TD_CC_DATATOGGLEM && uPtdCompletionCode  != TD_CC_CRC && 
							uPtdCompletionCode != TD_CC_BITSTUFFING && uPtdCompletionCode != TD_DEVNOTRESP)) 
						{


						pstCurrentHcdGtd->hwINFO &= ~HC_GTD_CC; /* Clear the original CC */
						pstCurrentHcdGtd->hwINFO |= uPtdCompletionCode << 28; /* Move CC into position */

//						if(uPtdCompletionCode) printk("Completion Code = 0x%d\n",uPtdCompletionCode);

						/* Data Toggle */
						/* Copy the data toggle from PTD to ED and TD if there is no error or if under run or over run (for IN transfers),
						for others it is treated as error and some error handling mechanism should be done */
						if (uPtdCompletionCode == TD_CC_NOERROR || 
						((uPtdCompletionCode == TD_DATAOVERRUN || uPtdCompletionCode == TD_DATAUNDERRUN) && (((pbyChar[5] & PTD_DIR) >> 2) == OHCI_IN))) {
							/* Get the data toggle bit from PTD in ATL */
							uData1 = 0;
							uData1 = (__u32) ((pbyChar[1] & PTD_TOGGLE) >> 2);
#ifdef __1161_ES3__
							if(pstCurrentHcdEd->type == PIPE_INTERRUPT && bytes == 0 ) {
								uData1 = ( (~uData1) & 0x01);
							}
#endif /* __1161_ES3__ */
							/* Set the toggle bit same as the PTD in ATL */
							pstCurrentHcdGtd->hwINFO &= ~HC_GTD_TLSB;    /* Clear the toggle bit */
							pstCurrentHcdGtd->hwINFO |= (uData1 << 24);  /* Move it into position */
	
							pstCurrentHcdEd->hwHeadP &= ~HC_ED_TOGGLE;		/* clear the toggle bit in ED */
							pstCurrentHcdEd->hwHeadP |= (uData1 << 1);		/* copy the ptd toggle bit here */

						}


						/*******************************/
						/* Move TDs to done-queue list */
						/*******************************/

						/* Dequeue the current TD (the first TD in the TD queue from ED */
						pstCurrentHcdEd->hwHeadP &= ~HC_ED_HEADP; 							/* Clear the head pointer */
						pstCurrentHcdEd->hwHeadP |= (pstCurrentHcdGtd->hwNextTD & HC_GTD_NEXTTD);

						/* Move the TD to the head of done queue */
						pstCurrentHcdGtd->hwNextTD = (__u32)pstDoneHead_hcd;
						pstDoneHead_hcd = pstCurrentHcdGtd;

						/* Take out the PID token from PTD byte 5 */
						byData = (pbyChar[5] & PTD_DIR) >> 2;

						/* Move data returned from device to the buffer for IN transfer */
						/* The data in case of under run as well as over run is valid and we have to copy the data to td buffer */
						if ((byData == OHCI_IN) && ((uPtdCompletionCode == TD_CC_NOERROR) ||
						(uPtdCompletionCode == TD_DATAUNDERRUN ) || (uPtdCompletionCode == TD_DATAOVERRUN))) {

							pbyData = (__u8*) pstCurrentHcdGtd->hwCBP; 						/* Starting address */

							/* Get the actual number of transferred bytes */
							uTotal = ((__u32) (pbyChar[1] & PTD_ACTUAL_BYTES98)) << 8; 		/* Bit 9..8 */
							uTotal |= (__u32) (pbyChar[0]); 								/* Bit 7..0 */

							for (uIndex = 0; uIndex < uTotal; uIndex++) {
								pbyData[uIndex] = pbyChar[uIndex + 8];  					/* PTD header is 8 byte long */

							} /* for (uIndex = 0; uIndex < uTotal; uIndex++) */

						} /* if (byData == OHCI_IN) */

						/* Update the Current Buffer pointer.
						if success:: make CBP = 0 to signal that complete TD is success.
						if underrun/overrun, update it to the received bytes so that the application knows how many bytes are transfered */
						if( uPtdCompletionCode == TD_CC_NOERROR) pstCurrentHcdGtd->hwCBP = 0;    /* To signal that last byte has been transferred */
						else if(byData == OHCI_IN && (uPtdCompletionCode == TD_DATAUNDERRUN || uPtdCompletionCode == TD_DATAOVERRUN) ) {
							pstCurrentHcdGtd->hwCBP += bytes;
						}

						/* If the buffer rounding bit is set, and its an under run case, we treat that as success */
						if(uPtdCompletionCode == TD_DATAUNDERRUN && (pstCurrentHcdGtd->hwINFO & TD_R)) {
							uPtdCompletionCode = TD_CC_NOERROR;
						}

						/* For any error other than success we have to halt the End Point */
						if(uPtdCompletionCode != TD_CC_NOERROR)
						{
#ifdef __TRACE_CTL_LEVEL__
							printk ("@@@@@@@@@@@@@@@@@@@@@@@@@@@@ HALT EP @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
#endif
							pstCurrentHcdGtd->ed->hwHeadP |= 0x00000001;
						}

						} else {
#if 0
							/* ERROR_HANDLING_FIX:: */
							/* Allt the transmission error cases are handled here.  Inverse and copy the toggle bit, 
							copy any data if present, update current buffer pointer and don't put this in done queue so that 
							this td is scheduled for transfer again */

							byData = (pbyChar[5] & PTD_DIR) >> 2;
							if(byData == OHCI_OUT|| (byData == OHCI_IN )) {			/* Assuming SETUP tokens does not have this problem */

								/* Get the data toggle bit from PTD in ATL */
								uData1 = 0;
								uData1 = (__u32) ((pbyChar[1] & PTD_TOGGLE) >> 2);

								uData1 = ( (~uData1) & 0x01);						/* Inverse the PTD toggle bit */
								/* Set the toggle bit same as the PTD in ATL */
								pstCurrentHcdGtd->hwINFO &= ~HC_GTD_TLSB;    /* Clear the toggle bit */
								pstCurrentHcdGtd->hwINFO |= (uData1 << 24);  /* Move it into position */
	
								pstCurrentHcdEd->hwHeadP &= ~HC_ED_TOGGLE;		/* Clear the ED toggle bit */
								pstCurrentHcdEd->hwHeadP |= (uData1 << 1);		/* Move the toggle bit to ED */

								/* Copy the received bytes in case of IN Transfer */
								if(byData == OHCI_IN) {
									pbyData = (__u8*) pstCurrentHcdGtd->hwCBP; 			/* Starting address */
									for (uIndex = 0; uIndex < bytes; uIndex++) {
										pbyData[uIndex] = pbyChar[uIndex + 8];  		/* PTD header is 8 byte long */
									} /* for (uIndex = 0; uIndex < uTotal; uIndex++) */
								}
								pstCurrentHcdGtd->hwCBP += bytes;						/* Update the current buffer pointer */
							}
#endif
								
						}
					} /* if ((byData & PTD_ACTIVE) == 0) */

					byData = pbyChar[1];    /* Byte 1 of PTD contains the PTD active bit */
					if ((byData & PTD_ACTIVE)) {    /* Test if the PTD is still active */
						/* Get the actual number of transferred bytes */
						uTotal = 0;
						uTotal = ((__u32) (pbyChar[1] & PTD_ACTUAL_BYTES98)) << 8; /* Bit 9..8 */
						uTotal |= (__u32) (pbyChar[0]); /* Bit 7..0 */

						/* Copy completion code to GTD */
						uPtdCompletionCode = (__u32) ((byData & PTD_COMPLETION_CODE) >> 4);


						/* Set the toggle bit same as the PTD in ATL */

						uData1 = 0;
						uData1 = (__u32) ((pbyChar[1] & PTD_TOGGLE) >> 2);      /* Byte 1 contains data toggle bit */

						pstCurrentHcdGtd->hwINFO &= ~HC_GTD_TLSB;    /* Clear the toggle bit */
						pstCurrentHcdGtd->hwINFO |= (uData1 << 24);  /* Move it into position */

						pstCurrentHcdEd->hwHeadP &= ~HC_ED_TOGGLE;
						pstCurrentHcdEd->hwHeadP |= (uData1 << 1);

						/* If the transaction is an IN transaction, and still PTD is active means
						we have to copy the number of bytes received to the TD buffer and then
						increment the current buffer pointer */

						/* Take out the PID token from PTD byte 5 */
						byData = (pbyChar[5] & PTD_DIR) >> 2;
						if ((byData == OHCI_IN) && (uPtdCompletionCode == 0)) {
							pbyData = (__u8*) pstCurrentHcdGtd->hwCBP; /* Starting address */
							/* Get the actual number of transferred bytes */
							uTotal = ((__u32) (pbyChar[1] & PTD_ACTUAL_BYTES98)) << 8; /* Bit 9..8 */
							uTotal |= (__u32) (pbyChar[0]); /* Bit 7..0 */

							for (uIndex = 0; uIndex < uTotal; uIndex++) {
								pbyData[uIndex] = pbyChar[uIndex + 8];  /* PTD is 8 byte long */

							} /* for (uIndex = 0; uIndex < uTotal; uIndex++) */

						} /* if (byData == OHCI_IN) */
						pstCurrentHcdGtd->hwCBP += uTotal;              /* Advances the pointer to next block */

					} /* if ((byData & PTD_ACTIVE) == 1) */

					/* Get the address of next HCD PTD buffer */
					pbyChar += tstAtlBridge[uAtlIndex].uAtlNodeLength;

				} /* for */


			} /* if (tstAtlBridge[0].pstHcdTd > 0) */

			/* Clear the transfer-in-progress status bit */
			fnvIsp1161HcorRead(ohci,uHcHcdControl, &uData1);
			fnvIsp1161HcorWrite(ohci,uHcHcdControl, (__u32) (uData1 & ~OHCI_CTRL_TIP));
		} /* if ATL_BUFF_DONE */

		/* Clear the ATD (ATL List Done) bit of the HcInterruptStatus register */
		fnvIsp1161HcorRead(ohci,uHcInterruptStatus, &uData1);
		uData1 = OHCI_INTR_ATD;
		fnvIsp1161HcorWrite(ohci,uHcInterruptStatus, uData1);

#if 0
		/* Clear the ATLInt of the ISP1161 Interrupt register (0x24) */
		fnvIsp1161HcRead(REG_IRQ, &uData1);
		fnvIsp1161HcorWrite(ohci,REG_IRQ, (__u32) (uData1 & ~ATL_INT));
#endif

	} /* if (uData1 & ATL_INT) */

	/* Process the done-queue list */
	dl_done_list( ohci, dl_reverse_done_list(ohci,pstDoneHead_hcd));
	pstDoneHead_hcd = 0;

	if (ohci->ed_rm_list[!frame] != NULL) {
		dl_del_list (ohci, !frame);
	}

	/* Isochronous Transfer Scheduling */

	/* Check if Periodic Transfer and Isochronous transfer are enabled */

//	fnvIsp1161HcorRead(ohci,uHcFmNumber, &uFrameNumber);

	/* Select the td map buffer that is free, if none go to control tranfer */
	uIsoTdMapBuffSelector = 0;
	if( (aIsoTdMapBuffer[uIsoTdMapBuffSelector].byStatus) &&
		(aIsoTdMapBuffer[++uIsoTdMapBuffSelector].byStatus) ) 
			goto Control_Transfer_Scheduling;

	/* Initialize variables for list scheduling */
	pstItlTdMapBuffer = aIsoTdMapBuffer[uIsoTdMapBuffSelector].aItlTdTreeBridge;
	aIsoTdMapBuffer[uIsoTdMapBuffSelector].byActiveTds = 0;
	uHcdEdIndex = (uFrameNumber & 0x0000001F);
	pstCurrentHcdEd = ohci->ed_isohead;

	pstDoneHead_hcd = 0;

	/* Select the td map buffer that is free, if none go to control tranfer */
	while( pstCurrentHcdEd != NULL ) {

		/* Check if this is Isochronous Data Transfer ED and Skip bit is not
		set for this ED */
		if( (pstCurrentHcdEd->type == PIPE_ISOCHRONOUS) &&
			((pstCurrentHcdEd->hwINFO & OHCI_ED_SKIP) == 0) ) {

			uPhysicalHead = (pstCurrentHcdEd->hwHeadP) & HC_ED_HEADP;
			uPhysicalTail = (pstCurrentHcdEd->hwTailP) & HC_ED_TAILP;

			if( uPhysicalHead != uPhysicalTail ) {
				/* Find the td that should be sent in the next frame */
				pstCurrentHcdGtd = (td_t*)uPhysicalHead;

#if 1

				do {

					pstCurrentHcdGtd = (td_t*)uPhysicalHead;
					uTdFrameNumber = (pstCurrentHcdGtd->hwINFO & 0xFFFF);

					/* Compare the td frame number and current frame number */
					if(uTdFrameNumber == ((uFrameNumber +1)%0x10000)) {
					
						/* Get the pipe handle */
						uPipeHandle = pstCurrentHcdGtd->urb->pipe ;
//						printk("td scheduling :: %p fn# %x\n", pstCurrentHcdGtd->hwCBP, uTdFrameNumber);
						pstItlTdMapBuffer[uItlIndex].uPipeHandle = uPipeHandle;
						pstItlTdMapBuffer[uItlIndex++].pstHcdTd = (__u32)pstCurrentHcdGtd;
//						printk("td scheduling 1 :: %p fn# %x\n", pstCurrentHcdGtd, uFrameNumber);
						break;
					} else {

						if( (uTdFrameNumber - uFrameNumber) <= 0  ||
							(uTdFrameNumber - uFrameNumber) > 0xF000 ) {

//							printk("td skipping 1 :: %p fn# %x\n", pstCurrentHcdGtd, uTdFrameNumber);
							pstCurrentHcdGtd->hwINFO &= ~HC_GTD_CC;
							pstCurrentHcdGtd->hwINFO &= ((TD_CC_NOERROR) << 28);



							pstCurrentHcdEd->hwHeadP &= ~HC_ED_HEADP;
							pstCurrentHcdEd->hwHeadP |= (pstCurrentHcdGtd->hwNextTD & HC_GTD_NEXTTD);

							pstCurrentHcdGtd->hwNextTD = (__u32)pstDoneHead_hcd;
							pstDoneHead_hcd = pstCurrentHcdGtd;
						} else {
//							printk("td skipping 2 :: %p fn# %x\n", pstCurrentHcdGtd, uTdFrameNumber);
							break;
						}
					}
					uPhysicalHead = (pstCurrentHcdEd->hwHeadP) & HC_ED_HEADP;
					uPhysicalTail = (pstCurrentHcdEd->hwTailP) & HC_ED_TAILP;
				} while ( uPhysicalHead != uPhysicalTail);

#endif
			} /* uPhysicalHead != uPhysicalTail */

		} /* ISO & No Skip */

		pstCurrentHcdEd = (ed_t*)pstCurrentHcdEd->hwNextED ;
	} /* pstCurrentHcdEd != NULL */

	/* If there are any ISO Td's to be sent to the TD's */
	if(uItlIndex) {

		pbyHcdItlBuff = ohci->p_itl_buffer;
		uItlByteCount = 0;			/* Offset of the HCD ITL buffer, starting with byte 0 */
		uStartPTDByteCount = 0;		/* Starting address of a new PTD */
		aIsoTdMapBuffer[uIsoTdMapBuffSelector].byActiveTds = uItlIndex;
		aIsoTdMapBuffer[uIsoTdMapBuffSelector].pstIsoInDoneHead = 0;
		uLastPtd = uItlIndex;
		pbyChar = pbyHcdItlBuff;	/* Address of the first HCD PTD buffer */

//		printk("uLastPtd = %d\n",uLastPtd);
		
		/* Construct the HCD ITL buffer */
		for( uItlIndex = 0; uItlIndex < uLastPtd; uItlIndex++){

			pstCurrentHcdGtd = (td_t*)(pstItlTdMapBuffer[uItlIndex].pstHcdTd);
			pstCurrentHcdEd = pstCurrentHcdGtd->ed;

			/*****************************/
			/* PTD byte 0                */
			/* Bit 7..0: ActualByte(7:0) */
			/*****************************/
			pbyHcdItlBuff[uItlByteCount++] = 0;					/* Advances to byte 1 */

			/************************************/
			/* PTD byte 1                       */
			/* Bit 7..4: Completion code (3:0)) */
			/* Bit 3: Active                    */
			/* Bit 2: Toggle                    */
			/* Bit 1..0: ActualByte(9:8)        */
			/************************************/
			pbyHcdItlBuff[uItlByteCount] = 0;					/* Clear the current contents */

			/* Get the Compilation Code from Iso Ptd */
			byData = (__u8)((pstCurrentHcdGtd->hwINFO & HC_GTD_CC) >> 28);
			pbyHcdItlBuff[uItlByteCount] |= (byData << 4);		/* Move into position in PTD */
			pbyHcdItlBuff[uItlByteCount++] |= PTD_ACTIVE;		/* Set PTD to Active */

			/* Toggle bit is always 0 for Isochronous data transfers */
			/******************************/
			/* PTD byte 2                 */
			/* Bit 7..0: MaxPackSize(7:0) */
			/******************************/
			pbyHcdItlBuff[uItlByteCount] = 0;					/* Clear the current contents */
			uData1 = (__u16) ((pstCurrentHcdEd->hwINFO & HC_ED_MPS) >> 16);
			pbyHcdItlBuff[uItlByteCount++] = (__u8)(uData1);

			/*********************************/
			/* PTD byte 3                    */
			/* Bit 7..4: EndpointNumber(3:0) */
			/* Bit 3: Last                   */
			/* Bit 2: Speed                  */
			/* Bit 1..0: MaxPacketSize(9:8)  */
			/*********************************/
			pbyHcdItlBuff[uItlByteCount] = 0;					/* Clear the current contents */
			byData = (__u8) ((pstCurrentHcdEd->hwINFO & HC_ED_EN) >> 7);	/* get endpoint # */
			pbyHcdItlBuff[uItlByteCount] |= (byData << 4);		/* Move to ptd */

			if(uItlIndex == (uLastPtd -1)) {							/* Set the PTD as Last PTD */
				pbyHcdItlBuff[uItlByteCount] |= PTD_LAST;
			}

			if(pstCurrentHcdEd->hwINFO & HC_ED_SPD) {			/* Set High Speed Device */
				pbyHcdItlBuff[uItlByteCount] |= PTD_SPEED;
			}

			 /* Set MaxPacketSize(9:8) */
			pbyHcdItlBuff[uItlByteCount++] |= (__u8)((uData1 >> 8) & 0x03);/* ISP1161 uses 9..8 */

			/*******************************/
			/* PTD byte 4                  */
			/* Bit 7..0: TotalBytes(7:0)   */
			/*******************************/
			/* PTD byte 5                  */
			/* Bit 3..2: DirectionPID(1:0) */
			/* Bit 1..0: TotalBytes(9:8)   */
			/*******************************/
			pbyHcdItlBuff[uItlByteCount++] = 0;					/* Clear the current contents */
			pbyHcdItlBuff[uItlByteCount] = 0;					/* Clear the current contents */

			/* For filling the total bytes, we can depend on the CBP and BE as each Iso TD is split 
			to different TD's before filling */
			if(pstCurrentHcdGtd->hwCBP != 0) {					/* NULL data packet */	

				uData1 = pstCurrentHcdGtd->hwBE - (pstCurrentHcdGtd->hwCBP+(pstCurrentHcdGtd->hwPSW[0] & 0x0FFF)) + 1;
				pbyHcdItlBuff[uItlByteCount-1] = (__u8)uData1;	/* Fill 0-7 bits */
				pbyHcdItlBuff[uItlByteCount] = (__u8)(uData1 >> 8);	/* Fill 8-9 bits */
				uTotal = uData1;								/* total Number of payload bytes */
			} else {											/* NULL data packet */
				uTotal = 0;										/* total Number of payload bytes */
			}	

			byData = (__u8) (((pstCurrentHcdEd->hwINFO & HC_ED_DIR) >> 9) & 0x0C) ; /* Bit 12..11 in 3..2 */
			pbyHcdItlBuff[uItlByteCount++] |= byData;

			/******************************/
			/* PTD byte 6                 */
			/* Bit 7: Format              */
			/* Bit 6..0: Function address */
			/******************************/
			pbyHcdItlBuff[uItlByteCount] = 0;					/* Clear the current contents */

			/* Format bit is 1 for Isochronous transfers */
			pbyHcdItlBuff[uItlByteCount] |= PTD_FORMAT ;
			
			/* Set function address */
            byData = (__u8)(pstCurrentHcdEd->hwINFO & HC_ED_FA);
			pbyHcdItlBuff[uItlByteCount++] |= byData ;

			/**************/
			/* PTD byte 7 */
			/* Reserved   */
			/**************/
			pbyHcdItlBuff[uItlByteCount++] = 0;					/* Clear the current contents */

			/* Copy the payload if exists uTotal */
			pbyChar = (__u8*)(pstCurrentHcdGtd->hwCBP + (pstCurrentHcdGtd->hwPSW[0] & 0x0FFF) );
			if(uTotal) memcpy((pbyHcdItlBuff+uItlByteCount),pbyChar,uTotal);
			uItlByteCount += uTotal;

			/* Adjust the uItlByteCount to the next double word boundary */
			while(uItlByteCount & (ITL_ALIGNMENT - 1)) {
				 /* Clear the padding byte and advance to next byte */
                pbyHcdItlBuff[uItlByteCount++] = 0;
			}

			/* Save number of bytes of this PTD+Data */
			pstItlTdMapBuffer[uItlIndex].uAtlNodeLength = uItlByteCount - uStartPTDByteCount;
			uStartPTDByteCount = uItlByteCount;             /* Next PTD starts here */

			
			pstCurrentHcdGtd->hwINFO &= ~HC_GTD_CC; /* Clear the original CC */
			pstCurrentHcdGtd->hwINFO |= (TD_CC_NOERROR) << 28; /* Move CC into position */


			pstCurrentHcdEd->hwHeadP &= ~HC_ED_HEADP; /* Clear the head pointer */
			pstCurrentHcdEd->hwHeadP |= (pstCurrentHcdGtd->hwNextTD & HC_GTD_NEXTTD);

			if(pstCurrentHcdEd->hwINFO & 0x800) {		/* ISO OUT, move to done queue */
				/* Write the CC part of psw for the td */
				pstCurrentHcdGtd->hwPSW[0] = 0;	
				pstCurrentHcdGtd->hwPSW[0] |= (TD_CC_NOERROR) << 12;	
				pstCurrentHcdGtd->hwNextTD = (__u32)pstDoneHead_hcd;
				pstDoneHead_hcd = pstCurrentHcdGtd;
			} else {
				pstCurrentHcdGtd->hwNextTD = (__u32)(aIsoTdMapBuffer[uIsoTdMapBuffSelector].pstIsoInDoneHead);
				aIsoTdMapBuffer[uIsoTdMapBuffSelector].pstIsoInDoneHead = pstCurrentHcdGtd;
				iso_in_flag = 0x02;
			}

		} /* for uItlIndex */
		

		/* At this point, uAtlByteCount = number of total bytes in the HCD ATL buffer */
		aIsoTdMapBuffer[uIsoTdMapBuffSelector].byStatus = (0x01 | iso_in_flag);
		aIsoTdMapBuffer[uIsoTdMapBuffSelector].uByteCount = uItlByteCount;
		aIsoTdMapBuffer[uIsoTdMapBuffSelector].uFrameNumber = uFrameNumber +1;


#if 0
		uIndex = 0;
		printk("\n");
		for(uIndex=0;uIndex<uItlByteCount;uIndex++){
			if(uIndex%8==0) printk("\n");
			printk(" 0x%x ",pbyHcdItlBuff[uIndex]);
		}
#endif

#if 1

		/* Dump the HCD ITL buffer to the ISP1161 internal ITL buffer */
		/* Pass HCD ITL to ISP1161 internal ITL through the 32-bit ITLBuffer register */
		/* Use PIO for the time being. Will use DMA */
		fnvIsp1161ItlWrite(pbyHcdItlBuff, uItlByteCount);	/* Make sure uItlByteCount is multiple of 4 */

//		printk("ITL buffer bytes written:: %d\n", uItlByteCount);
	

#endif
	} /* if uItlIndex */


	/* Check if transfer is in progress */
	fnvIsp1161HcorRead(ohci,uHcHcdControl, &uData1);
	if (uData1 & OHCI_CTRL_TIP) {                          /* If the transfer is in progress */
		goto End_of_Transfer_List;                              /* you can not touch ATL */
	}

	/* I get too many errors when ATL buffer */
	/* is written immediately after it has been read */
	if (uAtlLastDoneMs < ATL_DONE_DELAY) {
		goto End_of_Transfer_List;
	}

	/****************************/
	/* Transfer list scheduling */
	/****************************/

Control_Transfer_Scheduling:

	/* Clear total number of bytes */
	tstAtlBridge[0].uAtlNodeLength = 0;		/* uPipeHandle field contains total number of bytes */

	/* Clear the index of last entry */
	tstAtlBridge[0].pstHcdTd = 0;			/* pstHcdTd field contains the index of the last entry */
	uAtlIndex = 1;							/* Starting from 1; index 0 is reserved */

	/* Control transfer scheduling */
	fnvIsp1161HcorRead(ohci,uHcHcdControl, &uData1);
	if ((uData1 & OHCI_CTRL_CLE) == 0)
		goto Bulk_Transfer_Scheduling;

	fnvIsp1161HcorRead(ohci,uHcHcdCommandStatus, &uData1);
	if ((uData1 & OHCI_CLF) == 0)
		goto Bulk_Transfer_Scheduling;

	pstCurrentHcdEd = ohci->p_ed_controlhead;

	while( pstCurrentHcdEd != NULL ) {

		/* If the SKIP bit of the current ED is set, skip it */
		if ((pstCurrentHcdEd->hwINFO & OHCI_ED_SKIP) == 0) {
			/* SKIP bit is not set */

			/* Check if this is an empty transfer list under this ED */
			/* If head TD pointer is equal to tail TD pointer, it is an empty TD list */

			uPhysicalHead = (pstCurrentHcdEd->hwHeadP) & HC_ED_HEADP;
			uPhysicalTail = (pstCurrentHcdEd->hwTailP) & HC_ED_TAILP;
			if (uPhysicalHead != uPhysicalTail) {

				/* Find out the index number of the TD in the HCD GTD pool */
				/* Must be greater than 0 */
				uPhysical = (pstCurrentHcdEd->hwHeadP) & HC_ED_HEADP;     /* Important: mask the CH bits */
				if(uPhysical != 0 ) {
					uPipeHandle = ((td_t*)uPhysical)->urb->pipe ;
					tstAtlBridge[uAtlIndex].pstHcdTd = uPhysical;
					tstAtlBridge[0].pstHcdTd = uAtlIndex;
					tstAtlBridge[uAtlIndex].uPipeHandle = uPipeHandle;
					/* Reserved entry (tstAtlBridge[0])is used to save the index to the last item */

					uAtlIndex++;

				} /* if uTdIndex > 0 */
			} /* if uEdHeadP != uEdTailP */

		} /* if OHCI_ED_SKIP */
		/* Advances to next HCD ED in the control transfer ED list */

		pstCurrentHcdEd = (ed_t*)(pstCurrentHcdEd->hwNextED);
	}

	/* Bulk transfer scheduling */
Bulk_Transfer_Scheduling:

	/* Check if bulk transfer is enabled */
	fnvIsp1161HcorRead(ohci,uHcHcdControl, &uData1);
	if ((uData1 & OHCI_CTRL_BLE) == 0)
		goto Int_Transfer_Scheduling;                   /* If bulk transfer is not enabled, don't do it */

	/* Check if bulk-list-filled bit is set */
	fnvIsp1161HcorRead(ohci,uHcHcdCommandStatus, &uData1);
	if ((uData1 & OHCI_BLF) == 0)
		goto Int_Transfer_Scheduling;                   /* If BLF is not set, don't do it */


	/*****************
	... Codes for bulk transfer list scheduling
	...
	******************/
	pstCurrentHcdEd = ohci->p_ed_bulkhead ;

	while ( pstCurrentHcdEd != NULL ) {

		/* If the SKIP bit of the current ED is set, skip it */
		if ((pstCurrentHcdEd->hwINFO & OHCI_ED_SKIP) == 0) {
			/* SKIP bit is not set */

			/* Check if this is an empty transfer list under this ED */
			/* If head TD pointer is equal to tail TD pointer, it is an empty TD list */
			uPhysicalHead = (pstCurrentHcdEd->hwHeadP) & HC_ED_HEADP;
			uPhysicalTail = (pstCurrentHcdEd->hwTailP) & HC_ED_TAILP;
			if (uPhysicalHead != uPhysicalTail) {

				/* Find out the index number of the TD in the HCD GTD pool */
				/* Must be greater than 0 */
				uPhysical = (pstCurrentHcdEd->hwHeadP) & HC_ED_HEADP;     /* Important: mask the CH bits */

				/* TD is found in the HCD GTD pool */
				if (uPhysical != 0) {
					uPipeHandle = ((td_t*)uPhysical)->urb->pipe;

					tstAtlBridge[uAtlIndex].uPipeHandle = uPipeHandle;
					tstAtlBridge[uAtlIndex].pstHcdTd = uPhysical;
					tstAtlBridge[0].pstHcdTd = uAtlIndex;
					/* Reserved entry (tstAtlBridge[0])is used to save the index to the last item */

					uAtlIndex++;

				} /* if uTdIndex > 0 */
			} /* if uEdHeadP != uEdTailP */

		} /* if OHCI_ED_SKIP */

		/* Advances to next HCD ED in the bulk transfer ED list */
		pstCurrentHcdEd = (ed_t*)pstCurrentHcdEd->hwNextED ;
	}

	/* Interrupt transfer scheduling */
Int_Transfer_Scheduling:

//	goto Transfer_List_Done;

	/* Check if periodic transfer (int. and iso. transfer) is enabled */
	fnvIsp1161HcorRead(ohci,uHcHcdControl, &uData1);
	if ((uData1 & OHCI_CTRL_PLE) == 0)
		goto Transfer_List_Done;

	/*****************
	.. Codes for interrupt transfer and IsoChronous list scheduling
	...
	******************/
	uHcdEdIndex = (uFrameNumber & 0x0000001F);

	pstCurrentHcdEd = ohci->p_int_table[uHcdEdIndex];
	
	while( pstCurrentHcdEd != NULL ) {
		
		/* If the SKIP bit of the current ED is set, skip it */
		if ((pstCurrentHcdEd->hwINFO & OHCI_ED_SKIP) == 0 &&
			(pstCurrentHcdEd->type == PIPE_INTERRUPT) ) {
			/* SKIP bit is not set */

			/* Check if this is an empty transfer list under this ED */
			/* If head TD pointer is equal to tail TD pointer, it is an empty TD list */
			uPhysicalHead = (pstCurrentHcdEd->hwHeadP) & HC_ED_HEADP;
			uPhysicalTail = (pstCurrentHcdEd->hwTailP) & HC_ED_TAILP;
			if (uPhysicalHead != uPhysicalTail) {

				/* Find out the index number of the TD in the HCD GTD pool */
				/* Must be greater than 0 */
				uPhysical = (pstCurrentHcdEd->hwHeadP) & HC_ED_HEADP;     /* Important: mask the CH bits */

				/* TD is found in the HCD GTD pool */
				if (uPhysical != 0) {

					/* Make up a pipehandle from host ID and ED index */
										
					uPipeHandle = ((td_t*)uPhysical)->urb->pipe ;

					tstAtlBridge[uAtlIndex].uPipeHandle = uPipeHandle;
					tstAtlBridge[uAtlIndex].pstHcdTd = uPhysical;
					tstAtlBridge[0].pstHcdTd = uAtlIndex;
					/* Reserved entry (tstAtlBridge[0])is used to save the index to the last item */

					uAtlIndex++;

				} /* if uTdIndex > 0 */
			} /* if uEdHeadP != uEdTailP */

		} /* if OHCI_ED_SKIP */

		/* Advances to next HCD ED in the control transfer ED list */
				
		pstCurrentHcdEd = (ed_t*)pstCurrentHcdEd->hwNextED ;
	}

	/* Isochronous transfer scheduling */
	/* Check if isochronous transfer is enabled */
	fnvIsp1161HcorRead(ohci,uHcHcdControl, &uData1);
	if ((uData1 & OHCI_CTRL_IE) == 0)
		goto Transfer_List_Done;

	/*****************
	... Codes for isochronous transfer list scheduling
	...
	******************/

Transfer_List_Done:

	if (tstAtlBridge[0].pstHcdTd > 0) {
		/* tstAtlBridge[0].pstHcdTd is a reserved entry which contains index to */
		/* the last PTD in the tstAtlBridge[] */
	
		uAtlByteCount = 0;					/* Offset of the HCD ATL buffer, starting with byte 0 */
		uStartPTDByteCount = 0;         	/* Starting address of a new PTD */
		uLastPtd = tstAtlBridge[0].pstHcdTd; /* Last index number of HCD GTD in PTD */

		/* Construct HCD ATL buffer */
		for (uAtlIndex = 1; uAtlIndex <= uLastPtd; uAtlIndex++) {

			pstCurrentHcdGtd = (td_t*)(tstAtlBridge[uAtlIndex].pstHcdTd);

			pstCurrentHcdEd = pstCurrentHcdGtd->ed ;

			/*****************************/
			/* PTD byte 0                */
			/* Bit 7..0: ActualByte(7:0) */
			/*****************************/
			pbyHcdAtlBuff[uAtlByteCount] = 0;
			uAtlByteCount++;							/* Advances to byte 1 */


			/************************************/
			/* PTD byte 1                       */
				/* Bit 7..4: Completion code (3:0)) */
			/* Bit 3: Active                    */
			/* Bit 2: Toggle                    */
			/* Bit 1..0: ActualByte(9:8)        */
			/************************************/
			pbyHcdAtlBuff[uAtlByteCount] = 0;			/* Clear the contents before write */

			/* Gte completion code from OHCI GTD */
			byData = (__u8) ((pstCurrentHcdGtd->hwINFO & HC_GTD_CC) >> 28);
			pbyHcdAtlBuff[uAtlByteCount] |= (byData << 4);          /* Move into position in PTD */
			/* Set active bit to a 1 */
			pbyHcdAtlBuff[uAtlByteCount] |= PTD_ACTIVE;

			/* Get toggle bit from GTD if MSB of T field of the stGtd.uGtdControl is set */
			if (pstCurrentHcdGtd->hwINFO & HC_GTD_MLSB) {

				/* Get toggle bit from LSB of T field of the GTD stGtd.uGtdControl */
				byData = (__u8) ((pstCurrentHcdGtd->hwINFO & HC_GTD_TLSB) >> 24);

			} else {
				/* Get toggle bit from ED */
				byData = (__u8) ((pstCurrentHcdEd->hwHeadP & HC_ED_TOGGLE) >> 1);
			}

			/* Move toggle bit into position in PTD */
			pbyHcdAtlBuff[uAtlByteCount] |= (byData << 2);
	
			uAtlByteCount++;                                                /* Advances to byte 2 */

			/******************************/
			/* PTD byte 2                 */
			/* Bit 7..0: MaxPackSize(7:0) */
			/******************************/
			pbyHcdAtlBuff[uAtlByteCount] = 0;                       /* Clear the contents before write */
			byData = (__u8) ((pstCurrentHcdEd->hwINFO & HC_ED_MPS) >> 16);
			pbyHcdAtlBuff[uAtlByteCount] = byData;
	
			uAtlByteCount++;                                                /* Advances to byte 3 */

			/*********************************/
			/* PTD byte 3                    */
			/* Bit 7..4: EndpointNumber(3:0) */
			/* Bit 3: Last                   */
			/* Bit 2: Speed                  */
			/* Bit 1..0: MaxPacketSize(9:8)  */
			/*********************************/

			/* Set endpoint number */
			pbyHcdAtlBuff[uAtlByteCount] = 0;                       /* Clear the contents before write */
			byData = (__u8) ((pstCurrentHcdEd->hwINFO & HC_ED_EN) >> 7);
			pbyHcdAtlBuff[uAtlByteCount] |= (byData << 4);  /* Move it into position in PTD */

			/* Set device speed */
			if (pstCurrentHcdEd->hwINFO & HC_ED_SPD)
				pbyHcdAtlBuff[uAtlByteCount] |= PTD_SPEED;

			/* Set MaxPacketSize(9:8) */
			byData = (__u8) ((pstCurrentHcdEd->hwINFO & HC_ED_MPS) >> 24); /* OHCI bit 10..8 */
			pbyHcdAtlBuff[uAtlByteCount] |= (byData & 0x03);        /* ISP1161 uses 9..8 */
	
			uAtlByteCount++;                                        /* Advances to byte 4 */
		
			/*******************************/
			/* PTD byte 4                  */
			/* Bit 7..0: TotalBytes(7:0)   */
			/*******************************/
			/* PTD byte 5                  */
			/* Bit 3..2: DirectionPID(1:0) */
			/* Bit 1..0: TotalBytes(9:8)   */
			/*******************************/
			pbyHcdAtlBuff[uAtlByteCount] = 0;			/* Clear the contents before write, byte 4 */
			pbyHcdAtlBuff[uAtlByteCount + 1] = 0;		/* Clear the contents before write, byte 5 */
	
			/* Set total bytes */
			if (pstCurrentHcdGtd->hwCBP == 0) {			/* NULL data packet */
				uTotal = 0;								/* Important! Must do it! */
				pbyHcdAtlBuff[uAtlByteCount] = 0;                       /* PTD byte 4: TotalByte(7:0) */
				pbyHcdAtlBuff[uAtlByteCount + 1] = 0;           /* PTD byte 5: TotalByte(9:8) */
			} else {
				/* Compute number of bytes of the data buffer */
				uData1 = pstCurrentHcdGtd->hwBE - pstCurrentHcdGtd->hwCBP + 1;
				pbyHcdAtlBuff[uAtlByteCount] = (__u8) uData1;          /* PTD byte 4 */
	
				/* ISP1161 PTD byte 5 uses 9..8 */
				pbyHcdAtlBuff[uAtlByteCount + 1] = (__u8) (uData1 >> 8);
	
				uTotal = uData1;        /* Save number of byte to transfer */
			} /* else */

			/* Set DIR PID in byte 5 */
	
			uData1 = (__u32) ((pstCurrentHcdEd->hwINFO & HC_ED_DIR) >> 11); /* Bit 12..11 */
			switch (uData1) {
				case OHCI_OUT:
					byData = OHCI_OUT;
					break;
	
				case OHCI_IN:
					byData = OHCI_IN;
					break;
	
				default:
					byData = (__u8) ((pstCurrentHcdGtd->hwINFO & HC_GTD_DP) >> 19);
			} /* switch */


			pbyHcdAtlBuff[uAtlByteCount + 1] |= (byData << 2);      /* Byte 5 */

#if __1161_ES3__
			if(pstCurrentHcdEd->type == PIPE_INTERRUPT) {
				pbyHcdAtlBuff[uAtlByteCount+1] |= 0x20;
			}
#endif /* __1161_ES3__ */

			uAtlByteCount += 2;             /* Advances to byte 6 from byte 4 */

			/******************************/
			/* PTD byte 6                 */
			/* Bit 7: Format              */
			/* Bit 6..0: Function address */
			/******************************/
			pbyHcdAtlBuff[uAtlByteCount] = 0;                       /* Clear the contents before write */
	
			/* Set Format */
			if (pstCurrentHcdEd->hwINFO & HC_ED_F)
				pbyHcdAtlBuff[uAtlByteCount] |= PTD_FORMAT;
	
			/* Set function address */
			byData = (__u8)(pstCurrentHcdEd->hwINFO & HC_ED_FA);
			pbyHcdAtlBuff[uAtlByteCount] |= byData;
	
			uAtlByteCount++;                                        /* Advances to byte 7 */
	
	
			/**************/
			/* PTD byte 7 */
			/* Reserved   */
			/**************/
			pbyHcdAtlBuff[uAtlByteCount] = 0;                       /* Clear the contents before write */
			uAtlByteCount++;                                        /* Advances to byte 8 */
	
			/* if there is any byte to transfer, i.e. uTotal > 0 */
			pbyChar = (__u8*) pstCurrentHcdGtd->hwCBP;     /* The address of the first byte */
	
			for (uIndex = 0; uIndex < uTotal; uIndex++) {
				pbyHcdAtlBuff[uAtlByteCount++] = *pbyChar;
				pbyChar++;              /* Advances to next byte */
			} /* for */
	
			/* Adjust the uAtlByteCount to the next PTD block boundary */
			while (uAtlByteCount % ATL_ALIGNMENT) {
	
				/* Clear the padding byte and advance to next byte */
				pbyHcdAtlBuff[uAtlByteCount++] = 0;
	
			} /* while */
	
			/* Save number of bytes of this PTD+Data */
			tstAtlBridge[uAtlIndex].uAtlNodeLength = uAtlByteCount - uStartPTDByteCount;
			uStartPTDByteCount = uAtlByteCount;             /* Next PTD starts here */
	
		} /* for */
	
		/* At this point, uAtlByteCount = number of total bytes in the HCD ATL buffer */
		tstAtlBridge[0].uAtlNodeLength = uAtlByteCount; /* uPipeHandle field contains total number of bytes */

#ifdef __TRACE_HIGH_LEVEL__
		for(uIndex=0; uIndex <uAtlByteCount;uIndex++) {
			if(uIndex%8 == 0) printk("\n");	
			printk("0x%x ",pbyHcdAtlBuff[uIndex]);
		}
		printk("\n");	
#endif /* __TRACE_HIGH_LEVEL__ */

		fnvIsp1161AtlWrite(pbyHcdAtlBuff, uAtlByteCount);       /* Make sure uAtlByteCount is multiple of 4 */

		uAtlDone = 0;
		uAtlLastStartMs = 0;
		uAtlActive = (1 << uLastPtd) - 1;

		fnvIsp1161HcorWrite(ohci, REG_ATL_SKIP, ~uAtlActive);

#ifdef __TRACE_CTL_LEVEL__
		fnvIsp1161HcorRead(ohci,uHcFmNumber, &uData32);
		printk("[FM-%d] +++++++++++++++++++++++ WRITE %d PTDs\n", uData32, uLastPtd);
		for (count = 0; count < uLastPtd; count ++)
		{
			u_char * p = pbyHcdAtlBuff + count * ATL_ALIGNMENT;
			printk("PTD#%d: %02X %02X %02X %02X %02X %02X %02X %02X - "
						 "%02X %02X %02X %02X %02X %02X %02X %02X ...\n", count + 1,
						 p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
						 p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
		}
#endif

		/* Finally, set the HCD Transfer-In-Progress bit. This bit will be cleared */
		/* in ATL Done ISR */
		fnvIsp1161HcorRead(ohci,uHcHcdControl, &uData1);
		uData1 |= OHCI_CTRL_TIP;
		fnvIsp1161HcorWrite(ohci,uHcHcdControl, uData1);

		/* Start ATL processing */
		fnvIsp1161HcorRead(ohci,REG_BUFF_STS, &uData1);
		uData1 |= ATL_ACTIVE;
		fnvIsp1161HcorWrite(ohci,REG_BUFF_STS, uData1);
	} /* if */

End_of_Transfer_List:

	/***************************************************************************/
	/* Clear the OHCI_INTR_SF bit of HCOR */
	uData1 = OHCI_INTR_SF;
	fnvIsp1161HcorWrite(ohci,uHcInterruptStatus, uData1);        /* Write to HCOR HcInterruptEnable */

	if(0) {
	/* Read the current frame number from HCOR uHcRmNumber */
	fnvIsp1161HcorRead(ohci,uHcFmRemaining, &uFrameNumber);
	printk("Frame Remaining = %d\n",uFrameNumber&0x00003FFF);
	}

} /* End of fnvProcessSofItlInt() */


/*-------------------------------------------------------------------------*
 * TD handling functions
 *-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*
 * fill the td with the information
 *-------------------------------------------------------------------------*/
static void td_fill (unsigned int info, void * data, int len, struct urb * urb, int index)
{
	td_t  * td, * td_pt;
	urb_priv_t * urb_priv = urb->hcpriv;

	/* Check if TD Index is > maximum number of td's for this urb */
	if (index >= urb_priv->length) {
		err("internal OHCI error: TD index > length");
		return;
	}

#ifdef __TRACE_CTL_LEVEL__
	printk("td_fill( info= %x, data = 0x%p, len = %d, urb = 0x%p, index = %d)\n",info,data, len, urb, index);
#endif	/* __TRACE_CTL_LEVEL__ */
	
	td_pt = urb_priv->td [index];

	/* fill the old dummy TD */
	td = urb_priv->td [index] = (td_t *) ((urb_priv->ed->hwTailP) & 0xfffffff0);

	/* Fill the non-ohci elements of td data structure */
	td->ed = urb_priv->ed;
	td->next_dl_td = NULL;
	td->index = index;
	td->urb = urb; 

	/* Fill the ohci elements of td data structure */
	td->hwINFO = (info);

	/* Fill the current buffer pointer ,based on length */
	if ((td->ed->type) == PIPE_ISOCHRONOUS) {
		td->hwCBP = ((!data || !len) ? 0 : ((__u32)data)) & 0xFFFFF000;
		td->ed->last_iso = info & 0xffff;
	} else {
		td->hwCBP = (!data || !len) ? 0 : ((__u32)data); 
	}			
	td->hwBE = (!data || !len ) ? 0 : ((__u32)data + len - 1);
	td->hwNextTD = (__u32)td_pt;
	td_pt->hwNextTD = 0;
	td->ed->hwTailP = td->hwNextTD;

	
	/* Only one buffer per Iso TD */
	td->hwPSW [0] = (((__u32)data) & 0x0FFF) | 0xE000;

//	if(td->ed->type == PIPE_ISOCHRONOUS) 	printk("td_fill :: %p , fn# %x\n", td->hwCBP, (td->hwINFO & 0xFFFF));


	/* Using of unused field for retry count for errors like
	crc check, bit stuffing, dev not responding pid check etc */
	td->retry_count = 3;

	if(data && len && (info & TD_DP_IN)){
		memset(data,0,(td->hwBE - td->hwCBP +1));
	}	

} /* End of td_fill */

/*-------------------------------------------------------------------------*
 * prepare all TDs of a transfer
 *-------------------------------------------------------------------------*/
static void td_submit_urb (struct urb * urb)
{ 
	urb_priv_t * urb_priv = urb->hcpriv;
	ohci_t * ohci = (ohci_t *) urb->dev->bus->hcpriv;
	void * ctrl = urb->setup_packet;
	void * data = urb->transfer_buffer;
	int data_len = urb->transfer_buffer_length;
	int cnt = 0; 
	__u32 info = 0;
  	unsigned int toggle = 0;
	__u32	data_read;

#ifdef __TRACE_MID_LEVEL__
	printk("td_submit_urb( urb = 0x%p)\n", urb);
#endif /* __TRACE_MID_LEVEL__ */

	/* OHCI handles the DATA-toggles itself, we just use the USB-toggle bits for reseting */
  	if(usb_gettoggle(urb->dev, usb_pipeendpoint(urb->pipe), usb_pipeout(urb->pipe))) {
  		toggle = TD_T_TOGGLE;
	} else {
  		toggle = TD_T_DATA0;
		usb_settoggle(urb->dev, usb_pipeendpoint(urb->pipe), usb_pipeout(urb->pipe), 1);
	}
	
	urb_priv->td_cnt = 0;
	
	switch (usb_pipetype (urb->pipe)) {
		case PIPE_BULK:
#ifdef __TRACE_CTL_LEVEL__
			printk("========== BULK TRANSFER (len=%d) ==========\n", data_len);
#endif
//			printk("spd = %d\n", (urb->transfer_flags & USB_DISABLE_SPD));
			info = usb_pipeout (urb->pipe)? 
				TD_CC | TD_DP_OUT : TD_CC | TD_DP_IN ;
			/* Fill the transfer descriptors of size MAX_BULK_TD_BUFF_SIZE.
			data toggle bit for first is set to 0 and rest depends on the
			toggle bit of previous td */
			while(data_len > MAX_BULK_TD_BUFF_SIZE) {		
//				printk("td_r:: not set\n");
				td_fill (info | (cnt? TD_T_TOGGLE:toggle), data, MAX_BULK_TD_BUFF_SIZE, urb, cnt);
				data += MAX_BULK_TD_BUFF_SIZE; data_len -= MAX_BULK_TD_BUFF_SIZE; cnt++;
			}

			/* Fill the last TD of bulk transfer */
//			printk("td_r:: set\n");
			info = usb_pipeout (urb->pipe)?
				TD_CC | TD_DP_OUT : TD_CC | TD_R | TD_DP_IN ;
			td_fill (info | (cnt? TD_T_TOGGLE:toggle), data, data_len, urb, cnt);
			cnt++;

			/* Set yje bulk list filled status on command status register */
			fnvIsp1161HcorRead(ohci,uHcHcdCommandStatus, &data_read);
			data_read |= OHCI_BLF;
			fnvIsp1161HcorWrite(ohci,uHcHcdCommandStatus, data_read);
			break;

		case PIPE_INTERRUPT:
#ifdef __TRACE_CTL_LEVEL__
			printk("========== INTERRUPT TRANSFER (len=%d) ==========\n", data_len);
#endif
			/* For interrupt transfer there is only one td and fill it */
			info = usb_pipeout (urb->pipe)? 
				TD_CC | TD_DP_OUT | toggle: TD_CC | TD_R | TD_DP_IN | toggle;
			td_fill (info, data, data_len, urb, cnt++);
			break;

		case PIPE_CONTROL:
#ifdef __TRACE_CTL_LEVEL__
			printk("========== CONTROL TRANSFER (len=%d) ==========\n", data_len);
#endif
			/* set up stage, one td */
			info = TD_CC | TD_DP_SETUP | TD_T_DATA0;
			td_fill (info, ctrl, 8, urb, cnt++); 

			if (data_len > 0) {  
				info = usb_pipeout (urb->pipe)? 
					TD_CC | TD_DP_OUT | TD_T_DATA1 : TD_CC | TD_DP_IN | TD_T_DATA1;
				/* Fill the transfer descriptors of size MAX_CNTL_TD_BUFF_SIZE */
				while(data_len > MAX_CNTL_TD_BUFF_SIZE) {		
					td_fill (info, data, MAX_CNTL_TD_BUFF_SIZE, urb, cnt);
					data += MAX_CNTL_TD_BUFF_SIZE; data_len -= MAX_CNTL_TD_BUFF_SIZE; cnt++;
				}

				/* Fill the last TD of control transfer */
				info = usb_pipeout (urb->pipe)?
					TD_CC | TD_R | TD_DP_OUT | TD_T_DATA1 : TD_CC | TD_R | TD_DP_IN | TD_T_DATA1;
				td_fill (info, data, data_len, urb, cnt);
				cnt++;
			}
			
			/* Status stage, one td */
			info = usb_pipeout (urb->pipe)? 
 				TD_CC | TD_DP_IN | TD_T_DATA1: TD_CC | TD_DP_OUT | TD_T_DATA1;
			td_fill (info, NULL, 0, urb, cnt++);

			fnvIsp1161HcorRead(ohci,uHcHcdCommandStatus, &data_read);
			data_read |= OHCI_CLF;
			fnvIsp1161HcorWrite(ohci,uHcHcdCommandStatus, data_read);
			break;

		case PIPE_ISOCHRONOUS:
#ifdef __TRACE_CTL_LEVEL__
			printk("========== ISOCHRONOUS TRANSFER (len=%d) ==========\n", data_len);
#endif
			/* Td's based on number of packets to be sent */
			for (cnt = 0; cnt < urb->number_of_packets; cnt++) {
//				printk("fn=%d,offset=%d\n",urb->start_frame + cnt,urb->iso_frame_desc[cnt].offset);
				td_fill (TD_CC|TD_ISO | ((urb->start_frame + cnt) & 0xffff), 
 					(__u8 *) data + urb->iso_frame_desc[cnt].offset, 
					urb->iso_frame_desc[cnt].length, urb, cnt); 
			}
			break;
	} 
	if (urb_priv->length != cnt) 
		dbg("TD LENGTH %d != CNT %d", urb_priv->length, cnt);

} /* End of td_submit_urb */

/*-------------------------------------------------------------------------*
 * ED handling functions
 *-------------------------------------------------------------------------*/  
		
/* search for the right branch to insert an interrupt ed into the int tree 
 * do some load ballancing;
 * returns the branch and 
 * sets the interval to interval = 2^integer (ld (interval)) */

static int ep_int_ballance (ohci_t * ohci, int interval, int load)
{
	int i, branch = 0;
   
	/* search for the least loaded interrupt endpoint branch of all 32 branches */
	for (i = 0; i < 32; i++) 
		if (ohci->ohci_int_load [branch] > ohci->ohci_int_load [i]) branch = i; 
  
	branch = branch % interval;
	for (i = branch; i < 32; i += interval) ohci->ohci_int_load [i] += load;

	return branch;
} /* End of ep_int_ballance */

/*-------------------------------------------------------------------------*/

/*  2^int( ld (inter)) */

/* Finding the nearest 2^n interval (1,2,4,8,16,32) for a given inter of any value */
static int ep_2_n_interval (int inter)
{	
	int i;
	for (i = 0; ((inter >> i) > 1 ) && (i < 5); i++); 
	return 1 << i;
} /* End of ep_2_n_interval */

/*-------------------------------------------------------------------------*/

/* the int tree is a binary tree 
 * in order to process it sequentially the indexes of the branches have to be mapped 
 * the mapping reverses the bits of a word of num_bits length */
 
static int ep_rev (int num_bits, int word)
{
	int i, wout = 0;

	for (i = 0; i < num_bits; i++) wout |= (((word >> i) & 1) << (num_bits - i - 1));
	return wout;
} /* End of ep_rev */

/*-------------------------------------------------------------------------*/

/* link an ed into one of the HC chains */

static int ep_link (ohci_t * ohci, ed_t * edi)
{	 
	int int_branch;
	int i;
	int inter;
	int interval;
	int load;
	ed_t	**ed_p;
	volatile ed_t * ed = edi;
	__u32	data_read;
	
	ed->state = ED_OPER;
	
#ifdef __TRACE_MID_LEVEL__
	printk("ep_link( ohci = 0x%p, ed = 0x%p)\n", ohci, edi);
#endif /* __TRACE_MID_LEVEL__ */

	switch (ed->type) {
	case PIPE_CONTROL:
		ed->hwNextED = 0;

		/* Check if the control list is empty */
		if (ohci->ed_controltail == NULL) {
			ohci->p_ed_controlhead = edi;
		} else {
			ohci->ed_controltail->hwNextED = (__u32)edi;
		}
		ed->ed_prev = ohci->ed_controltail;

		/* Fill control list enable bit in control register 
		if no to be remved ed list and ed is correct */
		if (!ohci->ed_controltail && !ohci->ed_rm_list[0] &&
			!ohci->ed_rm_list[1]) {
			ohci->hc_control |= OHCI_CTRL_CLE;
			fnvIsp1161HcorRead(ohci,uHcHcdControl,&data_read);
			data_read |= OHCI_CTRL_CLE;
			fnvIsp1161HcorWrite(ohci,uHcHcdControl, data_read);
		}

		/* set the ed control list tail to the current ed to be linked */
		ohci->ed_controltail = edi;	  
		break;
		
	case PIPE_BULK:
		ed->hwNextED = 0;

		/* Check if the bulk ed list is empty */
		if (ohci->ed_bulktail == NULL) {
			ohci->p_ed_bulkhead = edi;	
		} else {
			ohci->ed_bulktail->hwNextED = (__u32)edi;
		}
		ed->ed_prev = ohci->ed_bulktail;

		/* Fill bulk list enable bit in control register 
		if no to be remved ed list and ed is correct */

		if (!ohci->ed_bulktail && !ohci->ed_rm_list[0] &&
			!ohci->ed_rm_list[1]) {
			ohci->hc_control |= OHCI_CTRL_BLE;
			fnvIsp1161HcorRead(ohci,uHcHcdControl,&data_read);
			data_read |= OHCI_CTRL_BLE;
			fnvIsp1161HcorWrite(ohci,uHcHcdControl, data_read);
		}
		/* set the ed control list tail to the current ed to be linked */
		ohci->ed_bulktail = edi;	  
		break;
		
	case PIPE_INTERRUPT:
		load = edi->int_load;

		/* find the nearest 2^n interval */
		interval = ep_2_n_interval (ed->int_period);
		ed->int_interval = interval;

		/* find the interrupt brach where we can start linking this ed,
		for optimum load balancing */
		int_branch = ep_int_ballance (ohci, interval, load);
		ed->int_branch = int_branch;
		
		/* link the ed to all the lists in the interrupt table for
		the set interval */
		for (i = 0; i < ep_rev (6, interval); i += inter) {
			inter = 1;
			for (ed_p = &(ohci->p_int_table[ep_rev (5, i) + int_branch]); 
				(*ed_p != 0) && ((*ed_p)->int_interval >= interval); 
				ed_p = (ed_t**)(&((*ed_p)->hwNextED))) 
					inter = ep_rev (6, (*ed_p)->int_interval);
			ed->hwNextED = (__u32)(*ed_p); 
			*ed_p = edi;
		}  
#ifdef DEBUG
		print_int_ed_list(ohci);
		ep_print_int_eds (ohci, "LINK_INT");
#endif
		break;
		
	case PIPE_ISOCHRONOUS:
		ed->hwNextED = 0;
		ed->int_interval = 1;

		/* XXXX */
		if(ohci->ed_isotail != NULL) {
			ohci->ed_isotail->hwNextED = (__u32)edi;
		} else {
			ohci->ed_isohead = edi;
		}
		ed->ed_prev = ohci->ed_isotail;
		ohci->ed_isotail = edi;  
		
		
#if 0
		if (ohci->ed_isotail != NULL) {
			ohci->ed_isotail->hwNextED = (__u32)edi;
			ed->ed_prev = ohci->ed_isotail;
		} else {
		
			/* Since Isochronous list interval is 1 msec, link it to
			the ed lists based on the interval of 1 msec */
			for ( i = 0; i < 32; i += inter) {
				inter = 1;
				for (ed_p = &(ohci->p_int_table[ep_rev (5, i)]); 
					*ed_p != 0; 
					ed_p = (ed_t**)(&(*ed_p)->hwNextED)) 
						inter = ep_rev (6, (*ed_p)->int_interval);
				*ed_p = edi;	
			}	
			ed->ed_prev = NULL;
		}	
		ohci->ed_isotail = edi;  
#endif
#ifdef DEBUG
		ep_print_int_eds (ohci, "LINK_ISO");
#endif
		break;
	}	 	
	return 0;
} /* End of ep_link */

/*-------------------------------------------------------------------------*/

/* unlink an ed from one of the HC chains. 
 * just the link to the ed is unlinked.
 * the link from the ed still points to another operational ed or 0
 * so the HC can eventually finish the processing of the unlinked ed */

static int ep_unlink (ohci_t * ohci, ed_t * ed) 
{
	int 	int_branch;
	int 	i;
	int 	inter;
	int 	interval;
	ed_t	**ed_p;
	__u32	data_read;

#ifdef __TRACE_MID_LEVEL__
	printk("ep_unlink( ohci = 0x%p, ed = 0x%p)\n",ohci,ed);
#endif /* __TRACE_MID_LEVEL__ */

	ed->hwINFO |= (OHCI_ED_SKIP);

	switch (ed->type) {
	case PIPE_CONTROL:
		if (ed->ed_prev == NULL) {
			if (!ed->hwNextED) {
				/* THis is the only ed in the list */
				ohci->hc_control &= ~OHCI_CTRL_CLE;
				fnvIsp1161HcorRead(ohci,uHcHcdControl,&data_read);
				data_read &= ~OHCI_CTRL_CLE;
				fnvIsp1161HcorWrite(ohci, uHcHcdControl, data_read);

			}
			ohci->p_ed_controlhead = (ed_t*)(ed->hwNextED);
		} else {
			ed->ed_prev->hwNextED = ed->hwNextED;
		}

		/* set control list tail  if this is the last ed in the list */
		if (ohci->ed_controltail == ed) {
			ohci->ed_controltail = ed->ed_prev;
		} else {
			((ed_t *)(ed->hwNextED))->ed_prev = ed->ed_prev;
		}
		break;
      
	case PIPE_BULK:
		if (ed->ed_prev == NULL) {
			if (!ed->hwNextED) {
				/* THis is the only ed in the list */

				ohci->hc_control &= ~OHCI_CTRL_BLE;
				fnvIsp1161HcorRead(ohci,uHcHcdControl,&data_read);
				data_read &= ~OHCI_CTRL_BLE;
				fnvIsp1161HcorWrite(ohci, uHcHcdControl, data_read);
			}
			ohci->p_ed_bulkhead = (ed_t*)(ed->hwNextED);
		} else {
			ed->ed_prev->hwNextED = ed->hwNextED;
		}

		/* set control list tail  if this is the last ed in the list */
		if (ohci->ed_bulktail == ed) {
			ohci->ed_bulktail = ed->ed_prev;
		} else {
			((ed_t *)(ed->hwNextED))->ed_prev = ed->ed_prev;
		}
		break;
      
	case PIPE_INTERRUPT:
		int_branch = ed->int_branch;
		interval = ed->int_interval;

		/* remove ed from all the lists in the interrupt table */
		for (i = 0; i < ep_rev (6, interval); i += inter) {
			for (ed_p = &(ohci->p_int_table[ep_rev (5, i) + int_branch]), inter = 1; 
				(*ed_p != 0) && ((__u32)(*ed_p) != ed->hwNextED); 
				ed_p = (ed_t**)(&((*ed_p)->hwNextED)), 
				inter = ep_rev (6, (*ed_p)->int_interval)) {				
					if((*ed_p) == ed) {
			  			*ed_p = (ed_t*)(ed->hwNextED);		
			  			break;
			  		}
			  }
		}

		/* remove the load of this ed from the interrupt table lists */
		for (i = int_branch; i < 32; i += interval)
		    ohci->ohci_int_load[i] -= ed->int_load;

#ifdef DEBUG
		print_int_ed_list(ohci);
		ep_print_int_eds (ohci, "UNLINK_INT");
#endif
		break;
		
	case PIPE_ISOCHRONOUS:

		/* XXXX */
		if(ohci->ed_isotail == ed)
			ohci->ed_isotail = ed->ed_prev;

		if(ohci->ed_isohead == ed)
			ohci->ed_isohead = (ed_t*)(ed->hwNextED);

		if(ed->hwNextED != 0) 
		    ((ed_t *)(ed->hwNextED))->ed_prev = ed->ed_prev;

		if (ed->ed_prev != NULL) 
			ed->ed_prev->hwNextED = ed->hwNextED;
		
#if 0
		if (ohci->ed_isotail == ed)
			ohci->ed_isotail = ed->ed_prev;
		if (ed->hwNextED != 0) 
		    ((ed_t *)(ed->hwNextED))->ed_prev = ed->ed_prev;
				    
		if (ed->ed_prev != NULL) {
			ed->ed_prev->hwNextED = ed->hwNextED;
		} else {
			/* remove ed from all the lists in the interrupt table */
			for (i = 0; i < 32; i++) {
				for (ed_p = &(ohci->p_int_table[ep_rev (5, i)]); 
						*ed_p != 0; 
						ed_p = (ed_t**)(&((*ed_p)->hwNextED))) {
					// inter = ep_rev (6, ((ed_t *) bus_to_virt (le32_to_cpup (ed_p)))->int_interval);
					if((*ed_p) == ed) {
						*ed_p = (ed_t*)(ed->hwNextED);		
						break;
					}
				}
			}	
		}	
#endif

#ifdef DEBUG
		ep_print_int_eds (ohci, "UNLINK_ISO");
#endif
		break;
	}
	ed->state = ED_UNLINK;
	return 0;
} /* End of ep_unlink */


/*-------------------------------------------------------------------------*/
 
/* request the removal of an endpoint
 * put the ep on the rm_list and request a stop of the bulk or ctrl list 
 * real removal is done at the next start frame (SF) hardware interrupt */
 
static void ep_rm_ed (struct usb_device * usb_dev, ed_t * ed)
{    
	unsigned int frame;
	ohci_t * ohci = usb_dev->bus->hcpriv;
	__u32			data_read;

#ifdef __TRACE_MID_LEVEL__
	printk("ep_rm_ed( usb_dev = 0x%p, ed = 0x%p)\n",usb_dev,ed);
#endif /* __TRACE_MID_LEVEL__ */

	if ((ed->state & ED_DEL) || (ed->state & ED_URB_DEL))
		return;
	
	ed->hwINFO |= (OHCI_ED_SKIP);

	if (!ohci->disabled) {
		/* Stop control and bulk list processing till ed's are removed */
		switch (ed->type) {
			case PIPE_CONTROL: /* stop control list */
				ohci->hc_control &= ~OHCI_CTRL_CLE;
				fnvIsp1161HcorRead(ohci,uHcHcdControl, &data_read);
				data_read &= ~OHCI_CTRL_CLE;
				fnvIsp1161HcorWrite(ohci,uHcHcdControl,data_read);
			case PIPE_BULK: /* stop bulk list */
				ohci->hc_control &= ~OHCI_CTRL_BLE;
				fnvIsp1161HcorRead(ohci,uHcHcdControl, &data_read);
				data_read &= ~OHCI_CTRL_BLE;
				fnvIsp1161HcorWrite(ohci,uHcHcdControl,data_read);
				break;
		}
	}

	/* the concept of removing an ed from the list is to 
	put it in rm list and on the next SOF interrupt these will be
	cleared one done list is processed */

	fnvIsp1161HcorRead(ohci,uHcFmNumber,&data_read);
	frame = data_read & 0x1;
	ed->ed_rm_list = ohci->ed_rm_list[frame];
	ohci->ed_rm_list[frame] = ed;

	return;
} /* End of ep_rm_ed */

/*-------------------------------------------------------------------------*/

/* add/reinit an endpoint; this should be done once at the usb_set_configuration command,
 * but the USB stack is a little bit stateless  so we do it at every transaction
 * if the state of the ed is ED_NEW then a dummy td is added and the state is changed to ED_UNLINK
 * in all other cases the state is left unchanged
 * the ed info fields are setted anyway even though most of them should not change */
 
static ed_t * ep_add_ed (struct usb_device * usb_dev, unsigned int pipe, int interval, int load)
{
   	ohci_t * ohci = usb_dev->bus->hcpriv;
	td_t * td;
	ed_t * ed_ret;
	volatile ed_t * ed; 
	unsigned long flags;
 	
 	
	spin_lock_irqsave (&usb_ed_lock, flags);

	ed = ed_ret = &(usb_to_ohci (usb_dev)->ed[(usb_pipeendpoint (pipe) << 1) | 
			(usb_pipecontrol (pipe)? 0: usb_pipeout (pipe))]);

	if ((ed->state & ED_DEL) || (ed->state & ED_URB_DEL)) {
		/* pending delete request */
		spin_unlock_irqrestore (&usb_ed_lock, flags);
		return NULL;
	}
	
	if (ed->state == ED_NEW) {
		ed->hwINFO = (OHCI_ED_SKIP); /* skip ed */
  		/* dummy td; end of td list for ed */
		td = td_alloc (ohci);
  		if (!td) {
			/* out of memory */
			spin_unlock_irqrestore (&usb_ed_lock, flags);
			return NULL;
		}
		ed->hwTailP = (__u32)td;
		ed->hwHeadP = ed->hwTailP;	
		ed->state = ED_UNLINK;
		ed->type = usb_pipetype (pipe);
		usb_to_ohci (usb_dev)->ed_cnt++;
	}

	ohci->dev[usb_pipedevice (pipe)] = usb_dev;
	
	/* Fill the ed hardware information based on the given information */
	ed->hwINFO = (usb_pipedevice (pipe)
			| usb_pipeendpoint (pipe) << 7
			| (usb_pipeisoc (pipe)? 0x8000: 0)
			| (usb_pipecontrol (pipe)? 0: (usb_pipeout (pipe)? 0x800: 0x1000)) 
			| usb_pipeslow (pipe) << 13
			| usb_maxpacket (usb_dev, pipe, usb_pipeout (pipe)) << 16);
  
  	if (ed->type == PIPE_INTERRUPT && ed->state == ED_UNLINK) {
  		ed->int_period = interval;
  		ed->int_load = load;
  	}
  	
	spin_unlock_irqrestore (&usb_ed_lock, flags);
#ifdef __TRACE_MID_LEVEL__
	printk("ep_add_ed() ed_ret = 0x%p\n",ed_ret);
#endif /* __TRACE_MID_LEVEL__ */

	return ed_ret; 
} /* End of ep_add_ed */


/*-------------------------------------------------------------------------*
 * Done List handling functions
 *-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*
 * calculate the transfer length and update the urb
 *-------------------------------------------------------------------------*/
static void dl_transfer_length(td_t * td)
{
	__u32 		tdINFO, tdBE, tdCBP;
 	__u16 		tdPSW;
 	struct urb 		* urb = td->urb;
 	urb_priv_t 	* urb_priv = urb->hcpriv;
	int 		dlen = 0;
	int 		cc = 0;
	
	tdINFO = td->hwINFO;
  	tdBE   = td->hwBE;
  	tdCBP  = td->hwCBP;


#ifdef __TRACE_MID_LEVEL__
	printk("dl_transfer_length(td = 0x%p)\n",td);
#endif /* __TRACE_MID_LEVEL__ */
	/* Check if it is general td or ISOchronous td */
  	if (tdINFO & TD_ISO) {
 		tdPSW = td->hwPSW[0];
 		cc = (tdPSW >> 12) & 0xF;
		if (cc < 0xE)  {
			if (usb_pipeout(urb->pipe)) {
				dlen = urb->iso_frame_desc[td->index].length;
			} else {
				dlen = tdPSW & 0x3ff;
			}
			urb->actual_length += dlen;
			urb->iso_frame_desc[td->index].actual_length = dlen;
			if (!(urb->transfer_flags & USB_DISABLE_SPD) && (cc == TD_DATAUNDERRUN))
				cc = TD_CC_NOERROR;
					 
			urb->iso_frame_desc[td->index].status = cc_to_error[cc];
		}
	} else { /* BULK, INT, CONTROL DATA */
		/* if the transfer is for control and there exists a data stage for this get 
		the actual legth frp, the current buffer pointer, transfer bnuffer in urb
		and buffer end pointer */
		if (!(usb_pipetype (urb->pipe) == PIPE_CONTROL && 
				((td->index == 0) || (td->index == urb_priv->length - 1)))) {
 			if (tdBE != 0) {
 				if (td->hwCBP == 0)
					urb->actual_length = tdBE - (__u32)urb->transfer_buffer + 1;
  				else
					urb->actual_length = tdCBP - (__u32)urb->transfer_buffer;
			
#ifdef __TRACE_HIGH_LEVEL__
				{
					int i;
				printk("urb->actual_length = 0x%x\n",urb->actual_length);
				for(i=0;i<urb->actual_length;i++){
					if(i%8==0) printk("\n");
					printk("0x%x ",((__u8*)urb->transfer_buffer)[i]);
				}
				printk("\n");
				}
#endif /* __TRACE_HIGH_LEVEL__ */
			}
  		}
  	}
} /* End of dl_transfer_length */

/*-------------------------------------------------------------------------*
 * handle an urb that is being unlinked 
 *-------------------------------------------------------------------------*/
static void dl_del_urb (struct urb * urb)
{
	wait_queue_head_t * wait_head = ((urb_priv_t *)(urb->hcpriv))->wait;

#ifdef __TRACE_MID_LEVEL__
	printk("dl_del_urb(urb = 0x%p)\n",urb);
#endif /* __TRACE_MID_LEVEL__ */

	urb_rm_priv_locked (urb);

	if (urb->transfer_flags & USB_ASYNC_UNLINK) {
		urb->status = -ECONNRESET;
		if (urb->complete)
			urb->complete (urb);
	} else {
		urb->status = -ENOENT;

		/* unblock s1161_unlink_urb */
		if (wait_head)
			wake_up (wait_head);
	}
} /* End of dl_del_urb */

/*-------------------------------------------------------------------------*/

void print_int_ed_list(ohci_t	*ohci) {
ed_t	*ed;
int	i;
	
	printk("print_int_ed_list\n");
	for(i=0; i<32; i++) {

		ed = ohci->p_int_table[i];
		if(ed) printk("\n %d:::",i);
		while(ed!=NULL) {
			printk(":%p ::%d ",ed, ed->type);
			ed = (ed_t*)(ed->hwNextED);
		}
	}
}
/* there are some pending requests to remove 
 * - some of the eds (if ed->state & ED_DEL (set by s1161_free_dev)
 * - some URBs/TDs if urb_priv->state == URB_DEL */
 
static void dl_del_list (ohci_t  * ohci, unsigned int frame)
{
	unsigned long 	flags;
	ed_t 			* ed;
	__u32 			edINFO;
	__u32 			tdINFO;
	td_t 			* td = NULL, * td_next = NULL, * tdHeadP = NULL, * tdTailP;
	__u32 			* td_p;


#ifdef __TRACE_MID_LEVEL__
	printk("dl_del_list(ohci = 0x%p, frame = %d)\n",ohci, frame);
#endif /* __TRACE_MID_LEVEL__ */

	spin_lock_irqsave (&usb_ed_lock, flags);

	/* go through all the to be removed list of this frame */
	for (ed = ohci->ed_rm_list[frame]; ed != NULL; ed = ed->ed_rm_list) {

   	 	tdTailP = (td_t*)((ed->hwTailP) & 0xfffffff0);
		tdHeadP = (td_t*)((ed->hwHeadP) & 0xfffffff0);
		edINFO = ed->hwINFO;
		td_p = &(ed->hwHeadP);

		/* process all the td's of the ed */
		for (td = (td_t*)tdHeadP; td != (td_t*)tdTailP; td = td_next) { 
			struct urb * urb = td->urb;
			urb_priv_t * urb_priv = td->urb->hcpriv;
			
			td_next = (td_t*)((td->hwNextTD) & 0xfffffff0);
			if ((urb_priv->state == URB_DEL) || (ed->state & ED_DEL)) {
				/* If the delete is for urb delete request or ed delete request,
				update the processed td's count */
				tdINFO = td->hwINFO;
				if (TD_CC_GET (tdINFO) < 0xE)
					dl_transfer_length (td);
				*td_p = td->hwNextTD | (*td_p & (0x3));

				/* URB is done; clean up */
				if (++(urb_priv->td_cnt) == urb_priv->length)
					dl_del_urb (urb);
			} else {
				td_p = &td->hwNextTD;
			}
		}

		if (ed->state & ED_DEL) { /* set by s1161_free_dev */
			struct ohci_device * dev = usb_to_ohci (ohci->dev[edINFO & 0x7F]);
			td_free (ohci, tdTailP); /* free dummy td */
   	 		ed->hwINFO = OHCI_ED_SKIP; 
   	 		ed->state = ED_NEW; 
   	 		/* if all eds are removed wake up s1161_free_dev */
   	 		if (!--dev->ed_cnt) {
				wait_queue_head_t *wait_head = dev->wait;

				dev->wait = 0;
				if (wait_head)
					wake_up (wait_head);
			}
   	 	} else {
			/* URB delete */
   	 		ed->state &= ~ED_URB_DEL;
			tdHeadP = (td_t*)((ed->hwHeadP) & 0xfffffff0);

			/* If ed's td list is free un link it from the ed list */
			if (tdHeadP == tdTailP) {
				if (ed->state == ED_OPER)
					ep_unlink(ohci, ed);

				/* free the dummy td */
				td_free (ohci, tdTailP);
				ed->hwINFO = OHCI_ED_SKIP;
				
				/* set the ed state to new */
				ed->state = ED_NEW;
				--(usb_to_ohci (ohci->dev[edINFO & 0x7F]))->ed_cnt;
			} else
   	 			ed->hwINFO &= ~(OHCI_ED_SKIP);
   	 	}
   	}
   	
	/* make the remove list to null */
   	ohci->ed_rm_list[frame] = NULL;
   	spin_unlock_irqrestore (&usb_ed_lock, flags);
} /* End of dl_del_list */


/*-------------------------------------------------------------------------*/

/* replies to the request have to be on a FIFO basis so
 * we reverse the reversed done-list */
 
static td_t * dl_reverse_done_list (ohci_t * ohci, td_t *td_list)
{
	td_t * td_rev = NULL;
  	urb_priv_t * urb_priv = NULL;
  	unsigned long flags;
  	
  	spin_lock_irqsave (&usb_ed_lock, flags);
  	
	/* Go through all the td list in the done queue */
	while (td_list) {		

#if 1
		if (TD_CC_GET (td_list->hwINFO)) {
			urb_priv = (urb_priv_t *) td_list->urb->hcpriv;

#ifdef __TRACE_LOW_LEVEL__
			printk(" USB-error/status: %x : %p\n", TD_CC_GET (td_list->hwINFO), td_list);
#endif /* __TRACE_LOW_LEVEL__ */

			/* IF the ed halt bit is set, delete all the tds for that urb */
			if (td_list->ed->hwHeadP & (0x1)) {
				if (urb_priv && ((td_list->index + 1) < urb_priv->length)) {
					td_list->ed->hwHeadP = 
						(urb_priv->td[urb_priv->length - 1]->hwNextTD & 0xfffffff0) |
									(td_list->ed->hwHeadP & 0x2);
					urb_priv->td_cnt += urb_priv->length - td_list->index - 1;
				} else 
					td_list->ed->hwHeadP &= 0xfffffff2;
			}

		}
#endif

		/* attach td to the reverse list */
		td_list->next_dl_td = td_rev;	
		td_rev = td_list;
		td_list = (td_t*)((td_list->hwNextTD) & 0xfffffff0);	
	}	
	spin_unlock_irqrestore (&usb_ed_lock, flags);
	return td_rev;
} /* End of dl_reverse_done_list */

/*-------------------------------------------------------------------------*/

/* td done list */

static void dl_done_list (ohci_t * ohci, td_t * td_list)
{
  	td_t * td_list_next = NULL;
	ed_t * ed;
	int cc = 0;
	struct urb * urb;
	urb_priv_t * urb_priv;
 	__u32 tdINFO, edHeadP, edTailP;
//	int	i;
 	
 	unsigned long flags;
 	
#ifdef __TRACE_MID_LEVEL__
	/* only print if something interesting happens */
	if (td_list) printk("dl_done_list( ohci = 0x%p, td_list = 0x%p)\n", ohci, td_list);
#endif /* __TRACE_MID_LEVEL__ */

	/* Go through all the td's in the done queue */
  	while (td_list) {
   		td_list_next = td_list->next_dl_td;
   		
  		urb = td_list->urb;
  		urb_priv = urb->hcpriv;
  		tdINFO = td_list->hwINFO;
  		
   		ed = td_list->ed;
   		
		/* Get the trasnsfered length to the urb length field */
   		dl_transfer_length(td_list);
 			
  		/* error code of transfer */
  		cc = TD_CC_GET (tdINFO);
//		if(ed->type == PIPE_BULK) {
//			printk("cc = %d, length = %d, out= %d\n", cc, urb->actual_length, usb_pipeout(urb->pipe));
//			if(!usb_pipeout(urb->pipe)){
//				for(i=0;i<urb->actual_length;i++){
//					if(i%16==0) printk("\n");
//					printk("  %x ",((__u8*)urb->transfer_buffer)[i]);
//				}	
//				printk("\n");
//			}
//		}
		/* If the error code is stall call endpoint stall for that device */
  		if (cc == TD_CC_STALL)
			usb_endpoint_halt(urb->dev,
				usb_pipeendpoint(urb->pipe),
				usb_pipeout(urb->pipe));
  		
		/* If it is data underrun case and disable spd that is not an error case */
  		if (!(urb->transfer_flags & USB_DISABLE_SPD)
				&& (cc == TD_DATAUNDERRUN))
			cc = TD_CC_NOERROR;

  		if (urb_priv && ++(urb_priv->td_cnt) == urb_priv->length) {
#ifdef __TRACE_CTL_LEVEL__
			printk("dl_done_list: %d OF %d DONE\n", urb_priv->td_cnt, urb_priv->length);
#endif
			/* IF all the td's of this urb are finished, 
			call the corresponding call back function */
			if ((ed->state & (ED_OPER | ED_UNLINK))
					&& (urb_priv->state != URB_DEL)) {
				MARKER();
  				urb->status = cc_to_error[cc];			/* Convert the cc to USB stack status */
//				if(usb_pipetype(urb->pipe)!= PIPE_ISOCHRONOUS) s1161_return_urb (urb);
				s1161_return_urb(urb);
				MARKER();
  			} else {
				MARKER();
				/* If the opetation state of ed is deleted or unlinked meanwhile, delete the urb */
				spin_lock_irqsave (&usb_ed_lock, flags);
  				dl_del_urb (urb);
				spin_unlock_irqrestore (&usb_ed_lock, flags);
				MARKER();
			}
  		}

		MARKER();
  		spin_lock_irqsave (&usb_ed_lock, flags);
  		if (ed->state != ED_NEW) { 
  			edHeadP = (ed->hwHeadP) & 0xfffffff0;
  			edTailP = ed->hwTailP;

			/* unlink eds if they are not busy */
     			if ((edHeadP == edTailP) && (ed->state == ED_OPER)) 
     				ep_unlink (ohci, ed);
     		}	
     		spin_unlock_irqrestore (&usb_ed_lock, flags);
     	
    		td_list = td_list_next;
		MARKER();
  	}  
} /* End of dl_done_list */


/* ROOT Hub Functions */

/*-------------------------------------------------------------------------*
 * Virtual Root Hub 
 *-------------------------------------------------------------------------*/
 
/* Device descriptor */
static __u8 root_hub_dev_des[] =
{
	0x12,       /*  __u8  bLength; */
	0x01,       /*  __u8  bDescriptorType; Device */
	0x10,	    /*  __u16 bcdUSB; v1.1 */
	0x01,
	0x09,	    /*  __u8  bDeviceClass; HUB_CLASSCODE */
	0x00,	    /*  __u8  bDeviceSubClass; */
	0x00,       /*  __u8  bDeviceProtocol; */
	0x08,       /*  __u8  bMaxPacketSize0; 8 Bytes */
	0x00,       /*  __u16 idVendor; */
	0x00,
	0x00,       /*  __u16 idProduct; */
 	0x00,
	0x00,       /*  __u16 bcdDevice; */
 	0x00,
	0x00,       /*  __u8  iManufacturer; */
	0x02,       /*  __u8  iProduct; */
	0x01,       /*  __u8  iSerialNumber; */
	0x01        /*  __u8  bNumConfigurations; */
};


/* Configuration descriptor */
static __u8 root_hub_config_des[] =
{
	0x09,       /*  __u8  bLength; */
	0x02,       /*  __u8  bDescriptorType; Configuration */
	0x19,       /*  __u16 wTotalLength; */
	0x00,
	0x01,       /*  __u8  bNumInterfaces; */
	0x01,       /*  __u8  bConfigurationValue; */
	0x00,       /*  __u8  iConfiguration; */
	0x40,       /*  __u8  bmAttributes; 
                 Bit 7: Bus-powered, 6: Self-powered, 5 Remote-wakwup, 4..0: resvd */
	0x00,       /*  __u8  MaxPower; */
      
	/* interface */	  
	0x09,       /*  __u8  if_bLength; */
	0x04,       /*  __u8  if_bDescriptorType; Interface */
	0x00,       /*  __u8  if_bInterfaceNumber; */
	0x00,       /*  __u8  if_bAlternateSetting; */
	0x01,       /*  __u8  if_bNumEndpoints; */
	0x09,       /*  __u8  if_bInterfaceClass; HUB_CLASSCODE */
	0x00,       /*  __u8  if_bInterfaceSubClass; */
	0x00,       /*  __u8  if_bInterfaceProtocol; */
	0x00,       /*  __u8  if_iInterface; */
     
	/* endpoint */
	0x07,       /*  __u8  ep_bLength; */
	0x05,       /*  __u8  ep_bDescriptorType; Endpoint */
	0x81,       /*  __u8  ep_bEndpointAddress; IN Endpoint 1 */
 	0x03,       /*  __u8  ep_bmAttributes; Interrupt */
 	0x02,       /*  __u16 ep_wMaxPacketSize; ((MAX_ROOT_PORTS + 1) / 8 */
 	0x00,
	0xff        /*  __u8  ep_bInterval; 255 ms */
};

/* Hub class-specific descriptor is constructed dynamically */


/*-------------------------------------------------------------------------*/

/* prepare Interrupt pipe data; HUB INTERRUPT ENDPOINT */ 
 
static int rh_send_irq (ohci_t * ohci, void * rh_data, int rh_len)
{
	int num_ports;
	int i;
	int ret;
	int len;

	__u8 data[8];

	__u32 data_read;


	/* Get the number of root hub ports */
	fnvIsp1161HcorRead(ohci,uHcRhDescriptorA,&data_read);
	num_ports = data_read & RH_A_NDP;
#ifndef FIXME
	if (num_ports > MAX_ROOT_PORTS) {
		printk("rereads as NDP=%d\n", data_read & RH_A_NDP);
		/* retry later; "should not happen" */
		return 0;
	}
#endif
	/* Read the root hub status */
	fnvIsp1161HcorRead(ohci,uHcRhStatus,&data_read);

	*(__u8 *) data = (data_read & (RH_HS_LPSC | RH_HS_OCIC))
		? 1: 0;
	ret = *(__u8 *) data;

	for ( i = 0; i < num_ports; i++) {
		/* Read status of each port of the root hub and set 
		the status bit accordingly */
		fnvIsp1161HcorRead(ohci,(uHcRhPort1Status+i),&data_read);
		*(__u8 *) (data + (i + 1) / 8) |= 
			((data_read & (RH_PS_CSC | RH_PS_PESC | RH_PS_PSSC | RH_PS_OCIC | RH_PS_PRSC))
			    ? 1: 0) << ((i + 1) % 8);
		ret += *(__u8 *) (data + (i + 1) / 8);
	}
	len = i/8 + 1;
  
	if (ret > 0) { 
		int min_rh_len = min (rh_len, (int) sizeof(data));
		/* If there is some change in the root hub status,
		copy the status data to root hub data */
		memcpy (rh_data, data, min (len, min_rh_len));
		return len;
	}
	return 0;
} /* End of rh_send_irq */

/*-------------------------------------------------------------------------*/

/* Virtual Root Hub INTs are polled by this timer every "interval" ms */
 
static void rh_int_timer_do (unsigned long ptr)
{
	int len;
	unsigned long flags;

	struct urb * urb = (struct urb *) ptr;
	ohci_t * ohci = urb->dev->bus->hcpriv;

	if (ohci->disabled)
		return;

	/* ignore timers firing during PM suspend, etc */
	if ((ohci->hc_control & OHCI_CTRL_HCFS) != OHCI_USB_OPER)
		goto out;

	if(ohci->rh.send) {
		spin_lock_irqsave (&usb_ed_lock,flags);
		/* Check the root hub status change */
		len = rh_send_irq (ohci, urb->transfer_buffer, urb->transfer_buffer_length);
		spin_unlock_irqrestore (&usb_ed_lock,flags);
		if (len > 0) {

			/* If there is a status change, call the call back function */
			urb->actual_length = len;
#ifdef DEBUG
			urb_print (urb, "RET-t(rh)", usb_pipeout (urb->pipe));
#endif
			if (urb->complete)
				urb->complete (urb);
		}
	}

	/* restart the periodic timer for accessing the 
	status next time */
 out:
	rh_init_int_timer (urb);
} /* End of rh_int_timer_do */

/*-------------------------------------------------------------------------*/

/* Root Hub INTs are polled by this timer */

static int rh_init_int_timer (struct urb * urb) 
{
	ohci_t * ohci = urb->dev->bus->hcpriv;

	/* fill and start the root hub status change interrupt timer */
	ohci->rh.interval = urb->interval;
	init_timer (&ohci->rh.rh_int_timer);
	ohci->rh.rh_int_timer.function = rh_int_timer_do;
	ohci->rh.rh_int_timer.data = (unsigned long) urb;
	ohci->rh.rh_int_timer.expires = 
			jiffies + (HZ * (urb->interval < 30? 30: urb->interval)) / 1000;
	add_timer (&ohci->rh.rh_int_timer);
	
	return 0;
} /* End of rh_init_int_timer */

/*-------------------------------------------------------------------------*/

#define OK(x) 				len = (x); break
#define WR_RH_STAT(x) 		fnvIsp1161HcorWrite(ohci,uHcRhStatus,x)
#define WR_RH_PORTSTAT(x) 	fnvIsp1161HcorWrite(ohci,(uHcRhPort1Status+wIndex-1),x)
#define RD_RH_STAT			read_data
#define RD_RH_PORTSTAT		read_data

/* request to virtual root hub */

static int rh_submit_urb (struct urb * urb)
{
	struct usb_device * usb_dev = urb->dev;
	ohci_t * ohci = usb_dev->bus->hcpriv;
	unsigned int pipe = urb->pipe;
	struct usb_ctrlrequest * cmd = (struct usb_ctrlrequest *) urb->setup_packet;
	void * data = urb->transfer_buffer;
	int leni = urb->transfer_buffer_length;
	int len = 0;
	int status = TD_CC_NOERROR;
	
	__u32 datab[4];
	__u8  * data_buf = (__u8 *) datab;
	
 	__u16 bmRType_bReq;
	__u16 wValue; 
	__u16 wIndex;
	__u16 wLength;

	__u32	read_data;

	if (usb_pipeint(pipe)) {
		ohci->rh.urb =  urb;
		ohci->rh.send = 1;
		ohci->rh.interval = urb->interval;
		rh_init_int_timer(urb);
		urb->status = cc_to_error [TD_CC_NOERROR];
		
		return 0;
	}

	bmRType_bReq  = cmd->bRequestType | (cmd->bRequest << 8);
	wValue        = le16_to_cpu (cmd->wValue);
	wIndex        = le16_to_cpu (cmd->wIndex);
	wLength       = le16_to_cpu (cmd->wLength);

	dbg ("rh_submit_urb, req = %d(%x) len=%d", bmRType_bReq,
		bmRType_bReq, wLength);

	switch (bmRType_bReq) {
	/* Request Destination:
	   without flags: Device, 
	   RH_INTERFACE: interface, 
	   RH_ENDPOINT: endpoint,
	   RH_CLASS means HUB here, 
	   RH_OTHER | RH_CLASS  almost ever means HUB_PORT here 
	*/
  
		case RH_GET_STATUS: 				 		/* Get root hub status */
				*(__u16 *) data_buf = cpu_to_le16 (1); OK (2);
		case RH_GET_STATUS | RH_INTERFACE: 	 		
				*(__u16 *) data_buf = cpu_to_le16 (0); OK (2);
		case RH_GET_STATUS | RH_ENDPOINT:	 		
				*(__u16 *) data_buf = cpu_to_le16 (0); OK (2);   
		case RH_GET_STATUS | RH_CLASS: 				
				fnvIsp1161HcorRead(ohci,uHcRhStatus,&read_data);
				*(__u32 *) data_buf = cpu_to_le32 (
					RD_RH_STAT & ~(RH_HS_CRWE | RH_HS_DRWE));
				OK (4);
		case RH_GET_STATUS | RH_OTHER | RH_CLASS: 	
				fnvIsp1161HcorRead(ohci,(uHcRhPort1Status+wIndex-1),&read_data);
				*(__u32 *) data_buf = cpu_to_le32 (RD_RH_PORTSTAT); OK (4);

		case RH_CLEAR_FEATURE | RH_ENDPOINT:  
			switch (wValue) {
				case (RH_ENDPOINT_STALL): OK (0);
			}
			break;

		case RH_CLEAR_FEATURE | RH_CLASS:
			switch (wValue) {
				case RH_C_HUB_LOCAL_POWER:
					OK(0);
				case (RH_C_HUB_OVER_CURRENT): 
						WR_RH_STAT(RH_HS_OCIC); OK (0);
			}
			break;
		
		case RH_CLEAR_FEATURE | RH_OTHER | RH_CLASS:
			switch (wValue) {
				case (RH_PORT_ENABLE): 			
						WR_RH_PORTSTAT (RH_PS_CCS ); OK (0);
				case (RH_PORT_SUSPEND):			
						WR_RH_PORTSTAT (RH_PS_POCI); OK (0);
				case (RH_PORT_POWER):			
						WR_RH_PORTSTAT (RH_PS_LSDA); OK (0);
				case (RH_C_PORT_CONNECTION):	
						WR_RH_PORTSTAT (RH_PS_CSC ); OK (0);
				case (RH_C_PORT_ENABLE):		
						WR_RH_PORTSTAT (RH_PS_PESC); OK (0);
				case (RH_C_PORT_SUSPEND):		
						WR_RH_PORTSTAT (RH_PS_PSSC); OK (0);
				case (RH_C_PORT_OVER_CURRENT):	
						WR_RH_PORTSTAT (RH_PS_OCIC); OK (0);
				case (RH_C_PORT_RESET):			
						WR_RH_PORTSTAT (RH_PS_PRSC); OK (0); 
			}
			break;
 
		case RH_SET_FEATURE | RH_OTHER | RH_CLASS:
			switch (wValue) {
				case (RH_PORT_SUSPEND):			
						WR_RH_PORTSTAT (RH_PS_PSS ); OK (0); 
				case (RH_PORT_RESET): /* BUG IN HUP CODE *********/
						fnvIsp1161HcorRead(ohci,(uHcRhPort1Status+wIndex-1),&read_data);
						if (RD_RH_PORTSTAT & RH_PS_CCS)
						    WR_RH_PORTSTAT (RH_PS_PRS);
						OK (0);
				case (RH_PORT_POWER):			
						WR_RH_PORTSTAT (RH_PS_PPS ); OK (0); 
				case (RH_PORT_ENABLE): /* BUG IN HUP CODE *********/
						fnvIsp1161HcorRead(ohci,(uHcRhPort1Status+wIndex-1),&read_data);
						if (RD_RH_PORTSTAT & RH_PS_CCS)
						    WR_RH_PORTSTAT (RH_PS_PES );
						OK (0);
			}
			break;

		case RH_SET_ADDRESS: ohci->rh.devnum = wValue; OK(0);

		case RH_GET_DESCRIPTOR:						/* Get root hub descriptors */
			switch ((wValue & 0xff00) >> 8) {
				case (0x01): 						/* device descriptor */
					len = min (leni, min ((__u16) sizeof (root_hub_dev_des), wLength));
					data_buf = root_hub_dev_des; OK(len);
				case (0x02): 						/* configuration descriptor */
					len = min (leni, min ((__u16) sizeof (root_hub_config_des), wLength));
					data_buf = root_hub_config_des; OK(len);
				case (0x03): 						/* string descriptors */
					len = usb_root_hub_string (wValue & 0xff,
						(int)(long) ohci, "OHCI",
						data, wLength);
					if (len > 0) {
						data_buf = data;
						OK (min (leni, len));
					}
					// else fallthrough
				default: 
					status = TD_CC_STALL;
			}
			break;
		
		case RH_GET_DESCRIPTOR | RH_CLASS:
		    {
			    __u32 temp;
				
				fnvIsp1161HcorRead(ohci,uHcRhDescriptorA, &temp);

			    data_buf [0] = 9;		// min length;
			    data_buf [1] = 0x29;
			    data_buf [2] = temp & RH_A_NDP;
			    data_buf [3] = 0;
			    if (temp & RH_A_PSM) 	/* per-port power switching? */
				data_buf [3] |= 0x1;
			    if (temp & RH_A_NOCP)	/* no overcurrent reporting? */
				data_buf [3] |= 0x10;
			    else if (temp & RH_A_OCPM)	/* per-port overcurrent reporting? */
				data_buf [3] |= 0x8;

			    datab [1] = 0;
			    data_buf [5] = (temp & RH_A_POTPGT) >> 24;
				fnvIsp1161HcorRead(ohci,uHcRhDescriptorB, &temp);
			    data_buf [7] = temp & RH_B_DR;
			    if (data_buf [2] < 7) {
				data_buf [8] = 0xff;
			    } else {
				data_buf [0] += 2;
				data_buf [8] = (temp & RH_B_DR) >> 8;
				data_buf [10] = data_buf [9] = 0xff;
			    }
				
			    len = min (leni, min ((__u16) data_buf [0], wLength));
			    OK (len);
			}
 
		case RH_GET_CONFIGURATION: 	*(__u8 *) data_buf = 0x01; OK (1);

		case RH_SET_CONFIGURATION: 	WR_RH_STAT (0x10000); OK (0);

		default: 
			dbg ("unsupported root hub command");
			status = TD_CC_STALL;
	}
	
#ifdef	DEBUG
	// ohci_dump_roothub (ohci, 0);
#endif

	len = min(len, leni);
	if (data != data_buf)
	    memcpy (data, data_buf, len);
  	urb->actual_length = len;
	urb->status = cc_to_error [status];
	
#ifdef DEBUG
	urb_print (urb, "RET(rh)", usb_pipeout (urb->pipe));
#endif

	urb->hcpriv = NULL;
	usb_dec_dev_use (usb_dev);
	urb->dev = NULL;
	if (urb->complete)
	    	urb->complete (urb);
	return 0;
} /* End of rh_submit_urb */

/*-------------------------------------------------------------------------*/

static int rh_unlink_urb (struct urb * urb)
{
	ohci_t * ohci = urb->dev->bus->hcpriv;
 
	/* If urb is root hub urb, delete periodic timer,
	decrement device use, clear rest of the fields 
	and call the callback function if present */
	if (ohci->rh.urb == urb) {
		ohci->rh.send = 0;
		del_timer (&ohci->rh.rh_int_timer);
		ohci->rh.urb = NULL;

		urb->hcpriv = NULL;
		usb_dec_dev_use(urb->dev);
		urb->dev = NULL;
		if (urb->transfer_flags & USB_ASYNC_UNLINK) {
			urb->status = -ECONNRESET;
			if (urb->complete)
				urb->complete (urb);
		} else
			urb->status = -ENOENT;
	}
	return 0;
} /* End of rh_unlink_urb */
 

/*------------------------------------------------------------------------*
 * URB support functions
 *------------------------------------------------------------------------*/

/* free HCD-private data asoociated with this URB */

static void urb_free_priv( struct ohci	*hc, urb_priv_t	*urb_priv)
{
	int i;
	
	/* Free all the transfer descriptors of the urb */
	for(i=0; i< urb_priv->length; i++){
		MARKER();
		if(urb_priv->td[i]){
			MARKER();
			td_free(hc,urb_priv->td[i]);
		}
	}

	MARKER();
	kfree(urb_priv);
} /* End of urb_free_priv */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
static void urb_rm_priv_locked (struct urb	*urb)
{
	urb_priv_t	*urb_priv = urb->hcpriv;
MARKER();
	if(urb_priv) {
		MARKER();
		urb->hcpriv = NULL;
		/* Release int/iso bandwidth */
		if(urb->bandwidth) {
			switch(usb_pipetype(urb->pipe)) {
				case PIPE_INTERRUPT:
				MARKER();
					usb_release_bandwidth(urb->dev, urb, 0 );
					break;
				case PIPE_ISOCHRONOUS:
				MARKER();
					usb_release_bandwidth(urb->dev, urb, 1);
					break;
				default:
				MARKER();
					break;
			}
		}

		MARKER();
		/* free urb, hc data strucure elements */
		urb_free_priv( (ohci_t*)(urb->dev->bus), urb_priv);
		MARKER();
		usb_dec_dev_use(urb->dev);
		urb->dev = NULL;
	}
	MARKER();
} /* End of urb_rm_priv_locked */

/*-------------------------------------------------------------------------*/
static void urb_rm_priv (struct urb	*urb)
{
	unsigned long	flags;
		
	spin_lock_irqsave( &usb_ed_lock, flags);
	urb_rm_priv_locked(urb);
	spin_unlock_irqrestore(&usb_ed_lock, flags);
} /* End of urb_rm_priv */

/*-------------------------------------------------------------------------*/
/* SOHCI functions */
/*-------------------------------------------------------------------------*/
static int s1161_alloc_dev (struct usb_device *usb_dev)
{
	struct ohci_device * dev;

	/* Allocate and initialize and set the 
	device HC device data structure elements */
	dev = dev_alloc (NULL);
	
	if(!dev)	return -ENOMEM;
	
	memset(dev, 0, sizeof *dev);

	usb_dev->hcpriv = dev;

#ifdef __TRACE_LOW_LEVEL__
	printk("s1161_alloc_dev(dev = 0x%p)\n",dev);
#endif /* __TRACE_LOW_LEVEL__ */
	
	return 0;
} /* End of s1161_alloc_dev */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
static int s1161_free_dev (struct usb_device *usb_dev)
{
	struct ohci_device 	*dev = usb_to_ohci (usb_dev);
	unsigned long 	flags;
	ed_t			*ed;
	int				i, cnt = 0;	
	ohci_t			*ohci = usb_dev->bus->hcpriv;

#ifdef __TRACE_LOW_LEVEL__
	printk("s1161_free_dev(usb_dev = 0x%p)\n",usb_dev);
#endif /* __TRACE_LOW_LEVEL__ */

	if(!dev)	return 0;
	if(usb_dev->devnum >=0) {
		/* driver disconnects should have unlinked all urbs
		 * (freeing all the TDs, unlinking EDs) but we need
		 * to defend against bugs that prevent that.
		 */
		spin_lock_irqsave( &usb_ed_lock, flags);
		for(i=0; i< NUM_EDS; i++) {					/* Go through all the ed's */
			ed = &(dev->ed[i]);

			if(ed->state != ED_NEW) {
				if(ed->state == ED_OPER) {
					/* driver on that interface didn't unlink an ed */
					printk("un freed urb: 0x%p\n", ed);
					ep_unlink( ohci, ed);			/* Unlink the ed if it is operational */
				}
				ep_rm_ed (usb_dev, ed);				/* delete the ed */
				ed->state = ED_DEL;
				cnt++;
			}	
		}
		spin_unlock_irqrestore(&usb_ed_lock,flags);

		/* if the controller is running, tds for those unlinked
         * urbs get freed by dl_del_list at the next SF interrupt
         */

		if(cnt > 0) {
			
			if(ohci->disabled) {
				/* FIXME: Something like this should kick in,
                 * though it's currently an exotic case ...
                 * the controller won't ever be touching
                 * these lists again!!
                dl_del_list (ohci,
                    le16_to_cpu (ohci->hcca.frame_no) & 1);
                 */
				printk("TD Leak, %d\n", cnt);
			} else if( !in_interrupt()) {
				DECLARE_WAIT_QUEUE_HEAD(freedev_wakeup);
				DECLARE_WAITQUEUE(wait, current);
				int timeout = OHCI_UNLINK_TIMEOUT;
		
				/* SF interrupt handler calls dl_del_list */
				add_wait_queue(&freedev_wakeup, &wait);
				dev->wait = &freedev_wakeup;
				set_current_state(TASK_UNINTERRUPTIBLE);
				while(timeout && dev->ed_cnt)
					timeout = schedule_timeout(timeout);
				current->state = TASK_RUNNING;
				remove_wait_queue(&freedev_wakeup, &wait);
				if(dev->ed_cnt) {
					printk("free device %d timeout\n", usb_dev->devnum);
					return -ETIMEDOUT;
				}
			} else {
				/* likely some interface's driver has a refcount bug */
				printk("dev num %d deletion in interrupt\n", usb_dev->devnum);
				BUG();
			}
		}
	}

	/* free device, and associated EDs */

	dev_free(dev);
	
	return 0;
} /* End of s1161_free_dev */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
static int s1161_get_current_frame_number (struct usb_device *usb_dev)
{
	__u32 		frame_number;
	ohci_t		*ohci = (ohci_t*)(usb_dev->bus->hcpriv);

	/* Get the current frame number from the 1161 register */
	fnvIsp1161HcorRead(ohci,uHcFmNumber,&frame_number);
	
	return frame_number;
} /* End of s1161_get_current_frame_number */


/*-------------------------------------------------------------------------*
 * Interface functions (URB)
 *-------------------------------------------------------------------------*/

/* return a request to the completion handler */
 
static int s1161_return_urb (struct urb * urb)
{
	urb_priv_t * urb_priv = urb->hcpriv;
	struct urb * urbt;
	unsigned long flags;
	int i;	//, count
MARKER();
	if (!urb_priv)
		return -1; /* urb already unlinked */
MARKER();
	/* just to be sure */
	if (!urb->complete) {
				printk("!urb->complete\n");
		urb_rm_priv (urb);
		return -1;
	}
MARKER();
#ifdef DEBUG
	urb_print (urb, "RET", usb_pipeout (urb->pipe));
#endif

	switch (usb_pipetype (urb->pipe)) {
  		case PIPE_INTERRUPT:
		MARKER();
			urb->complete (urb); /* call complete and requeue URB */	
  			urb->actual_length = 0;
  			urb->status = USB_ST_URB_PENDING;
  			if (urb_priv->state != URB_DEL)
  				td_submit_urb (urb);
  			break;
  			
		case PIPE_ISOCHRONOUS:
		MARKER();
//		printk("urb->next = 0x%p, urb = 0x%p\n",urb->next,urb);
#if 1
			for (urbt = urb->next; urbt && (urbt != urb); urbt = urbt->next);
//			printk("urb->actual_length = %d urb->status= %d\n", urb->actual_length, urb->status);
//			printk("urbt= 0x%p\n",urbt);
//				printk("URB Complete\n");
//				for(count=0; count< urb->actual_length;count++) { if(count%32==0) printk("\n"); printk("%x  ", ((__u8*)urb->transfer_buffer)[count]);}
//				printk("\n");
			if (urbt) { /* send the reply and requeue URB */	
				urb->complete (urb);
				
				spin_lock_irqsave (&usb_ed_lock, flags);
				urb->actual_length = 0;
  				urb->status = USB_ST_URB_PENDING;
  				urb->start_frame = urb_priv->ed->last_iso + 1;
  				if (urb_priv->state != URB_DEL) {
  					for (i = 0; i < urb->number_of_packets; i++) {
  						urb->iso_frame_desc[i].actual_length = 0;
  						urb->iso_frame_desc[i].status = -EXDEV;
  					}
  					td_submit_urb (urb);
  				}
  				spin_unlock_irqrestore (&usb_ed_lock, flags);
  				
  			} else { /* unlink URB, call complete */
				urb_rm_priv (urb);
//				printk("urb->complete\n");
				urb->complete (urb); 	
			}		
#endif
			break;
  				
		case PIPE_BULK:
		case PIPE_CONTROL: /* unlink URB, call complete */
		MARKER();
//			printk("Bulk/Control return %d\n", urb->actual_length);
			urb_rm_priv (urb);
		MARKER();
			urb->complete (urb);
		MARKER();
			break;
	}
	return 0;
} /* End of s1161_return_urb */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
static int s1161_submit_urb (struct urb	*urb)
{
	ohci_t			*ohci;
	unsigned int	pipe = urb->pipe;
	ed_t			*ed;
	int				i, size = 0;
	urb_priv_t		*urb_priv;
	__u32			data_read;
	int				bustime = 0;
	unsigned long	flags;

#ifdef __TRACE_LOW_LEVEL__
	printk("s1161_submit_urb(urb = 0x%p)\n",urb);
#endif /* __TRACE_LOW_LEVEL__ */

	if(!urb->dev || !urb->dev->bus) return -ENODEV;	

	if(urb->hcpriv)		return -EINVAL;

	usb_inc_dev_use(urb->dev);
	
	ohci = (ohci_t *)(urb->dev->bus->hcpriv);

	if(usb_pipedevice(pipe) == ohci->rh.devnum)
		return rh_submit_urb(urb);

	/* when controller's hung, permit only roothub cleanup attempts
     * such as powering down ports */
	if(ohci->disabled) {
		usb_dec_dev_use (urb->dev);
		return -ESHUTDOWN;
	}


	/* every endpoint has a ed, locate and fill it */
	if(!(ed = ep_add_ed (urb->dev, pipe, urb->interval, 1))) {
		usb_dec_dev_use(urb->dev);
		return -ENOMEM;
	}

	/* for the private part of the URB we need the number of TDs (size) */
	switch(usb_pipetype(pipe)) {
		case PIPE_BULK:
			/* one TD for every MAX_BULK_TD_BUFF_SIZE Byte */
			size = (urb->transfer_buffer_length -1)/MAX_BULK_TD_BUFF_SIZE +1;
			break;
		case PIPE_ISOCHRONOUS: /* number of packets from URB */
			size = urb->number_of_packets;
			if(size <= 0) {
				usb_dec_dev_use(urb->dev);
				return -EINVAL;
			}
			for(i=0; i< urb->number_of_packets; i++) {
				urb->iso_frame_desc[i].actual_length = 0;
				urb->iso_frame_desc[i].status = -EXDEV;
			}
			break;
		case PIPE_CONTROL: /* 1 TD for setup, 1 for ACK and 1 for every MAX_CNTL_TD_BUFF_SIZE B */
			size = (urb->transfer_buffer_length == 0) ? 2 :
						((urb->transfer_buffer_length -1)/MAX_CNTL_TD_BUFF_SIZE + 3) ;
			break;
		case PIPE_INTERRUPT: /* one TD */
			size = 1;
			break;
	}

	/* allocate the private part of the URB */
	urb_priv = kmalloc((sizeof(urb_priv_t) + size * sizeof(td_t*)),(in_interrupt() ? GFP_ATOMIC : GFP_KERNEL));
	if(!urb_priv) {
		usb_dec_dev_use(urb->dev);
		return -ENOMEM;
	}
	memset(urb_priv, 0, (sizeof(urb_priv_t) + size * sizeof(td_t*)));

	/* fill the private part of the URB */
	urb_priv->length = size;
	urb_priv->ed = ed;

	/* allocate the TDs */
	for(i=0; i < size; i++) {
		urb_priv->td[i] = td_alloc(ohci);
		if(!urb_priv->td[i]) {
			urb_free_priv(ohci, urb_priv);
			usb_dec_dev_use( urb->dev);
			return -ENOMEM;
		}	
	}

	if(ed->state == ED_NEW || (ed->state & ED_DEL)) {
		urb_free_priv(ohci, urb_priv);
		usb_dec_dev_use(urb->dev);
		return -EINVAL;
	}

	/* allocate and claim bandwidth if needed; ISO
	 * needs start frame index if it was't provided.
     */
	switch(usb_pipetype(pipe)) {
		case PIPE_ISOCHRONOUS:
			if(urb->transfer_flags & USB_ISO_ASAP) {
				fnvIsp1161HcorRead(ohci, uHcFmNumber, &data_read);
				urb->start_frame = ((ed->state == ED_OPER) ?
									 (ed->last_iso + 1) :
									 (data_read + 3)) & 0xFFFF;
			}
			/* FALL THROUGH */
		case PIPE_INTERRUPT:
			if(urb->bandwidth == 0) {
				bustime = usb_check_bandwidth(urb->dev,urb);
			}
			if(bustime < 0 ) {
				urb_free_priv(ohci,urb_priv);
				usb_dec_dev_use(urb->dev);
				return bustime;
			}
			usb_claim_bandwidth(urb->dev, urb, bustime,usb_pipeisoc(urb->pipe));
		break;
	}

	spin_lock_irqsave( &usb_ed_lock, flags);
	urb->actual_length = 0;
	urb->hcpriv = urb_priv;
	urb->status = USB_ST_URB_PENDING;

	/* link the ed into a chain if is not already */

	if( ed->state != ED_OPER)
		ep_link (ohci, ed);
	
	/* fill the TDs and link it to the ed */
	td_submit_urb(urb);
	spin_unlock_irqrestore(&usb_ed_lock, flags);

	return 0;
} /* End of s1161_submit_urb */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
static int s1161_unlink_urb (struct urb	*urb)
{
	ohci_t	*ohci;
	unsigned long flags;


#ifdef __TRACE_LOW_LEVEL__
	printk("s1161_unlink_urb(urb = 0x%p)\n",urb);
#endif /* __TRACE_LOW_LEVEL__ */

	if(!urb)	return -EINVAL;

	if(!urb->dev || !urb->dev->bus)	return -ENODEV;

	ohci = (ohci_t *)(urb->dev->bus->hcpriv);

	if(usb_pipedevice(urb->pipe) == ohci->rh.devnum)
		return rh_unlink_urb (urb);


	if(urb->hcpriv && (urb->status == USB_ST_URB_PENDING)) {
		if(!ohci->disabled) {
			urb_priv_t	*urb_priv;
			
			/* interrupt code may not sleep; it must use
             * async status return to unlink pending urbs.
             */
			if(!(urb->transfer_flags & USB_ASYNC_UNLINK) 
				&& in_interrupt()) {
				printk("bug in call to s1161_unlink_urb(); use async\n");
				
				return -EWOULDBLOCK;
			}
			/* flag the urb and its TDs for deletion in some
             * upcoming SF interrupt delete list processing
             */
			spin_lock_irqsave(&usb_ed_lock, flags);
			urb_priv = urb->hcpriv;
			
			if(!urb_priv || (urb_priv->state == URB_DEL)) {
				spin_unlock_irqrestore(&usb_ed_lock, flags);
				return 0;
			}
			urb_priv->state = URB_DEL;
			ep_rm_ed(urb->dev, urb_priv->ed);
			urb_priv->ed->state |= ED_URB_DEL;
			
			if(!(urb->transfer_flags & USB_ASYNC_UNLINK)) {
				DECLARE_WAIT_QUEUE_HEAD (unlink_wakeup);
				DECLARE_WAITQUEUE(wait, current);
				int timeout = OHCI_UNLINK_TIMEOUT;
	
				add_wait_queue(&unlink_wakeup, &wait);
				urb_priv->wait = &unlink_wakeup;
				spin_unlock_irqrestore(&usb_ed_lock,flags);
			
				/* wait until all TDs are deleted */
				set_current_state(TASK_UNINTERRUPTIBLE);
				while( timeout && (urb->status == USB_ST_URB_PENDING))
					timeout = schedule_timeout(timeout);
				current->state = TASK_RUNNING;
				remove_wait_queue(&unlink_wakeup, &wait);
				if(urb->status == USB_ST_URB_PENDING) {
					printk("unlink URB timeout\n");
					return -ETIMEDOUT;
				}
			} else {
				/* usb_dec_dev_use done in dl_del_list() */
				urb->status = -EINPROGRESS;
				spin_unlock_irqrestore(&usb_ed_lock, flags);
			}
		} else {	
			urb_rm_priv(urb);
			if(urb->transfer_flags & USB_ASYNC_UNLINK) {
				urb->status = -ECONNRESET;
				if(urb->complete)	
					urb->complete(urb);
			} else 
				urb->status = -ENOENT;		
		}
	}

	return 0;
} /* End of s1161_unlink_urb */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
static void hc_release_1161 (ohci_t * ohci)
{
#ifdef __TRACE_LOW_LEVEL__
	printk("hc_release_1161(ohci = 0x%p)\n", ohci);
#endif /* __TRACE_LOW_LEVEL__ */

	if( ohci->bus->root_hub)	usb_disconnect(&ohci->bus->root_hub);
	
	if(ohci->disabled)
		fnvIsp1161HostReset(ohci);

	ohci->ohci_dev->driver_data = 0;

	usb_deregister_bus(ohci->bus);		/* Deregister 1161 hc from bus stack */
	usb_free_bus (ohci->bus);			/* Free the data structure */

	list_del (&ohci->ohci_hcd_list);	/* Delete the 1161 hc from 1161 hc list */
	INIT_LIST_HEAD (&ohci->ohci_hcd_list);

	if(ohci->p_itl_buffer) kfree(ohci->p_itl_buffer);	/* delete ITL buffer */

	if(ohci->p_atl_buffer) kfree(ohci->p_atl_buffer);	/* delete ATL buffer */

	kfree(ohci);						/* Free ohci data structure */
} /* End of hc_release_1161 */

struct usb_operations s1161_device_operations = {
		s1161_alloc_dev,s1161_free_dev,s1161_get_current_frame_number,s1161_submit_urb,s1161_unlink_urb
};


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
static ohci_t * __devinit hc_alloc_1161 (struct pci_dev *dev)
{
	ohci_t			*ohci;
	struct usb_bus	*bus;

	ohci = (ohci_t *) kmalloc( sizeof *ohci, GFP_KERNEL );

	if(!ohci)	return NULL;

	memset(ohci,0,sizeof(ohci_t));

	ohci->disabled = 1;				/* Set ohci to disabled */
	
	ohci->irq = IRQ4HC_CHNNL;		/* set the irq number */

	dev->driver_data = ohci;
	ohci->ohci_dev = dev;

	INIT_LIST_HEAD(&(ohci->ohci_hcd_list));
	list_add(&(ohci->ohci_hcd_list), &(ohci_1161_hcd_list));	
	
	bus = usb_alloc_bus(&s1161_device_operations);	/* Allocate usb bus (1161) in the bus stack */
	if(!bus) {
		kfree(ohci);
		return NULL;
	}
	
	ohci->bus = bus;
	bus->hcpriv = (void *)ohci;
	bus->bus_name = "L";

	return ohci;
} /* End of hc_alloc_1161 */

struct pci_dev 	ohci_pci_dev;

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
static int __devinit
hc_found_1161 ( void) 
{
	ohci_t	*ohci;

	strcpy(ohci_pci_dev.name,hc_1161_name);		/* make it pci compatible */

	ohci = hc_alloc_1161(&ohci_pci_dev);		/* allocate 1161 hc data */

	if(!ohci) {
		return -ENOMEM;
	}
		
	/* Allocate ATL Buffer */
	ohci->p_atl_buffer = kmalloc(ATL_BUFF_LENGTH,GFP_KERNEL);
	if(!(ohci->p_atl_buffer)) {
		hc_release_1161(ohci);
		return -ENOMEM;
	}
	memset((ohci->p_atl_buffer), 0, ATL_BUFF_LENGTH);

	/* Allocate ITL Buffer */
	ohci->p_itl_buffer = kmalloc(ITL_BUFF_LENGTH,GFP_KERNEL);
	if(!(ohci->p_itl_buffer)) {
		hc_release_1161(ohci);
		return -ENOMEM;
	}
	memset((ohci->p_itl_buffer), 0, ITL_BUFF_LENGTH);

	/* Reset 1161 host controller */
   	if(fnvIsp1161HostReset(ohci) != 0 ){
		hc_release_1161(ohci);
		return -EBUSY;
	}

	usb_register_bus(ohci->bus);		/* Register 1161 hc to the bus stack */

#ifndef CONFIG_SA1100_FRODO
	/* IO Region is checked already before calling this
		function */
	request_region(HC_IO_BASE, HC_IO_SIZE,ohci_pci_dev.name);
#endif

	/* set ohci state to reset */
	ohci->hc_control = OHCI_USB_RESET;

   	ohci->irq = IRQ4HC_CHNNL;
   	if(fnuHci1161HostInit(ohci) != 0) {		/* Initialize 1161 hc */
		hc_release_1161(ohci);
		return -ENOMEM;
	}

	/* Request the Interrupt line (shared) */
#ifndef CONFIG_NSCU
   	request_irq(IRQ4HC_CHNNL,fnvHci1161IrqHandler,SA_SHIRQ,ohci_pci_dev.name,ohci);
#else
   	request_8xxirq(IRQ4HC_CHNNL,fnvHci1161IrqHandler,SA_SHIRQ,ohci_pci_dev.name,ohci);
#endif
	return 0;
} /* End of hc_found_1161 */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
static int __init hcd_1161_init (void)
{
	int	uRet;

	if((uRet = ohci_1161_mem_init()) < 0 ) {
		return uRet;
	}
		
#ifdef CONFIG_NSCU
	hc_port = ioremap(HC_IO_BASE, HC_IO_SIZE);
#endif
	
	/* Detect the 1161 host controller */
	if( (uRet = fnuIsp1161HostDetect()) < 0 ) {
		printk("Couldn't find 1362-HC\n");
		ohci_1161_mem_cleanup ();		
		return uRet;
	}
			
	printk("1362-HC Detected\n");
   			
	/* HC detected, do the initialization */
	if((uRet = hc_found_1161()) < 0 ) {
		printk("1362-HC Initialization Failed\n");
		ohci_1161_mem_cleanup ();		
		return uRet;
	}
				
	printk("1362-HC Initialization Successful\n");
	return uRet;
} /* End of hcd_1161_init */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
static void __exit hcd_1161_cleanup (void)
{
	ohci_t  *ohci = ohci_pci_dev.driver_data;
	int irq = ohci->irq;

	hc_release_1161 (ohci);						/* release the 1161 hc */
	free_irq (irq,ohci);						/* Free interrupt line */

#ifndef CONFIG_SA1100_FRODO
	release_region (HC_IO_BASE,HC_IO_SIZE);		/* release IO space */
#endif
	
	ohci_1161_mem_cleanup ();					/* Clean up global memory */
} /* End of hcd_1161_cleanup */


module_init (hcd_1161_init);
module_exit (hcd_1161_cleanup);


MODULE_AUTHOR ("Roman Weissgaerber <weissg@vienna.at>, David Brownell, Original Code modified for Philips 1161 USB HostController by Srinivas.Yarra <Srinivas.yarra@philips.com>");
MODULE_DESCRIPTION ("USB 1362 Host Controller Driver");
MODULE_LICENSE("GPL");
