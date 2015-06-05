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

#include <stdio.h>
#include <string.h>

#include "ppctypes.h"
#include "mgt5200/sdma.h"
#include "mgt5200/mgt5200.h"

#include "dma_image.h"
#include "bestcomm_api.h"

#ifdef __MWERKS__
__declspec(section ".text") extern const uint32 taskTable;
__declspec(section ".text") extern const uint32 taskTableBytes;
__declspec(section ".text") extern const uint32 taskTableTasks;
__declspec(section ".text") extern const uint32 offsetEntry;
#else
extern const uint32 taskTable;
extern const uint32 taskTableBytes;
extern const uint32 taskTableTasks;
extern const uint32 offsetEntry;
#endif

extern uint8	*MBarGlobal;
extern sint64 MBarPhysOffsetGlobal;
extern uint32 SramOffsetGlobal;

typedef struct SCTDT {
	uint32 start;
	uint32 stop;
	uint32 var;
	uint32 fdt;
	uint32 rsvd1;
	uint32 rsvd2;
	uint32 context;
	uint32 litbase;
} SCTDT_T;


/*!
 * \brief	Load BestComm tasks into SRAM.
 * \param	sdma	Base address of the BestComm register set
 *
 * The BestComm tasks must be loaded before any task can be setup,
 * enabled, etc. This might be called as part of a boot sequence before
 * any BestComm drivers are required.
 */
void TasksLoadImage (sdma_regs *sdma)
{
	uint32 i;
	SCTDT_T *tt;

	/* copy task table from source to destination */
	memcpy ((void *)((uint8 *)(sdma->taskBar) - MBarPhysOffsetGlobal), (void *) &taskTable, (unsigned long) taskTableBytes);
	/* adjust addresses in task table */
	for (i=0; i < (uint32) taskTableTasks; i++) {
		tt = (SCTDT_T *) (((uint8 *)(sdma->taskBar) - MBarPhysOffsetGlobal) + (uint32) offsetEntry + (i * sizeof (SCTDT_T)));
		tt->start 	+= sdma->taskBar;
		tt->stop 	+= sdma->taskBar;
		tt->var		+= sdma->taskBar;
		tt->fdt		= (sdma->taskBar & 0xFFFFFF00) + tt->fdt;
		tt->context	+= sdma->taskBar;
	}
	/* initialize task variable pointers */
	init_dma_image ((uint8 *)(sdma->taskBar), MBarPhysOffsetGlobal);
	
	SramOffsetGlobal = taskTableBytes;
}
