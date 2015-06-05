/*
 *    Copyright 2002 MontaVista Software Inc.
 *      Completed implementation.
 *      Author: Armin Kuster <akuster@mvista.com>
 *      MontaVista Software, Inc.  <source@mvista.com>
 *
 *    Module name: ocp_stbxxxx.c
 *
 *    Description:
 *
 *    Based on stb03xxx.c
 *
 *    Version 07/23/02 - Armin
 *    removed many mtdcr/mfdcr dma calls to standard 4xx dma calls
 */

#include <linux/types.h>
#include <linux/hdreg.h>
#include <linux/delay.h>
#include <linux/ide.h>

#include <asm/io.h>
#include <asm/ppc4xx_dma.h>

#include "ide_modes.h"

#define IDEVR			"1.3"
ppc_dma_ch_t dma_ch;

/* use DMA channel 2 for IDE DMA operations */
#define IDE_DMACH	2	/* 2nd DMA channel */
#define IDE_DMA_INT	6	/* IDE dma channel 2 interrupt */

#define WMODE	0 		/* default to DMA line mode */
#define PIOMODE	0

/* psc=00, pwc=000001 phc=010, resvd-must-be-one=1 */

unsigned long dmacr_def_line = 0x00002A02;

/* psc=00, pwc=000110 phc=010, resvd-must-be-one=1 */

unsigned long dmacr_def_word = 0x0000CA02;

#ifdef CONFIG_REDWOOD_4
#define DCRXBCR_MDMA2		0xC0000000
#else /* CONFIG_REDWOOD_6 */
#define DCRXBCR_MDMA2		0x80000000
#endif

#define DCRXBCR_WRITE		0x20000000
#define DCRXBCR_ACTIVATE	0x10000000

#ifdef CONFIG_REDWOOD_4
#define IDE_CMD_OFF		0x00100000
#define IDE_CTL_OFF		0x00100000
#endif

/* Function Prototypes */
static void redwood_ide_tune_drive(ide_drive_t *, byte);
static byte redwood_ide_dma_2_pio(byte);
static int redwood_ide_tune_chipset(ide_drive_t *, byte);

#ifdef DEBUG
static void dump_dcrs(void)
{
	printk("DMASR=%x\n", mfdcr(DCRN_DMASR));
	printk("DMACR2=%x\n", mfdcr(DCRN_DMACR2));
	printk("DMACT2=%d\n", mfdcr(DCRN_DMACT2));
	printk("DMAS2=%x\n", mfdcr(DCRN_DMAS2));
	printk("DMASA2=%x\n", mfdcr(DCRN_DMASA2));
	printk("DMADA2=%x\n", mfdcr(DCRN_DMADA2));

	if (mfdcr(DCRN_DMASR) & 0x00200000) {
		printk("BESR=%x\n", mfdcr(DCRN_BESR));
		printk("BEAR=%x\n", mfdcr(DCRN_BEAR));
		printk("PLB0_BESR=%x\n", mfdcr(DCRN_PLB0_BESR));
		printk("PLB0_BEAR=%x\n", mfdcr(DCRN_PLB0_BEAR));
		printk("PLB1_BESR=%x\n", mfdcr(DCRN_PLB1_BESR));
		printk("PLB1_BEAR=%x\n", mfdcr(DCRN_PLB1_BEAR));
		printk("OPB0_BESR0=%x\n", mfdcr(DCRN_POB0_BESR0));
		printk("OPB0_BEAR=%x\n", mfdcr(DCRN_POB0_BEAR));
		printk("SDRAM0_BESR=%x\n", mfdcr(0x1E1));
		printk("SDRAM0_BEAR=%x\n", mfdcr(0x1E2));
		printk("SDRAM1_BESR=%x\n", mfdcr(0x1C1));
		printk("SDRAM1_BEAR=%x\n", mfdcr(0x1C2));
	}

	return;
}
#endif /* DEBUG */

static void
redwood_ide_tune_drive(ide_drive_t * drive, byte pio)
{
	pio = ide_get_best_pio_mode(drive, pio, 5, NULL);
}

