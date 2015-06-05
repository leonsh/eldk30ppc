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
TaskId TaskSetup(TaskName_t           TaskName,
                 TaskSetupParamSet_t *TaskSetupParams)
{
   TaskId TaskNum;

   switch (TaskName) {
      case TASK_PCI_TX:
         TaskNum = TaskSetup_TASK_PCI_TX(TASK_PCI_TX_api, TaskSetupParams);
         break;
      case TASK_PCI_RX:
         TaskNum = TaskSetup_TASK_PCI_RX(TASK_PCI_RX_api, TaskSetupParams);
         break;
      case TASK_FEC_TX:
         TaskNum = TaskSetup_TASK_FEC_TX(TASK_FEC_TX_api, TaskSetupParams);
         break;
      case TASK_FEC_RX:
         TaskNum = TaskSetup_TASK_FEC_RX(TASK_FEC_RX_api, TaskSetupParams);
         break;
      case TASK_LPC:
         TaskNum = TaskSetup_TASK_LPC(TASK_LPC_api, TaskSetupParams);
         break;
      case TASK_ATA:
         TaskNum = TaskSetup_TASK_ATA(TASK_ATA_api, TaskSetupParams);
         break;
      case TASK_CRC16_DP_0:
         TaskNum = TaskSetup_TASK_CRC16_DP_0(TASK_CRC16_DP_0_api, TaskSetupParams);
         break;
      case TASK_CRC16_DP_1:
         TaskNum = TaskSetup_TASK_CRC16_DP_1(TASK_CRC16_DP_1_api, TaskSetupParams);
         break;
      case TASK_GEN_DP_0:
         TaskNum = TaskSetup_TASK_GEN_DP_0(TASK_GEN_DP_0_api, TaskSetupParams);
         break;
      case TASK_GEN_DP_1:
         TaskNum = TaskSetup_TASK_GEN_DP_1(TASK_GEN_DP_1_api, TaskSetupParams);
         break;
      case TASK_GEN_DP_2:
         TaskNum = TaskSetup_TASK_GEN_DP_2(TASK_GEN_DP_2_api, TaskSetupParams);
         break;
      case TASK_GEN_DP_3:
         TaskNum = TaskSetup_TASK_GEN_DP_3(TASK_GEN_DP_3_api, TaskSetupParams);
         break;
      case TASK_GEN_TX_BD:
         TaskNum = TaskSetup_TASK_GEN_TX_BD(TASK_GEN_TX_BD_api, TaskSetupParams);
         break;
      case TASK_GEN_RX_BD:
         TaskNum = TaskSetup_TASK_GEN_RX_BD(TASK_GEN_RX_BD_api, TaskSetupParams);
         break;
      case TASK_GEN_DP_BD_0:
         TaskNum = TaskSetup_TASK_GEN_DP_BD_0(TASK_GEN_DP_BD_0_api, TaskSetupParams);
         break;
      case TASK_GEN_DP_BD_1:
         TaskNum = TaskSetup_TASK_GEN_DP_BD_1(TASK_GEN_DP_BD_1_api, TaskSetupParams);
         break;
   }

   return TaskNum;
}

