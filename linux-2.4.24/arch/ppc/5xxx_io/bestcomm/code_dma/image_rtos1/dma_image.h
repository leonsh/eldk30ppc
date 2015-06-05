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
/* Header file for variable definitions */

#ifndef __DMA_IMAGE_H
#define __DMA_IMAGE_H 1

#include "ppctypes.h"

void init_dma_image(uint8 *vMem_taskBar, sint64 vMemOffset);

/* MBAR_TASK_TABLE is the first address of task table */
#ifndef MBAR_TASK_TABLE                    
#define MBAR_TASK_TABLE                     0xf0008000UL
#endif

/* MBAR_DMA_FREE is the first free address after task table */
#define MBAR_DMA_FREE                       MBAR_TASK_TABLE + 0x00001300UL

/* TASK_BAR is the first address of the Entry table */
#define TASK_BAR                            MBAR_TASK_TABLE + 0x00000000UL
#define TASK_BAR_OFFSET                     0x00000000UL

typedef struct task_info0 {
   volatile uint32  TaskNum;
   volatile uint32 *PtrStartTDT;
   volatile uint32 *PtrEndTDT;
   volatile uint32 *PtrVarTab;
   volatile uint32 *PtrFDT;
   volatile uint32 *PtrCSave;
   volatile uint32  NumDRD;
   volatile uint32 *DRD[7];
   volatile uint32  NumVar;
   volatile uint32 *var;
   volatile uint32  NumInc;
   volatile uint32 *inc;
   volatile uint8  *TaskPragma;
   volatile uint32 *AddrDstFIFO;
   volatile sint16 *IncrBytes;
   volatile uint32 *AddrPktSizeReg;
   volatile sint16 *IncrSrc;
   volatile uint32 *AddrSCStatusReg;
   volatile uint32 *Bytes;
   volatile uint32 *IterExtra;
   volatile uint32 *StartAddrSrc;
} TASK_PCI_TX_api_t;
extern TASK_PCI_TX_api_t *TASK_PCI_TX_api;

typedef struct task_info1 {
   volatile uint32  TaskNum;
   volatile uint32 *PtrStartTDT;
   volatile uint32 *PtrEndTDT;
   volatile uint32 *PtrVarTab;
   volatile uint32 *PtrFDT;
   volatile uint32 *PtrCSave;
   volatile uint32  NumDRD;
   volatile uint32 *DRD[5];
   volatile uint32  NumVar;
   volatile uint32 *var;
   volatile uint32  NumInc;
   volatile uint32 *inc;
   volatile uint8  *TaskPragma;
   volatile uint32 *AddrPktSizeReg;
   volatile sint16 *IncrBytes;
   volatile uint32 *AddrSrcFIFO;
   volatile sint16 *IncrDst;
   volatile uint32 *Bytes;
   volatile uint32 *IterExtra;
   volatile uint32 *StartAddrDst;
} TASK_PCI_RX_api_t;
extern TASK_PCI_RX_api_t *TASK_PCI_RX_api;

typedef struct task_info2 {
   volatile uint32  TaskNum;
   volatile uint32 *PtrStartTDT;
   volatile uint32 *PtrEndTDT;
   volatile uint32 *PtrVarTab;
   volatile uint32 *PtrFDT;
   volatile uint32 *PtrCSave;
   volatile uint32  NumDRD;
   volatile uint32 *DRD[12];
   volatile uint32  NumVar;
   volatile uint32 *var;
   volatile uint32  NumInc;
   volatile uint32 *inc;
   volatile uint8  *TaskPragma;
   volatile uint32 *AddrDRD;
   volatile uint32  AddrDRDIdx;
   volatile sint16 *IncrBytes;
   volatile uint32 *AddrDstFIFO;
   volatile sint16 *IncrSrc;
   volatile uint32 *BDTableBase;
   volatile sint16 *IncrSrcMA;
   volatile uint32 *BDTableLast;
   volatile uint32 *Bytes;
} TASK_FEC_TX_api_t;
extern TASK_FEC_TX_api_t *TASK_FEC_TX_api;