static byte
redwood_ide_dma_2_pio(byte xfer_rate)
{
	switch (xfer_rate) {
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
	case XFER_PIO_1:
	case XFER_PIO_0:
	case XFER_PIO_SLOW:
	default:
		return 0;
	}
}

static int
redwood_ide_tune_chipset(ide_drive_t * drive, byte speed)
{
	int err = 0;

	redwood_ide_tune_drive(drive, redwood_ide_dma_2_pio(speed));

	if (!drive->init_speed)
		drive->init_speed = speed;
	err = ide_config_drive_speed(drive, speed);
	drive->current_speed = speed;
	return err;
}

#ifdef CONFIG_BLK_DEV_IDEDMA
static int redwood_config_drive_for_dma(ide_drive_t *drive)
{
	struct hd_driveid *id = drive->id;
	ide_hwif_t *hwif = HWIF(drive);

	if (id && (id->capability & 1) && hwif->autodma) {
		/*
		 * Enable DMA on any drive that has
		 * UltraDMA (mode 0/1/2/3/4/5/6) enabled
		 */
		if ((id->field_valid & 4) && ((id->dma_ultra >> 8) & 0x7f))
			return hwif->ide_dma_on(drive);
		/*
		 * Enable DMA on any drive that has mode2 DMA
		 * (multi or single) enabled
		 */
		if (id->field_valid & 2)	/* regular DMA */
			if ((id->dma_mword & 0x404) == 0x404 ||
			    (id->dma_1word & 0x404) == 0x404)
				return hwif->ide_dma_on(drive);
	}

	return hwif->ide_dma_off_quietly(drive);
}

ide_startstop_t
redwood_ide_intr(ide_drive_t * drive)
{
	int i;
	byte dma_stat;
	unsigned int nsect;
	ide_hwgroup_t *hwgroup = HWGROUP(drive);
	struct request *rq = hwgroup->rq;
	unsigned long block, b1, b2, b3, b4;

	nsect = rq->current_nr_sectors;

	dma_stat = HWIF(drive)->ide_dma_end(drive);

	rq->sector += nsect;
	rq->buffer += nsect << 9;
	rq->errors = 0;
	i = (rq->nr_sectors -= nsect);
	ide_end_request(drive, 1);
	if (i > 0) {
		b1 = HWIF(drive)->INB(IDE_SECTOR_REG);
		b2 = HWIF(drive)->INB(IDE_LCYL_REG);
		b3 = HWIF(drive)->INB(IDE_HCYL_REG);
		b4 = HWIF(drive)->INB(IDE_SELECT_REG);
		block = ((b4 & 0x0f) << 24) + (b3 << 16) + (b2 << 8) + (b1);
		block++;
		if (drive->select.b.lba) {
			HWIF(drive)->OUTB(block, IDE_SECTOR_REG);
			HWIF(drive)->OUTB(block >>= 8, IDE_LCYL_REG);
			HWIF(drive)->OUTB(block >>= 8, IDE_HCYL_REG);
			HWIF(drive)->OUTB(((block >> 8) & 0x0f) | drive->select.all,
				 IDE_SELECT_REG);
		} else {
			unsigned int sect, head, cyl, track;
			track = block / drive->sect;
			sect = block % drive->sect + 1;
			HWIF(drive)->OUTB(sect, IDE_SECTOR_REG);
			head = track % drive->head;
			cyl = track / drive->head;
			HWIF(drive)->OUTB(cyl, IDE_LCYL_REG);
			HWIF(drive)->OUTB(cyl >> 8, IDE_HCYL_REG);
			HWIF(drive)->OUTB(head | drive->select.all, IDE_SELECT_REG);
		}

		if (rq->cmd == READ)
			dma_stat = HWIF(drive)->ide_dma_read(drive);
		else
			dma_stat = HWIF(drive)->ide_dma_write(drive);
		return ide_started;
	}
	return ide_stopped;
}

