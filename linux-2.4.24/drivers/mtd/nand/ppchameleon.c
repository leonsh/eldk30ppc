/*
 *  drivers/mtd/nand/ppchameleon.c
 *
 *  Copyright (C) 2003 DAVE Srl (info@wawnet.biz)
 *
 *  Derived from drivers/mtd/nand/edb7312.c
 *       Copyright (C) 2002 Marius Gröger (mag@sysgo.de)
 *
 * $Id: ppchameleon.c,v 1.0 2003/07/14 12:58:16 mag Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Overview:
 *   This is a device driver for the NAND flash device found on the
 *   PPChameleon board which utilizes the Samsung K9F5608U0B part. This is
 *   a 256Mibit (32MiB x 8 bits) NAND flash device.
 */

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <asm/io.h>
#include <platforms/PPChameleonEVB.h>

/* handy sizes */
#define SZ_1K                           0x00000400



/*
 * MTD structure for PPChameleon board
 */
static struct mtd_info *ppchameleon_mtd = NULL;

/*
 * Values specific to the EDB7312 board (used with EP7312 processor)
 */

/*
 * Module stuff
 */

static int ppchameleon_fio_pbase = CFG_NAND0_PADDR;
static int ppchameleon_pxdr = 0;
static int ppchameleon_pxddr = 0;

#ifdef MODULE
MODULE_PARM(ppchameleon_fio_pbase, "i");
MODULE_PARM(ppchameleon_pxdr, "i");
MODULE_PARM(ppchameleon_pxddr, "i");

__setup("ppchameleon_fio_pbase=",ppchameleon_fio_pbase);
__setup("ppchameleon_pxdr=",ppchameleon_pxdr);
__setup("ppchameleon_pxddr=",ppchameleon_pxddr);
#endif

#if ((defined CONFIG_MTD_PARTITIONS) || (defined CONFIG_MTD_PARTITIONS_MODULE))
/*
 * Define static partitions for flash device
 */
static struct mtd_partition partition_info[] = {
	{ name: "PPChameleon Nand Flash",
		  offset: 0,
		  size: 32*1024*1024 }
};
#define NUM_PARTITIONS 1

extern int parse_cmdline_partitions(struct mtd_info *master,
				    struct mtd_partition **pparts,
				    const char *mtd_id);
#endif


/*
 *	hardware specific access to control-lines
 */
static void ppchameleon_hwcontrol(int cmd)
{
	switch(cmd) {

	case NAND_CTL_SETCLE:
        	MACRO_NAND_CTL_SETCLE(CFG_NAND0_PADDR);
		break;
	case NAND_CTL_CLRCLE:
        	MACRO_NAND_CTL_CLRCLE(CFG_NAND0_PADDR);
		break;

	case NAND_CTL_SETALE:
        	MACRO_NAND_CTL_SETALE(CFG_NAND0_PADDR);
		break;
	case NAND_CTL_CLRALE:
        	MACRO_NAND_CTL_CLRALE(CFG_NAND0_PADDR);
		break;

	case NAND_CTL_SETNCE:
        	MACRO_NAND_ENABLE_CE(CFG_NAND0_PADDR);
		break;
	case NAND_CTL_CLRNCE:
        	MACRO_NAND_DISABLE_CE(CFG_NAND0_PADDR);
		break;
	}
}

/*
 *	read device ready pin
 */
static int ppchameleon_device_ready(void)
{
	return 1;
}

/*
 * Main initialization routine
 */
