/*
 * drivers/ide/ppc/mpc5xxx_ide.c
 *
 * Driver for MPC5xxx on-chip IDE interface
 *
 *  Copyright (c) 2003 Mipsys - Benjamin Herrenschmidt
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mpc5xxx.h>

#include <bestcomm_api.h>

#include "mpc5xxx_ide.h"
#include "ide_modes.h"

#define MAX_DMA_BUFFERS		4

#undef DEBUG

#ifdef DEBUG
#define DPRINTK(fmt, args...)	printk(fmt, ## args)
#else
#define DPRINTK(fmt, args...)
#endif

/*
 * TODO: Move these to a structure in hwif datas in case
 * Motorola ever release a version with more than one IDE
 * channel
 */
static int			mpc5xxx_hwif_index;
static struct mpc5xxx_ata	*mpc5xxx_ataregs;
static int			mpc5xxx_clock_period;
static struct mpc5xxx_ata_timings mpc5xxx_timings[2];
#if MPC5xxx_IDE_HAS_DMA
static int			mpc5xxx_ata_sdma_nextbd;
static int 			mpc5xxx_ata_sdma_task;
static unsigned long		mpc5xxx_ata_dma_cur_addr;
static unsigned long		mpc5xxx_ata_dma_cur_len;
static int			mpc5xxx_ata_dma_cur_sg;
static int			mpc5xxx_ata_free_bds;
static int			mpc5xxx_ata_dma_last_write;
#endif

extern int mpc5xxx_sdma_load_tasks_image(void);

extern int ide_build_sglist (ide_hwif_t *hwif, struct request *rq, int ddir);
extern int ide_raw_build_sglist (ide_hwif_t *hwif, struct request *rq);


/*
 * Add count for hold state. (Check exactly what that means with spec)
 */
#define ATA_ADD_COUNT		1

/*
 * Timing calculation helpers
 */
#define CALC_CLK_VALUE_UP(v)	(((v) + mpc5xxx_clock_period - 1) / mpc5xxx_clock_period)
#define CALC_CLK_VALUE_DOWN(v)	((v) / mpc5xxx_clock_period)

/* ATAPI-4 PIO specs */
/* numbers in ns, extrapolation done by code */
static int t0_spec[5]	=	{600,	383,	240,	180,	120};
static int t1_spec[5]	=	{ 70,	50,	30,	30,	25};
static int t2_8spec[5]	=	{290,	290,	290,	80,	70};
static int t2_16spec[5] =	{165,	125,	100,	80,	70};
static int t2i_spec[5]	=	{0,	0,	0,	70,	25};
static int t4_spec[5]	=	{30,	20,	15,	10,	10};
static int ta_spec[5]	=	{35,	35,	35,	35,	35};

#if MPC5xxx_IDE_HAS_DMA

/* ATAPI-4 MDMA specs */
/* numbers in ns, extrapolation done by code */
static int t0M_spec[3]	=	{480,	150,	120};
static int td_spec[3]	=	{215,	80,	70};
static int th_spec[3]	=	{20,	15,	10};
static int tj_spec[3]	=	{20,	5,	5};
static int tkw_spec[3]	=	{215,	50,	25};
static int tm_spec[3]	=	{50,	30,	25};
static int tn_spec[3]	=	{15,	10,	10};

/* ATAPI-4 UDMA specs */
static int tcyc_spec[3]		= {120,	80,	60};
static int t2cyc_spec[3]	= {240,	160,	120};
static int tds_spec[3]		= {15,	10,	7};
static int tdh_spec[3]		= {5,	5,	5};
static int tdvs_spec[3]		= {70,	48,	34};
static int tdvh_spec[3]		= {6,	6,	6};
static int tfs_minspec[3]	= {0,	0,	0};
static int tli_maxspec[3]	= {150,	150,	150};
static int tmli_spec[3]		= {20,	20,	20};
static int taz_spec[3]		= {10,	10,	10};
static int tzah_spec[3]		= {20,	20,	20};
static int tenv_minspec[3]	= {20,	20,	20};
static int tsr_spec[3]		= {50,	30,	20};
static int trfs_spec[3]		= {75,	60,	50};
static int trp_spec[3]		= {160,	125,	100};
static int tack_spec[3]		= {20,	20,	20};
static int tss_spec[3]		= {50,	50,	50};

#endif /* MPC5xxx_IDE_HAS_DMA */

static u8 mpc5xxx_dma2pio (u8 xfer_rate)
{
	switch(xfer_rate) {
#if MPC5xxx_IDE_HAS_DMA
	case XFER_UDMA_6:
	case XFER_UDMA_5:
	case XFER_UDMA_4:
	case XFER_UDMA_3:
	case XFER_UDMA_2:
	case XFER_UDMA_1:
	case XFER_UDMA_0:
	case XFER_MW_DMA_2:
	case XFER_PIO_4:
		return 4;
	case XFER_MW_DMA_1:
	case XFER_PIO_3:
		return 3;
	case XFER_SW_DMA_2:
	case XFER_PIO_2:
		return 2;
	case XFER_MW_DMA_0:
	case XFER_SW_DMA_1:
	case XFER_SW_DMA_0:
#else
	case XFER_PIO_4:
		return 4;
	case XFER_PIO_3:
		return 3;
	case XFER_PIO_2:
		return 2;
#endif /* MPC5xxx_IDE_HAS_DMA */
	case XFER_PIO_1:
	case XFER_PIO_0:
	case XFER_PIO_SLOW:
	default:
		return 0;
	}
}