static int redwood_dma_timer_expiry(ide_drive_t *drive)
{
	ide_hwif_t *hwif	= HWIF(drive);
	u8 dma_stat		= hwif->INB(hwif->dma_status);

	printk(KERN_WARNING "%s: dma_timer_expiry: dma status == 0x%02x\n",
		drive->name, dma_stat);

	if ((dma_stat & 0x18) == 0x18)	/* BUSY Stupid Early Timer !! */
		return WAIT_CMD;

	HWGROUP(drive)->expiry = NULL;	/* one free ride for now */

	/* 1 dmaing, 2 error, 4 intr */
	
	if (dma_stat & 2) {	/* ERROR */
		(void) hwif->ide_dma_end(drive);
		return DRIVER(drive)->error(drive,
			"dma_timer_expiry", hwif->INB(IDE_STATUS_REG));
	}
	if (dma_stat & 1)	/* DMAing */
		return WAIT_CMD;

	if (dma_stat & 4)	/* Got an Interrupt */
		HWGROUP(drive)->handler(drive);

	return 0;
}

static int redwood_ide_dma_end(ide_drive_t *drive)
{
	drive->waiting_for_dma = 0;

	/* disable DMA */
	ppc4xx_disable_dma_interrupt(IDE_DMACH);
	ppc4xx_disable_dma(IDE_DMACH);
	return 0;
}

static int redwood_ide_dma_off_quietly(ide_drive_t *drive)
{
	drive->using_dma = 0;
	return redwood_ide_dma_end(drive);
}

static int redwood_ide_dma_off(ide_drive_t *drive)
{
	printk(KERN_INFO "%s: DMA disabled\n", drive->name);
	return redwood_ide_dma_off_quietly(drive);
}

static int redwood_ide_dma_on(ide_drive_t *drive)
{
	mtdcr(DCRN_DMACR2, 0);
	ppc4xx_clr_dma_status(IDE_DMACH);

#if WMODE
	mtdcr(DCRN_DCRXBCR, 0);
	mtdcr(DCRN_CICCR, mfdcr(DCRN_CICCR) | 0x00000400);
#else
	/* Configure CIC reg for line mode dma */
	mtdcr(DCRN_CICCR, mfdcr(DCRN_CICCR) & ~0x00000400);
#endif
        drive->using_dma = 1;
	return 0;
}

static int redwood_ide_dma_check(ide_drive_t *drive)
{
	return redwood_config_drive_for_dma(drive);
}

static int redwood_ide_dma_begin(ide_drive_t *drive)
{
	/* enable DMA */

	ppc4xx_enable_dma_interrupt(IDE_DMACH);
	ppc4xx_enable_dma(IDE_DMACH);
	return 0;
}

