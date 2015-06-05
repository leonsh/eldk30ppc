/******************************************************************************
*
* Copyright (C) 2003  Motorola, Inc.
* 
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
* 
*
* Filename:     $Source$
* Author:       $Author$
* Locker:       $Locker$
* State:        $State$
* Revision:     $Revision$
*
* Functions:    
*
* History:      Use the RCS command rlog to display revision history
*               information.
*
* Description:  
*
* Notes:                
*******************************************************************************/

/*
 * Would it be better to use typedefs so that we control the 
 * size of data written?  However bestcomm will not work off
 * these typedefs and it makes parsing the file more difficult
 * for the GUI.
 *
 * Need to change mgt5200.h to define MBAR_? to be an absolute
 * rather than an offset of MBAR, which should be called
 * MBAR_IPBI, or change the name to something like 
 * MBAR_OFFSET_?.
 *
 * dma_image.h/c have a dependency on sdma->, is it time to get
 * rid of it?
 */
#ifndef __BESTCOMM_API_MEM
#define __BESTCOMM_API_MEM 1

#include "mgt5200/mgt5200.h"
//#define MBarGlobal 0xf0000000

/*
 * An extern global variable is used here for the MBAR since it must 
 * be passed into the API for processes that use virtual memory.
 */
extern uint8 *MBarGlobal;

#define SDMA_TASK_BAR      (MBarGlobal+MBAR_SDMA+0x000)
#define SDMA_INT_PEND      (MBarGlobal+MBAR_SDMA+0x014)
#define SDMA_INT_MASK      (MBarGlobal+MBAR_SDMA+0x018)
#define SDMA_TCR           (MBarGlobal+MBAR_SDMA+0x01C)
#define SDMA_TASK_SIZE     (MBarGlobal+MBAR_SDMA+0x060)

#define PCI_TX_PKT_SIZE    (MBarGlobal+MBAR_SCPCI+0x000)
#define PCI_TX_NTBIT       (MBarGlobal+MBAR_SCPCI+0x01C)
#define PCI_TX_FIFO        (MBarGlobal+MBAR_SCPCI+0x040)
#define PCI_TX_FIFO_STAT   (MBarGlobal+MBAR_SCPCI+0x045)
#define PCI_TX_FIFO_GRAN   (MBarGlobal+MBAR_SCPCI+0x048)
#define PCI_TX_FIFO_ALARM  (MBarGlobal+MBAR_SCPCI+0x04E)

#define PCI_RX_PKT_SIZE    (MBarGlobal+MBAR_SCPCI+0x080)
#define PCI_RX_NTBIT       (MBarGlobal+MBAR_SCPCI+0x09C)
#define PCI_RX_FIFO        (MBarGlobal+MBAR_SCPCI+0x0C0)
#define PCI_RX_FIFO_STAT   (MBarGlobal+MBAR_SCPCI+0x0C5)
#define PCI_RX_FIFO_GRAN   (MBarGlobal+MBAR_SCPCI+0x0C8)
#define PCI_RX_FIFO_ALARM  (MBarGlobal+MBAR_SCPCI+0x0CE)


#define FEC_RX_FIFO        (MBarGlobal+MBAR_ETHERNET+0x184)
#define FEC_RX_FIFO_STAT   (MBarGlobal+MBAR_ETHERNET+0x188)
#define FEC_RX_FIFO_GRAN   (MBarGlobal+MBAR_ETHERNET+0x18C)
#define FEC_RX_FIFO_ALARM  (MBarGlobal+MBAR_ETHERNET+0x198)

#define FEC_TX_FIFO        (MBarGlobal+MBAR_ETHERNET+0x1A4)
#define FEC_TX_FIFO_STAT   (MBarGlobal+MBAR_ETHERNET+0x1A8)
#define FEC_TX_FIFO_GRAN   (MBarGlobal+MBAR_ETHERNET+0x1AC)
#define FEC_TX_FIFO_ALARM  (MBarGlobal+MBAR_ETHERNET+0x1B8)

#endif
