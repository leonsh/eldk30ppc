/*

Copyright (C) 2003  Motorola, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/* sas/sccg, gas target */
.section        smartdmaInitData,"aw",@progbits	/* Initialized data for task variables */
.section        smartdmaTaskTable,"aw",@progbits	/* Task tables */
.align  9
.globl taskTable
taskTable:
.globl scPsc3Recv_Entry
scPsc3Recv_Entry:		/* Task 0 */
.long   scPsc3Recv_TDT	/* Task 0 Descriptor Table */
.long   scPsc3Recv_TDT + 0x00000124
.long   scPsc3Recv_VarTab	/* Task 0 Variable Table */
.long   scPsc3Recv_FDT + 0x03	/* Task 0 Function Descriptor Table & Flags */
.long   0x00000000
.long   0x00000000
.long   scPsc3Recv_CSave	/* Task 0 context save space */
.long   0x00000000
.globl scPsc3Xmit_Entry
scPsc3Xmit_Entry:		/* Task 1 */
.long   scPsc3Xmit_TDT	/* Task 1 Descriptor Table */
.long   scPsc3Xmit_TDT + 0x00000088
.long   scPsc3Xmit_VarTab	/* Task 1 Variable Table */
.long   scPsc3Xmit_FDT + 0x03	/* Task 1 Function Descriptor Table & Flags */
.long   0x00000000
.long   0x00000000
.long   scPsc3Xmit_CSave	/* Task 1 context save space */
.long   0x00000000
.globl scEthernetRecv_Entry
scEthernetRecv_Entry:		/* Task 2 */
.long   scEthernetRecv_TDT	/* Task 2 Descriptor Table */
.long   scEthernetRecv_TDT + 0x000000b4
.long   scEthernetRecv_VarTab	/* Task 2 Variable Table */
.long   scEthernetRecv_FDT + 0x03	/* Task 2 Function Descriptor Table & Flags */
.long   0x00000000
.long   0x00000000
.long   scEthernetRecv_CSave	/* Task 2 context save space */
.long   0x00f00000
.globl scEthernetXmit_Entry
scEthernetXmit_Entry:		/* Task 3 */
.long   scEthernetXmit_TDT	/* Task 3 Descriptor Table */
.long   scEthernetXmit_TDT + 0x000000dc
.long   scEthernetXmit_VarTab	/* Task 3 Variable Table */
.long   scEthernetXmit_FDT + 0x03	/* Task 3 Function Descriptor Table & Flags */
.long   0x00000000
.long   0x00000000
.long   scEthernetXmit_CSave	/* Task 3 context save space */
.long   0x00f00000
.globl scAC97Rx_Entry
scAC97Rx_Entry:		/* Task 4 */
.long   scAC97Rx_TDT	/* Task 4 Descriptor Table */
.long   scAC97Rx_TDT + 0x0000001c
.long   scAC97Rx_VarTab	/* Task 4 Variable Table */
.long   scAC97Rx_FDT + 0x03	/* Task 4 Function Descriptor Table & Flags */
.long   0x00000000
.long   0x00000000
.long   scAC97Rx_CSave	/* Task 4 context save space */
.long   0xf0000000
.globl scAC97Tx_Entry
scAC97Tx_Entry:		/* Task 5 */
.long   scAC97Tx_TDT	/* Task 5 Descriptor Table */
.long   scAC97Tx_TDT + 0x0000001c
.long   scAC97Tx_VarTab	/* Task 5 Variable Table */
.long   scAC97Tx_FDT + 0x03	/* Task 5 Function Descriptor Table & Flags */
.long   0x00000000
.long   0x00000000
.long   scAC97Tx_CSave	/* Task 5 context save space */
.long   0xf0000000
.globl ATArw_Entry
ATArw_Entry:		/* Task 6 */
.long   ATArw_TDT	/* Task 6 Descriptor Table */
.long   ATArw_TDT + 0x000000f8
.long   ATArw_VarTab	/* Task 6 Variable Table */
.long   ATArw_FDT + 0x03	/* Task 6 Function Descriptor Table & Flags */
.long   0x00000000
.long   0x00000000
.long   ATArw_CSave	/* Task 6 context save space */
.long   0xf0000000


