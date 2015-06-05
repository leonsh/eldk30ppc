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


/*
 * Generated by GUI, one per configured tasks.
 */

#define TASK_BASE         TASK_GEN_TX_BD
#define TASK_API          TASK_GEN_TX_BD_api_t
#define TASKSETUP_NAME    TaskSetup_TASK_GEN_TX_BD

/* Pragma settings */
#define PRECISE_INC       0
#define NO_ERROR_RST      0
#define PACK_DATA         0
#define INTEGER_MODE      0
#define SPEC_READS        1
#define WRITE_LINE_BUFFER 1
#define READ_LINE_BUFFER  1

#define MISALIGNED        1

#define INITIATOR_DATA   -1

#define AUTO_START       -1
#define ITERATIONS       -1

#define MAX_BD            25
#define BD_FLAG           0

#define INCR_TYPE_SRC     1
#define INCR_SRC          0
#define TYPE_SRC          FLEX_T

#define INCR_TYPE_DST     0
#define INCR_DST          0
#define TYPE_DST          FLEX_T

#include "tasksetup_general.c"