static void mpc5xxx_ide_apply_timings(ide_drive_t *drive)
{
	out_be32(&mpc5xxx_ataregs->pio1,  mpc5xxx_timings[drive->select.b.unit & 0x01].pio1);
	out_be32(&mpc5xxx_ataregs->pio2,  mpc5xxx_timings[drive->select.b.unit & 0x01].pio2);
	out_be32(&mpc5xxx_ataregs->mdma1, mpc5xxx_timings[drive->select.b.unit & 0x01].mdma1);
	out_be32(&mpc5xxx_ataregs->mdma2, mpc5xxx_timings[drive->select.b.unit & 0x01].mdma2);
	out_be32(&mpc5xxx_ataregs->udma1, mpc5xxx_timings[drive->select.b.unit & 0x01].udma1);
	out_be32(&mpc5xxx_ataregs->udma2, mpc5xxx_timings[drive->select.b.unit & 0x01].udma2);
	out_be32(&mpc5xxx_ataregs->udma3, mpc5xxx_timings[drive->select.b.unit & 0x01].udma3);
	out_be32(&mpc5xxx_ataregs->udma4, mpc5xxx_timings[drive->select.b.unit & 0x01].udma4);
	out_be32(&mpc5xxx_ataregs->udma5, mpc5xxx_timings[drive->select.b.unit & 0x01].udma5);
}

static void mpc5xxx_ide_tuneproc(ide_drive_t *drive, u8 pio)
{
	int which = drive->select.b.unit & 0x01;
	u32 t0, t2_8, t2_16, t2i, t4, t1, ta;

	pio = ide_get_best_pio_mode(drive, pio, 5, NULL);

	printk("%s: Setting PIO %d timings\n", drive->name, pio);

	t0 = ATA_ADD_COUNT + CALC_CLK_VALUE_UP(t0_spec[pio]);
	t2_8 = ATA_ADD_COUNT + CALC_CLK_VALUE_UP(t2_8spec[pio]);
	t2_16 = ATA_ADD_COUNT + CALC_CLK_VALUE_UP(t2_16spec[pio]);
	t2i = ATA_ADD_COUNT + CALC_CLK_VALUE_UP(t2i_spec[pio]);
	t4 = ATA_ADD_COUNT + CALC_CLK_VALUE_UP(t4_spec[pio]);
	t1 = ATA_ADD_COUNT + CALC_CLK_VALUE_UP(t1_spec[pio]);
	ta = ATA_ADD_COUNT + CALC_CLK_VALUE_UP(ta_spec[pio]);

	mpc5xxx_timings[which].pio1 = (t0 << 24) | (t2_8 << 16) | (t2_16 << 8) | (t2i);
	mpc5xxx_timings[which].pio2 = (t4 << 24) | (t1 << 16) | (ta << 8);

	if (drive->select.all == HWIF(drive)->INB(IDE_SELECT_REG))
		mpc5xxx_ide_apply_timings(drive);
}

#if MPC5xxx_IDE_HAS_DMA

static void mpc5xxx_ide_calc_udma_timings(ide_drive_t *drive, u8 speed)
{
	int which = drive->select.b.unit & 0x01;
	u32 t2cyc, tcyc, tds, tdh, tdvs, tdvh, tfs, tli, tmli, taz, tenv, tsr, tss, trfs, trp, tack, tzah;
	u32 add_udma2 = 0;

	mpc5xxx_timings[which].using_udma = 1;

	if (speed == 2) {
		add_udma2 = 3;
		t2cyc = CALC_CLK_VALUE_UP(t2cyc_spec[speed]) - 3;
	} else
		t2cyc = add_udma2 + ATA_ADD_COUNT + CALC_CLK_VALUE_UP(t2cyc_spec[speed]);

	tcyc = add_udma2 + ATA_ADD_COUNT + CALC_CLK_VALUE_UP(tcyc_spec[speed]);
	tds = add_udma2 + ATA_ADD_COUNT + CALC_CLK_VALUE_UP(tds_spec[speed]);
	tdh = add_udma2 + ATA_ADD_COUNT + CALC_CLK_VALUE_UP(tdh_spec[speed]);
	tdvs = add_udma2 + ATA_ADD_COUNT + CALC_CLK_VALUE_UP(tdvs_spec[speed]);
	tdvh = add_udma2 + ATA_ADD_COUNT + CALC_CLK_VALUE_UP(tdvh_spec[speed]);
	tfs = add_udma2 + ATA_ADD_COUNT + CALC_CLK_VALUE_UP(tfs_minspec[speed]);
	tmli = add_udma2 + ATA_ADD_COUNT + CALC_CLK_VALUE_UP(tmli_spec[speed]);
	tenv = CALC_CLK_VALUE_UP(tenv_minspec[speed]);
	tss = add_udma2 + ATA_ADD_COUNT + CALC_CLK_VALUE_UP(tss_spec[speed]);
	trp = add_udma2 + ATA_ADD_COUNT + CALC_CLK_VALUE_UP(trp_spec[speed]);
	/*tack = 0x07;*/ //ATA_ADD_COUNT + CALC_CLK_VALUE_UP(tack_spec[speed]);
	tack = ATA_ADD_COUNT + CALC_CLK_VALUE_UP(tack_spec[speed]);
	tzah = add_udma2 + ATA_ADD_COUNT + CALC_CLK_VALUE_UP(tzah_spec[speed]);
	taz = add_udma2 + ATA_ADD_COUNT + CALC_CLK_VALUE_DOWN(taz_spec[speed]);
	trfs = CALC_CLK_VALUE_DOWN(trfs_spec[speed]);
	tsr = CALC_CLK_VALUE_DOWN(tsr_spec[speed]);
	tli = CALC_CLK_VALUE_UP(tli_maxspec[speed]);

	mpc5xxx_timings[which].udma1 = (t2cyc << 24) | (tcyc << 16) | (tds << 8) | (tdh);
	mpc5xxx_timings[which].udma2 = (tdvs << 24) | (tdvh << 16) | (tfs << 8) | (tli);
	mpc5xxx_timings[which].udma3 = (tmli << 24) | (taz << 16) | (tenv << 8) | (tsr);
	mpc5xxx_timings[which].udma4 = (tss << 24) | (trfs << 16) | (trp << 8) | (tack);
	mpc5xxx_timings[which].udma5 = (tzah << 24);
}