.globl scPsc3Recv_TDT
scPsc3Recv_TDT:	/* Task 0 Descriptor Table */
.long   0x81980000
.long   0x00000380
.long   0xc0186000	
.long   0x82c48004	
.long   0x70000000	
.long   0x004c5c0a	
.long   0x60000000	
.long   0x020c5c0b	
.long   0x80980249	
.long   0xc2c66000	
.long   0xe044e011	
.long   0x86998092	
.long   0x112acb88	
.long   0x00001b18	
.long   0x80188000	
.long   0x70000000	
.long   0x0c4c5c4e	
.long   0x60000200	
.long   0x0c4c5c46	
.long   0x82c48004	
.long   0x00001f88	
.long   0x83988000	
.long   0x60000000	
.long   0x018c584a	
.long   0x83000258	
.long   0x70000200	
.long   0x0c8c5c8f	
.long   0x00008448	
.long   0x83988000	
.long   0x60000000	
.long   0x018c5850	
.long   0x83000258	
.long   0x70000200	
.long   0x0c8c5c91	
.long   0x00008448	
.long   0x83988000	
.long   0x60000000	
.long   0x018c584b	
.long   0x83000258	
.long   0x70000200	
.long   0x0c8c5c92	
.long   0x00008448	
.long   0x83988000	
.long   0x60000000	
.long   0x018c5853	
.long   0x83000258	
.long   0x70000200	
.long   0x0c8c5c94	
.long   0x00008448	
.long   0x83988000	
.long   0x60000000	
.long   0x018c5855	
.long   0x83000258	
.long   0x70000200	
.long   0x0c8c5c96	
.long   0x00008448	
.long   0x80188000	
.long   0x64000100	
.long   0x0c4c5c4a	
.long   0xc418825b	
.long   0x81990000	
.long   0x10000020	
.long   0x0000c820	
.long   0xc4188264	
.long   0xc04b8000	
.long   0x8199e000	
.long   0x1000cf10	
.long   0x00000318	
.long   0x80188000	
.long   0x70000000	
.long   0x080c5c4a	
.long   0x60000000	
.long   0x020c5c4b	
.long   0x000001f8	


.globl scPsc3Xmit_TDT
scPsc3Xmit_TDT:	/* Task 1 Descriptor Table */
.long   0x82180000	
.long   0x00000780	
.long   0x80980000	
.long   0x70000000	
.long   0x008c5c09	
.long   0x60000000	
.long   0x020c5c0a	
.long   0x811802c9	
.long   0xc098e000	
.long   0x80c58004	
.long   0x70000000	
.long   0x00cc5c4c	
.long   0x00000390	
.long   0xc2c6e000	
.long   0x858000d2	
.long   0x014ac798	
.long   0x80988000	
.long   0x60000100	
.long   0x0c4c5c49	
.long   0xc41882db	
.long   0x82190000	
.long   0x10000438	
.long   0x0000c838	
.long   0xc40202e0	
.long   0x80c70000	
.long   0x1000cb18	
.long   0x00000718	
.long   0x80188000	
.long   0x040001f8	
.long   0x80988000	
.long   0x70000000	
.long   0x080c5c49	
.long   0x60000000	
.long   0x020c5c4a	
.long   0x000001f8	


.globl scEthernetRecv_TDT
scEthernetRecv_TDT:	/* Task 2 Descriptor Table */
.long   0xc0186000	
.long   0xd8458340	
.long   0x98460008	
.long   0x10002710	
.long   0x10000788	
.long   0x00001390	
.long   0x82180009	
.long   0x10000b80	
.long   0x60000000	
.long   0x014c508e	
.long   0x82980340	
.long   0x80004000	
.long   0x8246800c	
.long   0x70000000	
.long   0x01cc508f	
.long   0x70000000	
.long   0x028c5090	
.long   0x00000f90	
.long   0xc000d184	
.long   0x86818452	
.long   0x0060cf88	
.long   0xc000d184	
.long   0x8682049b	
.long   0x000acf88	
.long   0x82188009	
.long   0x60000100	
.long   0x0c4c5c4e	
.long   0xc39883e4	
.long   0x80190009	
.long   0x04008468	
.long   0x80004000	
.long   0xc5048360	
.long   0x82458008	
.long   0x1000cf20	
.long   0x00001320	
.long   0x80004000	
.long   0xc519036d	
.long   0x84998009	
.long   0x10001008	
.long   0x0000cc08	
.long   0x80004000	
.long   0x82190009	
.long   0x10000b90	
.long   0x60000000	
.long   0x080c508e	
.long   0x000001f8	


