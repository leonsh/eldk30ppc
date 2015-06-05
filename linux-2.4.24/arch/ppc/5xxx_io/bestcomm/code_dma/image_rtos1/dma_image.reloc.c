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

uint32 taskTableBytes = 0x00001300;  /* Number of bytes in image */

uint32 taskTableTasks = 0x00000010;  /* Number of tasks in image */

uint32 offsetEntry = 0x00000000;  /* Offset to Entry section */

uint32 taskTable[] = {

/* SmartDMA image contains 4864 bytes (411 bytes unused) */

/* Task0(TASK_PCI_TX): Start of Entry -> 0xf0008000 */
0x00000200,  /* Task 0 Descriptor Table */
0x0000022c,
0x00000500,  /* Task 0 Variable Table */
0x00000d07,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x00000e00,  /* Task 0 context save space */
0x00000000, 
/* Task1(TASK_PCI_RX): Start of Entry -> 0xf0008020 */
0x00000230,  /* Task 0 Descriptor Table */
0x00000250,
0x00000580,  /* Task 0 Variable Table */
0x00000007, /* No FDT */
0x00000000, 
0x00000000, 
0x00000e50,  /* Task 0 context save space */
0x00000000, 
/* Task2(TASK_FEC_TX): Start of Entry -> 0xf0008040 */
0x00000254,  /* Task 0 Descriptor Table */
0x00000298,
0x00000600,  /* Task 0 Variable Table */
0x00000d07,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x00000ea0,  /* Task 0 context save space */
0x00000000, 
/* Task3(TASK_FEC_RX): Start of Entry -> 0xf0008060 */
0x0000029c,  /* Task 0 Descriptor Table */
0x000002bc,
0x00000680,  /* Task 0 Variable Table */
0x00000007, /* No FDT */
0x00000000, 
0x00000000, 
0x00000ef0,  /* Task 0 context save space */
0x00000000, 
/* Task4(TASK_LPC): Start of Entry -> 0xf0008080 */
0x000002c0,  /* Task 0 Descriptor Table */
0x000002e8,
0x00000700,  /* Task 0 Variable Table */
0x00000007, /* No FDT */
0x00000000, 
0x00000000, 
0x00000f40,  /* Task 0 context save space */
0x00000000, 
/* Task5(TASK_ATA): Start of Entry -> 0xf00080a0 */
0x000002ec,  /* Task 0 Descriptor Table */
0x0000030c,
0x00000780,  /* Task 0 Variable Table */
0x00000d07,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x00000f90,  /* Task 0 context save space */
0x00000000, 
/* Task6(TASK_CRC16_DP_0): Start of Entry -> 0xf00080c0 */
0x00000310,  /* Task 0 Descriptor Table */
0x0000034c,
0x00000800,  /* Task 0 Variable Table */
0x00000d07,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x00000fe0,  /* Task 0 context save space */
0x00000000, 
/* Task7(TASK_CRC16_DP_1): Start of Entry -> 0xf00080e0 */
0x00000350,  /* Task 0 Descriptor Table */
0x0000038c,
0x00000880,  /* Task 0 Variable Table */
0x00000d07,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x00001030,  /* Task 0 context save space */
0x00000000, 
/* Task8(TASK_GEN_DP_0): Start of Entry -> 0xf0008100 */
0x00000390,  /* Task 0 Descriptor Table */
0x000003b8,
0x00000900,  /* Task 0 Variable Table */
0x00000007, /* No FDT */
0x00000000, 
0x00000000, 
0x00001080,  /* Task 0 context save space */
0x00000000, 
/* Task9(TASK_GEN_DP_1): Start of Entry -> 0xf0008120 */
0x000003bc,  /* Task 0 Descriptor Table */
0x000003e4,
0x00000980,  /* Task 0 Variable Table */
0x00000007, /* No FDT */
0x00000000, 
0x00000000, 
0x000010d0,  /* Task 0 context save space */
0x00000000, 
/* Task10(TASK_GEN_DP_2): Start of Entry -> 0xf0008140 */
0x000003e8,  /* Task 0 Descriptor Table */
0x00000410,
0x00000a00,  /* Task 0 Variable Table */
0x00000007, /* No FDT */
0x00000000, 
0x00000000, 
0x00001120,  /* Task 0 context save space */
0x00000000, 
/* Task11(TASK_GEN_DP_3): Start of Entry -> 0xf0008160 */
0x00000414,  /* Task 0 Descriptor Table */
0x0000043c,
0x00000a80,  /* Task 0 Variable Table */
0x00000007, /* No FDT */
0x00000000, 
0x00000000, 
0x00001170,  /* Task 0 context save space */
0x00000000, 
/* Task12(TASK_GEN_TX_BD): Start of Entry -> 0xf0008180 */
0x00000440,  /* Task 0 Descriptor Table */
0x00000464,
0x00000b00,  /* Task 0 Variable Table */
0x00000d07,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x000011c0,  /* Task 0 context save space */
0x00000000, 
/* Task13(TASK_GEN_RX_BD): Start of Entry -> 0xf00081a0 */
0x00000468,  /* Task 0 Descriptor Table */
0x00000484,
0x00000b80,  /* Task 0 Variable Table */
0x00000d07,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x00001210,  /* Task 0 context save space */
0x00000000, 
/* Task14(TASK_GEN_DP_BD_0): Start of Entry -> 0xf00081c0 */
0x00000488,  /* Task 0 Descriptor Table */
0x000004a8,
0x00000c00,  /* Task 0 Variable Table */
0x00000d07,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x00001260,  /* Task 0 context save space */
0x00000000, 
/* Task15(TASK_GEN_DP_BD_1): Start of Entry -> 0xf00081e0 */
0x000004ac,  /* Task 0 Descriptor Table */
0x000004cc,
0x00000c80,  /* Task 0 Variable Table */
0x00000d07,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x000012b0,  /* Task 0 context save space */
0x00000000, 

/* Task0(TASK_PCI_TX): Start of TDT -> 0xf0008200 */
0xc080601b, /* 0000(../LIB_incl/hdplx.sc:167):  LCDEXT: idx0 = var1, idx1 = var0; ; idx0 += inc3, idx1 += inc3 */
0x82190292, /* 0004(../LIB_incl/hdplx.sc:177):  LCD: idx2 = var4; idx2 >= var10; idx2 += inc2 */
0x1004c018, /* 0008(../LIB_incl/hdplx.sc:179):    DRD1A: *idx0 = var3; FN=0 MORE init=0 WS=2 RS=0 */
0x8381a288, /* 000C(../LIB_incl/hdplx.sc:183):    LCD: idx3 = var7, idx4 = var3; idx4 > var10; idx3 += inc1, idx4 += inc0 */
0x011ec798, /* 0010(../LIB_incl/hdplx.sc:200):      DRD1A: *idx1 = *idx3; FN=0 init=8 WS=3 RS=3 */
0x10001f18, /* 0014(../LIB_incl/hdplx.sc:253):    DRD1A: var7 = idx3; FN=0 MORE init=0 WS=0 RS=0 */
0x850102db, /* 0018(../LIB_incl/hdplx.sc:263):    LCD: idx3 = var10, idx4 = var2; idx3 != var11; idx3 += inc3, idx4 += inc3 */
0x60080002, /* 001C(../LIB_incl/hdplx.sc:263):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=2 EXT init=0 WS=0 RS=1 */
0x08ccfd0b, /* 0020(../LIB_incl/hdplx.sc:263):      DRD2B1: idx3 = EU3(); EU3(*idx4,var11)  */
0x0002d058, /* 0024(../LIB_incl/hdplx.sc:263):    DRD1A: *idx4 = var11; FN=0 init=0 WS=1 RS=0 */
0x80180024, /* 0028(../LIB_incl/hdplx.sc:267):  LCD: idx0 = var0; idx0 once var0; idx0 += inc4 */
0x040001f8, /* 002C(../LIB_incl/hdplx.sc:267):    DRD1A: FN=0 INT init=0 WS=0 RS=0 */
/* Task1(TASK_PCI_RX): Start of TDT -> 0xf0008230 */
0xc000e01b, /* 0000(../LIB_incl/hdplx.sc:167):  LCDEXT: idx0 = var0, idx1 = var1; ; idx0 += inc3, idx1 += inc3 */
0x81990212, /* 0004(../LIB_incl/hdplx.sc:177):  LCD: idx2 = var3; idx2 >= var8; idx2 += inc2 */
0x1004c010, /* 0008(../LIB_incl/hdplx.sc:179):    DRD1A: *idx0 = var2; FN=0 MORE init=0 WS=2 RS=0 */
0x83012208, /* 000C(../LIB_incl/hdplx.sc:186):    LCD: idx3 = var6, idx4 = var2; idx4 > var8; idx3 += inc1, idx4 += inc0 */
0x00fecf88, /* 0010(../LIB_incl/hdplx.sc:200):      DRD1A: *idx3 = *idx1; FN=0 init=7 WS=3 RS=3 */
0x10001b18, /* 0014(../LIB_incl/hdplx.sc:258):    DRD1A: var6 = idx3; FN=0 MORE init=0 WS=0 RS=0 */
0x040001f8, /* 0018(../LIB_incl/hdplx.sc:263):    DRD1A: FN=0 INT init=0 WS=0 RS=0 */
0x8018001b, /* 001C(../LIB_incl/hdplx.sc:267):  LCD: idx0 = var0; idx0 once var0; idx0 += inc3 */
0x040001f8, /* 0020(../LIB_incl/hdplx.sc:267):    DRD1A: FN=0 INT init=0 WS=0 RS=0 */
/* Task2(TASK_FEC_TX): Start of TDT -> 0xf0008254 */
0x8018001b, /* 0000(../LIB_incl/bd_hdplx.sc:283):  LCD: idx0 = var0; idx0 <= var0; idx0 += inc3 */
0x60000005, /* 0004(../LIB_incl/bd_hdplx.sc:287):    DRD2A: EU0=0 EU1=0 EU2=0 EU3=5 EXT init=0 WS=0 RS=0 */
0x014cfc09, /* 0008(../LIB_incl/bd_hdplx.sc:287):    DRD2B1: var5 = EU3(); EU3(*idx0,var9)  */
0x808120e3, /* 000C(../LIB_incl/bd_hdplx.sc:296):  LCD: idx0 = var1, idx1 = var2; idx1 <= var3; idx0 += inc4, idx1 += inc3 */
0x850002e4, /* 0010(../LIB_incl/bd_hdplx.sc:309):    LCD: idx2 = var10, idx3 = var0; idx2 < var11; idx2 += inc4, idx3 += inc4 */
0x60800002, /* 0014(../LIB_incl/bd_hdplx.sc:315):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=2 EXT init=4 WS=0 RS=0 */
0x088cfc4c, /* 0018(../LIB_incl/bd_hdplx.sc:315):      DRD2B1: idx2 = EU3(); EU3(*idx1,var12)  */
0x70000002, /* 001C(../LIB_incl/bd_hdplx.sc:321):    DRD2A: EU0=0 EU1=0 EU2=0 EU3=2 EXT MORE init=0 WS=0 RS=0 */
0x01ccfc49, /* 0020(../LIB_incl/bd_hdplx.sc:321):    DRD2B1: var7 = EU3(); EU3(*idx1,var9)  */
0x70000003, /* 0024(../LIB_incl/bd_hdplx.sc:322):    DRD2A: EU0=0 EU1=0 EU2=0 EU3=3 EXT MORE init=0 WS=0 RS=0 */
0x0cccf1c5, /* 0028(../LIB_incl/bd_hdplx.sc:322):    DRD2B1: *idx3 = EU3(); EU3(var7,var5)  */
0xd9190340, /* 002C(../LIB_incl/bd_hdplx.sc:329):    LCDEXT: idx2 = idx2; idx2 > var13; idx2 += inc0 */
0xb8c56009, /* 0030(../LIB_incl/bd_hdplx.sc:329):    LCD: idx3 = *(idx1 + var00000014); ; idx3 += inc1 */
0x009ec398, /* 0034(../LIB_incl/bd_hdplx.sc:345):      DRD1A: *idx0 = *idx3; FN=0 init=4 WS=3 RS=3 */
0x991982ea, /* 0038(../LIB_incl/bd_hdplx.sc:370):    LCD: idx2 = idx2, idx3 = idx3; idx2 > var11; idx2 += inc5, idx3 += inc2 */
0x088ac398, /* 003C(../LIB_incl/bd_hdplx.sc:387):      DRD1A: *idx0 = *idx3; FN=0 TFD init=4 WS=1 RS=1 */
0x60000005, /* 0040(../LIB_incl/bd_hdplx.sc:400):    DRD2A: EU0=0 EU1=0 EU2=0 EU3=5 EXT init=0 WS=0 RS=0 */
0x0c4cf88b, /* 0044(../LIB_incl/bd_hdplx.sc:400):    DRD2B1: *idx1 = EU3(); EU3(idx2,var11)  */
/* Task3(TASK_FEC_RX): Start of TDT -> 0xf000829c */
0x8000a09a, /* 0000(../LIB_incl/bd_hdplx.sc:293):  LCD: idx0 = var0, idx1 = var1; idx1 <= var2; idx0 += inc3, idx1 += inc2 */
0x831901db, /* 0004(../LIB_incl/bd_hdplx.sc:311):    LCD: idx2 = var6; idx2 < var7; idx2 += inc3 */
0x00608b88, /* 0008(../LIB_incl/bd_hdplx.sc:317):      DRD1A: idx2 = *idx1; FN=0 init=3 WS=0 RS=0 */
0xd91901c0, /* 000C(../LIB_incl/bd_hdplx.sc:332):    LCDEXT: idx2 = idx2; idx2 > var7; idx2 += inc0 */
0xb8c36009, /* 0010(../LIB_incl/bd_hdplx.sc:332):    LCD: idx3 = *(idx1 + var0000000a); ; idx3 += inc1 */
0x047ecf80, /* 0014(../LIB_incl/bd_hdplx.sc:345):      DRD1A: *idx3 = *idx0; FN=0 INT init=3 WS=3 RS=3 */
0x98190024, /* 0018(../LIB_incl/bd_hdplx.sc:397):    LCD: idx2 = idx0; idx2 once var0; idx2 += inc4 */
0x0060c790, /* 001C(../LIB_incl/bd_hdplx.sc:398):      DRD1A: *idx1 = *idx2; FN=0 init=3 WS=0 RS=0 */
0x000001f8, /* 0020(:0):    NOP */
/* Task4(TASK_LPC): Start of TDT -> 0xf00082c0 */
0x809801ed, /* 0000(../LIB_incl/hdplx.sc:177):  LCD: idx0 = var1; idx0 >= var7; idx0 += inc5 */
0xc2826019, /* 0004(../LIB_incl/hdplx.sc:183):    LCDEXT: idx1 = var5, idx2 = var4; ; idx1 += inc3, idx2 += inc1 */
0x80198200, /* 0008(../LIB_incl/hdplx.sc:188):    LCD: idx3 = var0; idx3 > var8; idx3 += inc0 */
0x03fecb88, /* 000C(../LIB_incl/hdplx.sc:200):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8990019, /* 0010(../LIB_incl/hdplx.sc:208):    LCDEXT: idx1 = idx1, idx2 = idx2; idx1 once var0; idx1 += inc3, idx2 += inc1 */
0x9999e000, /* 0014(../LIB_incl/hdplx.sc:211):    LCD: idx3 = idx3; ; idx3 += inc0 */
0x03fecb88, /* 0018(../LIB_incl/hdplx.sc:218):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8996022, /* 001C(../LIB_incl/hdplx.sc:224):    LCDEXT: idx1 = idx1, idx2 = idx2; ; idx1 += inc4, idx2 += inc2 */
0x999981f6, /* 0020(../LIB_incl/hdplx.sc:229):    LCD: idx3 = idx3; idx3 > var7; idx3 += inc6 */
0x0beacb88, /* 0024(../LIB_incl/hdplx.sc:241):      DRD1A: *idx2 = *idx1; FN=0 TFD init=31 WS=1 RS=1 */
0x040001f8, /* 0028(../LIB_incl/hdplx.sc:263):    DRD1A: FN=0 INT init=0 WS=0 RS=0 */
/* Task5(TASK_ATA): Start of TDT -> 0xf00082ec */
0x8018005b, /* 0000(../LIB_incl/bd_hdplx.sc:301):  LCD: idx0 = var0; idx0 <= var1; idx0 += inc3 */
0x831881e4, /* 0004(../LIB_incl/bd_hdplx.sc:311):    LCD: idx1 = var6; idx1 < var7; idx1 += inc4 */
0x03e08780, /* 0008(../LIB_incl/bd_hdplx.sc:317):      DRD1A: idx1 = *idx0; FN=0 init=31 WS=0 RS=0 */
0xd89881c0, /* 000C(../LIB_incl/bd_hdplx.sc:329):    LCDEXT: idx1 = idx1; idx1 > var7; idx1 += inc0 */
0xf8436011, /* 0010(../LIB_incl/bd_hdplx.sc:329):    LCDEXT: idx2 = *(idx0 + var0000000a); ; idx2 += inc2 */
0xb843600a, /* 0014(../LIB_incl/bd_hdplx.sc:332):    LCD: idx3 = *(idx0 + var0000000e); ; idx3 += inc1 */
0x0bfecf90, /* 0018(../LIB_incl/bd_hdplx.sc:345):      DRD1A: *idx3 = *idx2; FN=0 TFD init=31 WS=3 RS=3 */
0x64000005, /* 001C(../LIB_incl/bd_hdplx.sc:400):    DRD2A: EU0=0 EU1=0 EU2=0 EU3=5 INT EXT init=0 WS=0 RS=0 */
0x0c0cf847, /* 0020(../LIB_incl/bd_hdplx.sc:400):    DRD2B1: *idx0 = EU3(); EU3(idx1,var7)  */
/* Task6(TASK_CRC16_DP_0): Start of TDT -> 0xf0008310 */
0x809801ed, /* 0000(../LIB_incl/hdplx.sc:177):  LCD: idx0 = var1; idx0 >= var7; idx0 += inc5 */
0x70000000, /* 0004(../LIB_incl/hdplx.sc:179):    DRD2A: EU0=0 EU1=0 EU2=0 EU3=0 EXT MORE init=0 WS=0 RS=0 */
0x2c87c7df, /* 0008(../LIB_incl/hdplx.sc:179):    DRD2B2: EU3(var8)  */
0xc2826019, /* 000C(../LIB_incl/hdplx.sc:183):    LCDEXT: idx1 = var5, idx2 = var4; ; idx1 += inc3, idx2 += inc1 */
0x80198240, /* 0010(../LIB_incl/hdplx.sc:188):    LCD: idx3 = var0; idx3 > var9; idx3 += inc0 */
0x63fe000c, /* 0014(../LIB_incl/hdplx.sc:200):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=12 EXT init=31 WS=3 RS=3 */
0x0c8cfc5f, /* 0018(../LIB_incl/hdplx.sc:200):      DRD2B1: *idx2 = EU3(); EU3(*idx1)  */
0xd8990019, /* 001C(../LIB_incl/hdplx.sc:208):    LCDEXT: idx1 = idx1, idx2 = idx2; idx1 once var0; idx1 += inc3, idx2 += inc1 */
0x9999e000, /* 0020(../LIB_incl/hdplx.sc:211):    LCD: idx3 = idx3; ; idx3 += inc0 */
0x63fe000c, /* 0024(../LIB_incl/hdplx.sc:218):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=12 EXT init=31 WS=3 RS=3 */
0x0c8cfc5f, /* 0028(../LIB_incl/hdplx.sc:218):      DRD2B1: *idx2 = EU3(); EU3(*idx1)  */
0xd8996022, /* 002C(../LIB_incl/hdplx.sc:224):    LCDEXT: idx1 = idx1, idx2 = idx2; ; idx1 += inc4, idx2 += inc2 */
0x999981f6, /* 0030(../LIB_incl/hdplx.sc:229):    LCD: idx3 = idx3; idx3 > var7; idx3 += inc6 */
0x6bea000c, /* 0034(../LIB_incl/hdplx.sc:241):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=12 TFD EXT init=31 WS=1 RS=1 */
0x0c8cfc5f, /* 0038(../LIB_incl/hdplx.sc:241):      DRD2B1: *idx2 = EU3(); EU3(*idx1)  */
0x0404c999, /* 003C(../LIB_incl/hdplx.sc:263):    DRD1A: *idx2 = EU3(); FN=1 INT init=0 WS=2 RS=0 */
/* Task7(TASK_CRC16_DP_1): Start of TDT -> 0xf0008350 */
0x809801ed, /* 0000(../LIB_incl/hdplx.sc:177):  LCD: idx0 = var1; idx0 >= var7; idx0 += inc5 */
0x70000000, /* 0004(../LIB_incl/hdplx.sc:179):    DRD2A: EU0=0 EU1=0 EU2=0 EU3=0 EXT MORE init=0 WS=0 RS=0 */
0x2c87c7df, /* 0008(../LIB_incl/hdplx.sc:179):    DRD2B2: EU3(var8)  */
0xc2826019, /* 000C(../LIB_incl/hdplx.sc:183):    LCDEXT: idx1 = var5, idx2 = var4; ; idx1 += inc3, idx2 += inc1 */
0x80198240, /* 0010(../LIB_incl/hdplx.sc:188):    LCD: idx3 = var0; idx3 > var9; idx3 += inc0 */
0x63fe000c, /* 0014(../LIB_incl/hdplx.sc:200):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=12 EXT init=31 WS=3 RS=3 */
0x0c8cfc5f, /* 0018(../LIB_incl/hdplx.sc:200):      DRD2B1: *idx2 = EU3(); EU3(*idx1)  */
0xd8990019, /* 001C(../LIB_incl/hdplx.sc:208):    LCDEXT: idx1 = idx1, idx2 = idx2; idx1 once var0; idx1 += inc3, idx2 += inc1 */
0x9999e000, /* 0020(../LIB_incl/hdplx.sc:211):    LCD: idx3 = idx3; ; idx3 += inc0 */
0x63fe000c, /* 0024(../LIB_incl/hdplx.sc:218):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=12 EXT init=31 WS=3 RS=3 */
0x0c8cfc5f, /* 0028(../LIB_incl/hdplx.sc:218):      DRD2B1: *idx2 = EU3(); EU3(*idx1)  */
0xd8996022, /* 002C(../LIB_incl/hdplx.sc:224):    LCDEXT: idx1 = idx1, idx2 = idx2; ; idx1 += inc4, idx2 += inc2 */
0x999981f6, /* 0030(../LIB_incl/hdplx.sc:229):    LCD: idx3 = idx3; idx3 > var7; idx3 += inc6 */
0x6bea000c, /* 0034(../LIB_incl/hdplx.sc:241):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=12 TFD EXT init=31 WS=1 RS=1 */
0x0c8cfc5f, /* 0038(../LIB_incl/hdplx.sc:241):      DRD2B1: *idx2 = EU3(); EU3(*idx1)  */
0x0404c999, /* 003C(../LIB_incl/hdplx.sc:263):    DRD1A: *idx2 = EU3(); FN=1 INT init=0 WS=2 RS=0 */
/* Task8(TASK_GEN_DP_0): Start of TDT -> 0xf0008390 */
0x809801ed, /* 0000(../LIB_incl/hdplx.sc:177):  LCD: idx0 = var1; idx0 >= var7; idx0 += inc5 */
0xc2826019, /* 0004(../LIB_incl/hdplx.sc:183):    LCDEXT: idx1 = var5, idx2 = var4; ; idx1 += inc3, idx2 += inc1 */
0x80198200, /* 0008(../LIB_incl/hdplx.sc:188):    LCD: idx3 = var0; idx3 > var8; idx3 += inc0 */
0x03fecb88, /* 000C(../LIB_incl/hdplx.sc:200):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8990019, /* 0010(../LIB_incl/hdplx.sc:208):    LCDEXT: idx1 = idx1, idx2 = idx2; idx1 once var0; idx1 += inc3, idx2 += inc1 */
0x9999e000, /* 0014(../LIB_incl/hdplx.sc:211):    LCD: idx3 = idx3; ; idx3 += inc0 */
0x03fecb88, /* 0018(../LIB_incl/hdplx.sc:218):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8996022, /* 001C(../LIB_incl/hdplx.sc:224):    LCDEXT: idx1 = idx1, idx2 = idx2; ; idx1 += inc4, idx2 += inc2 */
0x999981f6, /* 0020(../LIB_incl/hdplx.sc:229):    LCD: idx3 = idx3; idx3 > var7; idx3 += inc6 */
0x0beacb88, /* 0024(../LIB_incl/hdplx.sc:241):      DRD1A: *idx2 = *idx1; FN=0 TFD init=31 WS=1 RS=1 */
0x040001f8, /* 0028(../LIB_incl/hdplx.sc:263):    DRD1A: FN=0 INT init=0 WS=0 RS=0 */
/* Task9(TASK_GEN_DP_1): Start of TDT -> 0xf00083bc */
0x809801ed, /* 0000(../LIB_incl/hdplx.sc:177):  LCD: idx0 = var1; idx0 >= var7; idx0 += inc5 */
0xc2826019, /* 0004(../LIB_incl/hdplx.sc:183):    LCDEXT: idx1 = var5, idx2 = var4; ; idx1 += inc3, idx2 += inc1 */
0x80198200, /* 0008(../LIB_incl/hdplx.sc:188):    LCD: idx3 = var0; idx3 > var8; idx3 += inc0 */
0x03fecb88, /* 000C(../LIB_incl/hdplx.sc:200):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8990019, /* 0010(../LIB_incl/hdplx.sc:208):    LCDEXT: idx1 = idx1, idx2 = idx2; idx1 once var0; idx1 += inc3, idx2 += inc1 */
0x9999e000, /* 0014(../LIB_incl/hdplx.sc:211):    LCD: idx3 = idx3; ; idx3 += inc0 */
0x03fecb88, /* 0018(../LIB_incl/hdplx.sc:218):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8996022, /* 001C(../LIB_incl/hdplx.sc:224):    LCDEXT: idx1 = idx1, idx2 = idx2; ; idx1 += inc4, idx2 += inc2 */
0x999981f6, /* 0020(../LIB_incl/hdplx.sc:229):    LCD: idx3 = idx3; idx3 > var7; idx3 += inc6 */
0x0beacb88, /* 0024(../LIB_incl/hdplx.sc:241):      DRD1A: *idx2 = *idx1; FN=0 TFD init=31 WS=1 RS=1 */
0x040001f8, /* 0028(../LIB_incl/hdplx.sc:263):    DRD1A: FN=0 INT init=0 WS=0 RS=0 */
/* Task10(TASK_GEN_DP_2): Start of TDT -> 0xf00083e8 */
0x809801ed, /* 0000(../LIB_incl/hdplx.sc:177):  LCD: idx0 = var1; idx0 >= var7; idx0 += inc5 */
0xc2826019, /* 0004(../LIB_incl/hdplx.sc:183):    LCDEXT: idx1 = var5, idx2 = var4; ; idx1 += inc3, idx2 += inc1 */
0x80198200, /* 0008(../LIB_incl/hdplx.sc:188):    LCD: idx3 = var0; idx3 > var8; idx3 += inc0 */
0x03fecb88, /* 000C(../LIB_incl/hdplx.sc:200):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8990019, /* 0010(../LIB_incl/hdplx.sc:208):    LCDEXT: idx1 = idx1, idx2 = idx2; idx1 once var0; idx1 += inc3, idx2 += inc1 */
0x9999e000, /* 0014(../LIB_incl/hdplx.sc:211):    LCD: idx3 = idx3; ; idx3 += inc0 */
0x03fecb88, /* 0018(../LIB_incl/hdplx.sc:218):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8996022, /* 001C(../LIB_incl/hdplx.sc:224):    LCDEXT: idx1 = idx1, idx2 = idx2; ; idx1 += inc4, idx2 += inc2 */
0x999981f6, /* 0020(../LIB_incl/hdplx.sc:229):    LCD: idx3 = idx3; idx3 > var7; idx3 += inc6 */
0x0beacb88, /* 0024(../LIB_incl/hdplx.sc:241):      DRD1A: *idx2 = *idx1; FN=0 TFD init=31 WS=1 RS=1 */
0x040001f8, /* 0028(../LIB_incl/hdplx.sc:263):    DRD1A: FN=0 INT init=0 WS=0 RS=0 */
/* Task11(TASK_GEN_DP_3): Start of TDT -> 0xf0008414 */
0x809801ed, /* 0000(../LIB_incl/hdplx.sc:177):  LCD: idx0 = var1; idx0 >= var7; idx0 += inc5 */
0xc2826019, /* 0004(../LIB_incl/hdplx.sc:183):    LCDEXT: idx1 = var5, idx2 = var4; ; idx1 += inc3, idx2 += inc1 */
0x80198200, /* 0008(../LIB_incl/hdplx.sc:188):    LCD: idx3 = var0; idx3 > var8; idx3 += inc0 */
0x03fecb88, /* 000C(../LIB_incl/hdplx.sc:200):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8990019, /* 0010(../LIB_incl/hdplx.sc:208):    LCDEXT: idx1 = idx1, idx2 = idx2; idx1 once var0; idx1 += inc3, idx2 += inc1 */
0x9999e000, /* 0014(../LIB_incl/hdplx.sc:211):    LCD: idx3 = idx3; ; idx3 += inc0 */
0x03fecb88, /* 0018(../LIB_incl/hdplx.sc:218):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8996022, /* 001C(../LIB_incl/hdplx.sc:224):    LCDEXT: idx1 = idx1, idx2 = idx2; ; idx1 += inc4, idx2 += inc2 */
0x999981f6, /* 0020(../LIB_incl/hdplx.sc:229):    LCD: idx3 = idx3; idx3 > var7; idx3 += inc6 */
0x0beacb88, /* 0024(../LIB_incl/hdplx.sc:241):      DRD1A: *idx2 = *idx1; FN=0 TFD init=31 WS=1 RS=1 */
0x040001f8, /* 0028(../LIB_incl/hdplx.sc:263):    DRD1A: FN=0 INT init=0 WS=0 RS=0 */
/* Task12(TASK_GEN_TX_BD): Start of TDT -> 0xf0008440 */
0x8000a0a3, /* 0000(../LIB_incl/bd_hdplx.sc:296):  LCD: idx0 = var0, idx1 = var1; idx1 <= var2; idx0 += inc4, idx1 += inc3 */
0x831901e4, /* 0004(../LIB_incl/bd_hdplx.sc:311):    LCD: idx2 = var6; idx2 < var7; idx2 += inc4 */
0x01c08b88, /* 0008(../LIB_incl/bd_hdplx.sc:317):      DRD1A: idx2 = *idx1; FN=0 init=14 WS=0 RS=0 */
0xd9190200, /* 000C(../LIB_incl/bd_hdplx.sc:329):    LCDEXT: idx2 = idx2; idx2 > var8; idx2 += inc0 */
0xb8c36009, /* 0010(../LIB_incl/bd_hdplx.sc:329):    LCD: idx3 = *(idx1 + var0000000a); ; idx3 += inc1 */
0x01dec398, /* 0014(../LIB_incl/bd_hdplx.sc:345):      DRD1A: *idx0 = *idx3; FN=0 init=14 WS=3 RS=3 */
0x991981ea, /* 0018(../LIB_incl/bd_hdplx.sc:370):    LCD: idx2 = idx2, idx3 = idx3; idx2 > var7; idx2 += inc5, idx3 += inc2 */
0x0dcac398, /* 001C(../LIB_incl/bd_hdplx.sc:387):      DRD1A: *idx0 = *idx3; FN=0 TFD INT init=14 WS=1 RS=1 */
0x60000005, /* 0020(../LIB_incl/bd_hdplx.sc:400):    DRD2A: EU0=0 EU1=0 EU2=0 EU3=5 EXT init=0 WS=0 RS=0 */
0x0c4cf887, /* 0024(../LIB_incl/bd_hdplx.sc:400):    DRD2B1: *idx1 = EU3(); EU3(idx2,var7)  */
/* Task13(TASK_GEN_RX_BD): Start of TDT -> 0xf0008468 */
0x8000a09a, /* 0000(../LIB_incl/bd_hdplx.sc:293):  LCD: idx0 = var0, idx1 = var1; idx1 <= var2; idx0 += inc3, idx1 += inc2 */
0x831901db, /* 0004(../LIB_incl/bd_hdplx.sc:311):    LCD: idx2 = var6; idx2 < var7; idx2 += inc3 */
0x03e08b88, /* 0008(../LIB_incl/bd_hdplx.sc:317):      DRD1A: idx2 = *idx1; FN=0 init=31 WS=0 RS=0 */
0xd91901c0, /* 000C(../LIB_incl/bd_hdplx.sc:332):    LCDEXT: idx2 = idx2; idx2 > var7; idx2 += inc0 */
0xb8c36009, /* 0010(../LIB_incl/bd_hdplx.sc:332):    LCD: idx3 = *(idx1 + var0000000a); ; idx3 += inc1 */
0x07fecf80, /* 0014(../LIB_incl/bd_hdplx.sc:345):      DRD1A: *idx3 = *idx0; FN=0 INT init=31 WS=3 RS=3 */
0x60000005, /* 0018(../LIB_incl/bd_hdplx.sc:400):    DRD2A: EU0=0 EU1=0 EU2=0 EU3=5 EXT init=0 WS=0 RS=0 */
0x0c4cf887, /* 001C(../LIB_incl/bd_hdplx.sc:400):    DRD2B1: *idx1 = EU3(); EU3(idx2,var7)  */
/* Task14(TASK_GEN_DP_BD_0): Start of TDT -> 0xf0008488 */
0x8018005b, /* 0000(../LIB_incl/bd_hdplx.sc:301):  LCD: idx0 = var0; idx0 <= var1; idx0 += inc3 */
0x831881e4, /* 0004(../LIB_incl/bd_hdplx.sc:311):    LCD: idx1 = var6; idx1 < var7; idx1 += inc4 */
0x03e08780, /* 0008(../LIB_incl/bd_hdplx.sc:317):      DRD1A: idx1 = *idx0; FN=0 init=31 WS=0 RS=0 */
0xd89881c0, /* 000C(../LIB_incl/bd_hdplx.sc:329):    LCDEXT: idx1 = idx1; idx1 > var7; idx1 += inc0 */
0xf8436011, /* 0010(../LIB_incl/bd_hdplx.sc:329):    LCDEXT: idx2 = *(idx0 + var0000000a); ; idx2 += inc2 */
0xb843600a, /* 0014(../LIB_incl/bd_hdplx.sc:332):    LCD: idx3 = *(idx0 + var0000000e); ; idx3 += inc1 */
0x0bfecf90, /* 0018(../LIB_incl/bd_hdplx.sc:345):      DRD1A: *idx3 = *idx2; FN=0 TFD init=31 WS=3 RS=3 */
0x64000005, /* 001C(../LIB_incl/bd_hdplx.sc:400):    DRD2A: EU0=0 EU1=0 EU2=0 EU3=5 INT EXT init=0 WS=0 RS=0 */
0x0c0cf847, /* 0020(../LIB_incl/bd_hdplx.sc:400):    DRD2B1: *idx0 = EU3(); EU3(idx1,var7)  */
/* Task15(TASK_GEN_DP_BD_1): Start of TDT -> 0xf00084ac */
0x8018005b, /* 0000(../LIB_incl/bd_hdplx.sc:301):  LCD: idx0 = var0; idx0 <= var1; idx0 += inc3 */
0x831881e4, /* 0004(../LIB_incl/bd_hdplx.sc:311):    LCD: idx1 = var6; idx1 < var7; idx1 += inc4 */
0x03e08780, /* 0008(../LIB_incl/bd_hdplx.sc:317):      DRD1A: idx1 = *idx0; FN=0 init=31 WS=0 RS=0 */
0xd89881c0, /* 000C(../LIB_incl/bd_hdplx.sc:329):    LCDEXT: idx1 = idx1; idx1 > var7; idx1 += inc0 */
0xf8436011, /* 0010(../LIB_incl/bd_hdplx.sc:329):    LCDEXT: idx2 = *(idx0 + var0000000a); ; idx2 += inc2 */
0xb843600a, /* 0014(../LIB_incl/bd_hdplx.sc:332):    LCD: idx3 = *(idx0 + var0000000e); ; idx3 += inc1 */
0x0bfecf90, /* 0018(../LIB_incl/bd_hdplx.sc:345):      DRD1A: *idx3 = *idx2; FN=0 TFD init=31 WS=3 RS=3 */
0x64000005, /* 001C(../LIB_incl/bd_hdplx.sc:400):    DRD2A: EU0=0 EU1=0 EU2=0 EU3=5 INT EXT init=0 WS=0 RS=0 */
0x0c0cf847, /* 0020(../LIB_incl/bd_hdplx.sc:400):    DRD2B1: *idx0 = EU3(); EU3(idx1,var7)  */

0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
/* Task0(TASK_PCI_TX): Start of VarTab -> 0xf0008500 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000000, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000001, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xc000ffff, /* inc[2] */
0x60000000, /* inc[3] */
0x00000000, /* inc[4] */
0x00000000, /* inc[5] */
0x00000000, /* inc[6] */
0x00000000, /* inc[7] */
/* Task1(TASK_PCI_RX): Start of VarTab -> 0xf0008580 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000000, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xc000ffff, /* inc[2] */
0x00000000, /* inc[3] */
0x00000000, /* inc[4] */
0x00000000, /* inc[5] */
0x00000000, /* inc[6] */
0x00000000, /* inc[7] */
/* Task2(TASK_FEC_TX): Start of VarTab -> 0xf0008600 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000000, /* var[8] */
0x0c000000, /* var[9] */
0x00000000, /* var[10] */
0x40000000, /* var[11] */
0x43ffffff, /* var[12] */
0x40000004, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0xa0000008, /* inc[3] */
0x20000000, /* inc[4] */
0x4000ffff, /* inc[5] */
0x00000000, /* inc[6] */
0x00000000, /* inc[7] */
/* Task3(TASK_FEC_RX): Start of VarTab -> 0xf0008680 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x40000000, /* var[7] */
0x00000000, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xa0000008, /* inc[2] */
0x20000000, /* inc[3] */
0x00000000, /* inc[4] */
0x00000000, /* inc[5] */
0x00000000, /* inc[6] */
0x00000000, /* inc[7] */
/* Task4(TASK_LPC): Start of VarTab -> 0xf0008700 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000008, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0x00000000, /* inc[3] */
0xe0000000, /* inc[4] */
0xc000ffff, /* inc[5] */
0x4000ffff, /* inc[6] */
0x00000000, /* inc[7] */
/* Task5(TASK_ATA): Start of VarTab -> 0xf0008780 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x40000000, /* var[7] */
0x00000000, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0xa000000c, /* inc[3] */
0x20000000, /* inc[4] */
0x00000000, /* inc[5] */
0x00000000, /* inc[6] */
0x00000000, /* inc[7] */
/* Task6(TASK_CRC16_DP_0): Start of VarTab -> 0xf0008800 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000001, /* var[8] */
0x00000008, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0x00000000, /* inc[3] */
0xe0000000, /* inc[4] */
0xc000ffff, /* inc[5] */
0x4000ffff, /* inc[6] */
0x00000000, /* inc[7] */
/* Task7(TASK_CRC16_DP_1): Start of VarTab -> 0xf0008880 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000001, /* var[8] */
0x00000008, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0x00000000, /* inc[3] */
0xe0000000, /* inc[4] */
0xc000ffff, /* inc[5] */
0x4000ffff, /* inc[6] */
0x00000000, /* inc[7] */
/* Task8(TASK_GEN_DP_0): Start of VarTab -> 0xf0008900 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000008, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0x00000000, /* inc[3] */
0xe0000000, /* inc[4] */
0xc000ffff, /* inc[5] */
0x4000ffff, /* inc[6] */
0x00000000, /* inc[7] */
/* Task9(TASK_GEN_DP_1): Start of VarTab -> 0xf0008980 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000008, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0x00000000, /* inc[3] */
0xe0000000, /* inc[4] */
0xc000ffff, /* inc[5] */
0x4000ffff, /* inc[6] */
0x00000000, /* inc[7] */
/* Task10(TASK_GEN_DP_2): Start of VarTab -> 0xf0008a00 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000008, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0x00000000, /* inc[3] */
0xe0000000, /* inc[4] */
0xc000ffff, /* inc[5] */
0x4000ffff, /* inc[6] */
0x00000000, /* inc[7] */
/* Task11(TASK_GEN_DP_3): Start of VarTab -> 0xf0008a80 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000008, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0x00000000, /* inc[3] */
0xe0000000, /* inc[4] */
0xc000ffff, /* inc[5] */
0x4000ffff, /* inc[6] */
0x00000000, /* inc[7] */
/* Task12(TASK_GEN_TX_BD): Start of VarTab -> 0xf0008b00 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x40000000, /* var[7] */
0x40000004, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0xa0000008, /* inc[3] */
0x20000000, /* inc[4] */
0x4000ffff, /* inc[5] */
0x00000000, /* inc[6] */
0x00000000, /* inc[7] */
/* Task13(TASK_GEN_RX_BD): Start of VarTab -> 0xf0008b80 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x40000000, /* var[7] */
0x00000000, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xa0000008, /* inc[2] */
0x20000000, /* inc[3] */
0x00000000, /* inc[4] */
0x00000000, /* inc[5] */
0x00000000, /* inc[6] */
0x00000000, /* inc[7] */
/* Task14(TASK_GEN_DP_BD_0): Start of VarTab -> 0xf0008c00 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x40000000, /* var[7] */
0x00000000, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0xa000000c, /* inc[3] */
0x20000000, /* inc[4] */
0x00000000, /* inc[5] */
0x00000000, /* inc[6] */
0x00000000, /* inc[7] */
/* Task15(TASK_GEN_DP_BD_1): Start of VarTab -> 0xf0008c80 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x40000000, /* var[7] */
0x00000000, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0xa000000c, /* inc[3] */
0x20000000, /* inc[4] */
0x00000000, /* inc[5] */
0x00000000, /* inc[6] */
0x00000000, /* inc[7] */

/* Task0(TASK_PCI_TX): Start of FDT -> 0xf0008d00 */
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0xa0045670, /* load_acc(), EU# 3 */
0x80045670, /* unload_acc(), EU# 3 */
0x21800000, /* and(), EU# 3 */
0x21e00000, /* or(), EU# 3 */
0x21500000, /* xor(), EU# 3 */
0x21400000, /* andn(), EU# 3 */
0x21500000, /* not(), EU# 3 */
0x20400000, /* add(), EU# 3 */
0x20500000, /* sub(), EU# 3 */
0x20800000, /* lsh(), EU# 3 */
0x20a00000, /* rsh(), EU# 3 */
0xc0170000, /* crc8(), EU# 3 */
0xc0145670, /* crc16(), EU# 3 */
0xc0345670, /* crc32(), EU# 3 */
0xa0076540, /* endian32(), EU# 3 */
0xa0000760, /* endian16(), EU# 3 */

/* Task0(TASK_PCI_TX): Start of CSave -> 0xf0008e00 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task1(TASK_PCI_RX): Start of CSave -> 0xf0008e50 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task2(TASK_FEC_TX): Start of CSave -> 0xf0008ea0 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task3(TASK_FEC_RX): Start of CSave -> 0xf0008ef0 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task4(TASK_LPC): Start of CSave -> 0xf0008f40 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task5(TASK_ATA): Start of CSave -> 0xf0008f90 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task6(TASK_CRC16_DP_0): Start of CSave -> 0xf0008fe0 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task7(TASK_CRC16_DP_1): Start of CSave -> 0xf0009030 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task8(TASK_GEN_DP_0): Start of CSave -> 0xf0009080 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task9(TASK_GEN_DP_1): Start of CSave -> 0xf00090d0 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task10(TASK_GEN_DP_2): Start of CSave -> 0xf0009120 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task11(TASK_GEN_DP_3): Start of CSave -> 0xf0009170 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task12(TASK_GEN_TX_BD): Start of CSave -> 0xf00091c0 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task13(TASK_GEN_RX_BD): Start of CSave -> 0xf0009210 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task14(TASK_GEN_DP_BD_0): Start of CSave -> 0xf0009260 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task15(TASK_GEN_DP_BD_1): Start of CSave -> 0xf00092b0 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Start of free code space -> f0009300 */

};
