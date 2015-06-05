/*
 * Copyright(C) 2003
 * Wolfgang Denk, DENX Software Engineering, <wd@denx.de>
 *
 * This code is GPLed
 * 
 */

/*
 * SL8245 uses 4 x AMD AM29LV160BT (16 Mbit)
 * for a total of 8 MB flash in one bank
 *
 * Thus we have to chose:
 * - Support 64-bit buswidth => CONFIG_MTD_CFI_B8
 * - Support 4-chip flash interleave => CONFIG_MTD_CFI_I4
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <asm/ppcboot.h>

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

/* The "residual" data board information structure the boot loader
 * hands to us.
 */
extern unsigned char __res[];

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

static __u8 sl8245_read8(struct map_info *map, unsigned long ofs)
{
	return *((__u8 *)(map->map_priv_1 + ofs));
}

static __u16 sl8245_read16(struct map_info *map, unsigned long ofs)
{
	return *((__u16 *)(map->map_priv_1 + ofs));
}

static __u32 sl8245_read32(struct map_info *map, unsigned long ofs)
{
	return *((__u32 *)(map->map_priv_1 + ofs));
}

static __u64 sl8245_read64(struct map_info *map, unsigned long ofs)
{
	        return *((volatile __u64 *)(map->map_priv_1 + ofs));
}

static void sl8245_copy_from(struct map_info *map,
		       void *to, unsigned long from, ssize_t len)
{
	memcpy_fromio(to, (void *)(map->map_priv_1 + from), len);
}

static void sl8245_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	*((__u8 *)(map->map_priv_1 + adr)) = d;
}

static void sl8245_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	*((__u16 *)( map->map_priv_1 + adr)) = d;
}

static void sl8245_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	*((__u32 *)(map->map_priv_1 + adr)) = d;
}

static void sl8245_write64(struct map_info *map, __u64 d, unsigned long adr)
{
	ulong addr = map->map_priv_1 + adr;
	__u64 * data = &d;
	ulong flags;
	ulong msr;
	ulong saved_msr;
	volatile long saved_fr[2];

	save_flags(flags);
	cli();

	__asm__ __volatile__ ("mfmsr %0" : "=r" (msr):);
	saved_msr = msr;
	msr |= MSR_FP;
	msr &= ~(MSR_FE0 | MSR_FE1);

	__asm__ __volatile__ (
		"mtmsr %0\n"
		"isync\n"
		:
		: "r" (msr));

	__asm__ __volatile__ (
		"stfd 1, 0(%2)\n"  
		"lfd  1, 0(%0)\n"
		"stfd 1, 0(%1)\n"
		"lfd  1, 0(%2)\n"
		 :
		 : "r" (data), "r" (addr), "b" (saved_fr)
	);

	__asm__ __volatile__ (
		"mtmsr %0\n"
		"isync\n"
		:
		: "r" (saved_msr));

	restore_flags(flags);
}

static void sl8245_copy_to(struct map_info *map,
		     unsigned long to, const void *from, ssize_t len)
{
	memcpy_toio((void *)(map->map_priv_1 + to), from, len);
}

/*
 * The following defines the partition layout of SL8245 boards.
 *
 * See include/linux/mtd/partitions.h for definition of the
 * mtd_partition structure.
 */

#ifdef CONFIG_MTD_PARTITIONS

/* partition definition for first (and only) flash bank
 * also ref. to "drivers/char/flash_config.c" 
 */
