/*
 * drivers/mtd/maps/icecube.c
 *
 * MTD mapping driver for Motorola IceCube board
 *
 * 2003 (c) Wolfgang Denk, DENX Software Engineering, <wd@denx.de>.  This
 * file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of
 * any kind, whether express or implied.
 */

/*
 * Icecube uses 1 x AMD AM29LV065 (64 Mbit)
 * for a total of 8 MiB flash in one bank
 * or AM29LV652 (two LV065 in one chip) for 16 MiB.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>
#ifdef CONFIG_UBOOT
#include <asm/ppcboot.h>
#endif

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
#ifdef CONFIG_MTD_PARTITIONS
static struct mtd_part_def part_banks[FLASH_BANK_MAX];
#endif
static unsigned long num_banks;
static unsigned long start_scan_addr;

static __u8 icecube_read8(struct map_info *map, unsigned long ofs)
{
	return *((__u8 *)(map->map_priv_1 + ofs));
}

static __u16 icecube_read16(struct map_info *map, unsigned long ofs)
{
	return *((__u16 *)(map->map_priv_1 + ofs));
}

static __u32 icecube_read32(struct map_info *map, unsigned long ofs)
{
	return *((__u32 *)(map->map_priv_1 + ofs));
}

static __u64 icecube_read64(struct map_info *map, unsigned long ofs)
{
	return *((volatile __u64 *)(map->map_priv_1 + ofs));
}

static void *flash_memcpy_fromio(void *to, void *from, ssize_t len)
{
	void *ptr = to;
	while(len--) {
		*(char *)to++ = *(char *)from++;
	}
	return  ptr;
}

static void icecube_copy_from(struct map_info *map,
		       void *to, unsigned long from, ssize_t len)
{
#if 0
	memcpy_fromio(to, (void *)(map->map_priv_1 + from), len);
#else
	/* workaround for misaligned memory access: use byte copy. */
	flash_memcpy_fromio(to, (void *)(map->map_priv_1 + from), len);
#endif
}

static void icecube_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	*((__u8 *)(map->map_priv_1 + adr)) = d;
}

static void icecube_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	*((__u16 *)( map->map_priv_1 + adr)) = d;
}

static void icecube_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	*((__u32 *)(map->map_priv_1 + adr)) = d;
}

static void icecube_write64(struct map_info *map, __u64 d, unsigned long adr)
{
		*((__u64 *)(map->map_priv_1 + adr)) = d;
}

static void icecube_copy_to(struct map_info *map,
		     unsigned long to, const void *from, ssize_t len)
{
	memcpy_toio((void *)(map->map_priv_1 + to), from, len);
}

/*
 * The following defines the partition layout of Icecube boards.
 *
 * See include/linux/mtd/partitions.h for definition of the
 * mtd_partition structure.
 */

#ifdef CONFIG_MTD_PARTITIONS

/* partition definition for first (and only) flash bank
 * also ref. to "drivers/char/flash_config.c" 
 */
static struct mtd_partition icecube_partitions_16M[] = {
    {
	name:	"Spare",			/* first bank	*/
	offset:	0,
	size:	8 << 20,			/*  0 ...  7 MiB => 8 MiB */
    },
    {
	name:	"kernel",			/* Linux kernel image	*/
	offset: 8 << 20,
	size:	1 << 20,			/*  1MiB		*/
/*	mask_flags: MTD_WRITEABLE,		/ * force read-only	*/
    },
    {
	name:	"initrd",			/* Ramdisk image	*/
	offset:	9 << 20,
	size:	3 << 20,			/*  3 MiB		*/
    },
	{
	name:	"jffs",				/* JFFS(2) filesystem	*/
	offset:	12 << 20,
	size:	3 << 20,			/*  4 ...  7 MiB => 3 MiB */
    },
	{
	name:	"Firmware",			/* dBUG or U-Boot firmware */
	offset:	15 << 20,
	size:	1 << 20,			/*  7 ...  8 MiB => 1 MiB */
    }
};

static struct mtd_partition icecube_partitions_8M[] = {
    {
	name:	"kernel",			/* Linux kernel image	*/
	offset: 0,
	size:	1 << 20,			/*  1MiB		*/
/*	mask_flags: MTD_WRITEABLE,		/ * force read-only	*/
    },
    {
	name:	"initrd",			/* Ramdisk image	*/
	offset:	1 << 20,
	size:	3 << 20,			/*  3 MiB		*/
    },
	{
	name:	"jffs",				/* JFFS(2) filesystem	*/
	offset:	4 << 20,
	size:	3 << 20,			/*  4 ...  7 MiB => 3 MiB */
    },
	{
	name:	"Firmware",			/* dBUG or U-Boot firmware */
	offset:	7 << 20,
	size:	1 << 20,			/*  7 ...  8 MiB => 1 MiB */
    }
};