typedef struct task_info3 {
   volatile uint32  TaskNum;
   volatile uint32 *PtrStartTDT;
   volatile uint32 *PtrEndTDT;
   volatile uint32 *PtrVarTab;
   volatile uint32 *PtrFDT;
   volatile uint32 *PtrCSave;
   volatile uint32  NumDRD;
   volatile uint32 *DRD[3];
   volatile uint32  NumVar;
   volatile uint32 *var;
   volatile uint32  NumInc;
   volatile uint32 *inc;
   volatile uint8  *TaskPragma;
   volatile uint32 *AddrSrcFIFO;
   volatile sint16 *IncrBytes;
   volatile uint32 *BDTableBase;
   volatile sint16 *IncrDst;
   volatile uint32 *BDTableLast;
   volatile uint32 *Bytes;
} TASK_FEC_RX_api_t;
extern TASK_FEC_RX_api_t *TASK_FEC_RX_api;

typedef struct task_info4 {
   volatile uint32  TaskNum;
   volatile uint32 *PtrStartTDT;
   volatile uint32 *PtrEndTDT;
   volatile uint32 *PtrVarTab;
   volatile uint32 *PtrFDT;
   volatile uint32 *PtrCSave;
   volatile uint32  NumDRD;
   volatile uint32 *DRD[4];
   volatile uint32  NumVar;
   volatile uint32 *var;
   volatile uint32  NumInc;
   volatile uint32 *inc;
   volatile uint8  *TaskPragma;
   volatile uint32 *Bytes;
   volatile sint16 *IncrBytes;
   volatile uint32 *IterExtra;
   volatile sint16 *IncrDst;
   volatile sint16 *IncrDstMA;
   volatile sint16 *IncrSrc;
   volatile uint32 *StartAddrDst;
   volatile sint16 *IncrSrcMA;
   volatile uint32 *StartAddrSrc;
} TASK_LPC_api_t;
extern TASK_LPC_api_t *TASK_LPC_api;

typedef struct task_info5 {
   volatile uint32  TaskNum;
   volatile uint32 *PtrStartTDT;
   volatile uint32 *PtrEndTDT;
   volatile uint32 *PtrVarTab;
   volatile uint32 *PtrFDT;
   volatile uint32 *PtrCSave;
   volatile uint32  NumDRD;
   volatile uint32 *DRD[4];
   volatile uint32  NumVar;
   volatile uint32 *var;
   volatile uint32  NumInc;
   volatile uint32 *inc;
   volatile uint8  *TaskPragma;
   volatile uint32 *BDTableBase;
   volatile sint16 *IncrBytes;
   volatile uint32 *BDTableLast;
   volatile sint16 *IncrDst;
   volatile uint32 *Bytes;
   volatile sint16 *IncrSrc;
} TASK_ATA_api_t;
extern TASK_ATA_api_t *TASK_ATA_api;

typedef struct task_info6 {
   volatile uint32  TaskNum;
   volatile uint32 *PtrStartTDT;
   volatile uint32 *PtrEndTDT;
   volatile uint32 *PtrVarTab;
   volatile uint32 *PtrFDT;
   volatile uint32 *PtrCSave;
   volatile uint32  NumDRD;
   volatile uint32 *DRD[9];
   volatile uint32  NumVar;
   volatile uint32 *var;
   volatile uint32  NumInc;
   volatile uint32 *inc;
   volatile uint8  *TaskPragma;
   volatile uint32 *Bytes;
   volatile sint16 *IncrBytes;
   volatile uint32 *IterExtra;
   volatile sint16 *IncrDst;
   volatile sint16 *IncrDstMA;
   volatile sint16 *IncrSrc;
   volatile uint32 *StartAddrDst;
   volatile sint16 *IncrSrcMA;
   volatile uint32 *StartAddrSrc;
} TASK_CRC16_DP_0_api_t;
extern TASK_CRC16_DP_0_api_t *TASK_CRC16_DP_0_api;

typedef struct task_info7 {
   volatile uint32  TaskNum;
   volatile uint32 *PtrStartTDT;
   volatile uint32 *PtrEndTDT;
   volatile uint32 *PtrVarTab;
   volatile uint32 *PtrFDT;
   volatile uint32 *PtrCSave;
   volatile uint32  NumDRD;
   volatile uint32 *DRD[9];
   volatile uint32  NumVar;
   volatile uint32 *var;
   volatile uint32  NumInc;
   volatile uint32 *inc;
   volatile uint8  *TaskPragma;
   volatile uint32 *Bytes;
   volatile sint16 *IncrBytes;
   volatile uint32 *IterExtra;
   volatile sint16 *IncrDst;
   volatile sint16 *IncrDstMA;
   volatile sint16 *IncrSrc;
   volatile uint32 *StartAddrDst;
   volatile sint16 *IncrSrcMA;
   volatile uint32 *StartAddrSrc;
} TASK_CRC16_DP_1_api_t;
extern TASK_CRC16_DP_1_api_t *TASK_CRC16_DP_1_api;

