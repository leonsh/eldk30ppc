#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/config.h>

#include <asm/gt64260.h>
#include <platforms/hxeb100.h>

#define BANK_A_FLASH_ADDR	HXEB100_BANK_A_FLASH_BASE
#define BANK_A_FLASH_SIZE	HXEB100_BANK_A_FLASH_SIZE
#define BANK_A_FLASH_WIDTH	HXEB100_BANK_A_FLASH_BUSWIDTH

#ifdef CONFIG_MTD_HXEB100_BANK_B_FLASH
#define BANK_B_FLASH_ADDR	HXEB100_BANK_B_FLASH_BASE
#define BANK_B_FLASH_SIZE	HXEB100_BANK_B_FLASH_SIZE
#define BANK_B_FLASH_WIDTH	HXEB100_BANK_B_FLASH_BUSWIDTH
static struct mtd_info *bank_b_flash;
#endif

static struct mtd_info *bank_a_flash;

static __u8 hxeb100_read8(struct map_info *map, unsigned long ofs)
{
	return __raw_readb(map->map_priv_1 + ofs);
}

static __u16 hxeb100_read16(struct map_info *map, unsigned long ofs)
{
	return __raw_readw(map->map_priv_1 + ofs);
}

static __u32 hxeb100_read32(struct map_info *map, unsigned long ofs)
{
	return __raw_readl(map->map_priv_1 + ofs);
}

static void hxeb100_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	memcpy_fromio(to, map->map_priv_1 + from, len);
}

static void hxeb100_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	__raw_writeb(d, map->map_priv_1 + adr);
	mb();
}

static void hxeb100_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	__raw_writew(d, map->map_priv_1 + adr);
	mb();
}

static void hxeb100_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	__raw_writel(d, map->map_priv_1 + adr);
	mb();
}

static void hxeb100_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	memcpy_toio(map->map_priv_1 + to, from, len);
}

static struct map_info hxeb100_bank_a_flash_map = {
	name: "HXEB100 Flash Bank A",
	size: BANK_A_FLASH_SIZE,
	buswidth: BANK_A_FLASH_WIDTH,
	read8: hxeb100_read8,
	read16: hxeb100_read16,
	read32: hxeb100_read32,
	copy_from: hxeb100_copy_from,
	write8: hxeb100_write8,
	write16: hxeb100_write16,
	write32: hxeb100_write32,
	copy_to: hxeb100_copy_to
};

#ifdef CONFIG_MTD_HXEB100_BANK_B_FLASH
static struct map_info hxeb100_bank_b_flash_map = {
	name: "HXEB100 Flash Bank B",
	size: BANK_B_FLASH_SIZE,
	buswidth: BANK_B_FLASH_WIDTH,
	read8: hxeb100_read8,
	read16: hxeb100_read16,
	read32: hxeb100_read32,
	copy_from: hxeb100_copy_from,
	write8: hxeb100_write8,
	write16: hxeb100_write16,
	write32: hxeb100_write32,
	copy_to: hxeb100_copy_to
};
#endif /* CONFIG_MTD_HXEB100_BANK_B_FLASH */

int __init init_hxeb100(void)
{
	printk(KERN_NOTICE "Flash Bank A: 0x%x at 0x%x\n", BANK_A_FLASH_SIZE, BANK_A_FLASH_ADDR);
	hxeb100_bank_a_flash_map.map_priv_1 = (unsigned long)ioremap(BANK_A_FLASH_ADDR, BANK_A_FLASH_SIZE);
	if (!hxeb100_bank_a_flash_map.map_priv_1) {
		printk("Failed to ioremap bank_a_flash\n");
		return -EIO;
	}

	/* blah. Not much error checking XXX */
	bank_a_flash = do_map_probe("cfi_probe", &hxeb100_bank_a_flash_map);

	if (bank_a_flash) {
		bank_a_flash->module = THIS_MODULE;
		add_mtd_device(bank_a_flash);
	} else {
		printk("map probe failed for bank_a_flash\n");
	}

#ifdef CONFIG_MTD_HXEB100_BANK_B_FLASH
	printk(KERN_NOTICE "Flash Bank B: 0x%x at 0x%x\n", BANK_B_FLASH_SIZE, BANK_B_FLASH_ADDR);
	hxeb100_bank_b_flash_map.map_priv_1 = (unsigned long)ioremap(BANK_B_FLASH_ADDR, BANK_B_FLASH_SIZE);
	if (!hxeb100_bank_b_flash_map.map_priv_1) {
		printk("Failed to ioremap bank_b_flash\n");
		return -EIO;
	}

	bank_b_flash = do_map_probe("jedec_probe", &hxeb100_bank_b_flash_map);
	if (!bank_b_flash)
		bank_b_flash = do_map_probe("cfi_probe", &hxeb100_bank_b_flash_map);
	if (bank_b_flash) {
		bank_b_flash->module = THIS_MODULE;
		add_mtd_device(bank_b_flash);
	} else {
		printk("map probe failed for bank_b_flash\n");
		return -ENXIO;
	}
#endif /* CONFIG_MTD_HXEB100_BANK_B_FLASH */

	return 0;
}

static void __exit cleanup_hxeb100(void)
{
	if (bank_a_flash) {
		del_mtd_device(bank_a_flash);
		map_destroy(bank_a_flash);
	}
	if (hxeb100_bank_a_flash_map.map_priv_1) {
		iounmap((void *)hxeb100_bank_a_flash_map.map_priv_1);
		hxeb100_bank_a_flash_map.map_priv_1 = 0;
	}
#ifdef CONFIG_MTD_HXEB100_BANK_B_FLASH
	if (bank_b_flash) {
		del_mtd_device(bank_b_flash);
		map_destroy(bank_b_flash);
	}
	if (hxeb100_bank_b_flash_map.map_priv_1) {
		iounmap((void *)hxeb100_bank_b_flash_map.map_priv_1);
		hxeb100_bank_b_flash_map.map_priv_1 = 0;
	}
#endif
}

module_init(init_hxeb100);
module_exit(cleanup_hxeb100);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HXEB100 flash map");
