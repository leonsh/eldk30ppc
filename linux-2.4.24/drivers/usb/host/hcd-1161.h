/*
 *  Philips 1362 HCD (Host Controller Driver) for USB.
 *  Copyright (C) 2003 DENX Software Engineering, Wolfgang Denk, wd@denx.de
 *
 *  The driver is based on
 *
 *  Philips 1161 HCD (Host Controller Driver) for USB.
 * 
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000 David Brownell <david-b@pacbell.net>
 *     Modified for Philips 1161 HCD development by
 *     Srinivas Yarra <Srinivas.Yarra@philips.com> 
 * hcd-1161.h
 */

 
static int cc_to_error[16] = { 

/* mapping of the OHCI CC status to error codes */ 
	/* No  Error  */               USB_ST_NOERROR,
	/* CRC Error  */               USB_ST_CRC,
	/* Bit Stuff  */               USB_ST_BITSTUFF,
	/* Data Togg  */               USB_ST_CRC,
	/* Stall      */               USB_ST_STALL,
	/* DevNotResp */               USB_ST_NORESPONSE,
	/* PIDCheck   */               USB_ST_BITSTUFF,
	/* UnExpPID   */               USB_ST_BITSTUFF,
	/* DataOver   */               USB_ST_DATAOVERRUN,
	/* DataUnder  */               USB_ST_DATAUNDERRUN,
	/* reservd    */               USB_ST_NORESPONSE,
	/* reservd    */               USB_ST_NORESPONSE,
	/* BufferOver */               USB_ST_BUFFEROVERRUN,
	/* BuffUnder  */               USB_ST_BUFFERUNDERRUN,
	/* Not Access */               USB_ST_NORESPONSE,
	/* Not Access */               USB_ST_NORESPONSE 
};


/*--------------------------*/
/* ED States 				*/
/*--------------------------*/
#define 	ED_NEW 					0x00
#define 	ED_UNLINK 				0x01
#define 	ED_OPER					0x02
#define 	ED_DEL					0x04
#define 	ED_URB_DEL  			0x08

/*--------------------------*/
/* usb_ohci_ed 				*/
/*--------------------------*/
typedef struct ed {
	__u32 		hwINFO;       	/* hw information */
	__u32 		hwTailP;		/* td Tail Pointer */
	__u32 		hwHeadP;		/* td Head poinetr */
	__u32 		hwNextED;		/* Next ED pointer */

	struct ed 	* ed_prev;  	/* Previous ed pointer */
	__u8 		int_period;		/* Interrupt period (Int,ISO) */
	__u8 		int_branch;		/* Interrupt table index (Int,ISO) */
	__u8 		int_load; 		/* load for this ed */
	__u8 		int_interval;
	__u8 		state;			/* ED State */
	__u8 		type; 			/* Type of transfer */
	__u16 		last_iso;		/* Last ISochronous? (ISO) */
    struct ed 	* ed_rm_list;	/* Rm list pointer */
   
} ed_t;

 
/*------------------------------*/
/* TD info field 				*/
/*------------------------------*/
#define 	TD_CC							0xf0000000			/* Condifion code mask */
#define 	TD_CC_GET(td_p) 				((td_p >>28) & 0x0f)		/* Condition code get */
#define 	TD_CC_SET(td_p, cc) 			(td_p) = ((td_p) & 0x0fffffff) | (((cc) & 0x0f) << 28) /* Condition code set */
#define 	TD_EC       					0x0C000000			/* Error count mask */
#define 	TD_T        					0x03000000			/* Toggle mask */
#define 	TD_T_DATA0  					0x02000000			/* Data0 toggle */
#define 	TD_T_DATA1  					0x03000000			/* DATA1 toggle */
#define 	TD_T_TOGGLE 					0x00000000			/* Toggle is carried from ED */
#define 	TD_R        					0x00040000			/* Buffer rounding */
#define 	TD_DI       					0x00E00000			/* Delay Interrupt mask */
#define 	TD_DI_SET(X) 					(((X) & 0x07)<< 21)	/* set delay Interrupt */
#define 	TD_DP       					0x00180000			/* Direction of PID mask */
#define 	TD_DP_SETUP 					0x00000000			/* Direction Set up */
#define 	TD_DP_IN    					0x00100000			/* Direction IN */
#define 	TD_DP_OUT   					0x00080000			/* Direction OUT */

#define 	TD_ISO							0x00010000
#define 	TD_DEL      					0x00020000

/* CC Codes */
#define 	TD_CC_NOERROR      				0x00		/* NO error */
#define 	TD_CC_CRC          				0x01		/* CRC Error */
#define 	TD_CC_BITSTUFFING  				0x02		/* Bit stuffing Error */
#define 	TD_CC_DATATOGGLEM  				0x03		/* Data toggle mismatch error */
#define 	TD_CC_STALL        				0x04		/* received stall */
#define 	TD_DEVNOTRESP      				0x05		/* Device not responding */
#define 	TD_PIDCHECKFAIL    				0x06		/* PID Check failure */
#define 	TD_UNEXPECTEDPID   				0x07		/* Unexpected error */
#define 	TD_DATAOVERRUN     				0x08		/* Data over run */
#define 	TD_DATAUNDERRUN    				0x09		/* Data under run */
#define 	TD_BUFFEROVERRUN   				0x0C		/* Buffer over run */
#define 	TD_BUFFERUNDERRUN  				0x0D		/* Buffer under run */
#define 	TD_NOTACCESSED     				0x0E		/* Not accessed */


