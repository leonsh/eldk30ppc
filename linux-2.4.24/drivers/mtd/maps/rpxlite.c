/*
 * $Id: rpxlite.c,v 1.15 2001/10/02 15:05:14 dwmw2 Exp $
 *
 * Handle mapping of the flash on the RPX Lite and CLLF boards
 * Extended for RMU by Yair Raz (yair_raz@mksinst.com)
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#define WINDOW_ADDR 0xfe000000
#define WINDOW_SIZE 0x800000

static struct mtd_info *mymtd;

__u8 rpxlite_read8(struct map_info *map, unsigned long ofs)
{
	return __raw_readb(map->map_priv_1 + ofs);
}

__u16 rpxlite_read16(struct map_info *map, unsigned long ofs)
{
	return __raw_readw(map->map_priv_1 + ofs);
}

__u32 rpxlite_read32(struct map_info *map, unsigned long ofs)
{
	return __raw_readl(map->map_priv_1 + ofs);
}

void rpxlite_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	memcpy_fromio(to, (void *)(map->map_priv_1 + from), len);
}

void rpxlite_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	__raw_writeb(d, map->map_priv_1 + adr);
	mb();
}

void rpxlite_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	__raw_writew(d, map->map_priv_1 + adr);
	mb();
}

void rpxlite_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	__raw_writel(d, map->map_priv_1 + adr);
	mb();
}

void rpxlite_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	memcpy_toio((void *)(map->map_priv_1 + to), from, len);
}

#ifndef CONFIG_RMU
struct map_info rpxlite_map = {
	name:		"RPX",
	size:		0,
	buswidth:	4,
	read8:		rpxlite_read8,
	read16:		rpxlite_read16,
	read32:		rpxlite_read32,
	copy_from:	rpxlite_copy_from,
	write8:		rpxlite_write8,
	write16:	rpxlite_write16,
	write32:	rpxlite_write32,
	copy_to:	rpxlite_copy_to
};

#else	/* CONFIG_RMU */
struct map_info rpxlite_map = {
	name:		"RMU",
	size:		WINDOW_SIZE,
	buswidth:	4,
	read8:		rpxlite_read8,
	read16:		rpxlite_read16,
	read32:		rpxlite_read32,
	copy_from:	rpxlite_copy_from,
	write8:		rpxlite_write8,
	write16:	rpxlite_write16,
	write32:	rpxlite_write32,
	copy_to:	rpxlite_copy_to
};
#endif /* CONFIG_RMU */


#ifdef	CONFIG_RMU
/*
 * The RMU seem to be working OK with a High BootLoader. Using this configuration
 * The 8MB Flash is used in the following way:
 *		0xff800000-0xffefffff,	7MB:	Free
 *		0xfff00000-0xfff3ffff, 256K:	BootLoader
 *		0xfff40000-0xffffffff, 768K:	Free
 *
 * Since it seem that the JFFS2 needs about 1.25MB for its own usage, the RMU uses
 * the following partition (relative to flash start at 0xff80ffff):
 *		5MB:	Kernel Image
 *		2MB:	Flash Filesystem
 *		256KB:	Bootloader
 *		768KB:	Spare
 */
static struct mtd_partition rpxlite_partitions_8MB[] =
{
	{
		name:		"Kernel Image",
		size:		0x00500000,
		offset:		0
	},
	{
		name:		"Flash Filesystem",
		size:		0x00200000,
		offset:		MTDPART_OFS_APPEND
	},
	{
		name:		"Bootloader",
		size:		0x00040000,
		offset:		MTDPART_OFS_APPEND
	},
	{
		name:		"Spare",
		size:		0x000c0000,
		offset:		MTDPART_OFS_APPEND
	}
};

/*
 * NOTE:
 * This partition is not correct. We will deal with it when we need to
 */
static struct mtd_partition rpxlite_partitions_16MB[] =
{
	{
		name:		"Bootloader Primary",
		size:		0x00040000,
		offset:		0
	},
	{
		name:		"Kernel Image",
		size:		0x00600000,
		offset:		MTDPART_OFS_APPEND
	},
	{
		name:		"Flash Filesystem",
		size:		0x00940000,
		offset:		MTDPART_OFS_APPEND
	},
	{
		name:		"Bootloader Secondary",
		size:		0x00080000,
		offset:		MTDPART_OFS_APPEND
	}
};
#endif /* CONFIG_RMU */


#ifndef CONFIG_RMU

int __init init_rpxlite(void)
{
	printk(KERN_NOTICE "RPX Lite or CLLF flash device: %x at %x\n", WINDOW_SIZE*4, WINDOW_ADDR);
	rpxlite_map.map_priv_1 = (unsigned long)ioremap(WINDOW_ADDR, WINDOW_SIZE * 4);

	if (!rpxlite_map.map_priv_1) {
		printk("Failed to ioremap\n");
		return -EIO;
	}
	mymtd = do_map_probe("cfi_probe", &rpxlite_map);
	if (mymtd) {
		mymtd->module = THIS_MODULE;
		add_mtd_device(mymtd);
		return 0;
	}

	iounmap((void *)rpxlite_map.map_priv_1);
	return -ENXIO;
}

#else	/* CONFIG_RMU */

#define NB_OF(x)	(sizeof(x)/sizeof(x[0]))

/*
 * Determines the size of the flash.
 * Currently returns 8MB all the time
 */
static int determine_flashSize (void)
{
	return 8;
}

static int __init init_rpxlite (void)
{
	struct mtd_partition *parts;
	int	nb_parts = 0;
	char	*part_type = "static";
	int	flashSize = determine_flashSize ();

	switch (flashSize) {
	case 8:
		parts = rpxlite_partitions_8MB;
		nb_parts = NB_OF (rpxlite_partitions_8MB);
		rpxlite_map.size = (8 * 1024 * 1024);
		break;

	case 16:
		parts = rpxlite_partitions_16MB;
		nb_parts = NB_OF (rpxlite_partitions_16MB);
		rpxlite_map.size = (16 * 1024 * 1024);
		break;

	default:
		return -EIO;
		break;
	}

	printk (KERN_NOTICE "RMU flash device: %dMB at 0x%08lx\n",
			flashSize, -rpxlite_map.size);
	rpxlite_map.map_priv_1 = (unsigned long) ioremap (-rpxlite_map.size,
													  rpxlite_map.size);
	if (rpxlite_map.map_priv_1 == NULL) {
		printk ("Failed to ioremap\n");
		return -EIO;
	}

	mymtd = do_map_probe ("cfi_probe", &rpxlite_map);
	if (!mymtd) {
		iounmap ((void *) rpxlite_map.map_priv_1);
		return -ENXIO;
	}

	mymtd->module = THIS_MODULE;
	printk (KERN_NOTICE "Using %s partition definition\n", part_type);
	add_mtd_partitions (mymtd, parts, nb_parts);

	return 0;
}
#endif	/* CONFIG_RMU */

static void __exit cleanup_rpxlite(void)
{
	if (mymtd) {
		del_mtd_device(mymtd);
		map_destroy(mymtd);
	}
	if (rpxlite_map.map_priv_1) {
		iounmap((void *)rpxlite_map.map_priv_1);
		rpxlite_map.map_priv_1 = 0;
	}
}

module_init(init_rpxlite);
module_exit(cleanup_rpxlite);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arnold Christensen <AKC@pel.dk>");
MODULE_DESCRIPTION("MTD map driver for RPX Lite and CLLF boards");