static int __init ppchameleon_init (void)
{
	struct nand_chip *this;
	const char *part_type = 0;
	int mtd_parts_nb = 0;
	struct mtd_partition *mtd_parts = 0;
	int ppchameleon_fio_base;
        
#ifdef CONFIG_MTD_DEBUG
	printk(  "[ppchameleon_init]\n");
#endif

	/* Allocate memory for MTD device structure and private data */
	ppchameleon_mtd = kmalloc(sizeof(struct mtd_info) +
			     sizeof(struct nand_chip),
			     GFP_KERNEL);
	if (!ppchameleon_mtd) {
		printk("Unable to allocate PPChameleon NAND MTD device structure.\n");
		return -ENOMEM;
	}

	/* map physical address */
	ppchameleon_fio_base = (unsigned long)ioremap(ppchameleon_fio_pbase, SZ_1K);
	if(!ppchameleon_fio_base) {
		printk("ioremap PPChameleon NAND flash failed\n");
		kfree(ppchameleon_mtd);
		return -EIO;
	}

	/* Get pointer to private data */
	this = (struct nand_chip *) (&ppchameleon_mtd[1]);

	/* Initialize structures */
	memset((char *) ppchameleon_mtd, 0, sizeof(struct mtd_info));
	memset((char *) this, 0, sizeof(struct nand_chip));

	/* Link the private data with the MTD structure */
	ppchameleon_mtd->priv = this;
        
        /* Thanx to U-Boot, no need to initialize GPIOs */
        /* FIXME */


	/* insert callbacks */
	this->IO_ADDR_R = ppchameleon_fio_base;
	this->IO_ADDR_W = ppchameleon_fio_base;
	this->hwcontrol = ppchameleon_hwcontrol;
	/*this->dev_ready = ppchameleon_device_ready;*/
        this->dev_ready = NULL;
	/* 15 us command delay time */
	this->chip_delay = 15;

	/* Scan to find existence of the device */
	if (nand_scan (ppchameleon_mtd)) {
		iounmap((void *)ppchameleon_fio_base);
		kfree (ppchameleon_mtd);
		return -ENXIO;
	}

	/* Allocate memory for internal data buffer */
	this->data_buf = kmalloc (sizeof(u_char) * (ppchameleon_mtd->oobblock + ppchameleon_mtd->oobsize), GFP_KERNEL);
	if (!this->data_buf) {
		printk("Unable to allocate NAND data buffer for PPChameleon.\n");
		iounmap((void *)ppchameleon_fio_base);
		kfree (ppchameleon_mtd);
		return -ENOMEM;
	}

	/* Allocate memory for internal data buffer */
	this->data_cache = kmalloc (sizeof(u_char) * (ppchameleon_mtd->oobblock + ppchameleon_mtd->oobsize), GFP_KERNEL);
	if (!this->data_cache) {
		printk("Unable to allocate NAND data cache for PPChameleon.\n");
		kfree (this->data_buf);
		iounmap((void *)ppchameleon_fio_base);
		kfree (ppchameleon_mtd);
		return -ENOMEM;
	}
	this->cache_page = -1;
	
#ifdef CONFIG_MTD_CMDLINE_PARTS
	mtd_parts_nb = parse_cmdline_partitions(ppchameleon_mtd, &mtd_parts, 
						"ppchameleon-nand");
	if (mtd_parts_nb > 0)
	  part_type = "command line";
	else
	  mtd_parts_nb = 0;
#endif
	if (mtd_parts_nb == 0)
	{
#if ((defined CONFIG_MTD_PARTITIONS) || (defined CONFIG_MTD_PARTITIONS_MODULE))
		mtd_parts = partition_info;
		mtd_parts_nb = NUM_PARTITIONS;
#endif
		part_type = "static";
	}

	/* Register the partitions */
	printk(KERN_NOTICE "Using %s partition definition\n", part_type);
	add_mtd_partitions(ppchameleon_mtd, mtd_parts, mtd_parts_nb);
	
	/* Return happy */
	return 0;
}
module_init(ppchameleon_init);

/*
 * Clean up routine
 */
static void __exit ppchameleon_cleanup (void)
{
	struct nand_chip *this = (struct nand_chip *) &ppchameleon_mtd[1];

#ifdef CONFIG_MTD_DEBUG
	printk(  "[ppchameleon_cleanup]\n");
#endif

	/* Unregister the device */
	del_mtd_device (ppchameleon_mtd);
	
	/* Free internal data buffer */
	kfree (this->data_buf);
	kfree (this->data_cache);
	
	/* Free the MTD device structure */
	kfree (ppchameleon_mtd);
}
module_exit(ppchameleon_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("llandre <llandre@wawnet.biz>");
MODULE_DESCRIPTION("MTD map driver for DAVE Srl PPChameleon board");
