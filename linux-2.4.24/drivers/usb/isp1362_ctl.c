/*
 * ISP1362 USB controller core driver.
 * Copyright (C) 2003 DENX Software Engineering, Wolfgang Denk, wd@denx.de
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/tqueue.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/io.h>

#include "isp1362.h"

/* #define	DEBUG */

#ifdef DEBUG
#define	debug(fmt...)	printk(## fmt)
#else
#define	debug(fmt...)
#endif


#define	ISP1362_DC_DATA		(isp1362_io_base + 4)
#define	ISP1362_DC_COM		(isp1362_io_base + 6)

#define	BYTE_SWAP(x)		(((x) >> 8) | (((x) & 0xFF) << 8))

#define	PROC_NODE_NAME		"isp1362"


static void * isp1362_io_base = NULL;
static desc_t isp1362_desc;
static int isp1362_ep0_empty_queued = 0;

#define MAX_STRING_DESC 8
static string_desc_t * string_desc_array[ MAX_STRING_DESC ];
static string_desc_t sd_zero;  /* special sd_zero holds language codes */

static ulong isp1362_cnt_int = 0;
static ulong isp1362_cnt_reset = 0;
static ulong isp1362_cnt_resume = 0;
static ulong isp1362_cnt_suspend = 0;
ulong isp1362_cnt_err_setup = 0;
ulong isp1362_cnt_err_in = 0;
ulong isp1362_cnt_err_out = 0;


#ifdef DEBUG
static inline void isp1362_print_request( usb_dev_request_t * pReq )
{
	 static char * tnames[] = { "dev", "intf", "ep", "oth" };
	 static char * rnames[] = { "std", "class", "vendor", "???" };
	 char * psz;
	 switch( pReq->bRequest ) {
	 case GET_STATUS: psz = "get stat"; break;
	 case CLEAR_FEATURE: psz = "clr feat"; break;
	 case SET_FEATURE: psz = "set feat"; break;
	 case SET_ADDRESS: psz = "set addr"; break;
	 case GET_DESCRIPTOR: psz = "get desc"; break;
	 case SET_DESCRIPTOR: psz = "set desc"; break;
	 case GET_CONFIGURATION: psz = "get cfg"; break;
	 case SET_CONFIGURATION: psz = "set cfg"; break;
	 case GET_INTERFACE: psz = "get intf"; break;
	 case SET_INTERFACE: psz = "set intf"; break;
	 default: psz = "unknown"; break;
	 }
	 debug( "- [%s: %s req to %s. dir=%s]\n", psz,
			 rnames[ (pReq->bmRequestType >> 5) & 3 ],
			 tnames[ pReq->bmRequestType & 3 ],
			 ( pReq->bmRequestType & 0x80 ) ? "in" : "out" );
}
#endif /* DEBUG */



static inline ulong isp1362_inw (volatile unsigned short * port)
{
	ulong res;
	mb();
	res = *port;
	mb();
	return res;
}

static inline void isp1362_outw (ulong data, volatile unsigned short * port)
{
	mb();
	*port = data;
	mb();
}

static ulong isp1362_read_reg32 (ulong reg)
{
	ulong res;

	isp1362_outw (reg, ISP1362_DC_COM);
	res = isp1362_inw (ISP1362_DC_DATA);
	res |= isp1362_inw (ISP1362_DC_DATA) << 16;

	return res;
}

static void isp1362_write_reg32 (ulong reg, ulong data)
{
	isp1362_outw (reg, ISP1362_DC_COM);
	isp1362_outw (data & 0xFFFF, ISP1362_DC_DATA);
	isp1362_outw (data >> 16, ISP1362_DC_DATA);
}

static ulong isp1362_read_reg16 (ulong reg)
{
	ulong res;

	isp1362_outw (reg, ISP1362_DC_COM);
	res = isp1362_inw (ISP1362_DC_DATA);

	return res;
}

static void isp1362_write_reg16 (ulong reg, ulong data)
{
	isp1362_outw (reg, ISP1362_DC_COM);
	isp1362_outw (data, ISP1362_DC_DATA);
}