#define 	MAXPSW 							1

typedef struct td { 
	__u32 		hwINFO;
  	__u32 		hwCBP;						/* Current Buffer Pointer */
  	__u32 		hwNextTD;					/* Next TD Pointer */
  	__u32 		hwBE;						/* Memory Buffer End Pointer */
  	__u16 		hwPSW[MAXPSW];

  	__u8 		retry_count;				/* Retry count */
  	__u8 		index;						/* Index in urb */
  	struct ed 	* ed;						/* pointer to ed attached to */
  	struct td 	* next_dl_td;				/* next td */
  	struct urb 		* urb;						/* Urb request it belongs to */
} td_t;


#define 	OHCI_ED_SKIP			(1 << 14)

/*
 * The HCCA (Host Controller Communications Area) is a 256 byte
 * structure defined in the OHCI spec. that the host controller is
 * told the base address of.  It must be 256-byte aligned.
 */
 
#define 	NUM_INTS 				32	/* part of the OHCI standard */

/*
 * Maximum number of root hub ports.  
 */
#define 	MAX_ROOT_PORTS				2	/* maximum OHCI root hub ports */

/* OHCI CONTROL AND STATUS REGISTER MASKS */

/*
 * HcControl (control) register masks
 */
#define 	OHCI_CTRL_CBSR				(3 << 0)			/* control/bulk service ratio */
#define 	OHCI_CTRL_PLE				(1 << 2)			/* periodic list enable */
#define 	OHCI_CTRL_IE				(1 << 3)			/* isochronous enable */
#define 	OHCI_CTRL_CLE				(1 << 4)			/* control list enable */
#define 	OHCI_CTRL_BLE				(1 << 5)			/* bulk list enable */
#define 	OHCI_CTRL_HCFS				(3 << 6)			/* host controller functional state */
#define 	OHCI_CTRL_IR				(1 << 8)			/* interrupt routing */
#define 	OHCI_CTRL_RWC				(1 << 9)			/* remote wakeup connected */
#define 	OHCI_CTRL_RWE				(1 << 10)			/* remote wakeup enable */
/* Added for ISP1161 Used i.o Interrupt routing as IR is not supported
by ISP1161 */
#define		OHCI_CTRL_TIP				(1 << 8)			/* Transfer in progress */

/* pre-shifted values for HCFS */
#	define OHCI_USB_RESET				(0 << 6)
#	define OHCI_USB_RESUME				(1 << 6)
#	define OHCI_USB_OPER				(2 << 6)
#	define OHCI_USB_SUSPEND				(3 << 6)

/*
 * HcCommandStatus (cmdstatus) register masks
 */
#define 	OHCI_HCR					(1 << 0)			/* host controller reset */
#define 	OHCI_CLF  					(1 << 1)			/* control list filled */
#define 	OHCI_BLF  					(1 << 2)			/* bulk list filled */
#define 	OHCI_OCR  					(1 << 3)			/* ownership change request */
#define 	OHCI_SOC  					(3 << 16)			/* scheduling overrun count */

/*
 * masks used with interrupt registers:
 * HcInterruptStatus (intrstatus)
 * HcInterruptEnable (intrenable)
 * HcInterruptDisable (intrdisable)
 */
#define 	OHCI_INTR_SO				(1 << 0)			/* scheduling overrun */
#define 	OHCI_INTR_WDH				(1 << 1)			/* writeback of done_head */
#define 	OHCI_INTR_SF				(1 << 2)			/* start frame */
#define 	OHCI_INTR_RD				(1 << 3)			/* resume detect */
#define 	OHCI_INTR_UE				(1 << 4)			/* unrecoverable error */
#define 	OHCI_INTR_FNO				(1 << 5)			/* frame number overflow */
#define 	OHCI_INTR_RHSC				(1 << 6)			/* root hub status change */
/* Added for 1161 */
#define		OHCI_INTR_ATD				(1 << 7)			/* Atl list done */
#define 	OHCI_INTR_OC				(1 << 30)			/* ownership change */
#define 	OHCI_INTR_MIE				(1 << 31)			/* master interrupt enable */



/* Virtual Root HUB */
struct virt_root_hub {
	int 				devnum; 			/* Address of Root Hub endpoint */ 
	void 				* urb;				/* Root hub urb pointer */
	void 				* int_addr;
	int 				send;
	int 				interval;
	struct timer_list 	rh_int_timer;		/* root hub interval timer */
};


/* USB HUB CONSTANTS (not OHCI-specific; see hub.h) */
 
/* destination of request */
#define 	RH_INTERFACE               	0x01			/* Root hub interface related request */
#define 	RH_ENDPOINT                	0x02			/* Root hub end point  related request */
#define 	RH_OTHER                   	0x03			/* Root hub other request */

#define 	RH_CLASS                   	0x20			/* Root hub class specific */
#define 	RH_VENDOR                  	0x40			/* Root hub vender specific */

