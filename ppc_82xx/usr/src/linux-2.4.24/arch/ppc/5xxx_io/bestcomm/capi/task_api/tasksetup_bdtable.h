#ifndef __TASKSETUP_BDTABLE_H
#define __TASKSETUP_BDTABLE_H 1
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
* ******************************************************************************/


/* only need this to pick up MBAR_DMA_FREE define */
#include "dma_image.h"

/*
 * Table of BD rings for all BestComm tasks indexed by task ID.
 *
 *    +-----+------+--------------+    +------+-------+
 * 0: |numBD|numPtr|BDTablePtr ---|--->|status|dataPtr|
 *    +-----+------+--------------+    +------+-------+
 * 1: |numBD|numPtr|BDTablePtr    |    |status|dataPtr|
 *    +-----+------+--------------+    .      .       .
 * 2: |numBD|numPtr|BDTablePtr ---|-+  .      .       .
 *    .            .              . |  .      .       .
 *    .            .              . |  |status|dataPtr|
 *    .            .              . |  +------+-------+
 * 15:|numBD|numPtr|BDTablePtr    | |
 *    +-----+------+--------------+ |
 *                                  |
 *                                  V
 *                                  +------+--------+--------+
 *                                  |status|dataPtr0|dataPtr1|
 *                                  +------+--------+--------+
 *                                  |status|dataPtr0|dataPtr1|
 *                                  .      .        .        .
 *                                  .      .        .        .
 *                                  .      .        .        .
 *                                  |status|dataPtr0|dataPtr1|
 *                                  +------+--------+--------+
 */
typedef struct {
	uint16 numBD;		/* Size of BD ring									*/
	uint8  numPtr;		/* Number of data buffer pointers per BD			*/
	uint8  apiConfig;	/* API configuration flags							*/
	void   *BDTablePtr;	/* Pointer to BD tables, must be cast to TaskBD1_t	*/
						/*   or TaskBD2_t									*/
	uint16 currBDInUse; /* Current number of buffer descriptors assigned but*/
						/* not released yet.                                */					
} TaskBDIdxTable_t;

typedef enum {
	API_CONFIG_NONE		= 0x00,
	API_CONFIG_BD_FLAG	= 0x01
} ApiConfig_t;

extern TaskBDIdxTable_t TaskBDIdxTable[MAX_TASKS];

/*
 * Allocates BD table if needed and updates the BD index table.
 * Do we want to hide this from the C API since it operates on task API?
 */
void TaskSetup_BDTable (volatile uint32 *BasePtr, 
                        volatile uint32 *LastPtr,
                        int TaskNum, uint32 NumBD, uint16 MaxBD,
                        uint8 NumPtr, ApiConfig_t ApiConfig, uint32 Status );

#endif	/* __TASKSETUP_BDTABLE_H */