static void isp1362_cmd_reg (ulong reg)
{
	isp1362_outw (reg, ISP1362_DC_COM);
}

  /* Set device address
   */
static void isp1362_set_address (ulong devaddr)
{
	isp1362_write_reg16 (WR_DcAddress, 0x80 | devaddr);
}

#ifdef UNUSED
  /* Resets the DC in the same way as an external hardware reset
   */
static void isp1362_reset_device (void)
{
	isp1362_cmd_reg (DO_ResetDevice);
}
#endif /* UNUSED */

  /* Read endpoint status
   */
ulong isp1362_read_status (ulong ep, ulong dir)
{
	ulong code = ep ? RD_DcEndpointStatus + ep : dir == EP_DIR_IN ?
			  RD_DcEndpointStatus0_IN : RD_DcEndpointStatus0_OUT;

	return isp1362_read_reg16 (code);
}

  /* Check endpoint status
   */
ulong isp1362_check_status (ulong ep, ulong dir)
{
	ulong code = ep ? RD_DcEndpointStatusImage + ep : dir == EP_DIR_IN ?
			  RD_DcEndpointStatusImage0_IN :
			  RD_DcEndpointStatusImage0_OUT;

	return isp1362_read_reg16 (code);
}

  /* Validate endpoint buffer.
   */
static void isp1362_validate_buffer (ulong ep)
{
	ulong code = ep ? DO_ValidateEndpointBuffer + ep :
			  DO_ValidateEndpointBuffer0;

	isp1362_cmd_reg (code);
}

  /* Clear endpoint buffer.
   */
void isp1362_clear_buffer (ulong ep)
{
	ulong code = ep ? DO_ClearEndpointBuffer + ep :
			  DO_ClearEndpointBuffer0;

	isp1362_cmd_reg (code);
}

  /* Acknowledge Set-up.
   */
static void isp1362_acknowledge_setup (void)
{
	isp1362_cmd_reg (DO_AcknowledgeSetup);
}

  /* Unlock device
   */
static void isp1362_unlock_device (void)
{
	isp1362_write_reg16 (WR_UnlockDevice, 0xAA37);
}

  /* Chip identification code and hardware version number
   */
static ulong isp1362_chip_id (void)
{
	return isp1362_read_reg16 (RD_DcChipID);
}

  /* Stall endpoint.
   */
static void isp1362_stall_endpoint (ulong ep, ulong dir)
{
	ulong code = ep ? DO_StallEndpoint + ep : dir == EP_DIR_IN ?
			  DO_StallEndpoint0_IN : DO_StallEndpoint0_OUT;

	isp1362_cmd_reg (code);
}

#ifdef UNUSED
  /* Unstall endpoint.
   */
static void isp1362_unstall_endpoint (ulong ep, ulong dir)
{
	ulong code = ep ? DO_UnstallEndpoint + ep : dir == EP_DIR_IN ?
			  DO_UnstallEndpoint0_IN : DO_UnstallEndpoint0_OUT;

	isp1362_cmd_reg (code);
}

  /* Read endpoint error code
   */
static ulong isp1362_error_code (ulong ep, ulong dir)
{
	ulong code = ep ? RD_DcErrorCode + ep : dir == EP_DIR_IN ?
			  RD_DcErrorCode0_IN : RD_DcErrorCode0_OUT;

	return isp1362_read_reg16 (code);
}

  /* Read scratch register
   */
static ulong isp1362_read_scratch (void)
{
	return isp1362_read_reg16 (RD_DcScratch);
}

  /* Write scratch register
   */
static void isp1362_write_scratch (ulong data)
{
	isp1362_write_reg16 (WR_DcScratch, data);
}

  /* Frame number of the last successfully received SOF
   */
static ulong isp1362_frame_number (void)
{
	return isp1362_read_reg16 (RD_DcFrameNumber) & 0x07FF;
}
#endif /* UNUSED */

  /* Write to endpoint buffer memory
   */