typedef struct task_info8 {
   volatile uint32  TaskNum;
   volatile uint32 *PtrStartTDT;
   volatile uint32 *PtrEndTDT;
   volatile uint32 *PtrVarTab;
   volatile uint32 *PtrFDT;
   volatile uint32 *PtrCSave;
   volatile uint32  NumDRD;
   volatile uint32 *DRD[4];
   volatile uint32  NumVar;
   volatile uint32 *var;
   volatile uint32  NumInc;
   volatile uint32 *inc;
   volatile uint8  *TaskPragma;
   volatile uint32 *Bytes;
   volatile sint16 *IncrBytes;
   volatile uint32 *IterExtra;
   volatile sint16 *IncrDst;
   volatile sint16 *IncrDstMA;
   volatile sint16 *IncrSrc;
   volatile uint32 *StartAddrDst;
   volatile sint16 *IncrSrcMA;
   volatile uint32 *StartAddrSrc;
} TASK_GEN_DP_0_api_t;
extern TASK_GEN_DP_0_api_t *TASK_GEN_DP_0_api;

typedef struct task_info9 {
   volatile uint32  TaskNum;
   volatile uint32 *PtrStartTDT;
   volatile uint32 *PtrEndTDT;
   volatile uint32 *PtrVarTab;
   volatile uint32 *PtrFDT;
   volatile uint32 *PtrCSave;
   volatile uint32  NumDRD;
   volatile uint32 *DRD[4];
   volatile uint32  NumVar;
   volatile uint32 *var;
   volatile uint32  NumInc;
   volatile uint32 *inc;
   volatile uint8  *TaskPragma;
   volatile uint32 *Bytes;
   volatile sint16 *IncrBytes;
   volatile uint32 *IterExtra;
   volatile sint16 *IncrDst;
   volatile sint16 *IncrDstMA;
   volatile sint16 *IncrSrc;
   volatile uint32 *StartAddrDst;
   volatile sint16 *IncrSrcMA;
   volatile uint32 *StartAddrSrc;
} TASK_GEN_DP_1_api_t;
extern TASK_GEN_DP_1_api_t *TASK_GEN_DP_1_api;

typedef struct task_info10 {
   volatile uint32  TaskNum;
   volatile uint32 *PtrStartTDT;
   volatile uint32 *PtrEndTDT;
   volatile uint32 *PtrVarTab;
   volatile uint32 *PtrFDT;
   volatile uint32 *PtrCSave;
   volatile uint32  NumDRD;
   volatile uint32 *DRD[4];
   volatile uint32  NumVar;
   volatile uint32 *var;
   volatile uint32  NumInc;
   volatile uint32 *inc;
   volatile uint8  *TaskPragma;
   volatile uint32 *Bytes;
   volatile sint16 *IncrBytes;
   volatile uint32 *IterExtra;
   volatile sint16 *IncrDst;
   volatile sint16 *IncrDstMA;
   volatile sint16 *IncrSrc;
   volatile uint32 *StartAddrDst;
   volatile sint16 *IncrSrcMA;
   volatile uint32 *StartAddrSrc;
} TASK_GEN_DP_2_api_t;
extern TASK_GEN_DP_2_api_t *TASK_GEN_DP_2_api;

typedef struct task_info11 {
   volatile uint32  TaskNum;
   volatile uint32 *PtrStartTDT;
   volatile uint32 *PtrEndTDT;
   volatile uint32 *PtrVarTab;
   volatile uint32 *PtrFDT;
   volatile uint32 *PtrCSave;
   volatile uint32  NumDRD;
   volatile uint32 *DRD[4];
   volatile uint32  NumVar;
   volatile uint32 *var;
   volatile uint32  NumInc;
   volatile uint32 *inc;
   volatile uint8  *TaskPragma;
   volatile uint32 *Bytes;
   volatile sint16 *IncrBytes;
   volatile uint32 *IterExtra;
   volatile sint16 *IncrDst;
   volatile sint16 *IncrDstMA;
   volatile sint16 *IncrSrc;
   volatile uint32 *StartAddrDst;
   volatile sint16 *IncrSrcMA;
   volatile uint32 *StartAddrSrc;
} TASK_GEN_DP_3_api_t;
extern TASK_GEN_DP_3_api_t *TASK_GEN_DP_3_api;

