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


#include "bestcomm_api.h"
#include "tasksetup_bdtable.h"
#include "mgt5200/mgt5200.h"

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

TaskBDIdxTable_t TaskBDIdxTable[MAX_TASKS];
extern uint8 *MBarGlobal;
extern sint64 MBarPhysOffsetGlobal;
extern uint32 SramOffsetGlobal;

static uint8 *TaskTableFree = 0;

void TaskSetup_BDTable (volatile uint32 *BasePtr, volatile uint32 *LastPtr,
                        int TaskNum, uint32 NumBD, uint16 MaxBD,
                        uint8 NumPtr, ApiConfig_t ApiConfig, uint32 Status) 
{
   int i, j;
   uint32 *ptr;


   /*
    * If another process has not used SRAM already, then the start value
    * will have to be passed in using the TasksSetSramOffset() function.
    */
   if (SramOffsetGlobal == 0) {
       SramOffsetGlobal = taskTableBytes;
   }
     
   TaskTableFree = MBarGlobal + MBAR_SRAM + SramOffsetGlobal;
       	
   /*
    * First time through the Buffer Descriptor table configuration
    * set the buffer descriptor table with parameters that will not
    * change since they are determined by the task itself.	The
    * SramOffsetGlobal variable must be updated to reflect the new SRAM
    * space used by the buffer descriptor table.  The next time through
    * this function (i.e. TaskSetup called again) the only parameters
    * that should be changed are the LastPtr pointers and the NumBD part
    * of the table.
    */
   if (TaskBDIdxTable[TaskNum].BDTablePtr == 0) {
       TaskBDIdxTable[TaskNum].BDTablePtr = TaskTableFree;
       TaskBDIdxTable[TaskNum].numPtr     = NumPtr;
       TaskBDIdxTable[TaskNum].apiConfig  = ApiConfig;

       *BasePtr = (uint32)((uint32)TaskBDIdxTable[TaskNum].BDTablePtr  + MBarPhysOffsetGlobal);

       switch (NumPtr) {
          case 1:
             SramOffsetGlobal += MaxBD*sizeof(TaskBD1_t);
             break;
          case 2:
             SramOffsetGlobal += MaxBD*sizeof(TaskBD2_t);
             break;
          default:
             /* error */
             break;
       }
   }

   TaskBDIdxTable[TaskNum].currBDInUse = 0;
   TaskBDIdxTable[TaskNum].numBD = (uint16)NumBD;
   switch (NumPtr) {
      case 1:
         *LastPtr = (uint32)(*BasePtr + sizeof(TaskBD1_t) * (NumBD - 1));
         break;
      case 2:
         *LastPtr = (uint32)(*BasePtr + sizeof(TaskBD2_t) * (NumBD - 1));
         break;
      default:
         /* error */
         break;
   }

   /*
    * Set the status bits. Clear the data pointers.
    */
   if (MaxBD > 0) {
      ptr = TaskBDIdxTable[TaskNum].BDTablePtr;
      for (i = 0; i < NumBD; i++) {
         *(ptr++) = Status;
         for (j = 0; j < NumPtr; j++) {
            *(ptr++) = 0x0;
         }
      }
   }
}