void isp1362_write_buffer (ulong ep, void * buf, ulong size)
{
	ulong code = ep ? DO_WriteEndpointBuffer + ep : DO_WriteEndpointBuffer0;
	ushort data;
	int i;

	isp1362_cmd_reg (code);

	isp1362_outw (size, ISP1362_DC_DATA);

	for (i = 0; i < size / 2; i ++)
	{
		data = ((ushort *)buf)[i];
		data = BYTE_SWAP (data);
		isp1362_outw (data, ISP1362_DC_DATA);
	}
	if (size & 1)
	{
		data = ((u_char *)buf)[size - 1];
		isp1362_outw (data, ISP1362_DC_DATA);
	}

	isp1362_validate_buffer (ep);
}

  /* Read from endpoint buffer memory
   */
ulong isp1362_read_buffer (ulong ep, void * buf, long size)
{
	ulong code = ep ? DO_ReadEndpointBuffer + ep : DO_ReadEndpointBuffer0;
	ulong res;
	ushort data;
	int i;

	isp1362_cmd_reg (code);

	res = isp1362_inw (ISP1362_DC_DATA);
	if (res > size)
	{
		printk ("isp1362: Packet is too long: %04X > %04X\n",
			(uint)res, (uint)size);
		res = size;
	}

	for (i = 0; i < res / 2; i ++)
	{
		data = isp1362_inw (ISP1362_DC_DATA);
		data = BYTE_SWAP (data);
		((ushort *)buf)[i] = data;
	}
	if (res & 1)
	{
		data = isp1362_inw (ISP1362_DC_DATA);
		((u_char *)buf)[res - 1] = data;
	}

	if (ep == 0 && (isp1362_check_status (0, EP_DIR_OUT) & EPSTAT_SETUPT))
	{
		isp1362_acknowledge_setup ();
	}

	isp1362_clear_buffer (ep);

	return res;
}

  /* The allocation of buffer memory
   * only takes place after all 16 endpoints
   * have been configured in sequence
   */
static void isp1362_configure_endpoints (void)
{
	int i;

	isp1362_write_reg16 (
		WR_DcEndpointConfiguration0_OUT,
		EPCONF_FFOSZ_NONISO_64 | EPCONF_EPDIR_OUT | EPCONF_FIFOEN);

	isp1362_write_reg16 (
		WR_DcEndpointConfiguration0_IN,
		EPCONF_FFOSZ_NONISO_64 | EPCONF_EPDIR_IN | EPCONF_FIFOEN);

	isp1362_write_reg16 (
		WR_DcEndpointConfiguration + 1,
		EPCONF_FFOSZ_NONISO_64 | EPCONF_EPDIR_OUT | EPCONF_FIFOEN);

	isp1362_write_reg16 (
		WR_DcEndpointConfiguration + 2,
		EPCONF_FFOSZ_NONISO_64 | EPCONF_EPDIR_IN | EPCONF_FIFOEN);

	for (i = 3; i <= 14; i++)
	{
		isp1362_write_reg16 (WR_DcEndpointConfiguration + i, 0);
	}
}

  /* Reset handler
   */
static void isp1362_reset (void)
{
	isp1362_ep1_reset();
	isp1362_ep2_reset();

	isp1362_configure_endpoints ();
}

  /* Resume handler
   */
static void isp1362_resume (void)
{
	isp1362_unlock_device();
}

  /* Suspend handler
   */
static void isp1362_suspend (void)
{

}

  /* Setup default descriptors
   */