static int redwood_ide_dma_io(ide_drive_t *drive, int reading)
{
	ide_hwif_t *hwif = HWIF(drive);
	struct request *rq = HWGROUP(drive)->rq;
	unsigned long length;

        if (drive->media != ide_disk)
                return 0;

	if (ppc4xx_get_channel_config(IDE_DMACH, &dma_ch) & DMA_CHANNEL_BUSY )	/* DMA is busy? */
		return -1;

	if (reading) {
		dma_cache_inv((unsigned long) rq->buffer,
			      rq->current_nr_sectors * 512);

#if WMODE
		ppc4xx_set_src_addr(IDE_DMACH, 0);
		ppc4xx_set_dma_addr(IDE_DMACH, virt_to_bus(rq->buffer));
#else
		ppc4xx_set_src_addr(IDE_DMACH, IDE_DMA_ADDR);
		ppc4xx_set_dst_addr(IDE_DMACH, virt_to_bus(rq->buffer));
#endif
	} else {
		dma_cache_wback_inv((unsigned long) rq->buffer,
				    rq->current_nr_sectors * 512);
#if WMODE
		ppc4xx_set_dma_addr(IDE_DMACH, virt_to_bus(rq->buffer));
		ppc4xx_set_dst_addr(IDE_DMACH, 0);
#else
		ppc4xx_set_src_addr(IDE_DMACH, virt_to_bus(rq->buffer));
		ppc4xx_set_dst_addr(IDE_DMACH, IDE_DMA_ADDR);
#endif
	}

	hwif->OUTB(rq->current_nr_sectors, IDE_NSECTOR_REG);
	length = rq->current_nr_sectors * 512;

	/* set_dma_count doesn't do M2M line xfer sizes right. */

#if WMODE
	mtdcr(DCRN_DMACT2, length >> 2);
#else
	mtdcr(DCRN_DMACT2, length >> 4);
#endif

	if (reading) {
#if WMODE
	  mtdcr(DCRN_DMACR2, DMA_TD | 
		SET_DMA_TM(TM_PERIPHERAL) | 
		SET_DMA_PW(PW_16) | 
		SET_DMA_DAI(1) | dmacr_def_word);
	  set_dma_mode(IDE_DMACH, DMA_MODE_READ);

#else
	  mtdcr(DCRN_DCRXBCR, DCRXBCR_MDMA2 | DCRXBCR_ACTIVATE);
	  mtdcr(DCRN_DMACR2, SET_DMA_DAI(1) | SET_DMA_SAI(0) |
		DMA_MODE_MM_DEVATSRC | SET_DMA_PW(PW_64) |
		dmacr_def_line);
	  ppc4xx_set_dma_mode(IDE_DMACH, DMA_MODE_MM_DEVATSRC);
#endif
	} else {
#if WMODE
	  mtdcr(DCRN_DMACR2, SET_DMA_TM(TM_PERIPHERAL) | 
		SET_DMA_PW(PW_16) | 
		SET_DMA_DAI(1) | dmacr_def_word);
	  ppc4xx_set_dma_mode(IDE_DMACH, DMA_MODE_WRITE);
#else
	  mtdcr(DCRN_DCRXBCR, DCRXBCR_WRITE | DCRXBCR_MDMA2 | 
		DCRXBCR_ACTIVATE);
	  mtdcr(DCRN_DMACR2,SET_DMA_DAI(0) | SET_DMA_SAI(1) | 
		DMA_MODE_MM_DEVATDST| SET_DMA_PW(PW_64) | 
		dmacr_def_line);
	  ppc4xx_set_dma_mode(IDE_DMACH, DMA_MODE_MM_DEVATDST);
#endif
	}

	drive->waiting_for_dma = 1;
	ide_set_handler(drive, &redwood_ide_intr, 2*WAIT_CMD, 
			redwood_dma_timer_expiry);
	hwif->OUTB(reading ? WIN_READDMA : WIN_WRITEDMA, IDE_COMMAND_REG);
	return HWIF(drive)->ide_dma_begin(drive);
}

static int redwood_ide_dma_read(ide_drive_t *drive)
{
	return redwood_ide_dma_io(drive, 1);
}

static int redwood_ide_dma_write(ide_drive_t *drive)
{
	return redwood_ide_dma_io(drive, 0);
}

/* returns 1 if dma irq issued, 0 otherwise */
static int redwood_ide_dma_test_irq(ide_drive_t *drive)
{
	return 1;
}

static int redwood_ide_dma_verbose(ide_drive_t * drive)
{
        struct hd_driveid *id = drive->id;

        if (id->field_valid & 2) {
                if (id->dma_mword & 0x0004) {
                        printk(", mdma2");
                } else if (id->dma_mword & 0x0002) {
                        printk(", mdma1");
                } else if (id->dma_mword & 1) {
                        printk(", mdma0");
                } else if (id->dma_1word & 0x0004) {
                        printk(", sdma2");
                } else {
                        printk(", pio%d",
                               ide_get_best_pio_mode(drive, 255, 5, NULL));
                }
        }

        return 0;
}

#endif

