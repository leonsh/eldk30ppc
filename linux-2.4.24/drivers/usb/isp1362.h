#ifndef _ISP1362_H
#define _ISP1362_H

/*================================================
 * ISP1362 Device Controller interface
 */

#define	ISP1362_IO_PHYS		0xD0000000

#define	ISP1362_IRQ_HC		6
#define	ISP1362_IRQ_DC		8

/* #define	DEBUG */

#define	EP_DIR_IN		1
#define	EP_DIR_OUT		0

  /* Initialization commands
   */
#define	WR_DcEndpointConfiguration0_OUT		0x20
#define	WR_DcEndpointConfiguration0_IN		0x21
#define	WR_DcEndpointConfiguration		0x21 /* 1..14 */

#define	RD_DcEndpointConfiguration0_OUT		0x30
#define	RD_DcEndpointConfiguration0_IN		0x31
#define	RD_DcEndpointConfiguration		0x31 /* 1..14 */

#define	WR_DcAddress				0xB6
#define	RD_DcAddress				0xB7

#define	WR_DcMode				0xB8
#define	RD_DcMode				0xB9

#define	WR_DcHardwareConfiguration		0xBA
#define	RD_DcHardwareConfiguration		0xBB

#define	WR_DcInterruptEnable			0xC2
#define	RD_DcInterruptEnable			0xC3

#define	WR_DcDMAConfiguration			0xF0
#define	RD_DcDMAConfiguration			0xF1

#define	WR_DcDMACounter				0xF2
#define	RD_DcDMACounter				0xF3

#define	DO_ResetDevice				0xF6

  /* Data flow commands
   */
#define	DO_WriteEndpointBuffer0			0x01
#define	DO_WriteEndpointBuffer			0x01 /* 1..14 */

#define	DO_ReadEndpointBuffer0			0x10
#define	DO_ReadEndpointBuffer			0x11 /* 1..14 */

#define	DO_StallEndpoint0_OUT			0x40
#define	DO_StallEndpoint0_IN			0x41
#define	DO_StallEndpoint			0x41 /* 1..14 */

#define	RD_DcEndpointStatus0_OUT		0x50
#define	RD_DcEndpointStatus0_IN			0x51
#define	RD_DcEndpointStatus			0x51 /* 1..14 */

#define	DO_ValidateEndpointBuffer0		0x61
#define	DO_ValidateEndpointBuffer		0x61 /* 1..14 */

#define	DO_ClearEndpointBuffer0			0x70
#define	DO_ClearEndpointBuffer			0x71 /* 1..14 */

#define	DO_UnstallEndpoint0_OUT			0x80
#define	DO_UnstallEndpoint0_IN			0x81
#define	DO_UnstallEndpoint			0x81 /* 1..14 */

#define	RD_DcEndpointStatusImage0_OUT		0xD0
#define	RD_DcEndpointStatusImage0_IN		0xD1
#define	RD_DcEndpointStatusImage		0xD1 /* 1..14 */

#define	DO_AcknowledgeSetup			0xF4

  /* General commands
   */
#define	RD_DcErrorCode0_OUT			0xA0
#define	RD_DcErrorCode0_IN			0xA1
#define	RD_DcErrorCode				0xA1 /* 1..14 */

#define	WR_UnlockDevice				0xB0

#define	WR_DcScratch				0xB2
#define	RD_DcScratch				0xB3

#define	RD_DcFrameNumber			0xB4

#define	RD_DcChipID				0xB5

#define	RD_DcInterrupt				0xC0

  /* DcEndpointConfiguration register: bit allocation
   */