.globl scEthernetXmit_TDT
scEthernetXmit_TDT:	/* Task 3 Descriptor Table */
.long   0xc0180000	
.long   0x9846834c	
.long   0x10002b08	
.long   0x10001388	
.long   0x00000780	
.long   0x82180000	
.long   0x10000b80	
.long   0x60000000	
.long   0x02cc508e	
.long   0x85980349	
.long   0x80004003	
.long   0x82468004	
.long   0x70000000	
.long   0x018c508f	
.long   0x70000000	
.long   0x01cc5090	
.long   0x70000000	
.long   0x030c5091	
.long   0x70000000	
.long   0x020c5092	
.long   0x10000f90	
.long   0x60000000	
.long   0x024c50e1	
.long   0x80004000	
.long   0xc3990352	
.long   0x84498000	
.long   0x00002318	
.long   0xc000d1a4	
.long   0x8401835c	
.long   0x008ac798	
.long   0xc000d1a4	
.long   0xc1c40000	
.long   0x83998352	
.long   0x088ac790	
.long   0x82188000	
.long   0x60000100	
.long   0x0c4c5c4e	
.long   0xc31883ed	
.long   0x80190000	
.long   0x04008468	
.long   0x80004000	
.long   0xc5062345	
.long   0x824a0000	
.long   0x1000cb20	
.long   0x00001320	
.long   0x80004000	
.long   0xc5190000	
.long   0x86198352	
.long   0x10001008	
.long   0x1000c808	
.long   0x00008c68	
.long   0x82188000	
.long   0x10000b88	
.long   0x60000000	
.long   0x080c508e	
.long   0x000001f8	


.globl scAC97Rx_TDT
scAC97Rx_TDT:	/* Task 4 Descriptor Table */
.long   0x80186000	
.long   0x820320d1	
.long   0x10000788	
.long   0x8000c060	
.long   0x8300814a	
.long   0x05a0d798	
.long   0x000001f8	
.long   0x000001f8	


.globl scAC97Tx_TDT
scAC97Tx_TDT:	/* Task 5 Descriptor Table */
.long   0x80186000	
.long   0x820320d1	
.long   0x10000788	
.long   0x8000c080	
.long   0x8300814a	
.long   0x05c0cfa8	
.long   0x000001f8	
.long   0x000001f8	


.globl ATArw_TDT
ATArw_TDT:	/* Task 6 Descriptor Table */
.long   0x80180000	
.long   0x70000000	
.long   0x020c504a	
.long   0x60000000	
.long   0x00cc504b	
.long   0xc4180309	
.long   0x80188000	
.long   0x86190000	
.long   0x60000000	
.long   0x008c504d	
.long   0x8000da3c	
.long   0x860000d3	
.long   0x00c0cba0	
.long   0x000001f8	
.long   0x84180324	
.long   0x81c70000	
.long   0x10000f08	
.long   0x10001060	
.long   0x60000000	
.long   0x008c504d	
.long   0xc1988309	
.long   0x80190000	
.long   0x80198000	
.long   0x000001f8	
.long   0x8000da3c	
.long   0x86000c53	
.long   0x00a0d798	
.long   0xc0004000	
.long   0x9a9a0000	
.long   0x00000320	
.long   0x000001f8	
.long   0x860003e8	
.long   0x86198412	
.long   0x000001f8	
.long   0x8000da4c	
.long   0x8000da50	
.long   0x8000da40	
.long   0x861b0000	
.long   0x70000000	
.long   0x01cc5d51	
.long   0x70000100	
.long   0x018c54b3	
.long   0x100027a0	
.long   0x00001460	
.long   0xc3998324	
.long   0x84c36001	
.long   0xda1a8336	
.long   0x9a47e000	
.long   0x10009330	
.long   0x00009498	
.long   0x10001720	
.long   0x00008c98	
.long   0x8000da3c	
.long   0xd9426018	
.long   0xc29a853f	
.long   0x8246601c	
.long   0x1000d398	
.long   0x00001330	
.long   0x00008420	
.long   0x00008098	
.long   0xc1180309	
.long   0x80188000	
.long   0x04000060	

.align  8