/* Requests: bRequest << 8 | bmRequestType */
#define 	RH_GET_STATUS           	0x0080			/* Root hub get status */
#define 	RH_CLEAR_FEATURE        	0x0100			/* Root hub clear feature */
#define 	RH_SET_FEATURE          	0x0300			/* Root hub clear feature */
#define 	RH_SET_ADDRESS				0x0500			/* Root hub set address */
#define 	RH_GET_DESCRIPTOR			0x0680			/* Root hub get descriptor */
#define 	RH_SET_DESCRIPTOR       	0x0700			/* Root hub set descriptor */
#define 	RH_GET_CONFIGURATION		0x0880			/* Root hub get configuration */
#define 	RH_SET_CONFIGURATION		0x0900			/* Root hub set configuration */
#define 	RH_GET_STATE            	0x0280			/* Root hub get state */
#define 	RH_GET_INTERFACE        	0x0A80			/* root hub get inferface */
#define 	RH_SET_INTERFACE        	0x0B00			/* Root hub set Interface */
#define 	RH_SYNC_FRAME          	 	0x0C80
/* Our Vendor Specific Request */
#define 	RH_SET_EP               	0x2000


/* Hub port features */
#define 	RH_PORT_CONNECTION         0x00
#define 	RH_PORT_ENABLE             0x01
#define 	RH_PORT_SUSPEND            0x02
#define 	RH_PORT_OVER_CURRENT       0x03
#define 	RH_PORT_RESET              0x04
#define 	RH_PORT_POWER              0x08
#define 	RH_PORT_LOW_SPEED          0x09

#define 	RH_C_PORT_CONNECTION       0x10
#define 	RH_C_PORT_ENABLE           0x11
#define 	RH_C_PORT_SUSPEND          0x12
#define 	RH_C_PORT_OVER_CURRENT     0x13
#define 	RH_C_PORT_RESET            0x14  

/* Hub features */
#define 	RH_C_HUB_LOCAL_POWER       0x00				/* Hub local powered */
#define 	RH_C_HUB_OVER_CURRENT      0x01				/* Hub over current */

#define 	RH_DEVICE_REMOTE_WAKEUP    0x00				
#define 	RH_ENDPOINT_STALL          0x01

#define 	RH_ACK                     0x01
#define 	RH_REQ_ERR                 -1
#define 	RH_NACK                    0x00


/* OHCI ROOT HUB REGISTER MASKS */
 
/* roothub.portstatus [i] bits */
#define 	RH_PS_CCS            		0x00000001   			/* current connect status */
#define 	RH_PS_PES            		0x00000002   			/* port enable status*/
#define 	RH_PS_PSS            		0x00000004   			/* port suspend status */
#define 	RH_PS_POCI           		0x00000008   			/* port over current indicator */
#define 	RH_PS_PRS            		0x00000010  			/* port reset status */
#define 	RH_PS_PPS            		0x00000100   			/* port power status */
#define 	RH_PS_LSDA           		0x00000200    			/* low speed device attached */
#define 	RH_PS_CSC            		0x00010000 				/* connect status change */
#define 	RH_PS_PESC           		0x00020000   			/* port enable status change */
#define 	RH_PS_PSSC           		0x00040000    			/* port suspend status change */
#define 	RH_PS_OCIC           		0x00080000    			/* over current indicator change */
#define 	RH_PS_PRSC           		0x00100000   			/* port reset status change */
	
/* roothub.status bits */
#define 	RH_HS_LPS	     			0x00000001				/* local power status */
#define 	RH_HS_OCI	     			0x00000002				/* over current indicator */
#define 	RH_HS_DRWE	     			0x00008000				/* device remote wakeup enable */
#define 	RH_HS_LPSC	     			0x00010000				/* local power status change */
#define 	RH_HS_OCIC	     			0x00020000				/* over current indicator change */
#define 	RH_HS_CRWE	     			0x80000000				/* clear remote wakeup enable */

/* roothub.b masks */
#define 	RH_B_DR						0x0000ffff				/* device removable flags */
#define 	RH_B_PPCM					0xffff0000				/* port power control mask */
	
/* roothub.a masks */
#define		RH_A_NDP					(3)				/* number of downstream ports */
#define		RH_A_PSM					(1 << 8)				/* power switching mode */
#define		RH_A_NPS					(1 << 9)				/* no power switching */
#define		RH_A_DT						(1 << 10)				/* device type (mbz) */
#define		RH_A_OCPM					(1 << 11)				/* over current protection mode */
#define		RH_A_NOCP					(1 << 12)				/* no over current protection */
#define		RH_A_POTPGT					(0xff << 24)			/* power on to power good time */

/* urb */
typedef struct 
{
	ed_t 				* ed;
	__u16 				length;									// number of tds associated with this request
	__u16 				td_cnt;									// number of tds already serviced
	int   				state;
	wait_queue_head_t 	* wait;
	td_t 				* td[0];								// list pointer to all corresponding TDs associated with this request

} urb_priv_t;

#define 	URB_DEL 					1

/*
 * This is the full ohci controller description
 *
 * Note how the "proper" USB information is just
 * a subset of what the full implementation needs. (Linus)
 */

