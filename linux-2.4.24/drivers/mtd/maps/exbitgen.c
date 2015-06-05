/*
 *	exbitgen.c - flash mapper for VITESSE ExbitGen CPU board.
 *
 * Copyright (C) 2002 Vitesse Semiconductor Corp. <mbr@vitesse.com>
 *
 * This code is GPL
 *
 * Based on pnc2000.c and physmap.c.
 *
 * $Id$
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#define WINDOW_ADDR 0x20000000
#define WINDOW_SIZE 0x02000000

#define BUSWIDTH 1

/* 
 * MAP DRIVER STUFF
 */

static __u8 exbitgen_read8(struct map_info *map, unsigned long ofs)
{
	return __raw_readb(map->map_priv_1 + ofs);
}

static __u16 exbitgen_read16(struct map_info *map, unsigned long ofs)
{
	return __raw_readw(map->map_priv_1 + ofs);
}

static __u32 exbitgen_read32(struct map_info *map, unsigned long ofs)
{
	return __raw_readl(map->map_priv_1 + ofs);
}

static void exbitgen_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	memcpy_fromio(to, map->map_priv_1 + from, len);
}

static void exbitgen_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	__raw_writeb(d, map->map_priv_1 + adr);
	mb();
}

static void exbitgen_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	__raw_writew(d, map->map_priv_1 + adr);
	mb();
}

static void exbitgen_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	__raw_writel(d, map->map_priv_1 + adr);
	mb();
}

static void exbitgen_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	memcpy_toio(map->map_priv_1 + to, from, len);
}

struct map_info exbitgen_map = {
	name: "exbitgen",
	size: WINDOW_SIZE,
	buswidth: BUSWIDTH,
	read8: exbitgen_read8,
	read16: exbitgen_read16,
	read32: exbitgen_read32,
	copy_from: exbitgen_copy_from,
	write8: exbitgen_write8,
	write16: exbitgen_write16,
	write32: exbitgen_write32,
	copy_to: exbitgen_copy_to
};


/*
 * MTD 'PARTITIONING' STUFF 
 */
static struct mtd_partition exbitgen_partitions[] = {
	{
		name: "ExbitGen user flash disk",
		size: 0x400000,
		offset: 0x000000,
	},
	{
		name: "ExbitGen kernel U-Boot image",
		size: 0x100000,
		offset: 0x400000
	},
	{
		name: "ExbitGen flash hole",
		size: 0x300000,
		offset: 0x500000,
	},
	{
		name: "ExbitGen initial ramdisk U-Boot image",
		size: 0x800000,
		offset: 0x800000
	},
#if 0
	/* uncomment this if there is 32Mb of Application Flash */
	{
		name: "ExbitGen extended flash disk",
		size: 0x1000000,
		offset: 0x1000000
	}
#endif
};

#define PARTITIONS (sizeof(exbitgen_partitions)/sizeof(exbitgen_partitions[0]))

/* 
 * This is the master MTD device for which all the others are just
 * auto-relocating aliases.
 */
static struct mtd_info *mymtd;

int __init init_exbitgen(void)
{
	printk(KERN_NOTICE "exbitgen: mapping 0x%x bytes at address 0x%x\n", WINDOW_SIZE, WINDOW_ADDR);

	exbitgen_map.map_priv_1 = (unsigned long)ioremap(WINDOW_ADDR, WINDOW_SIZE);
	if (!exbitgen_map.map_priv_1) {
		printk("Failed to ioremap\n");
		return -EIO;
	}

	mymtd = do_map_probe("cfi_probe", &exbitgen_map);
	if (mymtd) {
		mymtd->module = THIS_MODULE;
		return add_mtd_partitions(mymtd, exbitgen_partitions, PARTITIONS);
	}

	return -ENXIO;
}

static void __exit cleanup_exbitgen(void)
{
	if (mymtd) {
		del_mtd_partitions(mymtd);
		map_destroy(mymtd);
	}
	if (exbitgen_map.map_priv_1) {
		iounmap((void *)exbitgen_map.map_priv_1);
		exbitgen_map.map_priv_1 = 0;
	}
}

module_init(init_exbitgen);
module_exit(cleanup_exbitgen);