static void mpc5xxx_ide_calc_mdma_timings(ide_drive_t *drive, u8 speed)
{
	int which = drive->select.b.unit & 0x01;
	u32 t0M, td, tkw, tm, th, tj, tn;
	
	mpc5xxx_timings[which].using_udma = 0;

	t0M = ATA_ADD_COUNT + CALC_CLK_VALUE_UP(t0M_spec[speed]);
	td = ATA_ADD_COUNT + 1 + CALC_CLK_VALUE_UP(td_spec[speed]);
	tkw = CALC_CLK_VALUE_UP(tkw_spec[speed]);
	tm = ATA_ADD_COUNT + 1 + CALC_CLK_VALUE_UP(tm_spec[speed]);
	th = ATA_ADD_COUNT + CALC_CLK_VALUE_UP(th_spec[speed]);
	tj = ATA_ADD_COUNT + CALC_CLK_VALUE_UP(tj_spec[speed]);
	tn = ATA_ADD_COUNT + CALC_CLK_VALUE_UP(tn_spec[speed]);

	mpc5xxx_timings[which].mdma1 = (t0M << 24) | (td << 16) | (tkw << 8) | (tm);
	mpc5xxx_timings[which].mdma2 = (th << 24) | (tj << 16) | (tn << 8);
}

#endif /* MPC5xxx_IDE_HAS_DMA */

static int mpc5xxx_ide_speedproc(ide_drive_t *drive, u8 speed)
{
	if (speed > XFER_UDMA_2)
		speed = XFER_UDMA_2;

	switch(speed) {
#if MPC5xxx_IDE_HAS_DMA
	case XFER_UDMA_2:
	case XFER_UDMA_1:
	case XFER_UDMA_0:
		printk("%s: Setting UDMA %d timings\n", drive->name, speed - XFER_UDMA_0);
		mpc5xxx_ide_calc_udma_timings(drive, speed - XFER_UDMA_0);
		break;
	case XFER_MW_DMA_2:
	case XFER_MW_DMA_1:
	case XFER_MW_DMA_0:
		printk("%s: Setting MDMA %d timings\n", drive->name, speed - XFER_MW_DMA_0);
		mpc5xxx_ide_calc_mdma_timings(drive, speed - XFER_MW_DMA_0);
		break;
#endif /* MPC5xxx_IDE_HAS_DMA */
	case XFER_PIO_4:
	case XFER_PIO_3:
	case XFER_PIO_2:
	case XFER_PIO_0:
		break;
	default:
		return -EINVAL;
	}
	mpc5xxx_ide_tuneproc(drive, mpc5xxx_dma2pio(speed));
	return ide_config_drive_speed(drive, speed);
}

#if MPC5xxx_IDE_HAS_DMA

#ifdef DEBUG
static void dump_bds(void)
{
	TaskBD2_t *bd;

	bd = (TaskBD2_t *)TaskGetBD(mpc5xxx_ata_sdma_task, 0);
	if (bd)
		printk("bd0: %08lx %08lx %08lx\n", bd->Status, bd->DataPtr[0], bd->DataPtr[1]);
	bd = (TaskBD2_t *)TaskGetBD(mpc5xxx_ata_sdma_task, 1);
	if (bd)
		printk("bd1: %08lx %08lx %08lx\n", bd->Status, bd->DataPtr[0], bd->DataPtr[1]);
	bd = (TaskBD2_t *)TaskGetBD(mpc5xxx_ata_sdma_task, 2);
	if (bd)
		printk("bd2: %08lx %08lx %08lx\n", bd->Status, bd->DataPtr[0], bd->DataPtr[1]);
	bd = (TaskBD2_t *)TaskGetBD(mpc5xxx_ata_sdma_task, 3);
	if (bd)
		printk("bd3: %08lx %08lx %08lx\n", bd->Status, bd->DataPtr[0], bd->DataPtr[1]);
}
#else
static inline void dump_bds(void) { }
#endif

