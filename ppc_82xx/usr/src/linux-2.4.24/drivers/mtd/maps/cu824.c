/*
 * Handle mapping of the flash memory access routines
 * on MicroSys CU824 Board.
 *
 * based on impa7.c
 *
 * Sandor den Braven, Boskalis
 *
 * This code is GPLed
 *
 */

/*
 * The CU824 has 2 flash banks with
 * 4 x Intel 28F160F3 (16 Mbit) in one bank (64 bit buswidth)
 * for a total of 16MB flash
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

#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#undef	DEBUG

#ifdef	DEBUG
# define debugk(fmt,args...)	printk(fmt ,##args)
#else
# define debugk(fmt,args...)
#endif

#define WINDOW_ADDR0 0xFF000000
#define WINDOW_SIZE0 0x00800000
#define WINDOW_ADDR1 0xFF800000
#define WINDOW_SIZE1 0x00800000
#define NUM_FLASHBANKS 2
#define BUSWIDTH 8

/* can be { "cfi_probe", "jedec_probe", "map_rom", 0 }; */
#define PROBETYPES { "jedec_probe", 0 }

#define MSG_PREFIX "cu824:"	/* prefix for our printk()'s */
#define MTDID	   "cu824-%d"	/* for mtdparts= partitioning */

static struct mtd_info *cu824_mtd[NUM_FLASHBANKS];

static __u8 cu824_read8 (struct map_info *map, unsigned long ofs)
{
	return *((__u8 *) (map->map_priv_1 + ofs));
}

static __u16 cu824_read16 (struct map_info *map, unsigned long ofs)
{
	return *((__u16 *) (map->map_priv_1 + ofs));
}

static __u32 cu824_read32 (struct map_info *map, unsigned long ofs)
{
	return *((__u32 *) (map->map_priv_1 + ofs));
}

static __u64 cu824_read64 (struct map_info *map, unsigned long ofs)
{
	return *((__u64 *) (map->map_priv_1 + ofs));
}

static void cu824_copy_from (struct map_info *map,
			      void *to, unsigned long from, ssize_t len)
{
	memcpy_fromio (to, (void *) (map->map_priv_1 + from), len);
}

static void cu824_write8 (struct map_info *map, __u8 d, unsigned long adr)
{
	*((__u8 *) (map->map_priv_1 + adr)) = d;
}

static void cu824_write16 (struct map_info *map, __u16 d,
			   unsigned long adr)
{
	*((__u16 *) (map->map_priv_1 + adr)) = d;
}

static void cu824_write32 (struct map_info *map, __u32 d,
			   unsigned long adr)
{
	*((__u32 *) (map->map_priv_1 + adr)) = d;
}

static void cu824_write64 (struct map_info *map, __u64 d,
			   unsigned long adr)
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

static void cu824_copy_to (struct map_info *map,
			   unsigned long to, const void *from, ssize_t len)
{
	memcpy_toio ((void *) (map->map_priv_1 + to), from, len);
}

static struct map_info cu824_map[NUM_FLASHBANKS]  = {
    {
	name:	   "cu824 Flash Bank #0",
	size:	   WINDOW_SIZE0,
	buswidth:  BUSWIDTH,
	read8:	   cu824_read8,
	read16:	   cu824_read16,
	read32:	   cu824_read32,
	read64:    cu824_read64,
	copy_from: cu824_copy_from,
	write8:	   cu824_write8,
	write16:   cu824_write16,
	write32:   cu824_write32,
	write64:   cu824_write64,
	copy_to:   cu824_copy_to
    },
    {
	name:	   "cu824 Flash Bank #1",
	size:	   WINDOW_SIZE1,
	buswidth:  BUSWIDTH,
	read8:	   cu824_read8,
	read16:	   cu824_read16,
	read32:	   cu824_read32,
	read64:    cu824_read64,
	copy_from: cu824_copy_from,
	write8:	   cu824_write8,
	write16:   cu824_write16,
	write32:   cu824_write32,
	write64:   cu824_write64,
	copy_to:   cu824_copy_to
    }
};

/*
 * The following defines the partition layout of CU824 boards.
 *
 * See include/linux/mtd/partitions.h for definition of the
 * mtd_partition structure.
 *
 * The *_max_flash_size is the maximum possible mapped flash size
 * which is not necessarily the actual flash size. It must correspond
 * to the value specified in the mapping definition defined by the
 * "struct map_desc *_io_desc" for the corresponding machine.
 */

#ifdef CONFIG_MTD_PARTITIONS

/* partition definition for first (and only) flash bank
 * also ref. to "drivers/char/flash_config.c"
 * (we split the first partition here into 1 MB kernel + 3 MB ramdisk)
 */