typedef struct ohci {

	int 					disabled;							/* e.g. got a UE, we're hung */
	atomic_t 				resume_count;						/* defending against multiple resumes */

	struct list_head 		ohci_hcd_list;						/* list of all ohci_hcd */

	struct ohci 			* next; 							// chain of uhci device contexts
	// struct 				list_head urb_list; 				// list of all pending urbs
	// spinlock_t 			urb_list_lock; 						// lock to keep consistency 
  
	int 					ohci_int_load[32];					/* load of the 32 Interrupt Chains (for load balancing)*/
	ed_t 					* ed_rm_list[2];     				/* lists of all endpoints to be removed */
	ed_t 					* ed_bulktail;       				/* last endpoint of bulk list */
	ed_t 					* ed_controltail;    				/* last endpoint of control list */
 	ed_t 					* ed_isohead;        				/* first endpoint of iso list */
 	ed_t 					* ed_isotail;        				/* last endpoint of iso list */
	int 					intrstatus;
	__u32 					hc_control;							/* copy of the hc control reg */
	struct usb_bus 			* bus;    
	struct usb_device 		* dev[128];							/* Usb devices attached to this HCD */
	struct virt_root_hub 	rh;									/* Virtual Root Hub information */

	/* Added for 1161 */
	void 					*p_atl_buffer;						/* Atl Buffer memory */
	void 					*p_itl_buffer;						/* Atl Buffer memory */
	unsigned int			irq;								/* Interrupt line for 1161 */
	
	__u32					uHcHcdControl_hcd;					/* Software Control Register of 1161 */
																/* Bit 5: Bulk List Enable (BLE) */
																/* Bit 4: Control List Enable (CLE) */
																/* Bit 3: Isochronous Enable (IE) */
																/* Bit 2: Periodic List Enable (PLE) */
																/* Bit 1..0: Control/Bulk Service Ratio */

	__u32					uHcHcdCommandStatus_hcd;			/* Software Command Status Register of 1161 */
																/* Bit 1: Control List Filled (CLF) */
																/* Bit 2: Bulk List Filled (BLF) */

	ed_t					* p_int_table[NUM_INTS];			/* Interrupt EndPoint Table (supposdly part of HCCA) */
	ed_t					* p_ed_bulkhead;					/* Bulk ED Head (supposdly part of OHCI register) */
	ed_t					* p_ed_controlhead;					/* Control ED Head (supposdly part of OHCI register) */
	ed_t					* p_ed_isohead;						/* Is this needed ??? */
	td_t					* pstDoneHead_hcd;					/* DoneHead */
	struct isotd_map_buffer		* pstIsoaPtdMapBuffer;				/* ISOA buffer td-ptd map buffer */
	struct isotd_map_buffer		* pstIsobPtdMapBuffer;				/* ISOB buffer td-ptd map buffer */
	


	/* PCI device handle and settings */
	struct pci_dev			*ohci_dev;
} ohci_t;


#define 	NUM_TDS						0				/* num of preallocated transfer descriptors */
#define 	NUM_EDS 					32				/* num of preallocated endpoint descriptors */

struct ohci_device {
	ed_t 				ed[NUM_EDS];
	int 				ed_cnt;
	wait_queue_head_t 	* wait;
};

// #define ohci_to_usb(ohci)			((ohci)->usb)
#define 	usb_to_ohci(usb)			((struct ohci_device *)(usb)->hcpriv)

/* hcd */
/* endpoint */
static int 	ep_link(ohci_t 	* ohci, ed_t 	* ed);
static int 	ep_unlink(ohci_t 	* ohci, ed_t 	* ed);
static ed_t * ep_add_ed(struct usb_device 	* usb_dev, unsigned int 	pipe, int 	interval, int 	load);
static void ep_rm_ed(struct usb_device 	* usb_dev, ed_t 	* ed);

/* td */
static void td_fill(unsigned int 	info, void 	* data, int 	len, struct urb 	* urb, int 	index);
static void td_submit_urb(struct urb 	* urb);

/* root hub */
static int rh_submit_urb(struct urb 	* urb);
static int rh_unlink_urb(struct urb 	* urb);
static int rh_init_int_timer(struct urb	* urb);

/*-------------------------------------------------------------------------*/

#define 	ALLOC_FLAGS 					(in_interrupt () ? GFP_ATOMIC : GFP_KERNEL)
 
#ifdef OHCI_MEM_SLAB
#define		__alloc(t,c) 					kmem_cache_alloc(c,ALLOC_FLAGS)
#define		__free(c,x) 					kmem_cache_free(c,x)
static 		kmem_cache_t 					*td_cache, *ed_cache;

/*
 * WARNING:  do NOT use this with "forced slab debug"; it won't respect
 * our hardware alignment requirement.
 */
#ifndef OHCI_MEM_FLAGS
#define		OHCI_MEM_FLAGS 		0
#endif

static int ohci_1161_mem_init (void)
{
	/* redzoning (or forced debug!) breaks alignment */
	int	flags = (OHCI_MEM_FLAGS) & ~SLAB_RED_ZONE;

	/* TDs accessed by controllers and host */
	td_cache = kmem_cache_create ("ohci_td", sizeof (struct td), 0,
		flags | SLAB_MUST_HWCACHE_ALIGN | SLAB_HWCACHE_ALIGN, NULL, NULL);
	if (!td_cache) {
		dbg ("no TD cache?");
		return -ENOMEM;
	}

	/* EDs are accessed by controllers and host;  dev part is host-only */
	ed_cache = kmem_cache_create ("ohci_ed", sizeof (struct ohci_device), 0,
		flags | SLAB_MUST_HWCACHE_ALIGN | SLAB_HWCACHE_ALIGN, NULL, NULL);
	if (!ed_cache) {
		dbg ("no ED cache?");
		kmem_cache_destroy (td_cache);
		td_cache = 0;
		return -ENOMEM;
	}
	dbg ("slab flags 0x%x, ed_cache 0x%p, td_cache 0x%p", flags, ed_cache, td_cache);
	return 0;
}