static int mpc5xxx_ide_dma_check(ide_drive_t *drive)
{
	u8 speed = ide_dma_speed(drive, 1);

	DPRINTK("dma_check: speed=%x\n", speed);
	if (speed == 0)
		return -1;
	mpc5xxx_ide_speedproc(drive, speed);

	/* Re-enable this code when DMA atually works ! */
#if 0

	if (ide_dma_enable(drive))
		drive->using_dma = 1;
	else
		drive->using_dma = 0;
#endif
	return 0;
}

static int mpc5xxx_ide_continue_dma(ide_hwif_t *hwif)
{
	struct scatterlist *sg = hwif->sg_table + mpc5xxx_ata_dma_cur_sg;
	int ddir = hwif->sg_dma_direction;
	u32 cur_addr = mpc5xxx_ata_dma_cur_addr;
	u32 cur_len = mpc5xxx_ata_dma_cur_len;
	int next_bd;

	while (mpc5xxx_ata_dma_cur_sg < hwif->sg_nents && sg_dma_len(sg)) {
		if (cur_len == 0) {
			cur_addr = sg_dma_address(sg);
			cur_len = sg_dma_len(sg);
		}
		while (cur_len) {
			unsigned int tc;

			if (mpc5xxx_ata_free_bds == 0) {
				mpc5xxx_ata_dma_cur_len = cur_len;
				mpc5xxx_ata_dma_cur_addr = cur_addr;
				return 0;
			}
			tc  = (cur_len < 0xfe00) ? cur_len: 0xfe00;
			if (ddir == PCI_DMA_FROMDEVICE)
				next_bd = TaskBDAssign(mpc5xxx_ata_sdma_task,
			       			     (void *)&mpc5xxx_ataregs->fifo_data,
			       			     (void *)cur_addr, tc, 0);
			else
				next_bd = TaskBDAssign(mpc5xxx_ata_sdma_task, (void *)cur_addr,
	       					     (void *)&mpc5xxx_ataregs->fifo_data, tc, 0);
			DPRINTK("SDMA setup @%08x, l: %x, nextbd: %d !\n", cur_addr, tc, next_bd);
			if (mpc5xxx_ata_sdma_nextbd < 0)
				mpc5xxx_ata_sdma_nextbd = next_bd;
			cur_addr += tc;
			cur_len -= tc;
			mpc5xxx_ata_free_bds--;
		}
		sg++;
		mpc5xxx_ata_dma_cur_sg++;
	}

	return 0;
}

static int mpc5xxx_ide_build_dmatable(ide_drive_t *drive, struct request *rq, int ddir)
{
	ide_hwif_t *hwif = HWIF(drive);
	TaskSetupParamSet_t ata_setup;

	/* Build sglist */
	if (rq->cmd == IDE_DRIVE_TASKFILE)
		hwif->sg_nents = ide_raw_build_sglist(hwif, rq);
	else
		hwif->sg_nents = ide_build_sglist(hwif, rq, ddir);
	if (!hwif->sg_nents)
		return 0;
	
	/* Setup BestComm task */
	ata_setup.NumBD = MAX_DMA_BUFFERS;
	ata_setup.Size.MaxBuf = 0xfe00;
	if (ddir == PCI_DMA_FROMDEVICE) {
		ata_setup.Initiator = INITIATOR_ATA_RX;
		ata_setup.StartAddrDst = 0;
		ata_setup.StartAddrSrc = (u32)&mpc5xxx_ataregs->fifo_data;
		ata_setup.IncrDst = 4;
		ata_setup.IncrSrc = 0;
		ata_setup.SzDst = 4;
		ata_setup.SzSrc = 4;
	} else {
		ata_setup.Initiator = INITIATOR_ATA_TX;
		ata_setup.StartAddrDst = (u32)&mpc5xxx_ataregs->fifo_data;
		ata_setup.StartAddrSrc = 0;
		ata_setup.IncrDst = 0;
		ata_setup.IncrSrc = 4;
		ata_setup.SzDst = 4;
		ata_setup.SzSrc = 4;
	}

	mpc5xxx_ata_sdma_task = TaskSetup (TASK_ATA, &ata_setup);
	DPRINTK("Task setup, src: %08lx, dst: %08lx, nr_sectors: %ld -> %d\n",
	       ata_setup.StartAddrSrc, ata_setup.StartAddrDst, rq->nr_sectors,
	       mpc5xxx_ata_sdma_task);


	mpc5xxx_ata_dma_cur_sg = 0;
	mpc5xxx_ata_dma_cur_addr = 0;
	mpc5xxx_ata_dma_cur_len = 0;
	mpc5xxx_ata_free_bds = MAX_DMA_BUFFERS;
	mpc5xxx_ata_sdma_nextbd = -1;

	if (mpc5xxx_ide_continue_dma(hwif) == 0) {
		dump_bds();
		return 1;
	}

	pci_unmap_sg(hwif->pci_dev,
		     hwif->sg_table,
		     hwif->sg_nents,
		     hwif->sg_dma_direction);
	hwif->sg_dma_active = 0;
	return 0; /* revert to PIO for this request */
}