.globl scPsc3Recv_VarTab
scPsc3Recv_VarTab:	/* Task 0 Variable Table */
.long   0x00000000	/* var[0] */
.long   0x00000000	/* var[1] */
.long   0x00000000	/* var[2] */
.long   0x00000000	/* var[3] */
.long   0x00000000	/* var[4] */
.long   0x00000000	/* var[5] */
.long   0x00000000	/* var[6] */
.long   0x00000000	/* var[7] */
.long   0x00000000	/* var[8] */
.long   0x00000000	/* var[9] */
.long   0x80000000	/* var[10] */
.long   0x20000000	/* var[11] */
.long   0x00000060	/* var[12] */
.long   0x00000001	/* var[13] */
.long   0xffff0000	/* var[14] */
.long   0x00100000	/* var[15] */
.long   0x40000000	/* var[16] */
.long   0x00080000	/* var[17] */
.long   0x00040000	/* var[18] */
.long   0x10000000	/* var[19] */
.long   0x00020000	/* var[20] */
.long   0x00800000	/* var[21] */
.long   0x00010000	/* var[22] */
.long   0x00000008	/* var[23] */
.long   0x00000000	/* inc[0] */
.long   0x60000000	/* inc[1] */
.long   0xa0000001	/* inc[2] */
.long   0x40000000	/* inc[3] */
.long   0x80000000	/* inc[4] */
.long   0x00000000	/* inc[5] */
.long   0x00000000	/* inc[6] */
.long   0x00000000	/* inc[7] */

.align  8

.globl scPsc3Xmit_VarTab
scPsc3Xmit_VarTab:	/* Task 1 Variable Table */
.long   0x00000000	/* var[0] */
.long   0x00000000	/* var[1] */
.long   0x00000000	/* var[2] */
.long   0x00000000	/* var[3] */
.long   0x00000000	/* var[4] */
.long   0x00000000	/* var[5] */
.long   0x00000000	/* var[6] */
.long   0x00000000	/* var[7] */
.long   0x00000000	/* var[8] */
.long   0x80000000	/* var[9] */
.long   0x20000000	/* var[10] */
.long   0x00000000	/* var[11] */
.long   0x0000ffff	/* var[12] */
.long   0x00000080	/* var[13] */
.long   0x00000008	/* var[14] */
.long   0x00000000	/* var[15] */
.long   0x00000000	/* var[16] */
.long   0x00000000	/* var[17] */
.long   0x00000000	/* var[18] */
.long   0x00000000	/* var[19] */
.long   0x00000000	/* var[20] */
.long   0x00000000	/* var[21] */
.long   0x00000000	/* var[22] */
.long   0x00000000	/* var[23] */
.long   0x00000000	/* inc[0] */
.long   0x60000000	/* inc[1] */
.long   0x20000001	/* inc[2] */
.long   0x40000000	/* inc[3] */
.long   0x80000000	/* inc[4] */
.long   0x00000000	/* inc[5] */
.long   0x00000000	/* inc[6] */
.long   0x00000000	/* inc[7] */

.align  8

.globl scEthernetRecv_VarTab
scEthernetRecv_VarTab:	/* Task 2 Variable Table */
.long   0x00000000	/* var[0] */
.long   0x00000000	/* var[1] */
.long   0x00000000	/* var[2] */
.long   0x00000000	/* var[3] */
.long   0x00000000	/* var[4] */
.long   0x00000000	/* var[5] */
.long   0x00000000	/* var[6] */
.long   0x00000000	/* var[7] */
.long   0x00000000	/* var[8] */
.long   0x00000000	/* var[9] */
.long   0x00000000	/* var[10] */
.long   0x00000008	/* var[11] */
.long   0x0000000c	/* var[12] */
.long   0x00000000	/* var[13] */
.long   0x80000000	/* var[14] */
.long   0x10000000	/* var[15] */
.long   0x20000000	/* var[16] */
.long   0x000005fa	/* var[17] */
.long   0x00000004	/* var[18] */
.long   0x00000000	/* var[19] */
.long   0x00000000	/* var[20] */
.long   0x00000000	/* var[21] */
.long   0x00000000	/* var[22] */
.long   0x00000000	/* var[23] */
.long   0x60000000	/* inc[0] */
.long   0x00000000	/* inc[1] */
.long   0x20000004	/* inc[2] */
.long   0x20000001	/* inc[3] */
.long   0x80000000	/* inc[4] */
.long   0x40000000	/* inc[5] */
.long   0x00000000	/* inc[6] */
.long   0x00000000	/* inc[7] */

