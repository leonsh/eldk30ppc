#ifndef __BESTCOMM_API_H
#define __BESTCOMM_API_H 1

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

#include "ppctypes.h"
#include "mgt5200/sdma.h"

#define MAX_TASKS 16

/*
 * This may need to be removed in certain implementations.
 */
#ifndef NULL
# define NULL ((void *)0)
#endif	/* NULL */

typedef sint8 TaskId;
typedef sint8 BDIdx;

/*
 * Special "task IDs" for interrupt handling API functions
 */
#define	DEBUG_INTR_ID	SDMA_INT_BIT_DBG	/*!< Debug interrupt "task ID"	*/
#define TEA_INTR_ID		SDMA_INT_BIT_TEA	/*!< TEA interrupt "task ID"	*/

#define TASK_AUTOSTART_ENABLE	1 /*!< Task start autostart enable */
#define TASK_AUTOSTART_DISABLE	0 /*!< Task start autostart disable */
#define TASK_INTERRUPT_ENABLE	1 /*!< Task start interrupt enable */
#define TASK_INTERRUPT_DISABLE	0 /*!< Task start interrupt disable */

/*!
 * \brief	Data transfer size
 */
typedef enum {
	SZ_FLEX		= 3,	/*!< invalid for TaskSetupParamSet_t */
	SZ_UINT8	= 1,	/*!< 1-byte	*/
	SZ_UINT16	= 2,	/*!< 2-byte	*/
	SZ_UINT32	= 4	/*!< 4-byte	*/
} Sz_t;

/*!
 * \brief	API error codes
 */
typedef enum {
	TASK_ERR_NO_ERR			= -1,	/*!< No error					*/
	TASK_ERR_NO_INTR		= TASK_ERR_NO_ERR,
									/*!< No interrupt				*/
	TASK_ERR_INVALID_ARG	= -2,	/*!< Invalid function argument	*/
	TASK_ERR_BD_RING_FULL	= -3,	/*!< Buffer descriptor ring full*/
	TASK_ERR_API_ALREADY_INITIALIZED = -4, /*!< API has already been initialized */
	TASK_ERR_SIZE_TOO_LARGE = -5, /*!< Buffer descriptor cannot support size parameter */
	TASK_ERR_BD_RING_EMPTY	= -6	/*!< Buffer descriptor ring is empty*/

} TaskErr_t;

/*!
 * \brief	BestComm initiators
 *
 * These are assigned by TaskSetup().
 */
typedef enum {

	INITIATOR_ALWAYS	=  0,
	INITIATOR_SCTMR_0	=  1,
	INITIATOR_SCTMR_1	=  2,
	INITIATOR_FEC_RX	=  3,
	INITIATOR_FEC_TX	=  4,
	INITIATOR_ATA_RX	=  5,
	INITIATOR_ATA_TX	=  6,
	INITIATOR_SCPCI_RX	=  7,
	INITIATOR_SCPCI_TX	=  8,
	INITIATOR_PSC3_RX	=  9,
	INITIATOR_PSC3_TX	= 10,
	INITIATOR_PSC2_RX	= 11,
	INITIATOR_PSC2_TX	= 12,
	INITIATOR_PSC1_RX	= 13,
	INITIATOR_PSC1_TX	= 14,
	INITIATOR_SCTMR_2	= 15,

	INITIATOR_SCLPC		= 16,
	INITIATOR_PSC5_RX	= 17,
	INITIATOR_PSC5_TX	= 18,
	INITIATOR_PSC4_RX	= 19,
	INITIATOR_PSC4_TX	= 20,
	INITIATOR_I2C2_RX	= 21,
	INITIATOR_I2C2_TX	= 22,
	INITIATOR_I2C1_RX	= 23,
	INITIATOR_I2C1_TX	= 24,
	INITIATOR_PSC6_RX	= 25,
	INITIATOR_PSC6_TX	= 26,
	INITIATOR_IRDA_RX	= 25,
	INITIATOR_IRDA_TX	= 26,
	INITIATOR_SCTMR_3	= 27,
	INITIATOR_SCTMR_4	= 28,
	INITIATOR_SCTMR_5	= 29,
	INITIATOR_SCTMR_6	= 30,
	INITIATOR_SCTMR_7	= 31

} MPC5200Initiator_t;