static struct mtd_partition sl8245_partitions[] = {
    {
	name:   "ramdisk",                      /* Ramdisk image        */
	offset: 0,
	size:   6400 << 10,                     /* 6400 kB = 6.25  MB   */
/*	mask_flags: MTD_WRITEABLE,              / * force read-only     */
    },
    {
	name:   "kernel",                       /* Linux kernel image   */
	offset: 6400 << 10,
	size:    768 << 10,                     /*  768 kB = 0.75 MB    */
/*      mask_flags: MTD_WRITEABLE,              / * force read-only     */
    },
    {
	name:   "U-Boot",                       /* U-Boot image         */
	offset:   7 << 20,
	size:   256 << 10,                      /*  256 kB              */
    },
    {
	name:   "data 1",                       /* data section 1       */
	offset: 7424 << 10,
	size:    256 << 10,                     /* 256 kB               */
    },
    {
	name:   "data 2",                       /* data section 2       */
	offset: 7680 << 10,
	size:    256 << 10,                     /* 256 kB               */
    },
    {
	name:   "data 3",                       /* data section 3       */
	offset: 7936 << 10,
	size:    128 << 10,                     /* 128 kB               */
    },
    {
	name:   "data 4",                       /* data section 4       */
	offset: 8064 << 10,
	size:     32 << 10,                     /* 32 kB                */
    },
    {
	name:   "data 5",                       /* data section 5       */
	offset: 8096 << 10,
	size:     32 << 10,                     /* 32 kB                */
    },
    {
	name:   "Environment",                  /* U-Boot Environment   */
	offset: 8128 << 10,
	size:     64 << 10,                     /* 64 kB                */
    },
};

#ifdef CONFIG_MTD_CMDLINE_PARTS
int parse_cmdline_partitions(struct mtd_info *master,
			     struct mtd_partition **pparts,
			     const char *mtd_id);
#endif	/* CONFIG_MTD_CMDLINE_PARTS */

#endif	/* CONFIG_MTD_PARTITIONS */

#define NB_OF(x)  (sizeof(x)/sizeof(x[0]))

int __init init_sl8245_mtd(void)
{
	int idx = 0, ret = 0;
	unsigned long flash_addr, flash_size, mtd_size = 0;
	/* pointer to SL8245 board info data */
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
		
		sprintf(map_banks[idx]->name, "SL8245 Bank %d", idx);
		map_banks[idx]->size      = 0x100;
		map_banks[idx]->buswidth  = 8;
		map_banks[idx]->read8     = sl8245_read8;
		map_banks[idx]->read16    = sl8245_read16;
		map_banks[idx]->read32    = sl8245_read32;
		map_banks[idx]->read64    = sl8245_read64;
		map_banks[idx]->copy_from = sl8245_copy_from;
		map_banks[idx]->write8    = sl8245_write8;
		map_banks[idx]->write16   = sl8245_write16;
		map_banks[idx]->write32   = sl8245_write32;
		map_banks[idx]->write64   = sl8245_write64;
		map_banks[idx]->copy_to   = sl8245_copy_to;
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
		printk("SL8245: No supported flash chips found!\n");
		ret = -ENXIO;
		goto error_mem;
	}

#ifdef CONFIG_MTD_PARTITIONS
	/*
	 * Select static partition definitions
	 */
	part_banks[0].mtd_part	= sl8245_partitions;
	part_banks[0].type	= "static image";
	part_banks[0].nums	= NB_OF(sl8245_partitions);

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
			    "SL8245 flash bank %d: no partition info "
			    "available, registering whole device\n", idx);
		    add_mtd_device(mtd_banks[idx]);
		} else {
		    printk (KERN_NOTICE 
			    "SL8245 flash bank %d: Using %s partition "
			    "definition\n", idx, part_banks[idx].type);
		    add_mtd_partitions (mtd_banks[idx],
					part_banks[idx].mtd_part, 
					part_banks[idx].nums);
		}
	}
#else	/* ! CONFIG_MTD_PARTITIONS */
	printk (KERN_NOTICE "SL8245 flash: registering %d flash banks "
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

static void __exit cleanup_sl8245_mtd(void)
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

module_init(init_sl8245_mtd);
module_exit(cleanup_sl8245_mtd);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wolfgang Denk <wd@denx.de>");
MODULE_DESCRIPTION("MTD map driver for SL8245 boards");