static void ohci_1161_mem_cleanup (void)
{
	if (ed_cache && kmem_cache_destroy (ed_cache))
		err ("ed_cache remained");
	ed_cache = 0;

	if (td_cache && kmem_cache_destroy (td_cache))
		err ("td_cache remained");
	td_cache = 0;
}

#else
#define		__alloc(t,c) 					kmalloc(sizeof(t),ALLOC_FLAGS)
#define		__free(dev,x) 					kfree(x)
#define 	td_cache 						0
#define 	ed_cache 						0

static inline int ohci_1161_mem_init (void) { return 0; }
static inline void ohci_1161_mem_cleanup (void) { return; }

/* FIXME: pci_consistent version */

#endif


static inline struct td *
td_alloc (struct ohci *hc)
{
	struct td *td = (struct td *) __alloc (struct td, td_cache);
#ifdef DEBUG
	printk (KERN_DEBUG "%s: %d: %s(): td == %p\n",__FILE__,__LINE__,__FUNCTION__,td);
#endif
	return td;
}

static inline void
td_free (struct ohci *hc, struct td *td)
{
#ifdef DEBUG
	printk (KERN_DEBUG "%s: %d: %s(): td == %p\n",__FILE__,__LINE__,__FUNCTION__,td);
#endif
	__free (td_cache, td);
}


/* DEV + EDs ... only the EDs need to be consistent */
static inline struct ohci_device *
dev_alloc (struct ohci *hc)
{
	struct ohci_device *dev = (struct ohci_device *)
		__alloc (struct ohci_device, ed_cache);
	return dev;
}

static inline void
dev_free (struct ohci_device *dev)
{
	__free (ed_cache, dev);
}











/* Added ISP1161 related Constants */

#define 	YES						1
#define 	NO						0
#define 	ON						1
#define 	OFF						0
#define		MAX_GTD					64

/*----------------------------------------------------------*/
/*  ATL buffer processing definitions 						*/
/*----------------------------------------------------------*/
typedef struct td_tree_addr{
        __u32           uPipeHandle;
		__u32			pstHcdTd;						/* pointer to td is stored i.o index */
        __u32           uAtlNodeLength;
} td_tree_addr_t ;

typedef struct isotd_map_buffer{
		__u8			byStatus;						/* Status: filled (1) empty (0) */
		__u8			byActiveTds;					/* # of td's when filled */
		__u16			uByteCount;						/* Actual # of bytes of buffer filled */
		__u32			uFrameNumber;
		td_t			*pstIsoInDoneHead;	
		td_tree_addr_t	aItlTdTreeBridge[MAX_GTD];		/* PTD <-> TD mapping */
} isotd_map_buffer_t;


#define 	ITL_BUFF_LENGTH					1024UL					/* 1 KB */

#define 	INTL_BUFF_LENGTH				512UL
#define 	INTL_BLOCK_SZ					64

#define 	ATL_BUFF_LENGTH					1512UL					/* (8+64)*21 */
#define 	ATL_BLOCK_SZ					64

#define 	ATL_ALIGNMENT   				(8 + ATL_BLOCK_SZ)		/* in bytes */
#define 	ITL_ALIGNMENT   				4						/* in bytes */

#define		INVALID_FRAME_NUMBER			0xFFFFFFFF				/* valid frame # is 0 - FFFF only */

/* Bit field definition for hwINFO of the td_t  */
#define 	HC_GTD_R						0x00040000UL			/* Buffer Rounding */
#define 	HC_GTD_DP						0x00180000UL			/* Direction/PID   */
#define 	HC_GTD_DI						0x00E00000UL			/* Delay Interrupt */
#define 	HC_GTD_T						0x03000000UL			/* Data Toggle     */
#define 	HC_GTD_MLSB						0x02000000UL			/* Data Toggle MSB */
#define 	HC_GTD_TLSB						0x01000000UL			/* Data Toggle LSB */
#define 	HC_GTD_EC						0x0C000000UL			/* Error Count     */
#define 	HC_GTD_CC						0xF0000000UL			/* Condition Code  */

/* Bit field definition for hwINFO of the ed_t */
#define 	HC_ED_FA						0x0000007FUL			/* Function Address */
#define 	HC_ED_EN						0x00000780UL			/* Endpoint Number */
#define 	HC_ED_DIR						0x00001800UL			/* Direction of data flow */
#define 	HC_ED_SPD						0x00002000UL			/* Device Speed */
#define 	HC_ED_SKIP						0x00004000UL			/* Skip this ED */
#define 	HC_ED_F							0x00008000UL			/* Format of this ED */
#define 	HC_ED_MPS						0x07FF0000UL			/* Maximum Packet Size */

/* Bit field definitions for hwNextP of the td_t */
#define 	HC_GTD_NEXTTD					0xFFFFFFF0UL			/* Next TD */