static void isp1362_initialize_descriptors(void)
{
	isp1362_desc.dev.bLength               = sizeof( device_desc_t );
	isp1362_desc.dev.bDescriptorType       = USB_DESC_DEVICE;
	isp1362_desc.dev.bcdUSB                = make_word_c(0x100);
	isp1362_desc.dev.bDeviceClass          = 0xFF;
	isp1362_desc.dev.bDeviceSubClass       = 0;
	isp1362_desc.dev.bDeviceProtocol       = 0;
	isp1362_desc.dev.bMaxPacketSize0       = 64;
	isp1362_desc.dev.idVendor              = 0;
	isp1362_desc.dev.idProduct             = 0;
	isp1362_desc.dev.bcdDevice             = 0;
	isp1362_desc.dev.iManufacturer         = 0;
	isp1362_desc.dev.iProduct              = 0;
	isp1362_desc.dev.iSerialNumber         = 0;
	isp1362_desc.dev.bNumConfigurations    = 1;

	isp1362_desc.b.cfg.bLength             = sizeof( config_desc_t );
	isp1362_desc.b.cfg.bDescriptorType     = USB_DESC_CONFIG;
	isp1362_desc.b.cfg.wTotalLength        = make_word_c( sizeof(struct cdb) );
	isp1362_desc.b.cfg.bNumInterfaces      = 1;
	isp1362_desc.b.cfg.bConfigurationValue = 1;
	isp1362_desc.b.cfg.iConfiguration      = 0;
	isp1362_desc.b.cfg.bmAttributes        = USB_CONFIG_BUSPOWERED;
	isp1362_desc.b.cfg.MaxPower            = USB_POWER( 500 );

	isp1362_desc.b.intf.bLength            = sizeof( intf_desc_t );
	isp1362_desc.b.intf.bDescriptorType    = USB_DESC_INTERFACE;
	isp1362_desc.b.intf.bInterfaceNumber   = 0; /* unique intf index*/
	isp1362_desc.b.intf.bAlternateSetting  = 0;
	isp1362_desc.b.intf.bNumEndpoints      = 2;
	isp1362_desc.b.intf.bInterfaceClass    = 0xFF; /* vendor specific */
	isp1362_desc.b.intf.bInterfaceSubClass = 0;
	isp1362_desc.b.intf.bInterfaceProtocol = 0;
	isp1362_desc.b.intf.iInterface         = 0;

	isp1362_desc.b.ep1.bLength             = sizeof( ep_desc_t );
	isp1362_desc.b.ep1.bDescriptorType     = USB_DESC_ENDPOINT;
	isp1362_desc.b.ep1.bEndpointAddress    = USB_EP_ADDRESS( 1, USB_OUT );
	isp1362_desc.b.ep1.bmAttributes        = USB_EP_BULK;
	isp1362_desc.b.ep1.wMaxPacketSize      = make_word_c( 64 );
	isp1362_desc.b.ep1.bInterval           = 0;

	isp1362_desc.b.ep2.bLength             = sizeof( ep_desc_t );
	isp1362_desc.b.ep2.bDescriptorType     = USB_DESC_ENDPOINT;
	isp1362_desc.b.ep2.bEndpointAddress    = USB_EP_ADDRESS( 2, USB_IN );
	isp1362_desc.b.ep2.bmAttributes        = USB_EP_BULK;
	isp1362_desc.b.ep2.wMaxPacketSize      = make_word_c( 64 );
	isp1362_desc.b.ep2.bInterval           = 0;

	/* set language */
	/* See: http://www.usb.org/developers/data/USB_LANGIDs.pdf */
	sd_zero.bDescriptorType = USB_DESC_STRING;
	sd_zero.bLength         = sizeof( string_desc_t );
	sd_zero.bString[0]      = make_word_c( 0x409 ); /* American English */
	isp1362_set_string_descriptor( 0, &sd_zero );
}

  /* Descriptor Manipulation.
   * Use these between open() and start() to setup
   * the descriptors for your device.
   */

  /* Get pointer to static default descriptor
   */
desc_t * isp1362_get_descriptor_ptr( void )
{
	return &isp1362_desc;
}

  /* Set a string descriptor
   */
int isp1362_set_string_descriptor( int i, string_desc_t * p )
{
	 int retval;
	 if ( i < MAX_STRING_DESC ) {
		  string_desc_array[i] = p;
		  retval = 0;
	 } else {
		  retval = -EINVAL;
	 }
	 return retval;
}

  /* Get a previously set string descriptor
   */
string_desc_t * isp1362_get_string_descriptor( int i )
{
	 return ( i < MAX_STRING_DESC )
		    ? string_desc_array[i]
		    : NULL;
}


  /* kmalloc and unicode up a string descriptor
   */