typedef struct task_info12 {
   volatile uint32  TaskNum;
   volatile uint32 *PtrStartTDT;
   volatile uint32 *PtrEndTDT;
   volatile uint32 *PtrVarTab;
   volatile uint32 *PtrFDT;
   volatile uint32 *PtrCSave;
   volatile uint32  NumDRD;
   volatile uint32 *DRD[5];
   volatile uint32  NumVar;
   volatile uint32 *var;
   volatile uint32  NumInc;
   volatile uint32 *inc;
   volatile uint8  *TaskPragma;
   volatile uint32 *AddrDstFIFO;
   volatile sint16 *IncrBytes;
   volatile uint32 *BDTableBase;
   volatile sint16 *IncrSrc;
   volatile uint32 *BDTableLast;
   volatile sint16 *IncrSrcMA;
   volatile uint32 *Bytes;
} TASK_GEN_TX_BD_api_t;
extern TASK_GEN_TX_BD_api_t *TASK_GEN_TX_BD_api;

typedef struct task_info13 {
   volatile uint32  TaskNum;
   volatile uint32 *PtrStartTDT;
   volatile uint32 *PtrEndTDT;
   volatile uint32 *PtrVarTab;
   volatile uint32 *PtrFDT;
   volatile uint32 *PtrCSave;
   volatile uint32  NumDRD;
   volatile uint32 *DRD[4];
   volatile uint32  NumVar;
   volatile uint32 *var;
   volatile uint32  NumInc;
   volatile uint32 *inc;
   volatile uint8  *TaskPragma;
   volatile uint32 *AddrSrcFIFO;
   volatile sint16 *IncrBytes;
   volatile uint32 *BDTableBase;
   volatile sint16 *IncrDst;
   volatile uint32 *BDTableLast;
   volatile uint32 *Bytes;
} TASK_GEN_RX_BD_api_t;
extern TASK_GEN_RX_BD_api_t *TASK_GEN_RX_BD_api;

typedef struct task_info14 {
   volatile uint32  TaskNum;
   volatile uint32 *PtrStartTDT;
   volatile uint32 *PtrEndTDT;
   volatile uint32 *PtrVarTab;
   volatile uint32 *PtrFDT;
   volatile uint32 *PtrCSave;
   volatile uint32  NumDRD;
   volatile uint32 *DRD[4];
   volatile uint32  NumVar;
   volatile uint32 *var;
   volatile uint32  NumInc;
   volatile uint32 *inc;
   volatile uint8  *TaskPragma;
   volatile uint32 *BDTableBase;
   volatile sint16 *IncrBytes;
   volatile uint32 *BDTableLast;
   volatile sint16 *IncrDst;
   volatile uint32 *Bytes;
   volatile sint16 *IncrSrc;
} TASK_GEN_DP_BD_0_api_t;
extern TASK_GEN_DP_BD_0_api_t *TASK_GEN_DP_BD_0_api;

typedef struct task_info15 {
   volatile uint32  TaskNum;
   volatile uint32 *PtrStartTDT;
   volatile uint32 *PtrEndTDT;
   volatile uint32 *PtrVarTab;
   volatile uint32 *PtrFDT;
   volatile uint32 *PtrCSave;
   volatile uint32  NumDRD;
   volatile uint32 *DRD[4];
   volatile uint32  NumVar;
   volatile uint32 *var;
   volatile uint32  NumInc;
   volatile uint32 *inc;
   volatile uint8  *TaskPragma;
   volatile uint32 *BDTableBase;
   volatile sint16 *IncrBytes;
   volatile uint32 *BDTableLast;
   volatile sint16 *IncrDst;
   volatile uint32 *Bytes;
   volatile sint16 *IncrSrc;
} TASK_GEN_DP_BD_1_api_t;
extern TASK_GEN_DP_BD_1_api_t *TASK_GEN_DP_BD_1_api;


#endif