#define	EPCONF_FFOSZ_MASK			0x0F
#define	EPCONF_FFOSZ_NONISO_8			0x00
#define	EPCONF_FFOSZ_NONISO_16			0x01
#define	EPCONF_FFOSZ_NONISO_32			0x02
#define	EPCONF_FFOSZ_NONISO_64			0x03
#define	EPCONF_FFOSZ_ISO_16			0x00
#define	EPCONF_FFOSZ_ISO_32			0x01
#define	EPCONF_FFOSZ_ISO_48			0x02
#define	EPCONF_FFOSZ_ISO_64			0x03
#define	EPCONF_FFOSZ_ISO_96			0x04
#define	EPCONF_FFOSZ_ISO_128			0x05
#define	EPCONF_FFOSZ_ISO_160			0x06
#define	EPCONF_FFOSZ_ISO_192			0x07
#define	EPCONF_FFOSZ_ISO_256			0x08
#define	EPCONF_FFOSZ_ISO_320			0x09
#define	EPCONF_FFOSZ_ISO_384			0x0a
#define	EPCONF_FFOSZ_ISO_512			0x0b
#define	EPCONF_FFOSZ_ISO_640			0x0c
#define	EPCONF_FFOSZ_ISO_768			0x0d
#define	EPCONF_FFOSZ_ISO_896			0x0e
#define	EPCONF_FFOSZ_ISO_1023			0x0f

#define	EPCONF_FFOISO				0x10
#define	EPCONF_DBLBUF				0x20
#define	EPCONF_EPDIR_IN				0x40
#define	EPCONF_EPDIR_OUT			0x00
#define	EPCONF_FIFOEN				0x80

  /* DcMode register: bit allocation
   */
#define	MODE_SOFTCT				0x01
#define	MODE_DBGMOD				0x04
#define	MODE_INTENA				0x08
#define	MODE_GOSUSP				0x20

  /* DcHardwareConfiguration register: bit allocation
   */
#define	HWCONF_INTPOL				0x0001
#define	HWCONF_INTLVL				0x0002
#define	HWCONF_WKUPCS				0x0008
#define	HWCONF_DAKPOL				0x0020
#define	HWCONF_DRQPOL				0x0040
#define	HWCONF_DAKOLY				0x0080
#define	HWCONF_CKDIV_MASK			0x0F00
#define	HWCONF_CKDIV_12MHZ			0x0300
#define	HWCONF_CLKRUN				0x1000
#define	HWCONF_NOLAZY				0x2000
#define	HWCONF_EXTPUL				0x4000

  /* DcInterruptEnable/DcInterrupt registers: bit allocation
   */
#define	INT_RST					0x00000001
#define	INT_RESM				0x00000002
#define	INT_SUSP				0x00000004
#define	INT_EOT					0x00000008
#define	INT_SOF					0x00000010
#define	INT_PSOF				0x00000020
#define	INT_SP_EOT				0x00000040
#define	INT_EP0_OUT				0x00000100
#define	INT_EP0_IN				0x00000200
#define	INT_EP					0x00000200 /* 1..14 */

  /* DcEndpointStatus register: bit allocation
   */
#define	EPSTAT_CPUBUF				0x02
#define	EPSTAT_SETUPT				0x04
#define	EPSTAT_OVERWRITE			0x08
#define	EPSTAT_DATA_PID				0x10
#define	EPSTAT_EPFULL0				0x20
#define	EPSTAT_EPFULL1				0x40
#define	EPSTAT_EPSTAL				0x80

  /* DcErrorCode register: bit allocation
   */
#define	EPERR_RTOK				0x01
#define	EPERR_ERR_MASK				0x1E
#define	EPERR_ERR_PID_ENC			0x02
#define	EPERR_ERR_PID_UNKNOWN			0x04
#define	EPERR_ERR_UNEXP_PACKET			0x06
#define	EPERR_ERR_TOKEN_CRC			0x08
#define	EPERR_ERR_DATA_CRC			0x0A
#define	EPERR_ERR_TIMEOUT			0x0C
#define	EPERR_ERR_BABBLE			0x0E
#define	EPERR_ERR_UNEXP_EOP			0x10
#define	EPERR_ERR_NAK				0x12
#define	EPERR_ERR_STALL				0x14
#define	EPERR_ERR_OVERFLOW			0x16
#define	EPERR_ERR_EMPTY_PACKET			0x18
#define	EPERR_ERR_BIT_STUFF			0x1A
#define	EPERR_ERR_SYNC				0x1C
#define	EPERR_ERR_WRONG_TOGGLE			0x1E
#define	EPERR_DATA01				0x40
#define	EPERR_UNREAD				0x80