string_desc_t * isp1362_kmalloc_string_descriptor( const char * p )
{
	 string_desc_t * pResult = NULL;

	 if ( p ) {
		  int len = strlen( p );
		  int uni_len = len * sizeof( __u16 );
		  pResult = (string_desc_t*) kmalloc( uni_len + 2, GFP_KERNEL ); /* ugh! */
		  if ( pResult != NULL ) {
			   int i;
			   pResult->bLength = uni_len + 2;
			   pResult->bDescriptorType = USB_DESC_STRING;
			   for( i = 0; i < len ; i++ ) {
					pResult->bString[i] = make_word( (__u16) p[i] );
			   }
		  }
	 }
	 return pResult;
}


static void isp1362_send_empty_packet (ulong ep)
{
	isp1362_write_buffer (ep, NULL, 0);
}

static int isp1362_setup_data_send (void * in, ulong requested, ulong have)
{
	if (requested > have)
	{
		debug ("isp1362: Wrong data size requested: %lu>%lu\n", requested, have);
		requested = have;
		isp1362_ep0_empty_queued = 1;
	}

	isp1362_write_buffer (0, in, requested);
	return 0;
}

  /* Get Descriptor request handler
   */
static void isp1362_get_descriptor( usb_dev_request_t * pReq )
{
	int rc = 0;
	string_desc_t * pString;
	ep_desc_t * pEndpoint;

	desc_t * pDesc = isp1362_get_descriptor_ptr();
	int type = __le16_to_cpu(pReq->wValue) >> 8;
	int idx  = __le16_to_cpu(pReq->wValue) & 0xFF;

	switch( type ) {
	case USB_DESC_DEVICE:
		rc = isp1362_setup_data_send (&pDesc->dev,
					      __le16_to_cpu(pReq->wLength),
					      sizeof(device_desc_t));
		break;

	case USB_DESC_CONFIG:
		rc = isp1362_setup_data_send (&pDesc->b,
					      __le16_to_cpu(pReq->wLength),
					      sizeof(struct cdb));
		break;

		  // not quite right, since doesn't do language code checking
	case USB_DESC_STRING:
		pString = isp1362_get_string_descriptor( idx );
		if ( pString ) {
			rc = isp1362_setup_data_send (pString,
						      __le16_to_cpu(pReq->wLength),
						      pString->bLength);
		}
		else {
			printk("isp1362: Unkown string index %d\n", idx );
			rc = -1;
		}
		break;

	case USB_DESC_INTERFACE:
		if ( idx == pDesc->b.intf.bInterfaceNumber ) {
			rc = isp1362_setup_data_send (&pDesc->b.intf,
						      __le16_to_cpu(pReq->wLength),
						      pDesc->b.intf.bLength);
		}
		break;

	case USB_DESC_ENDPOINT:
		if ( idx == 1 )
			pEndpoint = &pDesc->b.ep1;
		else if ( idx == 2 )
			pEndpoint = &pDesc->b.ep2;
		else
			pEndpoint = NULL;
		if ( pEndpoint ) {
			rc = isp1362_setup_data_send (pEndpoint,
						      __le16_to_cpu(pReq->wLength),
						      pEndpoint->bLength);
		} else {
			printk("isp1362: Unkown endpoint index %d\n", idx );
			rc = -1;
		}
		break;

	default :
		printk("isp1362: Unknown descriptor type %d\n", type );
		rc = -1;
		break;

	}

	if (rc)
	{
		printk ("isp1362: Get Descriptor failed. Stall.\n");
		isp1362_cnt_err_setup++;
		isp1362_stall_endpoint (0, EP_DIR_IN);
	}
}

  /* Get Status request handler
   */
