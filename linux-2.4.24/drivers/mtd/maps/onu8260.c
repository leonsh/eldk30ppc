/*
 * Copyright(C) 2005
 * Henry Wuang
 *
 * This code is GPLed
 * 
 */

/*
 * ONU8260 uses 1 x Intel 28F128J3A (128 Mbit, 128 x 128K)
 * for a total of 16 MB flash in 128 sectors in one bank
 * Thus we have to chose:
 * - Support 16-bit buswidth => CONFIG_MTD_CFI_B2
 * - Support 1-chip flash interleave => CONFIG_MTD_CFI_I1
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/reboot.h>

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

static __u8 onu8260_read8(struct map_info *map, unsigned long ofs)
{
	return *((__u8 *)(map->map_priv_1 + ofs));
}

static __u16 onu8260_read16(struct map_info *map, unsigned long ofs)
{
	return *((__u16 *)(map->map_priv_1 + ofs));
}

static __u32 onu8260_read32(struct map_info *map, unsigned long ofs)
{
	return *((__u32 *)(map->map_priv_1 + ofs));
}

static __u64 onu8260_read64(struct map_info *map, unsigned long ofs)
{
	        return *((volatile __u64 *)(map->map_priv_1 + ofs));
}

static void onu8260_copy_from(struct map_info *map,
		       void *to, unsigned long from, ssize_t len)
{
	memcpy_fromio(to, (void *)(map->map_priv_1 + from), len);
}

static void onu8260_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	*((__u8 *)(map->map_priv_1 + adr)) = d;
}

static void onu8260_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	*((__u16 *)( map->map_priv_1 + adr)) = d;
}

static void onu8260_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	*((__u32 *)(map->map_priv_1 + adr)) = d;
}

static void onu8260_write64(struct map_info *map, __u64 d, unsigned long adr)
{
	        *((__u64 *)(map->map_priv_1 + adr)) = d;
}

static void onu8260_copy_to(struct map_info *map,
		     unsigned long to, const void *from, ssize_t len)
{
	memcpy_toio((void *)(map->map_priv_1 + to), from, len);
}

/*
 * The following defines the partition layout of ONU8260 boards.
 *
 * See include/linux/mtd/partitions.h for definition of the
 * mtd_partition structure.
 */

#ifdef CONFIG_MTD_PARTITIONS

/* partition definition for first (and only) flash bank
 * also ref. to "drivers/char/flash_config.c" 
 */
static struct mtd_partition onu8260_partitions[] = {
    {
	name:	"U-Boot",			/* U-Boot Firmware	*/
	offset:	0,
	size:	256 << 10,			/* 256 kB		*/
/*	mask_flags: MTD_WRITEABLE,		/ * force read-only	*/
    },
    {
	name:	"kernel",			/* Linux kernel image	*/
	offset:	256 << 10,
	size:	768 << 10,			/*  768 kB		*/
/*	mask_flags: MTD_WRITEABLE,		/ * force read-only	*/
    },
    {
	name:	"initrd",			/* Ramdisk image	*/
	offset:	1 << 20,
	size:	8 << 20,			/*  8 MB		*/
    },
    {
	name:	"jffs",				/* JFFS(2) filesystem	*/
	offset:	9 << 20,
	size:	6 << 20,			/*  9 ...  15 MB => 6 MB	*/
    },
};

/*
 *	Set the Intel flash back to read mode since some old boot
 *	loaders don't.
 */
static int onu_reboot_notifier(struct notifier_block *nb, unsigned long val, void *v)
{
    ulong startaddr;
    int i;

    startaddr = 0xff000000;

    for(i =0; i<128; i++) {
      *((__u16 *)(startaddr)) = (__u16)(0xffff);
      startaddr = startaddr + 0x20000;
    }

    return 0;
}

static struct notifier_block onu_notifier_block = {
	onu_reboot_notifier, NULL, 0
};

#ifdef CONFIG_MTD_CMDLINE_PARTS
int parse_cmdline_partitions(struct mtd_info *master,
			     struct mtd_partition **pparts,
			     const char *mtd_id);
#endif	/* CONFIG_MTD_CMDLINE_PARTS */

#endif	/* CONFIG_MTD_PARTITIONS */

#define NB_OF(x)  (sizeof(x)/sizeof(x[0]))

int __init init_onu8260_mtd(void)
{
	int idx = 0, ret = 0;
	unsigned long flash_addr, flash_size, mtd_size = 0;
	/* pointer to ONU8260 board info data */
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
		
		sprintf(map_banks[idx]->name, "ONU8260 Bank %d", idx);
		map_banks[idx]->size      = 0x20000;// hwuang, 01/12/05
		map_banks[idx]->buswidth  = 2;      // hwuang, 01/12/05
		map_banks[idx]->read8     = onu8260_read8;
		map_banks[idx]->read16    = onu8260_read16;
		map_banks[idx]->read32    = onu8260_read32;
		map_banks[idx]->read64    = onu8260_read64;
		map_banks[idx]->copy_from = onu8260_copy_from;
		map_banks[idx]->write8    = onu8260_write8;
		map_banks[idx]->write16   = onu8260_write16;
		map_banks[idx]->write32   = onu8260_write32;
		map_banks[idx]->write64   = onu8260_write64;
		map_banks[idx]->copy_to   = onu8260_copy_to;
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
		printk("ONU8260: No supported flash chips found!\n");
		ret = -ENXIO;
		goto error_mem;
	}

#ifdef CONFIG_MTD_PARTITIONS
	/*
	 * Select static partition definitions
	 */
	part_banks[0].mtd_part	= onu8260_partitions;
	part_banks[0].type	= "static image";
	part_banks[0].nums	= NB_OF(onu8260_partitions);

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
			    "ONU8260 flash bank %d: no partition info "
			    "available, registering whole device\n", idx);
		    add_mtd_device(mtd_banks[idx]);
		} else {
		    printk (KERN_NOTICE 
			    "ONU8260 flash bank %d: Using %s partition "
			    "definition\n", idx, part_banks[idx].type);
		    add_mtd_partitions (mtd_banks[idx],
					part_banks[idx].mtd_part, 
					part_banks[idx].nums);
		}
	}
#else	/* ! CONFIG_MTD_PARTITIONS */
	printk (KERN_NOTICE "ONU8260 flash: registering %d flash banks "
	        "at once\n", num_banks);

	for(idx = 0 ; idx < num_banks ; idx++) {
		add_mtd_device(mtd_banks[idx]);
	}
#endif	/* CONFIG_MTD_PARTITIONS */
    register_reboot_notifier(&onu_notifier_block);
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

static void __exit cleanup_onu8260_mtd(void)
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
    unregister_reboot_notifier(&onu_notifier_block);
}

module_init(init_onu8260_mtd);
module_exit(cleanup_onu8260_mtd);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("henry wuang");
MODULE_DESCRIPTION("MTD map driver for ONU8260 boards");