static struct mtd_partition cu824_partitions_banks[][4] = {
  /* Bank 1 */
  {
    {
	name:	"kernel",			/* Linux kernel image	*/
	offset:	0,
	size:	1 << 20,			/*  0 ...  4 MB => 1 MB */
	/*  mask_flags: MTD_WRITEABLE,		/ * force read-only	*/
    },
    {
	name:	"initrd",			/* Ramdisk image	*/
	offset:	1 << 20,
	size:	3 << 20,			/*  1 ...  4 MB => 3 MB */
    },
    {
	name:	"jffs",				/* JFFS	 filesystem	*/
	offset:	4 << 20,
	size:	4 << 20,			/*  4 ...  8 MB => 4 MB */
    },
  },
  /* Bank 2 */
  {
    {
	name:	"jffs bank #1",			/* JFFS  filesystem	*/
	offset:	0,
	size:	7 << 20,			/*  0 ...  7 MB => 7 MB	*/
    },
    {
	name:	"ppcboot",			/* PPCBoot Firmware	*/
	offset:	7 << 20,
	size:	256 << 10,			/* 256 kB		*/
	/*  mask_flags: MTD_WRITEABLE,		/ * force read-only	*/
    },
    {
	name:	"free",				/* free			*/
	offset:	((7 << 20) + (256 << 10)),
	size:	512 << 10,			/* 512 kB		*/
    },
    {
	name:	"environment",			/* PPCBoot Environment	*/
	offset:	((7 << 20) + (768 << 10)),
	size:	256 << 10,			/* 256 kB		*/
    },
  },
};

#define NB_OF(x) (sizeof (x) / sizeof (x[0]))

#ifdef CONFIG_MTD_CMDLINE_PARTS
int parse_cmdline_partitions (struct mtd_info *master,
			      struct mtd_partition **pparts,
			      const char *mtd_id);
#endif	/* CONFIG_MTD_CMDLINE_PARTS */

#endif	/* CONFIG_MTD_PARTITIONS */

static int mtd_parts_nb = 0;
static struct mtd_partition *mtd_parts = 0;

int __init init_cu824_mtd (void)
{
	static const char *probe_types[] = PROBETYPES;
	const char **type;
	const char *part_type = 0;
	static struct {
		u_long addr;
		u_long size;
	} pt[NUM_FLASHBANKS] = {
		{ WINDOW_ADDR0, WINDOW_SIZE0 },
		{ WINDOW_ADDR1, WINDOW_SIZE1 },
	};
	int devicesfound = 0;
	int i;
#if defined(CONFIG_MTD_PARTITIONS) && defined(CONFIG_MTD_CMDLINE_PARTS)
	char mtdid[10];
#endif

	for (i = 0; i < NUM_FLASHBANKS; i++) {
		printk (KERN_NOTICE MSG_PREFIX
			"probing 0x%08lx at 0x%08lx\n",
				pt[i].size, pt[i].addr);

		cu824_map[i].map_priv_1 =
			(unsigned long) ioremap (pt[i].addr, pt[i].size);

		if (!cu824_map[i].map_priv_1) {
			printk (MSG_PREFIX "failed to ioremap\n");
			return -EIO;
		}
		cu824_mtd[i] = 0;
		type = probe_types;
		for (; !cu824_mtd[i] && *type; type++) {
			cu824_mtd[i] = do_map_probe ("jedec_probe", &cu824_map[i]);
		}

		if (cu824_mtd[i]) {
			cu824_mtd[i]->module = THIS_MODULE;
			add_mtd_device (cu824_mtd[i]);
			devicesfound++;
#ifdef CONFIG_MTD_PARTITIONS
#ifdef CONFIG_MTD_CMDLINE_PARTS
			sprintf (mtdid, MTDID, i);
			mtd_parts_nb = parse_cmdline_partitions (cu824_mtd[i],
													 &mtd_parts, mtdid);
			if (mtd_parts_nb > 0)
				part_type = "command line";
#endif	/* CONFIG_MTD_CMDLINE_PARTS */
			mtd_parts = cu824_partitions_banks[i];
			mtd_parts_nb = NB_OF(cu824_partitions_banks[i]);

			part_type = "static";

			if (mtd_parts_nb <= 0) {
				printk (KERN_NOTICE MSG_PREFIX
					"no partition info available\n");
			} else {
				printk (KERN_NOTICE MSG_PREFIX
					"using %s partition definition\n",
					part_type);
				add_mtd_partitions (cu824_mtd[i],
						    mtd_parts,
						    mtd_parts_nb);
			}
#endif	/* CONFIG_MTD_PARTITIONS */
		} else {
			iounmap ((void *) cu824_map[i].map_priv_1);
		}
	}

	return devicesfound == 0 ? -ENXIO : 0;
}

static void __exit cleanup_cu824_mtd (void)
{
	int i;

	for (i = 0; i < NUM_FLASHBANKS; i++) {
		if (cu824_mtd[i]) {
			del_mtd_device (cu824_mtd[i]);
			map_destroy (cu824_mtd[i]);
		}
		if (cu824_map[i].map_priv_1) {
			iounmap ((void *) cu824_map[i].map_priv_1);
			cu824_map[i].map_priv_1 = 0;
		}
	}
}

module_init (init_cu824_mtd);
module_exit (cleanup_cu824_mtd);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Sandor den Braven <s.denbraven@boskalis.nl>");
MODULE_DESCRIPTION ("MTD map driver for CU824 boards");