void
ibm4xx_ide_spinup(int index)
{
	int i;
	ide_ioreg_t *io_ports;

	printk("ide_redwood: waiting for drive ready..");
	io_ports = ide_hwifs[index].io_ports;

	/* wait until drive is not busy (it may be spinning up) */
	for (i = 0; i < 30; i++) {
		unsigned char stat;
		stat = inb_p(io_ports[7]);
		/* wait for !busy & ready */
		if ((stat & 0x80) == 0) {
			break;
		}

		udelay(1000 * 1000);	/* 1 second */
	}

	printk("..");

	/* select slave */
	outb_p(0xa0 | 0x10, io_ports[6]);

	for (i = 0; i < 30; i++) {
		unsigned char stat;
		stat = inb_p(io_ports[7]);
		/* wait for !busy & ready */
		if ((stat & 0x80) == 0) {
			break;
		}

		udelay(1000 * 1000);	/* 1 second */
	}

	printk("..");

	outb_p(0xa0, io_ports[6]);
	printk("Drive spun up \n");
}

int
nonpci_ide_default_irq(ide_ioreg_t base)
{
	return IDE0_IRQ;
}

void
nonpci_ide_init_hwif_ports(hw_regs_t * hw, ide_ioreg_t data_port,
			   ide_ioreg_t ctrl_port, int *irq)
{
	unsigned long ioaddr;
#ifdef CONFIG_REDWOOD_4
	unsigned long reg = data_port;
	unsigned long xilinx;
#endif
	int i, index;
	ide_hwif_t *hwif;

	printk("IBM Redwood 4/6 IDE driver version %s\n", IDEVR);

	if (!request_region(REDWOOD_IDE_CMD, 0x10, "IDE"))
		return;

	if (!request_region(REDWOOD_IDE_CTRL, 2, "IDE")) {
		release_region(REDWOOD_IDE_CMD, 0x10);
		return;
	}

#ifdef CONFIG_REDWOOD_4
	mtdcr(DCRN_DCRXICR, 0x40000000);	/* set dcrx internal arbiter */

	/* add RE & OEN to value set by boot rom */
	mtdcr(DCRN_BRCR3, 0x407cfffe);

	/* reconstruct phys addrs from EBIU config regs for CS2# */
	reg = ((mfdcr(DCRN_BRCR2) & 0xff000000) >> 4) | 0xf0000000;
	xilinx = reg | 0x00040000;
	reg = reg | IDE_CMD_OFF;

	ioaddr = (unsigned long)ioremap(reg, 0x10);
	xilinx = (unsigned long)ioremap(xilinx, 0x10);

	i=readw(xilinx);
	if(i & 0x0001) {
		writew( i & ~0x8001, xilinx);
		writew( 0, xilinx+7*2);
		udelay(10*1000);	/* 10 ms */
	}

	/* init xilinx control registers - enable ide mux, clear reset bit */
	writew( i | 0x8001, xilinx);
	writew( 0, xilinx+7*2);

#else /* CONFIG_REDWOOD_6 */
	ioaddr = (unsigned long) ioremap(REDWOOD_IDE_CMD, 0x10);
#endif

	hw->irq = IDE0_IRQ;

	for (i = IDE_DATA_OFFSET; i <= IDE_STATUS_OFFSET; i++) {
		hw->io_ports[i] = ioaddr;
		ioaddr += 2;
	}
	hw->io_ports[IDE_CONTROL_OFFSET] =
	    (unsigned long) ioremap(REDWOOD_IDE_CTRL, 2);

	/* use DMA channel 2 for IDE DMA operations */
	hw->dma = IDE_DMACH;
#if WMODE
	/*Word Mode psc(11-12)=00,pwc(13-18)=000110, phc(19-21)=010, 22=1, 30=1  ----  0xCB02*/

	dma_ch.mode	=DMA_MODE_READ;	  /* xfer from peripheral to mem */
	dma_ch.td	= 1;
	dma_ch.buffer_enable = 0;
	dma_ch.tce_enable = 0;
	dma_ch.etd_output = 0;
	dma_ch.pce = 0;
	dma_ch.pl = EXTERNAL_PERIPHERAL;    /* no op */
	dma_ch.pwidth = PW_16;
	dma_ch.dai = 1;
	dma_ch.sai = 0;
	dma_ch.psc = 0;                      /* set the max setup cycles */
	dma_ch.pwc = 6;                     /* set the max wait cycles  */
	dma_ch.phc = 2;                      /* set the max hold cycles  */
	dma_ch.cp = PRIORITY_LOW;
	dma_ch.int_enable = 0;
	dma_ch.ch_enable = 0;		/* No chaining */
	dma_ch.tcd_disable = 1;		/* No chaining */
#else
/*Line Mode psc(11-12)=00,pwc(13-18)=000001, phc(19-21)=010, 22=1, 30=1  ----  0x2B02*/

	dma_ch.mode	=DMA_MODE_MM_DEVATSRC;	  /* xfer from peripheral to mem */
	dma_ch.td	= 1;
	dma_ch.buffer_enable = 0;
	dma_ch.tce_enable = 0;
	dma_ch.etd_output = 0;
	dma_ch.pce = 0;
	dma_ch.pl = EXTERNAL_PERIPHERAL;    /* no op */
	dma_ch.pwidth = PW_64;		/* Line mode on stbs */
	dma_ch.dai = 1;
	dma_ch.sai = 0;
	dma_ch.psc = 0;                      /* set the max setup cycles */
	dma_ch.pwc = 1;                     /* set the max wait cycles  */
	dma_ch.phc = 2;                      /* set the max hold cycles  */
	dma_ch.cp = PRIORITY_LOW;
	dma_ch.int_enable = 0;
	dma_ch.ch_enable = 0;		/* No chaining */
	dma_ch.tcd_disable = 1;		/* No chaining */
#endif

	if (ppc4xx_init_dma_channel(IDE_DMACH, &dma_ch) != DMA_STATUS_GOOD)
		return;

	ppc4xx_disable_dma_interrupt(IDE_DMACH);

	/* init CIC control reg to enable IDE interface PIO mode */
	mtdcr(DCRN_CICCR, (mfdcr(DCRN_CICCR) & 0xffff7bff) | 0x0003);

	/* 
	 * init CIC select2 reg to connect external DMA port 3 to internal
	 * DMA channel 2
	 */
	
	/* FIXME: EXT_DMA_3 is out of bounds for map_dma_port. */
	/* map_dma_port(IDE_DMACH,EXT_DMA_3,DMA_CHAN_2);  */
	mtdcr(DCRN_DMAS2, (mfdcr(DCRN_DMAS2) & 0xfffffff0) | 0x00000002);
	
	/* Verified BRCR7 already set per manual. */
	
	index = 0;
	hwif = &ide_hwifs[index];
	hwif->tuneproc = &redwood_ide_tune_drive;
	hwif->drives[0].autotune = 1;
#ifdef CONFIG_BLK_DEV_IDEDMA
	hwif->autodma = 1;
	hwif->ide_dma_off = &redwood_ide_dma_off;
	hwif->ide_dma_off_quietly = &redwood_ide_dma_off_quietly;
	hwif->ide_dma_host_off = &redwood_ide_dma_off_quietly;
	hwif->ide_dma_on = &redwood_ide_dma_on;
	hwif->ide_dma_host_on = &redwood_ide_dma_on;
	hwif->ide_dma_check = &redwood_ide_dma_check;
	hwif->ide_dma_read = &redwood_ide_dma_read;
	hwif->ide_dma_write = &redwood_ide_dma_write;
	hwif->ide_dma_begin = &redwood_ide_dma_begin;
	hwif->ide_dma_end = &redwood_ide_dma_end;
	hwif->ide_dma_test_irq = &redwood_ide_dma_test_irq;
	hwif->ide_dma_verbose = &redwood_ide_dma_verbose;
#endif
	hwif->speedproc = &redwood_ide_tune_chipset;
	hwif->noprobe = 0;
	
	memcpy(hwif->io_ports, hw->io_ports, sizeof (hw->io_ports));
	hwif->irq = hw->irq;
	ibm4xx_ide_spinup(index);
}