static void isp1362_get_status( usb_dev_request_t * pReq )
{
	unsigned char status_buf[2] = {0, 0};
	int rc = 0;
	int n;

	/* return status bit flags */
	n = request_target(pReq->bmRequestType);
	switch( n ) {
	case kTargetDevice:
		if ( 0 ) /* self_powered */
			status_buf[0] |= 1;
		break;
	case kTargetInterface:
		break;
	case kTargetEndpoint:
		/* return stalled bit */
		n = windex_to_ep_num( __le16_to_cpu(pReq->wIndex) );
		if ( n == 1 || n == 2 )	{
			if (isp1362_check_status (n, 0) & EPSTAT_EPSTAL)
			{
				status_buf[0] |= 1;
			}
		}
		else {
			printk( "isp1362: Unknown endpoint (%d) in GET_STATUS\n", n );
		}
		break;
	default:
		printk( "isp1362: Unknown target (%d) in GET_STATUS\n", n );
		/* fall thru */
		break;
	}

	rc = isp1362_setup_data_send (status_buf,
				      __le16_to_cpu(pReq->wLength),
				      sizeof(status_buf));
	if (rc)
	{
		printk ("isp1362: Get Status failed. Stall.\n");
		isp1362_cnt_err_setup++;
		isp1362_stall_endpoint (0, EP_DIR_IN);
	}
}

  /* Endpoint 0 OUT interrupt handler
   */
static void isp1362_ep0_out_int_hndlr (void)
{
	ulong status = isp1362_read_status (0, EP_DIR_OUT);
	usb_dev_request_t req;
	int request_type;
	ulong res;

	debug ("isp1362: EP0 OUT status: %02X\n", (uint)status);

	if (!(status & EPSTAT_SETUPT))
	{
		goto Done;
	}

	isp1362_ep0_empty_queued = 0;

	res = isp1362_read_buffer (0, &req, sizeof(req));
	if (res != sizeof(req))
	{
		printk ("isp1362: Invalid SETUP size\n");
		isp1362_cnt_err_setup++;
		isp1362_stall_endpoint (0, EP_DIR_IN);
		goto Done;
	}

	 /* Is it a standard request? (not vendor or class request) */
	request_type = type_code_from_request (req.bmRequestType);
	if (request_type != 0)
	{
		printk ("isp1362: Unsupported bmRequestType: ignored\n");
		isp1362_cnt_err_setup++;
		isp1362_send_empty_packet (0);
		goto Done;
	}

#ifdef DEBUG
	{
		unsigned char * pdb = (unsigned char *) &req;
		debug ("isp1362: %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X ",
			pdb[0], pdb[1], pdb[2], pdb[3],
			pdb[4], pdb[5], pdb[6], pdb[7]);
		isp1362_print_request( &req );
	}
#endif

	/* Handle it */
	switch (req.bRequest)
	{

	  /* This first bunch have no data phase */

	case SET_ADDRESS:
		isp1362_set_address (__le16_to_cpu(req.wValue) & 0x7F);
		isp1362_send_empty_packet (0);
		break;

	case SET_CONFIGURATION:
		if (__le16_to_cpu(req.wValue) == 1) {
			/* configured */
		} else if (__le16_to_cpu(req.wValue) == 0) {
			/* de-configured */
		} else {
			printk ("isp1362: Unknown \"set configuration\" data\n");
			isp1362_cnt_err_setup++;
		}
		isp1362_send_empty_packet (0);
		break;

	case CLEAR_FEATURE:
		if ( __le16_to_cpu(req.wValue) == 0 ) { /* clearing ENDPOINT_HALT/STALL */
			int ep = windex_to_ep_num(__le16_to_cpu(req.wValue));
			if ( ep == 1 ) {
				debug( "isp1362: Clear feature "
				       "\"endpoint halt\" on receiver\n" );
				isp1362_ep1_reset ();
			}
			else if ( ep == 2 ) {
				debug( "isp1362: Clear feature "
				       "\"endpoint halt\" on xmitter\n" );
				isp1362_ep2_reset ();
			} else {
				isp1362_cnt_err_setup++;
				printk( "isp1362: Clear feature "
					"\"endpoint halt\" "
					"on unsupported ep # %d\n", ep );
			}
		}
		else {
			isp1362_cnt_err_setup++;
			printk( "isp1362: Unsupported feature selector "
				"(%d) in clear feature. Ignored.\n",
				__le16_to_cpu(req.wValue) );
		}
		isp1362_send_empty_packet (0);
		break;

	case SET_FEATURE:
		if ( req.wValue == 0 ) { /* setting ENDPOINT_HALT/STALL */
			int ep = windex_to_ep_num( __le16_to_cpu(req.wValue) );
			if ( ep == 1 ) {
				debug( "isp1362: Set feature "
				       "\"endpoint halt\" on receiver\n" );
				isp1362_stall_endpoint (1, EP_DIR_OUT);
			}
			else if ( ep == 2 ) {
				debug( "isp1362: Set feature "
				       "\"endpoint halt\" on xmitter\n" );
				isp1362_stall_endpoint (2, EP_DIR_IN);
			} else {
				isp1362_cnt_err_setup++;
				printk( "isp1362: Set feature "
					"\"endpoint halt\" "
					"on unsupported ep # %d\n", ep );
			}
		}
		else {
			isp1362_cnt_err_setup++;
			printk( "isp1362: Unsupported feature selector "
				"(%d) in set feature\n",
				__le16_to_cpu(req.wValue) );
		}
		isp1362_send_empty_packet (0);
		break;

	  /* The rest have a data phase that writes back to the host */

	case GET_STATUS:
		isp1362_get_status (&req);
		break;

	case GET_DESCRIPTOR:
		isp1362_get_descriptor (&req);
		break;

	default :
		isp1362_cnt_err_setup++;
		printk("isp1362: Unknown request 0x%x\n", req.bRequest);
		break;
	 }

Done:
	 return;
}

  /* Endpoint 0 IN interrupt handler
   */