.align  8

.globl scEthernetXmit_VarTab
scEthernetXmit_VarTab:	/* Task 3 Variable Table */
.long   0x00000000	/* var[0] */
.long   0x00000000	/* var[1] */
.long   0x00000000	/* var[2] */
.long   0x00000000	/* var[3] */
.long   0x00000000	/* var[4] */
.long   0x00000000	/* var[5] */
.long   0x00000000	/* var[6] */
.long   0x00000000	/* var[7] */
.long   0x00000000	/* var[8] */
.long   0x00000000	/* var[9] */
.long   0x00000000	/* var[10] */
.long   0x00000000	/* var[11] */
.long   0x00000000	/* var[12] */
.long   0x00000000	/* var[13] */
.long   0x80000000	/* var[14] */
.long   0x10000000	/* var[15] */
.long   0x08000000	/* var[16] */
.long   0x20000000	/* var[17] */
.long   0x0000ffff	/* var[18] */
.long   0xffffffff	/* var[19] */
.long   0x00000008	/* var[20] */
.long   0x00000000	/* var[21] */
.long   0x00000000	/* var[22] */
.long   0x00000000	/* var[23] */
.long   0x00000000	/* inc[0] */
.long   0x60000000	/* inc[1] */
.long   0x40000000	/* inc[2] */
.long   0x4000ffff	/* inc[3] */
.long   0xe0000001	/* inc[4] */
.long   0x80000000	/* inc[5] */
.long   0x00000000	/* inc[6] */
.long   0x00000000	/* inc[7] */

.align  8

.globl scAC97Rx_VarTab
scAC97Rx_VarTab:	/* Task 4 Variable Table */
.long   0x00000000	/* var[0] */
.long   0x00000000	/* var[1] */
.long   0x00000000	/* var[2] */
.long   0x00000000	/* var[3] */
.long   0x00000000	/* var[4] */
.long   0x00000000	/* var[5] */
.long   0x00000000	/* var[6] */
.long   0x00000000	/* var[7] */
.long   0x00000000	/* var[8] */
.long   0x00000000	/* var[9] */
.long   0x00000000	/* var[10] */
.long   0x00000000	/* var[11] */
.long   0x00000000	/* var[12] */
.long   0x00000000	/* var[13] */
.long   0x00000000	/* var[14] */
.long   0x00000000	/* var[15] */
.long   0x00000000	/* var[16] */
.long   0x00000000	/* var[17] */
.long   0x00000000	/* var[18] */
.long   0x00000000	/* var[19] */
.long   0x00000000	/* var[20] */
.long   0x00000000	/* var[21] */
.long   0x00000000	/* var[22] */
.long   0x00000000	/* var[23] */
.long   0xe0000000	/* inc[0] */
.long   0x20000001	/* inc[1] */
.long   0xe0000004	/* inc[2] */
.long   0x00000000	/* inc[3] */
.long   0x00000000	/* inc[4] */
.long   0x00000000	/* inc[5] */
.long   0x00000000	/* inc[6] */
.long   0x00000000	/* inc[7] */

.align  8

.globl scAC97Tx_VarTab
scAC97Tx_VarTab:	/* Task 5 Variable Table */
.long   0x00000000	/* var[0] */
.long   0x00000000	/* var[1] */
.long   0x00000000	/* var[2] */
.long   0x00000000	/* var[3] */
.long   0x00000000	/* var[4] */
.long   0x00000000	/* var[5] */
.long   0x00000000	/* var[6] */
.long   0x00000000	/* var[7] */
.long   0x00000000	/* var[8] */
.long   0x00000000	/* var[9] */
.long   0x00000000	/* var[10] */
.long   0x00000000	/* var[11] */
.long   0x00000000	/* var[12] */
.long   0x00000000	/* var[13] */
.long   0x00000000	/* var[14] */
.long   0x00000000	/* var[15] */
.long   0x00000000	/* var[16] */
.long   0x00000000	/* var[17] */
.long   0x00000000	/* var[18] */
.long   0x00000000	/* var[19] */
.long   0x00000000	/* var[20] */
.long   0x00000000	/* var[21] */
.long   0x00000000	/* var[22] */
.long   0x00000000	/* var[23] */
.long   0xe0000000	/* inc[0] */
.long   0x20000001	/* inc[1] */
.long   0xe0000004	/* inc[2] */
.long   0x00000000	/* inc[3] */
.long   0x00000000	/* inc[4] */
.long   0x00000000	/* inc[5] */
.long   0x00000000	/* inc[6] */
.long   0x00000000	/* inc[7] */