/*================================================
 * USB Protocol Stuff
 */

/* Request Codes   */
enum {	GET_STATUS=0,		CLEAR_FEATURE=1,	SET_FEATURE=3,
	SET_ADDRESS=5,		GET_DESCRIPTOR=6,	SET_DESCRIPTOR=7,
	GET_CONFIGURATION=8,	SET_CONFIGURATION=9,	GET_INTERFACE=10,
	SET_INTERFACE=11 };


/* USB Device Requests */
typedef struct
{
    __u8 bmRequestType;
    __u8 bRequest;
    __u16 wValue;
    __u16 wIndex;
    __u16 wLength;
} usb_dev_request_t  __attribute__ ((packed));


//////////////////////////////////////////////////////////////////////////////
// Descriptor Management
//////////////////////////////////////////////////////////////////////////////

#define DescriptorHeader \
	__u8 bLength;        \
	__u8 bDescriptorType


// --- Device Descriptor -------------------

typedef struct {
	 DescriptorHeader;
	 __u16 bcdUSB;	   	/* USB specification revision number in BCD */
	 __u8  bDeviceClass;	/* USB class for entire device */
	 __u8  bDeviceSubClass; /* USB subclass information for entire device */
	 __u8  bDeviceProtocol; /* USB protocol information for entire device */
	 __u8  bMaxPacketSize0; /* Max packet size for endpoint zero */
	 __u16 idVendor;        /* USB vendor ID */
	 __u16 idProduct;       /* USB product ID */
	 __u16 bcdDevice;       /* vendor assigned device release number */
	 __u8  iManufacturer;	/* index of manufacturer string */
	 __u8  iProduct;        /* index of string that describes product */
	 __u8  iSerialNumber;	/* index of string containing device serial number */
	 __u8  bNumConfigurations; /* number fo configurations */
}  __attribute__ ((packed)) device_desc_t;

// --- Configuration Descriptor ------------

typedef struct {
	 DescriptorHeader;
	 __u16 wTotalLength;	    /* total # of bytes returned in the cfg buf 4 this cfg */
	 __u8  bNumInterfaces;      /* number of interfaces in this cfg */
	 __u8  bConfigurationValue; /* used to uniquely ID this cfg */
	 __u8  iConfiguration;      /* index of string describing configuration */
	 __u8  bmAttributes;        /* bitmap of attributes for ths cfg */
	 __u8  MaxPower;		    /* power draw in 2ma units */
} __attribute__ ((packed)) config_desc_t;

// bmAttributes:
enum { USB_CONFIG_REMOTEWAKE=0x20, USB_CONFIG_SELFPOWERED=0x40,
	   USB_CONFIG_BUSPOWERED=0x80 };
// MaxPower:
#define USB_POWER( x)  ((x)>>1) /* convert mA to descriptor units of A for MaxPower */

// --- Interface Descriptor ---------------

typedef struct {
	 DescriptorHeader;
	 __u8  bInterfaceNumber;   /* Index uniquely identfying this interface */
	 __u8  bAlternateSetting;  /* ids an alternate setting for this interface */
	 __u8  bNumEndpoints;      /* number of endpoints in this interface */
	 __u8  bInterfaceClass;    /* USB class info applying to this interface */
	 __u8  bInterfaceSubClass; /* USB subclass info applying to this interface */
	 __u8  bInterfaceProtocol; /* USB protocol info applying to this interface */
	 __u8  iInterface;         /* index of string describing interface */
} __attribute__ ((packed)) intf_desc_t;

// --- Endpoint  Descriptor ---------------