static void isp1362_ep0_in_int_hndlr (void)
{
	ulong status = isp1362_read_status (0, EP_DIR_IN);

	debug ("isp1362: EP0 IN status: %02X\n", (uint)status);

	if (isp1362_ep0_empty_queued && !(status & EPSTAT_EPFULL0))
	{
		isp1362_ep0_empty_queued = 0;
		isp1362_send_empty_packet(0);
	}
}

  /* Open ISP1362 usb core on behalf of a client, but don't start running
   */
int isp1362_open( const char * client )
{
	 /* create descriptors for enumeration */
	 isp1362_initialize_descriptors();

	 return 0;
}

  /* Start running. Must have called usb_open (above) first
   */
int isp1362_start( void )
{
	isp1362_write_reg16 (
		WR_DcHardwareConfiguration,
		HWCONF_CLKRUN | HWCONF_CKDIV_12MHZ);

	isp1362_write_reg32 (
		WR_DcInterruptEnable,
		INT_RST | INT_RESM | INT_SUSP | INT_EOT |
		/*INT_SOF |*/ /*INT_PSOF |*/ INT_SP_EOT |
		INT_EP0_OUT | INT_EP0_IN | (INT_EP << 1) | (INT_EP << 2));

	isp1362_read_reg32 (RD_DcInterrupt);
	isp1362_set_address(0);

	isp1362_ep1_init();
	isp1362_ep2_init();

	isp1362_write_reg16 (WR_DcMode, MODE_SOFTCT | /*MODE_DBGMOD |*/ MODE_INTENA);

	return 0;
}

  /* Stop USB core from running
   */
int isp1362_stop( void )
{
	isp1362_write_reg16 (WR_DcMode, 0);
	isp1362_write_reg32 (WR_DcInterruptEnable, 0);
	isp1362_write_reg16 (WR_DcHardwareConfiguration, 0);
	return 0;
}

  /* Tell USB core client is through using it
   */
int isp1362_close( void )
{
	 return 0;
}

#if CONFIG_PROC_FS

#define SAY( fmt, args... )  p += sprintf(p, fmt, ## args )
#define SAYV(  num )         p += sprintf(p, "%25.25s: %8.8lX\n", "Value", num )
#define SAYC( label, yn )    p += sprintf(p, "%25.25s: %s\n", label, yn )
#define SAYS( label, v )     p += sprintf(p, "%25.25s: %lu\n", label, v )