/*
 * From experimentation, it seems that the BestComm task for ATA will
 * clear the Lenght (Status) of a BD that has been processed, we base
 * ourselves on this behaviour here.
 * Note that it seem also to interrupt us only after sending all 4 BDs
 * which is sub-optimal imho, 3 BDs would have been a better threshold
 * as it allows us to fill new ones while the last one is still beeing
 * processed. Maybe it's just that the disk is too fast though... It
 * would then be worth considering:
 * - turning the ATA task into a single ptr BD (double ptr BD is useless
 *   waste of memory in our case unless we want to use DMA for ATAPI
 *   command writes, but the kernel IDE layer isn't too much ready for
 *   us to do that in a single operation)
 * - increasing the max amount of BDs the ATA task may use
 */
#undef COUNT_BUFFS_PER_IRQ

static void mpc5xxx_ide_sdma_irq(int irq, void *dev_id, struct pt_regs * regs)
{
	ide_hwif_t *hwif = dev_id;
#ifdef COUNT_BUFFS_PER_IRQ
	int i = 0;
#endif

	DPRINTK("SDMA irq\n");
	dump_bds();
	do {
		TaskBD2_t *bd = (TaskBD2_t *)TaskGetBD(mpc5xxx_ata_sdma_task, mpc5xxx_ata_sdma_nextbd);
		if (bd->Status != 0)
			break;
		TaskBDRelease (mpc5xxx_ata_sdma_task);
		DPRINTK("released: %d\n", mpc5xxx_ata_sdma_nextbd);
		mpc5xxx_ata_sdma_nextbd = (mpc5xxx_ata_sdma_nextbd + 1) % MAX_DMA_BUFFERS;
		mpc5xxx_ata_free_bds++;
#ifdef COUNT_BUFFS_PER_IRQ
		i++;
#endif
	} while(mpc5xxx_ata_free_bds < MAX_DMA_BUFFERS);
#ifdef COUNT_BUFFS_PER_IRQ
	if (i < 4)
		printk("<%d>\n", i);
#endif
	mpc5xxx_ide_continue_dma(hwif);

	dump_bds();
}

static int mpc5xxx_ide_dma_read(ide_drive_t *drive)
{
	struct request *rq = HWGROUP(drive)->rq;
	int which = drive->select.b.unit & 0x01;
	u8 lba48 = (drive->addressing == 1) ? 1 : 0;
	task_ioreg_t command = WIN_NOP;
	u8 dma_mode = MPC5xxx_ATA_DMAMODE_IE | MPC5xxx_ATA_DMAMODE_READ | MPC5xxx_ATA_DMAMODE_FE;

	if (!mpc5xxx_ide_build_dmatable(drive, rq, PCI_DMA_FROMDEVICE))
		/* try PIO instead of DMA */
		return 1;

	/* Setup FIFO if direction changed */
	if (mpc5xxx_ata_dma_last_write) {
		mpc5xxx_ata_dma_last_write = 0;
		WAIT_TIP_BIT_CLEAR(IDE_COMMAND_REG);
		out_8(&mpc5xxx_ataregs->dma_mode, MPC5xxx_ATA_DMAMODE_FR);
		/* Configure it with granularity to 7 like sample code */
		out_8(&mpc5xxx_ataregs->fifo_control, 7);
		out_be16(&mpc5xxx_ataregs->fifo_alarm, 32);
	}
	if (mpc5xxx_timings[which].using_udma)
		dma_mode |= MPC5xxx_ATA_DMAMODE_UDMA;

	WAIT_TIP_BIT_CLEAR(IDE_COMMAND_REG);
	out_8(&mpc5xxx_ataregs->dma_mode, dma_mode);

	DPRINTK("SDMA starting read (dmamode: %x lba48: %d)\n", dma_mode, lba48);

	/* Start DMA. XXX FIXME: See if we can move that to ide_dma_begin */
	TaskStart(mpc5xxx_ata_sdma_task, TASK_AUTOSTART_ENABLE, mpc5xxx_ata_sdma_task, TASK_INTERRUPT_ENABLE);

	drive->waiting_for_dma = 1;

	if (drive->media != ide_disk)
		return 0;

	command = (lba48) ? WIN_READDMA_EXT : WIN_READDMA;
	if (rq->cmd == IDE_DRIVE_TASKFILE) {
		ide_task_t *args = rq->special;
		command = args->tfRegister[IDE_COMMAND_OFFSET];
	}
	 /* issue cmd to drive */
	ide_execute_command(drive, command, &ide_dma_intr, 2*WAIT_CMD, NULL);

	return HWIF(drive)->ide_dma_begin(drive);
}

