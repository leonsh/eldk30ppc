/*
 * Copyright(C) 2003
 * Wolfgang Denk, DENX Software Engineering, <wd@denx.de>
 *
 * This code is GPLed
 * 
 */

/*
 * ATC uses 1 x AMD AM29LV641D (64 Mbit)
 * for a total of 8 MB flash in one bank
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#undef	DEBUG

#ifdef  DEBUG
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

static __u8 atc_read8(struct map_info *map, unsigned long ofs)
{
	return *((__u8 *)(map->map_priv_1 + ofs));
}

static __u16 atc_read16(struct map_info *map, unsigned long ofs)
{
	return *((__u16 *)(map->map_priv_1 + ofs));
}

static __u32 atc_read32(struct map_info *map, unsigned long ofs)
{
	return *((__u32 *)(map->map_priv_1 + ofs));
}

static __u64 atc_read64(struct map_info *map, unsigned long ofs)
{
	return *((volatile __u64 *)(map->map_priv_1 + ofs));
}

static void atc_copy_from(struct map_info *map,
		       void *to, unsigned long from, ssize_t len)
{
	memcpy_fromio(to, (void *)(map->map_priv_1 + from), len);
}

static void atc_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	*((__u8 *)(map->map_priv_1 + adr)) = d;
}

static void atc_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	*((__u16 *)( map->map_priv_1 + adr)) = d;
}

static void atc_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	*((__u32 *)(map->map_priv_1 + adr)) = d;
}

static void atc_write64(struct map_info *map, __u64 d, unsigned long adr)
{
	        *((__u64 *)(map->map_priv_1 + adr)) = d;
}

static void atc_copy_to(struct map_info *map,
		     unsigned long to, const void *from, ssize_t len)
{
	memcpy_toio((void *)(map->map_priv_1 + to), from, len);
}

/*
 * The following defines the partition layout of ATC boards.
 *
 * See include/linux/mtd/partitions.h for definition of the
 * mtd_partition structure.
 */

#ifdef CONFIG_MTD_PARTITIONS

/* partition definition for first (and only) flash bank
 * also ref. to "drivers/char/flash_config.c" 
 */
static struct mtd_partition atc_partitions[] = {
    {
	name:	"u-boot",			/* U-Boot Firmware	*/
	offset:	0,
	size:	256 << 10,			/*  256 kB		*/
/*	mask_flags: MTD_WRITEABLE,		/ * force read-only	*/
    },
    {
	name:	"kernel",			/* Linux kernel image	*/
	offset: 256 << 10,
	size:	768 << 10,			/*  768 kB		*/
/*	mask_flags: MTD_WRITEABLE,		/ * force read-only	*/
    },
    {
	name:	"initrd",			/* Ramdisk image	*/
	offset:	1 << 20,
	size:	3 << 20,			/*  3 MB		*/
    },
    {
	name:	"jffs",				/* JFFS(2) filesystem	*/
	offset:	4 << 20,
	size:	4 << 20,			/*  4 ...  8 MB => 4 MB	*/
    },
};

#ifdef CONFIG_MTD_CMDLINE_PARTS
int parse_cmdline_partitions(struct mtd_info *master,
			     struct mtd_partition **pparts,
			     const char *mtd_id);
#endif	/* CONFIG_MTD_CMDLINE_PARTS */

#endif	/* CONFIG_MTD_PARTITIONS */

#define NB_OF(x)  (sizeof(x)/sizeof(x[0]))

int __init init_atc_mtd(void)
{
	int idx = 0, ret = 0;
	unsigned long flash_addr, flash_size, mtd_size = 0;
	/* pointer to ATC board info data */
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
		
		sprintf(map_banks[idx]->name, "ATC Bank %d", idx);
		map_banks[idx]->size      = flash_size;
		map_banks[idx]->buswidth  = 2;
		map_banks[idx]->read8     = atc_read8;
		map_banks[idx]->read16    = atc_read16;
		map_banks[idx]->read32    = atc_read32;
		map_banks[idx]->read64    = atc_read64;
		map_banks[idx]->copy_from = atc_copy_from;
		map_banks[idx]->write8    = atc_write8;
		map_banks[idx]->write16   = atc_write16;
		map_banks[idx]->write32   = atc_write32;
		map_banks[idx]->write64   = atc_write64;
		map_banks[idx]->copy_to   = atc_copy_to;
		map_banks[idx]->map_priv_1= 
			start_scan_addr + ((idx > 0) ? 
			(mtd_banks[idx-1] ? mtd_banks[idx-1]->size : 0) : 0);
		
		/* start to probe flash chips */
		mtd_banks[idx] = do_map_probe("cfi_probe", map_banks[idx]);
		if (mtd_banks[idx]) {
			mtd_banks[idx]->module = THIS_MODULE;
			mtd_size += mtd_banks[idx]->size;
			num_banks++;
			debugk ("%s: bank %ld, name: %s, size: %d bytes \n",
				__FUNCTION__,
				num_banks, 
				mtd_banks[idx]->name,
				mtd_banks[idx]->size);
		}
	}

	/* no supported flash chips found */
	if (!num_banks) {
		printk("ATC: No supported flash chips found!\n");
		ret = -ENXIO;
		goto error_mem;
	}

#ifdef CONFIG_MTD_PARTITIONS
	/*
	 * Select static partition definitions
	 */
	part_banks[0].mtd_part	= atc_partitions;
	part_banks[0].type	= "static image";
	part_banks[0].nums	= NB_OF(atc_partitions);

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
			    "ATC flash bank %d: no partition info "
			    "available, registering whole device\n", idx);
		    add_mtd_device(mtd_banks[idx]);
		} else {
		    printk (KERN_NOTICE 
			    "ATC flash bank %d: Using %s partition "
			    "definition\n", idx, part_banks[idx].type);
		    add_mtd_partitions (mtd_banks[idx],
					part_banks[idx].mtd_part, 
					part_banks[idx].nums);
		}
	}
#else	/* ! CONFIG_MTD_PARTITIONS */
	printk (KERN_NOTICE "ATC flash: registering %d flash banks "
	        "at once\n", num_banks);

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

static void __exit cleanup_atc_mtd(void)
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

module_init(init_atc_mtd);
module_exit(cleanup_atc_mtd);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wolfgang Denk <wd@denx.de>");
MODULE_DESCRIPTION("MTD map driver for ATC board");