/* Bit field definition for hwHeadP of the ed_t */
#define 	HC_ED_HEADP						0xFFFFFFF0UL			/* Bit 31 .. 4 */
#define 	HC_ED_TOGGLE					0x00000002UL			/* Bit 1, toggle carry */
#define 	HC_ED_HALTED					0x00000001UL			/* Bit 0, halted */

/* Bit field definition for hwTailP of the ed_t */
#define 	HC_ED_TAILP						HC_ED_HEADP				/* Bit 31 .. 4 */

#define 	OHCI_SETUP						0X00000000UL
#define 	OHCI_OUT						0x00000001UL
#define 	OHCI_IN							0x00000002UL



/*-----------------------------------------------------------*/
/* 1362 control and data port numbers */
/*-----------------------------------------------------------*/

/* #define HC_IO_BASE */
/* #define HC_IO_SIZE */

#if defined(CONFIG_NSCU)

static void * hc_port;

#define		HC_IO_BASE						0xd0000000
#define		HC_DATA							(hc_port)
#define		HC_COM							(hc_port+2)

#define		HC_IO_SIZE						0x100

#define 	IRQ4HC_CHNNL					6						/* ISP1362 host controller's IRQ line */

#elif defined(CONFIG_SA1100_FRODO)

#define		HC_DATA							FRODO_USB_HC_DATA
#define		HC_COM							FRODO_USB_HC_CTRL
#define		IRQ4HC_CHNNL					FRODO_USB_HC_IRQ

#else

#define		HC_IO_BASE						0x290
#define		HC_DATA							HC_IO_BASE
#define		HC_COM							(HC_IO_BASE+2)

#define		HC_IO_SIZE						(HC_COM - HC_DATA +1)

#define 	IRQ4HC_CHNNL            		10						/* ISP1161 host controller's IRQ line */

#endif

/*-----------------------------------------------------------*/
/* ISP1161 host controller operational registers */
/*-----------------------------------------------------------*/

#define 	uHcRevision                     0x00UL			/* Revision Register */
#define 	uHcControl                      0x01UL			/* Control Register */
#define 	uHcCommandStatus                0x02UL			/* Command Status Register */
#define 	uHcInterruptStatus              0x03UL			/* Interrupt Status Register */
#define 	uHcInterruptEnable              0x04UL			/* Interrupt Enable Register */
#define 	uHcInterruptDisable             0x05UL			/* Interrupt Disable Register */
#define 	uHcFmInterval                   0x0dUL			/* Frame Interval Register */
#define 	uHcFmRemaining                  0x0eUL			/* Frame Remaining Register */
#define 	uHcFmNumber                     0x0fUL			/* Frame Number Register */
#define 	uHcLsThreshold                  0x11UL			/* Threshold register */
#define 	uHcRhDescriptorA                0x12UL			/* Root Hub Descriptor A Register */
#define 	uHcRhDescriptorB                0x13UL			/* Root Hub Descriptor B Register */
#define 	uHcRhStatus                     0x14UL			/* Root Hub Status Register */
#define 	uHcRhPort1Status                0x15UL          /* Root Hub Port 1 status */  /* ISP1161 has only two root hub ports */
#define 	uHcRhPort2Status                0x16UL			/* Root Hub Port 2 status */ 

/* These two registers are used internally by software. They are not ISP1161 hardware 
	registers.*/
#define 	uHcHcdControl                   0x60UL			/* HCD Software Control Register */
#define 	uHcHcdCommandStatus             0x61UL			/* HCD Software Command Status Register */


/* Bit field definition for register HcCommandStatus */
#define HC_COMMAND_STATUS_HCR		0x00000001UL		/* Host Controller Reset */
#define HC_COMMAND_STATUS_CLF		0x00000002UL		/* Control List Filled   */
#define HC_COMMAND_STATUS_BLF		0x00000004UL		/* Bulk List Filled      */



/**********************/
/* HcControl Register */
/**********************/
#define CB_RATIO				3	/* Control/Bulk transfer ratio */
									/* 0 = 1:1                     */
									/* 1 = 2:1                     */
									/* 2 = 3:1                     */
									/* 3 = 4:1                     */

#define PERIODIC_LIST_ENABLE	YES	/* Periodic transfer enable    */
#define ISO_ENABLE				YES	/* Isochronous transfer enable */
#define CONTROL_LIST_ENABLE		YES	/* Control transfer enable     */
#define BULK_LIST_ENABLE		YES	/* Bulk transfer enable        */
#define HC_STATE				2	/* Host functional state */
									/* 0 = Reset             */
									/* 1 = Resume            */
									/* 2 = Operational       */
									/* 3 = Suspend           */

#define REMOTE_WAKEUP_CONN		NO	/* Remote wakeup connected */

#define REMOTE_WAKEUP_ENABLE	NO	/* Remote wakeup enable    */


#define INT_NUM_IRQ0			0x20


/* Bit field definition for register HcControl */
#define HC_CONTROL_HCFS				0x000000C0UL		/* Host Controller Functional State, bit 7..6 */
#define HC_CONTROL_RWC				0x00000200UL		/* Remote Wakeup Connected, bit 9 */
#define HC_CONTROL_RWE				0x00000400UL		/* Remote Wakeup Enable, bit 10 */