static int mpc5xxx_ide_dma_write(ide_drive_t *drive)
{
	struct request *rq = HWGROUP(drive)->rq;
	int which = drive->select.b.unit & 0x01;
	u8 lba48 = (drive->addressing == 1) ? 1 : 0;
	task_ioreg_t command = WIN_NOP;
	u8 dma_mode = MPC5xxx_ATA_DMAMODE_IE | MPC5xxx_ATA_DMAMODE_WRITE;

	if (!mpc5xxx_ide_build_dmatable(drive, rq, PCI_DMA_TODEVICE))
		/* try PIO instead of DMA */
		return 1;

	/* Setup FIFO if direction changed */
	if (!mpc5xxx_ata_dma_last_write) {
		mpc5xxx_ata_dma_last_write = 1;
		/* Configure FIFO with granularity to 4 like sample code */
		out_8(&mpc5xxx_ataregs->fifo_control, 4);
		out_be16(&mpc5xxx_ataregs->fifo_alarm, 32);
	}

	if (mpc5xxx_timings[which].using_udma)
		dma_mode |= MPC5xxx_ATA_DMAMODE_UDMA;

	WAIT_TIP_BIT_CLEAR(IDE_COMMAND_REG);
	out_8(&mpc5xxx_ataregs->dma_mode, dma_mode);

	DPRINTK("SDMA starting write (dmamode: %x lba48: %d)\n", dma_mode, lba48);

	/* Start DMA. XXX FIXME: See if we can move that to ide_dma_begin */
	TaskStart(mpc5xxx_ata_sdma_task, TASK_AUTOSTART_ENABLE, mpc5xxx_ata_sdma_task, TASK_INTERRUPT_ENABLE);

	drive->waiting_for_dma = 1;

	if (drive->media != ide_disk)
		return 0;

	command = (lba48) ? WIN_WRITEDMA_EXT : WIN_WRITEDMA;
	if (rq->cmd == IDE_DRIVE_TASKFILE) {
		ide_task_t *args = rq->special;
		command = args->tfRegister[IDE_COMMAND_OFFSET];
	}
	 /* issue cmd to drive */
	ide_execute_command(drive, command, &ide_dma_intr, 2*WAIT_CMD, NULL);

	return HWIF(drive)->ide_dma_begin(drive);
}

static int mpc5xxx_ide_dma_count(ide_drive_t *drive)
{
	return HWIF(drive)->ide_dma_begin(drive);
}

static int mpc5xxx_ide_dma_begin(ide_drive_t *drive)
{
	return 0;
}

static int mpc5xxx_ide_dma_end(ide_drive_t *drive)
{
	DPRINTK("SDMA end\n");

	drive->waiting_for_dma = 0;
	
	/* XXX TEST */
	//	out_8(&mpc5xxx_ataregs->dma_mode, MPC5xxx_ATA_DMAMODE_FR | MPC5xxx_ATA_DMAMODE_HUT);
	
	do {
		TaskBD2_t *bd = (TaskBD2_t *)TaskGetBD(mpc5xxx_ata_sdma_task, mpc5xxx_ata_sdma_nextbd);
		if (bd->Status != 0)
			printk(KERN_WARNING "%s: BD non free on command completion !\n", drive->name);
		TaskBDRelease (mpc5xxx_ata_sdma_task);
		mpc5xxx_ata_free_bds++;
	} while(mpc5xxx_ata_free_bds < MAX_DMA_BUFFERS);

	TaskStop(mpc5xxx_ata_sdma_task);

	return 0;
}

static int mpc5xxx_ide_dma_test_irq(ide_drive_t *drive)
{
	unsigned int timeout;
	while ((in_8(&mpc5xxx_ataregs->fifo_status) & MPC5xxx_ATA_FIFOSTAT_EMPTY) == 0) {
		if (timeout++ > 100000) {
			printk("ATA fifo empty timeout\n");
			dump_bds();
			return 1;
		}
		udelay(10);
	}
	return 1;
}

static int mpc5xxx_ide_dma_host_off(ide_drive_t *drive)
{
	return 0;
}

static int mpc5xxx_ide_dma_host_on(ide_drive_t *drive)
{
	return 0;
}

static inline void mpc5xxx_ide_toggle_bounce(ide_drive_t *drive, int on)
{
	u64 addr = BLK_BOUNCE_HIGH;	/* dma64_addr_t */

	if (on && drive->media == ide_disk)
		addr = 0xfffffffful;
	blk_queue_bounce_limit(&drive->queue, addr);
}

static int mpc5xxx_ide_dma_off_quietly (ide_drive_t *drive)
{
	drive->using_dma = 0;
	mpc5xxx_ide_toggle_bounce(drive, 0);
	return HWIF(drive)->ide_dma_host_off(drive);
}

int mpc5xxx_ide_dma_on (ide_drive_t *drive)
{
	drive->using_dma = 1;
	mpc5xxx_ide_toggle_bounce(drive, 1);
	return HWIF(drive)->ide_dma_host_on(drive);
}

static int mpc5xxx_ide_dma_lostirq(ide_drive_t *drive)
{
	printk(KERN_ERR "%s: lost interrupt\n", drive->name);
	dump_bds();
	return 0;
}

#endif /* MPC5xxx_IDE_HAS_DMA */


ide_ioreg_t mpc5xxx_ide_get_base(int index)
{
	if (mpc5xxx_ataregs == NULL || index != mpc5xxx_hwif_index)
		return 0;
	return (ide_ioreg_t)&mpc5xxx_ataregs->tf_data;
}