typedef struct {
	 DescriptorHeader;
	 __u8  bEndpointAddress;  /* 0..3 ep num, bit 7: 0 = 0ut 1= in */
	 __u8  bmAttributes;      /* 0..1 = 0: ctrl, 1: isoc, 2: bulk 3: intr */
	 __u16 wMaxPacketSize;    /* data payload size for this ep in this cfg */
	 __u8  bInterval;         /* polling interval for this ep in this cfg */
} __attribute__ ((packed)) ep_desc_t;

// bEndpointAddress:
enum { USB_OUT= 0, USB_IN=1 };
#define USB_EP_ADDRESS(a,d) (((a)&0xf) | ((d) << 7))
// bmAttributes:
enum { USB_EP_CNTRL=0, USB_EP_BULK=2, USB_EP_INT=3 };

// --- String Descriptor -------------------

typedef struct {
	 DescriptorHeader;
	 __u16 bString[1];		  /* unicode string .. actaully 'n' __u16s */
} __attribute__ ((packed)) string_desc_t;

/*=======================================================
 * Default descriptor layout for SA-1100 and SA-1110 UDC
 */

/* "config descriptor buffer" - that is, one config,
   ..one interface and 2 endpoints */
struct cdb {
	 config_desc_t cfg;
	 intf_desc_t   intf;
	 ep_desc_t     ep1;
	 ep_desc_t     ep2;
} __attribute__ ((packed));


/* all SA device descriptors */
typedef struct {
	 device_desc_t dev;   /* device descriptor */
	 struct cdb b;        /* bundle of descriptors for this cfg */
} __attribute__ ((packed)) desc_t;

// descriptor types
enum { USB_DESC_DEVICE=1, USB_DESC_CONFIG=2, USB_DESC_STRING=3,
	   USB_DESC_INTERFACE=4, USB_DESC_ENDPOINT=5 };

/*=======================================================
 * Handy helpers when working with above
 *
 */
// these are ppc-style 16 bit "words" ...
#define make_word_c( w ) __constant_cpu_to_le16(w)
#define make_word( w )   __cpu_to_le16(w)

/* Data extraction from usb_request_t fields */
enum { kTargetDevice=0, kTargetInterface=1, kTargetEndpoint=2 };
static inline int request_target( __u8 b ) { return (int) ( b & 0x0F); }

static inline int windex_to_ep_num( __u16 w ) { return (int) ( w & 0x000F); }
static inline int type_code_from_request( __u8 by ) { return (( by >> 4 ) & 3); }


/*=======================================================
 * Extern declarations
 *
 */
extern int isp1362_open( const char * client );
extern int isp1362_start( void );
extern int isp1362_stop( void );
extern int isp1362_close( void );

extern ulong isp1362_read_buffer (ulong ep, void * buf, long size);
extern void isp1362_write_buffer (ulong ep, void * buf, ulong size);
extern ulong isp1362_read_status (ulong ep, ulong dir);
extern ulong isp1362_check_status (ulong ep, ulong dir);
extern void isp1362_clear_buffer (ulong ep);


extern desc_t * isp1362_get_descriptor_ptr( void );
extern int isp1362_set_string_descriptor( int i, string_desc_t * p );
extern string_desc_t * isp1362_get_string_descriptor( int i );
extern string_desc_t * isp1362_kmalloc_string_descriptor( const char * p );


typedef void (*usb_callback_t)(int flag, int size);

extern int isp1362_recv(char *buf, int len, usb_callback_t callback);
extern void isp1362_recv_reset(void);

extern int isp1362_send(char *buf, int len, usb_callback_t callback);
extern void isp1362_send_reset(void);

extern int isp1362_ep1_init(void);
extern int isp1362_ep2_init(void);

extern void isp1362_ep1_reset(void);
extern void isp1362_ep2_reset(void);

extern void isp1362_ep1_int_hndlr(void);
extern void isp1362_ep2_int_hndlr(void);

extern ulong isp1362_cnt_err_out;
extern ulong isp1362_cnt_err_in;
extern ulong isp1362_cnt_err_setup;

#endif /* _ISP1362_H */
