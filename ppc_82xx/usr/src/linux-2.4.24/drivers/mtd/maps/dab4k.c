/*
 * Handle mapping of the flash memory access routines 
 * on DAB4K boards
 *
 * based on tqm8xxl.c
 *
 * Copyright(C) 2001 Kirk Lee <kirk@hpc.ee.ntu.edu.tw>
 * Modified (M) 2002 Steven Scholz <steven.scholz@imc-berlin.de>
 *
 * This code is GPLed
 * 
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#define FLASH_ADDR 0x40000000
#define FLASH_SIZE 0x00800000
#define FLASH_BANK_MAX 1

// trivial struct to describe partition information
struct mtd_part_def
{
	int nums;
	unsigned char *type;
	struct mtd_partition* mtd_part;
};

//static struct mtd_info *mymtd;
static struct mtd_info* mtd_banks[FLASH_BANK_MAX];
static struct map_info* map_banks[FLASH_BANK_MAX];
static struct mtd_part_def part_banks[FLASH_BANK_MAX];
static unsigned long num_banks;
static unsigned long start_scan_addr;

__u8 dab4k_read8(struct map_info *map, unsigned long ofs)
{
	return *((__u8 *)(map->map_priv_1 + ofs));
}

__u16 dab4k_read16(struct map_info *map, unsigned long ofs)
{
	return *((__u16 *)(map->map_priv_1 + ofs));
}

__u32 dab4k_read32(struct map_info *map, unsigned long ofs)
{
	return *((__u32 *)(map->map_priv_1 + ofs));
}

void dab4k_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	memcpy_fromio(to, (void *)(map->map_priv_1 + from), len);
}

void dab4k_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	*((__u8 *)(map->map_priv_1 + adr)) = d;
}

void dab4k_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	*((__u16 *)( map->map_priv_1 + adr)) = d;
}

void dab4k_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	*((__u32 *)(map->map_priv_1 + adr)) = d;
}

void dab4k_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	memcpy_toio((void *)(map->map_priv_1 + to), from, len);
}

/*
 * Here are partition information for DAB4K boards.
 * See include/linux/mtd/partitions.h for definition of the mtd_partition
 * structure.
 * 
 * The *_max_flash_size is the maximum possible mapped flash size which
 * is not necessarily the actual flash size.  It must correspond to the 
 * value specified in the mapping definition defined by the
 * "struct map_desc *_io_desc" for the corresponding machine.
 */

#ifdef CONFIG_MTD_PARTITIONS
/* Currently, DAB4K has 8MiB flash */
static unsigned long dab4k_max_flash_size = 0x00800000;

/* partition definition for first flash bank
 * also ref. to "drivers\char\flash_config.c" 
 */
static struct mtd_partition dab4k_partitions[] = {
	{
	      name:"jffs2",
	      offset:0x00100000,
	      size:0x00700000,
	},
	{
	      name:"ppcboot",
	      offset:0x00000000,
	      size:0x00040000,
	},
	{
	      name:"environment",
	      offset:0x00040000,
	      size:0x00010000,
	}
};
/* partition definition for second flahs bank */
static struct mtd_partition dab4k_fs_partitions[] = {
#if 0
	{
	  name: "cramfs",
	  offset: 0x00000000,
	  size: 0x00200000,
	},
	{
	  name: "jffs",
	  offset: 0x00200000,
	  size: 0x00200000,
	  //size: MTDPART_SIZ_FULL,
	}
#endif
};
#endif

#define NB_OF(x)  (sizeof(x)/sizeof(x[0]))