void mpc5xxx_ide_init_hwif_ports(hw_regs_t *hw,
				 ide_ioreg_t data_port, ide_ioreg_t ctrl_port,
				 int *irq)
{
	int i;

	if (data_port == 0)
		return;

	if (data_port != (ide_ioreg_t)&mpc5xxx_ataregs->tf_data) {
		/* Probably a PCI interface... */
		for (i = IDE_DATA_OFFSET; i <= IDE_STATUS_OFFSET; ++i)
			hw->io_ports[i] = data_port + i - IDE_DATA_OFFSET;
		hw->io_ports[IDE_CONTROL_OFFSET] = ctrl_port;
		return;
	}

       	hw->io_ports[IDE_DATA_OFFSET] = (ide_ioreg_t)&mpc5xxx_ataregs->tf_data;
	hw->io_ports[IDE_ERROR_OFFSET] = (ide_ioreg_t)&mpc5xxx_ataregs->tf_features;
       	hw->io_ports[IDE_NSECTOR_OFFSET] = (ide_ioreg_t)&mpc5xxx_ataregs->tf_sec_count;
       	hw->io_ports[IDE_SECTOR_OFFSET] = (ide_ioreg_t)&mpc5xxx_ataregs->tf_sec_num;
       	hw->io_ports[IDE_LCYL_OFFSET] = (ide_ioreg_t)&mpc5xxx_ataregs->tf_cyl_low;
       	hw->io_ports[IDE_HCYL_OFFSET] = (ide_ioreg_t)&mpc5xxx_ataregs->tf_cyl_high;
       	hw->io_ports[IDE_SELECT_OFFSET] = (ide_ioreg_t)&mpc5xxx_ataregs->tf_dev_head;
       	hw->io_ports[IDE_STATUS_OFFSET] = (ide_ioreg_t)&mpc5xxx_ataregs->tf_command;
       	hw->io_ports[IDE_CONTROL_OFFSET] = (ide_ioreg_t)&mpc5xxx_ataregs->tf_control;

	if (irq)
		*irq = MPC5xxx_ATA_IRQ;
}

static int mpc5xxx_ide_setup(void)
{
	struct mpc5xxx_gpio *gpio_regs = (struct mpc5xxx_gpio *)MPC5xxx_GPIO;
	struct mpc5xxx_sdma *sdma_regs = (struct mpc5xxx_sdma *)MPC5xxx_SDMA;
	u32 reg;

	/*
	 * Check port configuration & enable IDE chip selects
	 */
	reg = in_be32(&gpio_regs->port_config);
	printk(/*KERN_DEBUG*/ "Port Config is: 0x%08x\n", reg);
	reg = (reg & 0xfcfffffful) | 0x01000000ul;
	out_be32(&gpio_regs->port_config, reg);

	/*
	 * All sample codes do that...
	 */
	out_be32(&mpc5xxx_ataregs->invalid, 0);

	/*
	 * Configure & reset host
	 */
	out_be32(&mpc5xxx_ataregs->config, MPC5xxx_ATA_HOSTCONF_IE | MPC5xxx_ATA_HOSTCONF_IORDY
					| MPC5xxx_ATA_HOSTCONF_SMR | MPC5xxx_ATA_HOSTCONF_FR);
	udelay(10);
	out_be32(&mpc5xxx_ataregs->config, MPC5xxx_ATA_HOSTCONF_IE | MPC5xxx_ATA_HOSTCONF_IORDY);	

	/*
	 * Disable prefetch on commbus
	 */
	out_be16(&sdma_regs->PtdCntrl, in_be16(&sdma_regs->PtdCntrl) | 0x0001);

	mpc5xxx_timings[0].pio1 = 0x100a0a00;
	mpc5xxx_timings[0].pio2 = 0x02040600;
	mpc5xxx_timings[1].pio1 = 0x100a0a00;
	mpc5xxx_timings[1].pio2 = 0x02040600;
       	out_be32(&mpc5xxx_ataregs->pio1, 0x100a0a00);
       	out_be32(&mpc5xxx_ataregs->pio2, 0x02040600);
     
	printk("GPIO config: %08x\n", in_be32((u32 *)0xf0000b00));
	printk("ATA invalid: %08x\n", in_be32((u32 *)0xf0003a2c));
	printk("ATA hostcnf: %08x\n", in_be32((u32 *)0xf0003a00));
	printk("ATA pio1   : %08x\n", in_be32((u32 *)0xf0003a08));
	printk("ATA pio2   : %08x\n", in_be32((u32 *)0xf0003a0c));
	printk("XLB Arb cnf: %08x\n", in_be32((u32 *)0xf0001f40));

	// XXX FIXME: 7 for 133Mhz IPBI, 15 for 66Mhz
	mpc5xxx_clock_period = 7;
	return 0;
}