#ifdef CONFIG_MTD_CMDLINE_PARTS
int parse_cmdline_partitions(struct mtd_info *master,
			     struct mtd_partition **pparts,
			     const char *mtd_id);
#endif	/* CONFIG_MTD_CMDLINE_PARTS */

#endif	/* CONFIG_MTD_PARTITIONS */

#define NB_OF(x)  (sizeof(x)/sizeof(x[0]))

int __init init_icecube_mtd(void)
{
	int idx = 0, ret = 0;
	unsigned long flash_addr, flash_size, mtd_size = 0;
#ifdef CONFIG_MTD_CMDLINE_PARTS
	int n;
	char mtdid[4];
#endif
#ifdef CONFIG_UBOOT
	/* pointer to Icecube board info data */
	extern unsigned char __res[];
	bd_t *bd = (bd_t *)__res;

	flash_addr = bd->bi_flashstart;
	flash_size = bd->bi_flashsize;
#else
	flash_addr = CONFIG_PPC_5xxx_FLASH_ADDR;
	flash_size = CONFIG_PPC_5xxx_FLASH_SIZE;
#endif

	if (flash_size == 0x800000)
		flash_addr |= 0x800000; /* 8MiB flash starts at 0xff800000 */

	/* request maximum flash size address space */
	start_scan_addr = flash_addr; /* already mapped */

	for(idx = 0 ; idx < FLASH_BANK_MAX ; idx++) {
		if (mtd_size >= flash_size)
			break;
		
		debugk ("%s: chip probing count %d\n", __FUNCTION__, idx);
		
		map_banks[idx] = (struct map_info *)kmalloc(
					sizeof(struct map_info), GFP_KERNEL);
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
		
		sprintf(map_banks[idx]->name, "Icecube Bank %d", idx);
		map_banks[idx]->size      = flash_size;
		map_banks[idx]->buswidth  = 1;
		map_banks[idx]->read8     = icecube_read8;
		map_banks[idx]->read16    = icecube_read16;
		map_banks[idx]->read32    = icecube_read32;
		map_banks[idx]->read64    = icecube_read64;
		map_banks[idx]->copy_from = icecube_copy_from;
		map_banks[idx]->write8    = icecube_write8;
		map_banks[idx]->write16   = icecube_write16;
		map_banks[idx]->write32   = icecube_write32;
		map_banks[idx]->write64   = icecube_write64;
		map_banks[idx]->copy_to   = icecube_copy_to;
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
		printk("Icecube: No supported flash chips found!\n");
		ret = -ENXIO;
		goto error_mem;
	}

#ifdef CONFIG_MTD_PARTITIONS
	/*
	 * Select static partition definitions
	 */
	if (flash_size == 0x800000) {
		part_banks[0].mtd_part	= icecube_partitions_8M;
		part_banks[0].nums	= NB_OF(icecube_partitions_8M);
	} else {
		part_banks[0].mtd_part	= icecube_partitions_16M;
		part_banks[0].nums	= NB_OF(icecube_partitions_16M);
	}
	part_banks[0].type	= "static image";

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
			    "Icecube flash bank %d: no partition info "
			    "available, registering whole device\n", idx);
		    add_mtd_device(mtd_banks[idx]);
		} else {
		    printk (KERN_NOTICE 
			    "Icecube flash bank %d: Using %s partition "
			    "definition\n", idx, part_banks[idx].type);
		    add_mtd_partitions (mtd_banks[idx],
					part_banks[idx].mtd_part, 
					part_banks[idx].nums);
		}
	}
#else	/* ! CONFIG_MTD_PARTITIONS */
	printk (KERN_NOTICE "Icecube flash: registering %ld flash banks "
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

static void __exit cleanup_icecube_mtd(void)
{
	unsigned int idx = 0;
	for(idx = 0 ; idx < num_banks ; idx++) {
		/* destroy mtd_info previously allocated */
		if (mtd_banks[idx]) {
#ifdef CONFIG_MTD_PARTITIONS
			del_mtd_partitions(mtd_banks[idx]);
#endif
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

module_init(init_icecube_mtd);
module_exit(cleanup_icecube_mtd);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wolfgang Denk <wd@denx.de>");
MODULE_DESCRIPTION("MTD map driver for Icecube board");