/*!
 * \brief	Parameters for TaskSetup()
 *
 * All parameters can be hard-coded by the task API. Hard-coded values
 * will be changed in the struct passed to TaskSetup() for the user to
 * examine later.
 */
typedef struct {
	uint32	NumBD;			/*!< Number of buffer descriptors				*/

	union {
	   uint32 MaxBuf;		/*!< Maximum buffer size						*/
	   uint32 NumBytes;		/*!< Number of bytes to transfer				*/
	} Size;					/*!< Buffer size union for BD and non-BD tasks	*/

	MPC5200Initiator_t
			Initiator;		/*!< BestComm initiator (ignored if hard-wired)	*/
	uint32	StartAddrSrc;	/*!< Address of the DMA source (e.g. a FIFO)	*/
	sint16	IncrSrc;		/*!< Amount to increment source pointer			*/
	Sz_t	SzSrc;			/*!< Size of source data access					*/
	uint32	StartAddrDst;	/*!< Address of the DMA destination (e.g. a FIFO) */
	sint16	IncrDst;		/*!< Amount to increment data pointer			*/
	Sz_t	SzDst;			/*!< Size of destination data access			*/
} TaskSetupParamSet_t;

/*!
 * \brief	Parameters for TaskDebug()
 *
 * TaskDebug() and the contents of this data structure are yet to be
 * determined.
 */
typedef struct {
	int dummy;				/* CodeWarrior no likey empty struct typedefs? */
} TaskDebugParamSet_t;

/*!
 * \brief	Generic buffer descriptor.
 *
 * It is generally used as a pointer which should be cast to one of the
 * other BD types based on the number of buffers per descriptor.
 */
typedef struct {
	uint32 Status;			/*!< Status and length bits		*/
} TaskBD_t;

/*!
 * \brief	Single buffer descriptor.
 */
typedef struct {
	uint32 Status;			/*!< Status and length bits		*/
	uint32 DataPtr[1];		/*!< Pointer to data buffer		*/
} TaskBD1_t;

/*!
 * \brief	Dual buffer descriptor.
 */
typedef struct {
	uint32 Status;			/*!< Status and length bits		*/
	uint32 DataPtr[2];		/*!< Pointer to data buffers	*/
} TaskBD2_t;

/*
 * Ethernet task buffer descriptor flags to pass to TaskTransferStart().
 */
#define TASK_ETH_BD_TFD	0x08000000	/*!< Ethernet transmit frame done		*/
#define TASK_ETH_BD_INT	0x04000000	/*!< Ethernet interrupt on frame done	*/


/***************************
 * Start of API Prototypes
 ***************************/

#include "tasksetup.h"

int			TasksInitAPI( uint8 *MBarRef);

int			TasksInitAPI_VM( uint8 *MBarRef, uint8 *MBarPhys );

void		TasksLoadImage(sdma_regs *sdma );
int			TasksAttachImage(sdma_regs *sdma);

uint32		TasksGetSramOffset(void);
void		TasksSetSramOffset(uint32 sram_offset);

TaskId		TaskSetup( TaskName_t           TaskName,
					   TaskSetupParamSet_t *TaskParams );

int			TaskStart( TaskId taskId, uint32 autoStartEnable, TaskId autoStartTask,
						uint32 intrEnable );
int			TaskStop( TaskId taskId );
int			TaskStatus( TaskId taskId );
BDIdx		TaskBDAssign( TaskId taskId, void *buffer0, void *buffer1,
						  int size, uint32 bdFlags );
BDIdx		TaskBDRelease( TaskId taskId );
TaskBD_t	*TaskGetBD( TaskId taskId, BDIdx bd );
TaskBD_t	*TaskGetBDRing( TaskId taskId );
int			TaskDebug( TaskId taskId, TaskDebugParamSet_t *paramSet );
int			TaskIntClear( TaskId taskId );
TaskId		TaskIntStatus( TaskId taskId );
int			TaskIntPending( TaskId taskId );
TaskId		TaskIntSource( void );

#endif	/* __BESTCOMM_API_H */