.align  8

.globl ATArw_VarTab
ATArw_VarTab:	/* Task 6 Variable Table */
.long   0x00000000	/* var[0] */
.long   0x00000000	/* var[1] */
.long   0x00000000	/* var[2] */
.long   0x00000000	/* var[3] */
.long   0x00000000	/* var[4] */
.long   0x00000000	/* var[5] */
.long   0x00000000	/* var[6] */
.long   0x00000000	/* var[7] */
.long   0x00000000	/* var[8] */
.long   0x00000000	/* var[9] */
.long   0x40000000	/* var[10] */
.long   0x0000ffff	/* var[11] */
.long   0x00000000	/* var[12] */
.long   0x10000000	/* var[13] */
.long   0xffffff80	/* var[14] */
.long   0x00000200	/* var[15] */
.long   0x00000074	/* var[16] */
.long   0x00010000	/* var[17] */
.long   0xffffffff	/* var[18] */
.long   0x00000001	/* var[19] */
.long   0x00000004	/* var[20] */
.long   0x00000000	/* var[21] */
.long   0x00000000	/* var[22] */
.long   0x00000000	/* var[23] */
.long   0x00000000	/* inc[0] */
.long   0x40000000	/* inc[1] */
.long   0x20000001	/* inc[2] */
.long   0xe0000004	/* inc[3] */
.long   0x80000000	/* inc[4] */
.long   0x20000000	/* inc[5] */
.long   0xa0000000	/* inc[6] */
.long   0xc000fffc	/* inc[7] */

.align  8

.globl scPsc3Recv_FDT
scPsc3Recv_FDT:	/* Task 0 Function Descriptor Table */
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x05800000	
.long   0x05400000	
.long   0x05e00000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	

.align  8

.globl scPsc3Xmit_FDT
scPsc3Xmit_FDT:	/* Task 1 Function Descriptor Table */
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x05800000	
.long   0x05400000	
.long   0x05e00000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	

.align  8

.globl scEthernetRecv_FDT
scEthernetRecv_FDT:	/* Task 2 Function Descriptor Table */
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x05800000	
.long   0x05400000	
.long   0x05e00000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	

.align  8

.globl scEthernetXmit_FDT
scEthernetXmit_FDT:	/* Task 3 Function Descriptor Table */
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x05800000	
.long   0x05400000	
.long   0x05e00000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	

.align  8

.globl scAC97Rx_FDT
scAC97Rx_FDT:	/* Task 4 Function Descriptor Table */
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x05800000	
.long   0x05400000	
.long   0x05e00000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	

.align  8

.globl scAC97Tx_FDT
scAC97Tx_FDT:	/* Task 5 Function Descriptor Table */
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x05800000	
.long   0x05400000	
.long   0x05e00000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	

.align  8

.globl ATArw_FDT
ATArw_FDT:	/* Task 6 Function Descriptor Table */
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x05800000	
.long   0x05400000	
.long   0x05e00000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	
.long   0x00000000	


.globl scPsc3Recv_CSave
scPsc3Recv_CSave:	/* Task 0 context save space */
.space  256, 0x0


.globl scPsc3Xmit_CSave
scPsc3Xmit_CSave:	/* Task 1 context save space */
.space  256, 0x0


.globl scEthernetRecv_CSave
scEthernetRecv_CSave:	/* Task 2 context save space */
.space  256, 0x0


.globl scEthernetXmit_CSave
scEthernetXmit_CSave:	/* Task 3 context save space */
.space  256, 0x0


.globl scAC97Rx_CSave
scAC97Rx_CSave:	/* Task 4 context save space */
.space  256, 0x0


.globl scAC97Tx_CSave
scAC97Tx_CSave:	/* Task 5 context save space */
.space  256, 0x0


.globl ATArw_CSave
ATArw_CSave:	/* Task 6 context save space */
.space  256, 0x0
