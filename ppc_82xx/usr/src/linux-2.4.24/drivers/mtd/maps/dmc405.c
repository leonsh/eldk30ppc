/*
 * $Id: dmc405.c,v 1.15 2001/10/02 15:05:14 dwmw2 Exp $
 *
 * Handle mapping of the flash on the NEXTREAM DMC board
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>


#define BOOT_WINDOW_ADDR 0xff800000
#define BOOT_WINDOW_SIZE 0x800000
#define SPARE_WINDOW_ADDR 0xff000000
#define SPARE_WINDOW_SIZE 0x800000

static struct mtd_info *dmc405_boot, *dmc405_spare;

__u8 dmc405_read8(struct map_info *map, unsigned long ofs)
{
	return __raw_readb(map->map_priv_1 + ofs);
}

__u16 dmc405_read16(struct map_info *map, unsigned long ofs)
{
	return __raw_readw(map->map_priv_1 + ofs);
}

__u32 dmc405_read32(struct map_info *map, unsigned long ofs)
{
	return __raw_readl(map->map_priv_1 + ofs);
}

void dmc405_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	memcpy_fromio(to, (void *)(map->map_priv_1 + from), len);
}

void dmc405_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	__raw_writeb(d, map->map_priv_1 + adr);
	mb();
}

void dmc405_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	__raw_writew(d, map->map_priv_1 + adr);
	mb();
}

void dmc405_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	__raw_writel(d, map->map_priv_1 + adr);
	mb();
}

void dmc405_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	memcpy_toio((void *)(map->map_priv_1 + to), from, len);
}

struct map_info dmc405_boot_map = {
	name: "DMC Boot",
	size: BOOT_WINDOW_SIZE,
	buswidth: 2,
	read8: dmc405_read8,
	read16: dmc405_read16,
	read32: dmc405_read32,
	copy_from: dmc405_copy_from,
	write8: dmc405_write8,
	write16: dmc405_write16,
	write32: dmc405_write32,
	copy_to: dmc405_copy_to
};

int __init init_dmc405_boot(void)
{
	printk(KERN_NOTICE "DMC boot flash device: %x at %x\n", 
	       BOOT_WINDOW_SIZE, BOOT_WINDOW_ADDR);
	dmc405_boot_map.map_priv_1 = (unsigned long)ioremap(BOOT_WINDOW_ADDR, 
							    BOOT_WINDOW_SIZE);

	if (!dmc405_boot_map.map_priv_1) {
		printk("Failed to ioremap\n");
		return -EIO;
	}
	dmc405_boot = do_map_probe("cfi_probe", &dmc405_boot_map);
	if (dmc405_boot) {
		dmc405_boot->module = THIS_MODULE;
		add_mtd_device(dmc405_boot);
		return 0;
	}

	iounmap((void *)dmc405_boot_map.map_priv_1);
	return -ENXIO;
}

static void __exit cleanup_dmc405_boot(void)
{
	if (dmc405_boot) {
		del_mtd_device(dmc405_boot);
		map_destroy(dmc405_boot);
	}
	if (dmc405_boot_map.map_priv_1) {
		iounmap((void *)dmc405_boot_map.map_priv_1);
		dmc405_boot_map.map_priv_1 = 0;
	}
}

struct map_info dmc405_spare_map = {
	name: "DMC Spare",
	size: SPARE_WINDOW_SIZE,
	buswidth: 2,
	read8: dmc405_read8,
	read16: dmc405_read16,
	read32: dmc405_read32,
	copy_from: dmc405_copy_from,
	write8: dmc405_write8,
	write16: dmc405_write16,
	write32: dmc405_write32,
	copy_to: dmc405_copy_to
};

int __init init_dmc405_spare(void)
{
	printk(KERN_NOTICE "DMC spare flash device: %x at %x\n", 
	       SPARE_WINDOW_SIZE, SPARE_WINDOW_ADDR);
	dmc405_spare_map.map_priv_1 = (unsigned long)ioremap(SPARE_WINDOW_ADDR, 
							     SPARE_WINDOW_SIZE);

	if (!dmc405_spare_map.map_priv_1) {
		printk("Failed to ioremap\n");
		return -EIO;
	}
	dmc405_spare = do_map_probe("cfi_probe", &dmc405_spare_map);
	if (dmc405_spare) {
		dmc405_spare->module = THIS_MODULE;
		add_mtd_device(dmc405_spare);
		return 0;
	}

	iounmap((void *)dmc405_spare_map.map_priv_1);
	return -ENXIO;
}

static void __exit cleanup_dmc405_spare(void)
{
	if (dmc405_spare) {
		del_mtd_device(dmc405_spare);
		map_destroy(dmc405_spare);
	}
	if (dmc405_spare_map.map_priv_1) {
		iounmap((void *)dmc405_spare_map.map_priv_1);
		dmc405_spare_map.map_priv_1 = 0;
	}
}

int __init init_dmc405(void)
{
	init_dmc405_boot();
	init_dmc405_spare();
}

static void __exit cleanup_dmc405(void)
{
	cleanup_dmc405_boot();
	cleanup_dmc405_spare();
}

module_init(init_dmc405);
module_exit(cleanup_dmc405);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MTD map driver for NEXTREAM DMC board");