int __init init_dab4k_mtd(void)
{
	int idx = 0, ret = 0;
	unsigned long flash_addr, flash_size, mtd_size = 0;
	/* pointer to DAB4K board info data */
	bd_t *bd = (bd_t *)__res;

	flash_addr = bd->bi_flashstart;
	flash_size = bd->bi_flashsize;
	//request maximum flash size address spzce
	start_scan_addr = (unsigned long)ioremap(flash_addr, flash_size);
	if (!start_scan_addr) {
		//printk("%s:Failed to ioremap address:0x%x\n", __FUNCTION__, FLASH_ADDR);
		printk("%s:Failed to ioremap address:0x%lx\n", __FUNCTION__, flash_addr);
		return -EIO;
	}
	for(idx = 0 ; idx < FLASH_BANK_MAX ; idx++)
	{
		if(mtd_size >= flash_size)
			break;
		
		printk("%s: chip probing count %d\n", __FUNCTION__, idx);
		
		map_banks[idx] = (struct map_info *)kmalloc(sizeof(struct map_info), GFP_KERNEL);
		if(map_banks[idx] == NULL)
		{
			//return -ENOMEM;
			ret = -ENOMEM;
			goto error_mem;
		}
		memset((void *)map_banks[idx], 0, sizeof(struct map_info));
		map_banks[idx]->name = (char *)kmalloc(16, GFP_KERNEL);
		if(map_banks[idx]->name == NULL)
		{
			//return -ENOMEM;
			ret = -ENOMEM;
			goto error_mem;
		}
		memset((void *)map_banks[idx]->name, 0, 16);

		sprintf(map_banks[idx]->name, "DAB4K%d %d", CONFIG_DAB4K_VERSION, idx);
		map_banks[idx]->size = flash_size;
		map_banks[idx]->buswidth = 2;
		map_banks[idx]->read8 = dab4k_read8;
		map_banks[idx]->read16 = dab4k_read16;
		map_banks[idx]->read32 = dab4k_read32;
		map_banks[idx]->copy_from = dab4k_copy_from;
		map_banks[idx]->write8 = dab4k_write8;
		map_banks[idx]->write16 = dab4k_write16;
		map_banks[idx]->write32 = dab4k_write32;
		map_banks[idx]->copy_to = dab4k_copy_to;
		map_banks[idx]->map_priv_1 = 
		start_scan_addr + ((idx > 0) ? 
		(mtd_banks[idx-1] ? mtd_banks[idx-1]->size : 0) : 0);
		//start to probe flash chips
		mtd_banks[idx] = do_map_probe("cfi_probe", map_banks[idx]);
		if(mtd_banks[idx])
		{
			mtd_banks[idx]->module = THIS_MODULE;
			mtd_size += mtd_banks[idx]->size;
			num_banks++;
			printk("%s: bank%ld, name:%s, size:%dbytes \n", __FUNCTION__, num_banks, 
			mtd_banks[idx]->name, mtd_banks[idx]->size);
		}
	}

	/* no supported flash chips found */
	if(!num_banks)
	{
		printk("DAB4K: No support flash chips found!\n");
		ret = -ENXIO;
		goto error_mem;
	}

#ifdef CONFIG_MTD_PARTITIONS
	/*
	 * Select Static partition definitions
	 */
	part_banks[0].mtd_part = dab4k_partitions;
	part_banks[0].type = "Static image";
	part_banks[0].nums = NB_OF(dab4k_partitions);
#if 0
	part_banks[1].mtd_part = dab4k_fs_partitions;
	part_banks[1].type = "Static file system";
	part_banks[1].nums = NB_OF(dab4k_fs_partitions);
#endif
	for(idx = 0; idx < num_banks ; idx++)
	{
		if (part_banks[idx].nums == 0) {
			printk(KERN_NOTICE "DAB4K flash%d: no partition info available, registering whole flash at once\n", idx);
			add_mtd_device(mtd_banks[idx]);
		} else {
			printk(KERN_NOTICE "DAB4K flash%d: Using %s partition definition\n",
					idx, part_banks[idx].type);
			add_mtd_partitions(mtd_banks[idx], part_banks[idx].mtd_part, 
								part_banks[idx].nums);
		}
	}
#else
	printk(KERN_NOTICE "DAB4K flash: registering %d whole flash banks at once\n", num_banks);
	for(idx = 0 ; idx < num_banks ; idx++)
		add_mtd_device(mtd_banks[idx]);
#endif
	return 0;
error_mem:
	for(idx = 0 ; idx < FLASH_BANK_MAX ; idx++)
	{
		if(map_banks[idx] != NULL)
		{
			if(map_banks[idx]->name != NULL)
			{
				kfree(map_banks[idx]->name);
				map_banks[idx]->name = NULL;
			}
			kfree(map_banks[idx]);
			map_banks[idx] = NULL;
		}
	}
	//return -ENOMEM;
error:
	iounmap((void *)start_scan_addr);
	//return -ENXIO;
	return ret;
}

static void __exit cleanup_dab4k_mtd(void)
{
	unsigned int idx = 0;
	for(idx = 0 ; idx < num_banks ; idx++)
	{
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

module_init(init_dab4k_mtd);
module_exit(cleanup_dab4k_mtd);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kirk Lee <kirk@hpc.ee.ntu.edu.tw>");
MODULE_DESCRIPTION("MTD map driver for DAB4K boards");
