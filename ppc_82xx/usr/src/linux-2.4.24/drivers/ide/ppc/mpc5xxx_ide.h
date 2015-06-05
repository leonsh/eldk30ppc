/*
 * drivers/ide/ppc/mpc5xxx_ide.h
 *
 * Definitions for MPC5xxx on-chip IDE interface
 *
 *  Copyright (c) 2003 Mipsys - Benjamin Herrenschmidt
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#ifndef __MPC5XXX_IDE_H__
#define __MPC5XXX_IDE_H__

struct mpc5xxx_ata {

	/* Host interface registers */

	volatile u32	config;			/* MBAR_ATA + 0x00 Host configuration */
#define MPC5xxx_ATA_HOSTCONF_SMR		0x80000000UL	/* State machine reset */
#define MPC5xxx_ATA_HOSTCONF_FR			0x40000000UL	/* FIFO Reset */
#define MPC5xxx_ATA_HOSTCONF_IE			0x02000000UL	/* Enable interrupt in PIO */
#define MPC5xxx_ATA_HOSTCONF_IORDY	  	0x01000000UL	/* Drive supports IORDY protocol */
	volatile u32	host_status;		/* MBAR_ATA + 0x04 Host controller status */
#define MPC5xxx_ATA_HOSTSTAT_TIP		0x80000000UL	/* Transaction in progress */
#define MPC5xxx_ATA_HOSTSTAT_UREP		0x40000000UL	/* UDMA Read Extended Pause */
#define MPC5xxx_ATA_HOSTSTAT_RERR		0x02000000UL	/* Read Error */
#define MPC5xxx_ATA_HOSTSTAT_WERR		0x01000000UL	/* Write Error */
	volatile u32	pio1;			/* MBAR_ATA + 0x08 PIO Timing 1 */
	volatile u32	pio2;			/* MBAR_ATA + 0x0c PIO Timing 2 */
	volatile u32	mdma1;			/* MBAR_ATA + 0x10 MDMA Timing 1 */
	volatile u32	mdma2;			/* MBAR_ATA + 0x14 MDMA Timing 2 */
	volatile u32	udma1;			/* MBAR_ATA + 0x18 UDMA Timing 1 */
	volatile u32	udma2;			/* MBAR_ATA + 0x1c UDMA Timing 2 */
	volatile u32	udma3;			/* MBAR_ATA + 0x20 UDMA Timing 3 */
	volatile u32	udma4;			/* MBAR_ATA + 0x24 UDMA Timing 4 */
	volatile u32	udma5;			/* MBAR_ATA + 0x28 UDMA Timing 5 */
	volatile u32	invalid;		/* MBAR_ATA + 0x2c Invalid */
	volatile u32	reserved0[3];

	/* FIFO registers */

	volatile u32 	fifo_data;		/* MBAR_ATA + 0x3c */
	volatile u8	fifo_status_frame; 	/* MBAR_ATA + 0x40 */
	volatile u8	fifo_status;		/* MBAR_ATA + 0x41 */
#define MPC5xxx_ATA_FIFOSTAT_EMPTY	0x01	/* FIFO Empty */
	volatile u16	reserved7[1];
	volatile u8	fifo_control;		/* MBAR_ATA + 0x44; */
	volatile u8	reserved8[5];
	volatile u16	fifo_alarm;		/* MBAR_ATA + 0x4a */
	volatile u16	reserved9;
	volatile u16	fifo_rdp;		/* MBAR_ATA + 0x4e */
	volatile u16	reserved10;
	volatile u16	fifo_wrp;		/* MBAR_ATA + 0x52 */
	volatile u16	reserved11;
	volatile u16	fifo_lfrdp;		/* MBAR_ATA + 0x56 */
	volatile u16	reserved12;
	volatile u16	fifo_lfwrp;		/* MBAR_ATA + 0x5a */

	/* Drive TaskFile registers */

	volatile u8	tf_control;		/* MBAR_ATA + 0x5c TASKFILE Control/Alt Status */
	volatile u8	reserved13[3];
	volatile u16	tf_data;		/* MBAR_ATA + 0x60 TASKFILE Data */
	volatile u16	reserved14;
	volatile u8	tf_features;		/* MBAR_ATA + 0x64 TASKFILE Features/Error */
	volatile u8	reserved15[3];
	volatile u8	tf_sec_count;		/* MBAR_ATA + 0x68 TASKFILE Sector Count */
	volatile u8	reserved16[3];
	volatile u8	tf_sec_num;		/* MBAR_ATA + 0x6c TASKFILE Sector Number */
	volatile u8	reserved17[3];
	volatile u8	tf_cyl_low;		/* MBAR_ + 0x70 TASKFILE Cylinder Low */
	volatile u8	reserved18[3];
	volatile u8	tf_cyl_high;		/* MBAR_ATA + 0x74 TASKFILE Cylinder High */
	volatile u8	reserved19[3];
	volatile u8	tf_dev_head;		/* MBAR_ATA + 0x78 TASKFILE Device/Head */
	volatile u8	reserved20[3];
	volatile u8	tf_command;		/* MBAR_ATA + 0x7c TASKFILE Command/Status */
	volatile u8	dma_mode;		/* MBAR_ATA + 0x7d ATA Host DMA Mode configuration */
#define MPC5xxx_ATA_DMAMODE_WRITE	0x01	/* Write DMA */
#define MPC5xxx_ATA_DMAMODE_READ	0x02	/* Read DMA */
#define MPC5xxx_ATA_DMAMODE_UDMA	0x04	/* UDMA enabled */
#define MPC5xxx_ATA_DMAMODE_IE		0x08	/* Enable drive interrupt to CPU in DMA mode */
#define MPC5xxx_ATA_DMAMODE_FE		0x10	/* FIFO Flush enable in Rx mode */
#define MPC5xxx_ATA_DMAMODE_FR		0x20	/* FIFO Reset */
#define MPC5xxx_ATA_DMAMODE_HUT		0x40	/* Host UDMA burst terminate */

	volatile u8	reserved21[2];
};

struct mpc5xxx_ata_timings
{
	u32	pio1;
	u32	pio2;
	u32	mdma1;
	u32	mdma2;
	u32	udma1;
	u32	udma2;
	u32	udma3;
	u32	udma4;
	u32	udma5;
	int	using_udma;
};

extern void setup_hwif_mpc5xxx_iops (ide_hwif_t *hwif);

#define WAIT_TIP_BIT_CLEAR(port) \
	do {    int timeout = 100000; \
		struct mpc5xxx_ata *regs = \
			(struct mpc5xxx_ata *)(((unsigned long)(port)) & 0xffffff00ul); \
		while(readl(&regs->host_status) & MPC5xxx_ATA_HOSTSTAT_TIP) \
			if (timeout-- == 0) { \
				break; \
			} \
			udelay(10); \
	} while(0)

/*
 * Simplify some macros
 */
#define MPC5xxx_IDE_HAS_DMA	(defined(CONFIG_BLK_DEV_IDE_MPC5xxx_MDMA) \
				|| defined(CONFIG_BLK_DEV_IDE_MPC5xxx_UDMA))

#endif /* __MPC5XXX_IDE_H__ */