/* Bit field definition for HCD register HcHcdControl */
/* ISP1161 does not have this HC register as OHCI     */
/* It is added as a global variable to emulate the    */
/* OHCI transfer control functionality defined in the */
/* following bit field                                */
#define HC_CONTROL_CBSR				0x00000003UL		/* Control/Bulk ratio */
#define HC_CONTROL_PLE				0X00000004UL		/* Periodic List Enable */
#define HC_CONTROL_IE				0x00000008UL		/* Isochronous Enable */
#define HC_CONTROL_CLE				0x00000010UL		/* Control List Enable */
#define HC_CONTROL_BLE				0x00000020UL		/* Bulk List Enable */
#define HC_CONTROL_TIP				0x00000100UL		/* Transfer In Progress */

/* Bit field definition for HCD register HcHcdCommand. */
/* ISP1161 does not have this register. This register  */
/* is added as an HCD global variable to emulate the   */
/* OHCI transfer control functionality                 */
#define HC_COMMAND_STATUS_CLF		0x00000002UL		/* Control List Filled   */
#define HC_COMMAND_STATUS_BLF		0x00000004UL		/* Bulk List Filled      */

/* Bit field definition for register HcInterruptStatus */
/* HcInterruptEnable/HcInterruptDisable registers      */
#define HC_INTERRUPT_SO				0x00000001UL			/* Scheduling Overrun      */
#define HC_INTERRUPT_SF				0x00000004UL			/* Start of Frame          */
#define HC_INTERRUPT_RD				0x00000008UL			/* Resume Detect           */
#define HC_INTERRUPT_UE				0x00000010UL			/* Unrecoverable error     */
#define HC_INTERRUPT_FNO			0x00000020UL			/* Frame Number Overflow   */
#define HC_INTERRUPT_RHSC			0x00000040UL			/* Root Hub Status Change  */
#define HC_INTERRUPT_ATD			0X00000080UL			/* ATL List Done           */
#define HC_INTERRUPT_MIE			0x80000000UL			/* Master Interrupt Enable */
#define HC_INTERRUPT_ALL			0x8000007FUL			/* All interrupts          */


/*************************/
/* HcFmInterval Register */
/*************************/
#define FS_LARGEST_DATA				0x00002778UL		/* Largest data packet */

/****************************/
/* HcRhDescriptorA Register */
/****************************/
#define PORT_POWER_SWITCHING			NO				/* Must be NO for ISP1161 */
#define OVER_CURRENT_PROTECTION			YES
#define PER_PORT_OVER_CURRENT_REPORT	NO
#define POWER_ON_TO_POWER_GOOD_TIME		50UL			/* Max. = 512 Msec. Use even number */

/* Bit field definition for register HcRhDescriptorA */
#define HC_RH_DESCRIPTORA_NDP		3			/* Number of downstream ports  */
#define HC_RH_DESCRIPTORA_PSM		0x00000100UL			/* Power Switching Mode        */
#define HC_RH_DESCRIPTORA_NPS		0x00000200UL			/* No Power Switching          */
#define HC_RH_DESCRIPTORA_OCPM		0x00000800UL			/* OverCurrent Protection Mode */
#define HC_RH_DESCRIPTORA_NOCP		0x00001000UL			/* No OverCurrent Protection   */
#define HC_RH_DESCRIPTORA_POTPGT	0xFF000000UL			/* Power On To Power Good Time */

/****************************/
/* HcRhDescriptorB Register */
/****************************/
#define DEVICE_REMOVABLE			0x00000000UL
#define PORT_POWER_MASK				0x00000000UL

/* Bit field definition for register HcRhDescriptorB */
#define HC_RH_DESCRIPTORB_PPCM		0xFFFF0000UL			/* Port Power Control Mask     */
#define HC_RH_DESCRIPTORB_DR		0x0000FFFFUL			/* Device Removable            */


/* Bit field definition for register HcRhStatus */
#define HC_RH_STATUS_LPS			0x00000001UL			/* R: Local Power Status       */


/*---------------------------------------------------*/
/* Index of the ISP1161 HC extended 16-bit registers */
/*---------------------------------------------------*/
#define 	REG_HW_MODE						0x20			/* Hardware configuration register */
#define 	REG_DMA_CNFG					0x21			/* DMA configuration register      */
#define 	REG_XFER_CNTR					0x22			/* Transfer counter register       */
#define 	REG_IRQ							0x24			/* Interrupt register              */
#define 	REG_IRQ_MASK					0x25			/* Interrupt enable register       */
#define 	REG_CHIP_ID						0x27			/* Chip ID register                */
#define 	REG_SCRATCH						0x28			/* Scratch register                */
#define 	REG_RESET_DEV					0xA9			/* Reset register                  */

#define 	REG_BUFF_STS					0x2C			/* Buffer status register          */

#define 	REG_ITL_BUFLEN					0x30			/* ITL buffer length register      */
#define 	REG_ITL_BUFF_IO					0x40			/* ITL0 buffer register            */
#define 	REG_ITL1_BUFF_IO				0x42			/* ITL1 buffer register            */
#define 	REG_ITL_TGRATE					0x47			/* ITL toggle rate register        */

#define 	REG_INTL_BUFLEN					0x33			/* INT buffer length register      */
#define 	REG_INTL_BUFF_IO				0x43			/* INT buffer register             */
#define 	REG_INTL_BLOCK_SZ				0x53			/* INT block size register         */
#define		REG_INTL_DONE					0x17
#define		REG_INTL_SKIP					0x18
#define		REG_INTL_LAST					0x19
#define		REG_INTL_CURRENT				0x1A

