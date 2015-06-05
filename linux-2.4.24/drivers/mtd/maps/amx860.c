/*
 * Handle mapping of the flash memory access routines on AMX860 board.
 *
 * Copyright(C) 2003 Wolfgang Denk <wd@denx.de>
 *
 * This code is GPLed
 *
 */

/*
 * For the AMX860 we use CONFIG_MTD_CFI_I2 & CONFIG_MTD_CFI_B4
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#ifdef CONFIG_MTD_PARTITIONS
#include <linux/mtd/partitions.h>
#endif

/* #define  AMX860_DEBUG	1 */

#ifdef  AMX860_DEBUG
# define debugk(fmt,args...)	printk(fmt ,##args)
#else
# define debugk(fmt,args...)
#endif

#define FLASH_BANK_MAX 1

/* trivial struct to describe partition information */
struct mtd_part_def
{
	int nums;
	unsigned char *type;
	struct mtd_partition* mtd_part;
};

static struct mtd_info* mtd_banks[FLASH_BANK_MAX];
static struct map_info* map_banks[FLASH_BANK_MAX];
static struct mtd_part_def part_banks[FLASH_BANK_MAX];
static unsigned long num_banks;
static unsigned long start_scan_addr;

__u8 amx860_read8(struct map_info *map, unsigned long ofs)
{
	return *((__u8 *)(map->map_priv_1 + ofs));
}

__u16 amx860_read16(struct map_info *map, unsigned long ofs)
{
	return *((__u16 *)(map->map_priv_1 + ofs));
}

__u32 amx860_read32(struct map_info *map, unsigned long ofs)
{
	return *((__u32 *)(map->map_priv_1 + ofs));
}

void amx860_copy_from(struct map_info *map,
			void *to, unsigned long from, ssize_t len)
{
	memcpy_fromio(to, (void *)(map->map_priv_1 + from), len);
}

void amx860_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	*((__u8 *)(map->map_priv_1 + adr)) = d;
}

void amx860_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	*((__u16 *)( map->map_priv_1 + adr)) = d;
}

void amx860_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	*((__u32 *)(map->map_priv_1 + adr)) = d;
}

void amx860_copy_to(struct map_info *map,
			unsigned long to, const void *from, ssize_t len)
{
	memcpy_toio((void *)(map->map_priv_1 + to), from, len);
}

/*
 * Here is the partition information for the AMX860 board.
 *
 * See include/linux/mtd/partitions.h for definition of the mtd_partition
 * structure.
 *
 * The *_max_flash_size is the maximum possible mapped flash size which
 * is not necessarily the actual flash size.  It must correspond to the
 * value specified in the mapping definition defined by the
 * "struct map_desc *_io_desc" for the corresponding machine.
 */

#ifdef CONFIG_MTD_PARTITIONS

static struct mtd_partition amx860_partitions[] = {
    /*
     * partition definition for first flash bank
     * also ref. to "drivers/char/flash_config.c"
     */
    {
	name:	"U-Boot",			/* U-Boot Firmware */
	offset:	0x00000000,
	size:	0x00040000,			/* 256 KB */
/*	mask_flags: MTD_WRITEABLE,		/ * force read-only */
    },
    {
	name:	"Linux",			/* default kernel image */
	offset:	0x00040000,
	size:	0x000C0000,
/*	mask_flags: MTD_WRITEABLE,		/ * force read-only */
    },
    {
	name:	"User",
	offset:	0x00100000,
	size:	0x00300000,
    },
    /*
     * partition definition for second flash bank
     * Note: the MTD code handles this a ONE (1) logical bank
     */
    {
	name:	"Data",
	offset:	0x00400000,
	size:	0x00400000,
    },
};

#ifdef CONFIG_MTD_CMDLINE_PARTS
int parse_cmdline_partitions(struct mtd_info *master,
			     struct mtd_partition **pparts,
			     const char *mtd_id);
#endif	/* CONFIG_MTD_CMDLINE_PARTS */

#endif	/* CONFIG_MTD_PARTITIONS */

#define NB_OF(x)  (sizeof(x)/sizeof(x[0]))