void mpc5xxx_ide_probe(void)
{
	int i;
	ide_hwif_t *hwif;
#if MPC5xxx_IDE_HAS_DMA
	static TaskSetupParamSet_t ata_setup;
#endif

	for (i = 0; i < MAX_HWIFS && ide_hwifs[0].io_ports[IDE_DATA_OFFSET] != 0;)
		i++;
	if (i >= MAX_HWIFS) {
		printk(KERN_ERR "mpc5xxx_ide: No free hwif slot !\n");
		return;
	}

	mpc5xxx_hwif_index = i;
	mpc5xxx_ataregs = (struct mpc5xxx_ata *)MPC5xxx_ATA;

	if (mpc5xxx_ide_setup()) {
		printk(KERN_ERR "mpc5xxx_ide: Setting up interface failed !\n");
		return;
	}

	printk("mpc5xxx_ide: Setting up IDE interface ide%d...\n", i);

	hwif = &ide_hwifs[i];
	setup_hwif_mpc5xxx_iops(hwif);
	hwif->mmio = 2;
	hwif->irq = MPC5xxx_ATA_IRQ;
	mpc5xxx_ide_init_hwif_ports(&hwif->hw,
				  (ide_ioreg_t)&mpc5xxx_ataregs->tf_data, 0, &hwif->irq);
	memcpy(hwif->io_ports, hwif->hw.io_ports, sizeof(hwif->io_ports));
	hwif->chipset = ide_mpc5xxx;
	hwif->noprobe = 0;
	hwif->udma_four = 0;
	hwif->addressing = 1; /* LBA48 not supported */
	hwif->drives[0].unmask = hwif->drives[1].unmask = 1;
	hwif->tuneproc = mpc5xxx_ide_tuneproc;
	hwif->speedproc = mpc5xxx_ide_speedproc;
	hwif->selectproc = mpc5xxx_ide_apply_timings;
	hwif->atapi_dma = 0;
	hwif->swdma_mask = 0x00;
	hwif->mwdma_mask = 0x00;
	hwif->ultra_mask = 0x00;

#if MPC5xxx_IDE_HAS_DMA
	/*
	 * Setup DMA if enabled. We can "hijack" hwif->sg_table and hwif->dma_table_cpu
	 * provided that we leave hwif->dma_base clear. If we don't leave it clear, the
	 * ide common code will try to use it's own free() routines on these and will
	 * break as it assumes standard PRD table.
	 */
	hwif->sg_table = kmalloc(sizeof(struct scatterlist) * MAX_DMA_BUFFERS, GFP_KERNEL);
	if (hwif->sg_table == NULL)
		goto no_dma;
   
	/*
	 * Setup dummy ATA task for writing so we get the task number for requesting
	 * the interrupt
	 */
	mpc5xxx_sdma_load_tasks_image();

	ata_setup.NumBD = MAX_DMA_BUFFERS;
	ata_setup.Size.MaxBuf = 0xffe0;
	ata_setup.Initiator = INITIATOR_ATA_TX;
	ata_setup.StartAddrDst = (u32)&mpc5xxx_ataregs->fifo_data;
	ata_setup.StartAddrSrc = 0;
	ata_setup.IncrDst = 0;
	ata_setup.IncrSrc = 4;
	ata_setup.SzDst = 4;
	ata_setup.SzSrc = 4;
	mpc5xxx_ata_sdma_task = TaskSetup (TASK_ATA, &ata_setup);
	mpc5xxx_ata_dma_last_write = 1;

	printk("ATA DMA task: %d\n", mpc5xxx_ata_sdma_task);

	if (request_irq(MPC5xxx_SDMA_IRQ_BASE + mpc5xxx_ata_sdma_task, mpc5xxx_ide_sdma_irq,
			SA_INTERRUPT, "ide dma", hwif)) {
		printk(KERN_ERR "mpc5xxx_ide: SDMA irq allocation failed\n");
		goto no_dma;
	}

	hwif->atapi_dma = 1;
	hwif->autodma = 1;
#ifdef CONFIG_BLK_DEV_IDE_MPC5xxx_MDMA
	hwif->mwdma_mask = 0x07;
#endif
#ifdef CONFIG_BLK_DEV_IDE_MPC5xxx_UDMA
	hwif->ultra_mask = 0x07;
#endif
	hwif->ide_dma_off = &__ide_dma_off;
	hwif->ide_dma_off_quietly = &mpc5xxx_ide_dma_off_quietly;
	hwif->ide_dma_on = &mpc5xxx_ide_dma_on;
	hwif->ide_dma_check = &mpc5xxx_ide_dma_check;
	hwif->ide_dma_read = &mpc5xxx_ide_dma_read;
	hwif->ide_dma_write = &mpc5xxx_ide_dma_write;
	hwif->ide_dma_count = &mpc5xxx_ide_dma_count;
	hwif->ide_dma_begin = &mpc5xxx_ide_dma_begin;
	hwif->ide_dma_end = &mpc5xxx_ide_dma_end;
	hwif->ide_dma_test_irq = &mpc5xxx_ide_dma_test_irq;
	hwif->ide_dma_host_off = &mpc5xxx_ide_dma_host_off;
	hwif->ide_dma_host_on = &mpc5xxx_ide_dma_host_on;
	hwif->ide_dma_good_drive = &__ide_dma_good_drive;
	hwif->ide_dma_bad_drive = &__ide_dma_bad_drive;
	hwif->ide_dma_verbose = &__ide_dma_verbose;
	hwif->ide_dma_timeout = &__ide_dma_timeout;
	hwif->ide_dma_retune = &__ide_dma_retune;
	hwif->ide_dma_lostirq = &mpc5xxx_ide_dma_lostirq;
 no_dma:
#endif /* MPC5xxx_IDE_HAS_DMA */
	;
}