#define 	REG_ATL_BUFLEN					0x34			/* ATL buffer length register      */
#define 	REG_ATL_BUFF_IO					0x44			/* ATL buffer register             */
#define 	REG_ATL_BLOCK_SZ				0x54
#define		REG_ATL_DONE					0x1B
#define		REG_ATL_SKIP					0x1C
#define		REG_ATL_LAST					0x1D
#define		REG_ATL_CURRENT					0x1E
#define		REG_ATL_THRESH_CNT				0x51
#define		REG_ATL_THRESH_TOUT				0x52



#define 	PIC1_OCW1               		0x21
#define 	PIC1_CASCADE    				0xFB
#define 	PIC2_OCW1               		0xA1



/*------------------------------------------------------------------*/
/* ISP1161 external Interrupts configuration						*/
/*------------------------------------------------------------------*/

/**********************************/
/* HardwareConfiguration Register */
/**********************************/
#define 	GLOBAL_INT_PIN_ENABLE       	YES
#define 	INT_EDGE_TRIGGERED          	NO
#define 	INT_ACTIVE_HIGH             	YES

/****************************/
/* InterruptEnable Register */
/****************************/
#define 	SOF_INT_ENABLED             	YES
#define 	ATL_INT_ENABLED             	NO
#define 	EOT_INT_ENABLED             	NO
#define 	OPR_INT_ENABLED             	YES
#define 	HC_SUSPEND_INT_ENABLED      	NO
#define 	HC_RESUME_INT_ENABLED       	NO

/****************************/
/* HcFmInterval Register    */
/****************************/
#define		FRAME_INTERVAL					0x00002EDFUL
#define		FS_LARGEST_DATA					0x00002778UL

/*-----------------------------------------------*/
/* Bit field definition for REG_HW_MODE register */
/*-----------------------------------------------*/
#define 	INT_PIN_ENABLE					0x0001			/* Bit 0    */
#define 	INT_PIN_TRIGGER					0x0002			/* Bit 1    */
#define 	INT_OUTPUT_POLARITY				0x0004			/* Bit 2    */
#define 	DATA_BUS_WIDTH_16				0x0008			/* Bit 4..3 */
#define		DREQ_OUTPUT_POLARITY			0x0020
#define		DACK_INPUT_POLARITY				0x0040
#define		EOT_INPUT_POLARITY				0x0080
#define		DACK_MODE						0x0100
#define		ANALOG_OC_ENABLE				0x0400
#define		SUSPEND_CLK_NOT_STOP			0x0800
#define		DOWNSTREAM_PORT15K_SEL			0x1000

/*-------------------------------------------------------------*/
/* Bit field definition for REG_IRQ and REG_IRQ_MASK registers */
/*-------------------------------------------------------------*/
#define 	SOF_ITL_INT						0X0001			/* Bit 0: SOF and ITL interrupts */
#define 	ATL_INT							0X0100			/* Bit 8: ATL interrupt */
#define 	EOT_INT							0X0008			/* Bit 3: End of transfer interrupt */
#define 	OPR_INT							0X0010			/* Bit 4: HCOR int. */
#define 	HC_SUSPEND_INT					0X0020			/* Bit 5 */
#define 	HC_RESUME_INT					0X0040			/* Bit 6 */

/*-------------------------------------------------*/
/* Bit field definition for REG_BUFF_STS registers */
/*-------------------------------------------------*/

#define 	ATL_ACTIVE						0X0008			/* Bit 3: Process ATL buffer */

/*-----------------------------------------------------------*/
/* Definitions related to Philips Transfer Descriptors (PTD) */
/*-----------------------------------------------------------*/

/* Bit field definition for PTD byte 0 */
#define 	PTD_ACTUAL_BYTES70  			0XFF			/* Bit 7..0 */

/* Bit field definition for PTD byte 1 */
#define 	PTD_COMPLETION_CODE 			0XF0			/* Bit 7..4 */
#define 	PTD_ACTIVE      				0X08			/* Bit 3 */
#define 	PTD_TOGGLE      				0X04			/* Bit 2 */
#define 	PTD_ACTUAL_BYTES98  			0X03			/* Bit 1..0 */

/* Bit field definition for PTD byte 2 */
#define 	PTD_MAXPACKET70 				0XFF			/* Bit 7..0 */

/* Bit field definition for PTD byte 3 */
#define 	PTD_ED          				0XF0			/* Bit 7..4 */
#define 	PTD_LAST        				0X08			/* Bit 3 */
#define 	PTD_SPEED       				0X04			/* Bit 2 */
#define 	PTD_MAXPACKET98 				0X03 			/* Bit 1..0 */

/* Bit field definition for PTD byte 4 */
#define 	PTD_TOTAL70     				0XFF			/* Bit 7..0 */

/* Bit field definition for PTD byte 5 */
#define 	PTD_DIR         				0X0C			/* Bit 3..2 */
#define 	PTD_TOTAL98     				0X03			/* Bit 1..0 */

/* Bit field definition for PTD byte 6 */
#define 	PTD_FORMAT      				0X80			/* Bit 7 */
#define 	PTD_FUNCTION    				0X7F			/* Bit 6..0 */