int __init init_amx860_mtd(void)
{
	int idx = 0, ret = 0;
	unsigned long flash_addr, flash_size, mtd_size = 0;
	/* pointer to AMX860 board info data */
	bd_t *bd = (bd_t *)__res;
#ifdef CONFIG_MTD_CMDLINE_PARTS
	int n;
	char mtdid[4];
#endif

	flash_addr = bd->bi_flashstart;
	flash_size = bd->bi_flashsize;

	/* request maximum flash size address space */
	start_scan_addr = (unsigned long)ioremap(flash_addr, flash_size);
	if (!start_scan_addr) {
		printk("%s: Failed to ioremap address: 0x%lx\n",
			__FUNCTION__, flash_addr);
		return -EIO;
	}

	for(idx = 0 ; idx < FLASH_BANK_MAX ; idx++) {
		if (mtd_size >= flash_size)
			break;

		debugk ("%s: chip probing count %d\n", __FUNCTION__, idx);

		map_banks[idx] = (struct map_info *)kmalloc(sizeof(struct map_info),
							GFP_KERNEL);
		if (map_banks[idx] == NULL) {
			ret = -ENOMEM;
			goto error_mem;
		}
		memset((void *)map_banks[idx], 0, sizeof(struct map_info));
		map_banks[idx]->name = (char *)kmalloc(16, GFP_KERNEL);
		if (map_banks[idx]->name == NULL) {
			ret = -ENOMEM;
			goto error_mem;
		}
		memset((void *)map_banks[idx]->name, 0, 16);

		sprintf(map_banks[idx]->name, "AMX860 Bank %d", idx);
		map_banks[idx]->size	   = flash_size;
		map_banks[idx]->buswidth   = 4;
		map_banks[idx]->read8	   = amx860_read8;
		map_banks[idx]->read16	   = amx860_read16;
		map_banks[idx]->read32	   = amx860_read32;
		map_banks[idx]->copy_from  = amx860_copy_from;
		map_banks[idx]->write8	   = amx860_write8;
		map_banks[idx]->write16	   = amx860_write16;
		map_banks[idx]->write32	   = amx860_write32;
		map_banks[idx]->copy_to	   = amx860_copy_to;
		map_banks[idx]->map_priv_1 =
			start_scan_addr + ((idx > 0) ?
			(mtd_banks[idx-1] ? mtd_banks[idx-1]->size : 0) : 0);

		/* start to probe flash chips */
		mtd_banks[idx] = do_map_probe("cfi_probe", map_banks[idx]);
		if (mtd_banks[idx]) {
			mtd_banks[idx]->module = THIS_MODULE;
			mtd_size += mtd_banks[idx]->size;
			num_banks++;
			debugk ("%s: bank %d, name: %s, size: %d bytes\n",
				__FUNCTION__,
				num_banks,
				mtd_banks[idx]->name,
				mtd_banks[idx]->size);
		}
	}

	/* no supported flash chips found */
	if (!num_banks) {
		printk("AMX860: No supported flash chips found!\n");
		ret = -ENXIO;
		goto error_mem;
	}

#ifdef CONFIG_MTD_PARTITIONS
	/*
	 * Select static partition definitions
	 */
	part_banks[0].mtd_part	= amx860_partitions;
	part_banks[0].type	= "static image";
	part_banks[0].nums	= NB_OF(amx860_partitions);

	for(idx = 0; idx < num_banks ; idx++) {
#ifdef CONFIG_MTD_CMDLINE_PARTS
		sprintf(mtdid, "%d", idx);
		n = parse_cmdline_partitions(mtd_banks[idx],
					     &part_banks[idx].mtd_part,
					     mtdid);
		debugk ("%s: %d command line partitions on bank %s\n",
			__FUNCTION__, n, mtdid);
		if (n > 0) {
			part_banks[idx].type = "command line";
			part_banks[idx].nums = n;
		}
#endif	/* CONFIG_MTD_CMDLINE_PARTS */

		if (part_banks[idx].nums == 0) {
		    printk (KERN_NOTICE
			"AMX860 flash bank %d: no partition info available, "
			"registering whole device\n",
			idx);
		    add_mtd_device(mtd_banks[idx]);
		} else {
		    printk (KERN_NOTICE
			"AMX860 flash bank %d: Using %s partition definition\n",
			idx,
			part_banks[idx].type);
		    add_mtd_partitions (mtd_banks[idx],
					part_banks[idx].mtd_part,
					part_banks[idx].nums);
		}
	}
#else	/* ! CONFIG_MTD_PARTITIONS */
	printk (KERN_NOTICE "AMX860 flash: registering %d flash banks at once\n",
		num_banks);

	for(idx = 0 ; idx < num_banks ; idx++) {
		add_mtd_device(mtd_banks[idx]);
	}
#endif	/* CONFIG_MTD_PARTITIONS */

	return 0;
error_mem:
	for (idx = 0 ; idx < FLASH_BANK_MAX ; idx++) {
		if (map_banks[idx] != NULL) {
			if (map_banks[idx]->name != NULL) {
				kfree(map_banks[idx]->name);
				map_banks[idx]->name = NULL;
			}
			kfree(map_banks[idx]);
			map_banks[idx] = NULL;
		}
	}

	iounmap((void *)start_scan_addr);

	return ret;
}

static void __exit cleanup_amx860_mtd(void)
{
	unsigned int idx = 0;
	for(idx = 0 ; idx < num_banks ; idx++) {
		/* destroy mtd_info previously allocated */
		if (mtd_banks[idx]) {
			del_mtd_partitions(mtd_banks[idx]);
			map_destroy(mtd_banks[idx]);
		}
		/* release map_info not used anymore */
		kfree(map_banks[idx]->name);
		kfree(map_banks[idx]);
	}
	if (start_scan_addr) {
		iounmap((void *)start_scan_addr);
		start_scan_addr = 0;
	}
}

module_init(init_amx860_mtd);
module_exit(cleanup_amx860_mtd);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wolfgang Denk <wd@denx.de>");
MODULE_DESCRIPTION("MTD map driver for AMX860 board");