static int isp1362_read_proc(char *page, char **start, off_t off,
			    int count, int *eof, void *data)
{
	 char * p = page;
	 int len;

 	 SAY( "ISP1362 USB Controller Core\n" );

 	 SAYS( "Interrupt counter", isp1362_cnt_int );
 	 SAYS( "Reset counter", isp1362_cnt_reset );
 	 SAYS( "Resume counter", isp1362_cnt_resume );
 	 SAYS( "Suspend counter", isp1362_cnt_suspend );

 	 SAYS( "SETUP errors", isp1362_cnt_err_setup );
 	 SAYS( "IN errors", isp1362_cnt_err_in );
 	 SAYS( "OUT errors", isp1362_cnt_err_out );

	 len = ( p - page ) - off;
	 if ( len < 0 )
		  len = 0;
	 *eof = ( len <=count ) ? 1 : 0;
	 *start = page + off;
	 return len;
}

#endif  /* CONFIG_PROC_FS */

  /* ISP1362 DC interrupt handler
   */
static void isp1362_int_hndlr(int irq, void *dev_id, struct pt_regs *regs)
{
	ulong status = isp1362_read_reg32 (RD_DcInterrupt);

	isp1362_cnt_int++;

	debug ("isp1362: [%lu] DC INTERRUPT: %08X\n",
	       isp1362_cnt_int, (uint)status);

	if (status & INT_RST)
	{
		isp1362_cnt_reset++;
		isp1362_reset ();
		return;
	}

	if (status & INT_RESM)
	{
		isp1362_cnt_resume++;
		isp1362_resume ();
	}

	if (status & INT_SUSP)
	{
		isp1362_cnt_suspend++;
		isp1362_suspend ();
	}

	if (status & INT_EP0_OUT)
	{
		isp1362_ep0_out_int_hndlr ();
	}

	if (status & INT_EP0_IN)
	{
		isp1362_ep0_in_int_hndlr ();
	}

	if (status & (INT_EP << 1))
	{
		isp1362_ep1_int_hndlr ();
	}

	if (status & (INT_EP << 2))
	{
		isp1362_ep2_int_hndlr ();
	}
}

int __init isp1362_init( void )
{
	int retval = 0;
	ushort chip_id;

#if CONFIG_PROC_FS
	create_proc_read_entry (PROC_NODE_NAME, 0, NULL, isp1362_read_proc, NULL);
#endif

	isp1362_io_base = ioremap(ISP1362_IO_PHYS, PAGE_SIZE);

	chip_id = isp1362_chip_id ();
	if (chip_id != 0x3630)
	{
		printk ("isp1362: Invalid chip identification\n");
		return -1;
	}

	retval = request_8xxirq(ISP1362_IRQ_DC, isp1362_int_hndlr, 0,
				"ISP1362 DC", NULL);
	if (retval) {
		printk("isp1362: Couldn't request USB irq rc=%d\n", retval);
		return retval;
	}

	printk( "ISP1362 USB Controller Core Initialized\n");
	return 0;
}

void __exit isp1362_exit( void )
{
    printk("Unloading ISP1362 USB Controller\n");

#if CONFIG_PROC_FS
    remove_proc_entry ( PROC_NODE_NAME, NULL);
#endif

    free_irq(ISP1362_IRQ_DC, NULL);

    iounmap((void *)isp1362_io_base);
}

EXPORT_SYMBOL( isp1362_open );
EXPORT_SYMBOL( isp1362_start );
EXPORT_SYMBOL( isp1362_stop );
EXPORT_SYMBOL( isp1362_close );

EXPORT_SYMBOL( isp1362_get_descriptor_ptr );
EXPORT_SYMBOL( isp1362_set_string_descriptor );
EXPORT_SYMBOL( isp1362_get_string_descriptor );
EXPORT_SYMBOL( isp1362_kmalloc_string_descriptor );

MODULE_LICENSE("GPL");

module_init( isp1362_init );
module_exit( isp1362_exit );
