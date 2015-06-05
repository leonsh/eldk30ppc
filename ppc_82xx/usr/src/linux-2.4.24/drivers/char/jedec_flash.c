/*
 * linux/drivers/char/jedec_flash.c  Version 0.1
 *
 * Copyright (C) 1999 Alexander Larsson (alla@lysator.liu.se or alex@signum.se)
 * Copyright (C) 2000, 2001, 2002 Wolfgang Denk, wd@denx.de
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/flash.h>

#include <asm/byteorder.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/residual.h>

typedef	volatile unsigned int	vu_int;
typedef	volatile unsigned short	vu_short;
typedef	volatile unsigned char	vu_char;

extern void write_amd_command(struct embedded_flash *flash, off_t offset, u_char data);

#undef	DEBUGFLASH

#ifdef DEBUGFLASH
#define DEBUG(fmt,args...) printk(fmt ,##args)
#else
#define DEBUG(fmt,args...)
#endif


#ifdef CONFIG_AMD_FLASH
extern void __init amd_flash_init(struct flash_device_info *fdi);
extern void __init amd_flash_compat_init(struct flash_device_info *fdi);
#endif

#ifdef CONFIG_INTEL_FLASH
extern void __init intel_flash_init(struct flash_device_info *fdi);
#endif

struct region_definition {
  unsigned long size;
  u_short count;
};

struct jedec_part_table {
  u_short vendor;
  u_short device;
  unsigned long size;
  void (* init_ftn)(struct flash_device_info *fdi);
  char *name;
  struct region_definition *region;
};

#if defined(CONFIG_AMD_FLASH)
static struct region_definition Am29LV800BT[] = {
  {(64 * 1024), 15},
  {(32 * 1024), 1},
  {( 8 * 1024), 2},
  {(16 * 1024), 1},
  {0, 0}
};
static struct region_definition Am29LV800BB[] = {
  {(16 * 1024), 1},
  {( 8 * 1024), 2},
  {(32 * 1024), 1},
  {(64 * 1024), 15},
  {0, 0}
};
static struct region_definition Am29x040B[] = {
  {(64 * 1024), 8},
  {0, 0}
};
#endif

#if defined(CONFIG_INTEL_FLASH)
/* This is in fact a Micron chip */
static struct region_definition I28F400B3[] = {
  {(128 * 1024), 3},
  {( 96 * 1024), 1},
  {(  8 * 1024), 2},
  {( 16 * 1024), 1},
  {0, 0}
};

static struct region_definition I28F016SV[] = {
  {(64 * 1024), 32},
  {0, 0}
};

static struct region_definition I28F160F3[] = {
  {(8  * 1024),  8},
  {(64 * 1024), 31},
  {0, 0}
};

#endif

static struct jedec_part_table known_parts[] = {
/* VENDOR DEVICE   BYTES     INIT_FTN        NAME*/
#if defined(CONFIG_AMD_FLASH)
  {1,     0xDA,    0x100000, amd_flash_init, "AMD Am29LV800BT", Am29LV800BT},
  {1,     0x22DA,  0x100000, amd_flash_init, "AMD Am29LV800BT", Am29LV800BT},
  {1,     0x5B,    0x100000, amd_flash_init, "AMD Am29LV800BB", Am29LV800BB},
  {1,     0x225B,  0x100000, amd_flash_init, "AMD Am29LV800BB", Am29LV800BB},
  {0x20,  0x5B,    0x100000, amd_flash_init, "STM M29W800AB",   Am29LV800BB},
  {1,     0x4f,    0x080000, amd_flash_init, "AMD Am29LV040B",  Am29x040B},
  {1,     0xa4,    0x080000, amd_flash_compat_init, "AMD Am29F040B",   Am29x040B},
#endif
#if defined(CONFIG_INTEL_FLASH)
  {0x89,  0x4470,  0x080000, intel_flash_init, "Micron 28F400B3", I28F400B3},
  {0x89,  0x66a0,  0x200000, intel_flash_init, "Intel 28F016SV",  I28F016SV},
  {0x89,  0x88f4,  0x200000, intel_flash_init, "Intel 28F160F3",  I28F160F3},
#endif
  {0,     0,       0,           0,                 0}
};


/* Ultimately a function like this will return a pointer to drivers */
static int jedec_part_found(u_short primary_vendor, u_short device_id)
{
  int part;
  for (part=0; known_parts[part].vendor != 0; part++) {
    if ((primary_vendor == known_parts[part].vendor) &&
	(device_id == known_parts[part].device)) {
      break;
    }
  }
  if (known_parts[part].vendor) {
    DEBUG("Found %s, size=0x%lx\n", known_parts[part].name, known_parts[part].size);
  } else {
    part = -1;
    DEBUG("Found unknown flash: vendor 0x%x, device 0x%x\n", primary_vendor, device_id);
  }
  return part;
}

static u_int read_with_chip_width(struct embedded_flash *flash, u_long offs)
{
  u_int data;
  vu_char *ptr = flash->ptr + (offs << flash->addr_shft);

  DEBUG("%s[%d] bit_width=%d chip_cfg=0x%x => case 0x%x\n",
    __FUNCTION__,__LINE__,
    flash->bit_width, flash->chip_cfg, flash->bit_width - flash->chip_cfg);

  switch(flash->bit_width - flash->chip_cfg) {
  case 0: /* 8bit */
    data = *ptr;
    DEBUG("read %lx (8 bit) => %x\n", (unsigned long)ptr, data);
    break;
  case 1: /* 16bit */
    data = (*(u_short *)ptr);
    DEBUG("read %lx (16 bit) => %x\n", (unsigned long)ptr, data);
    break;
  case 2: /* 32bit */
    data = (*(u_int *)ptr);
    DEBUG("read %lx (32 bit) => %x\n", (unsigned long)ptr, data);
    break;
  }
  return data;
}

unsigned long __init jedec_flash_probe(volatile unsigned char *ptr, unsigned long phys_addr)
{
  struct embedded_flash *flash;
  unsigned char  readqry[8];
  unsigned char  qry[8];
  int found_flash;
  int bit_width;
  int chip_cfg;
  u_short primary_vendor;
  u_short device_id;
  u_long size;
  int i;
  int part;
  u_long offset;
  flash_region_info_t *region;

  /* Allocate flash record, since probe requires it */
  flash = kmalloc(sizeof(struct embedded_flash), GFP_KERNEL);
  if (!flash) {
    return 0;
  }

  size = 0;
  found_flash = 0;
  /* Start by reading while in READ ARRAY mode, as a reference, */
  /* to know when the flash is in query mode.                   */
  for (i=0; i<8; i++)
    readqry[i] = ptr[i];

  /* Probe at ptr[0] for JEDEC ID */
  part = -1;
#ifdef CONFIG_CU824
  ((volatile u_long *) ptr)[0] = 0x90909090;
#else
  ptr[0] = 0x90;
  ptr[1] = 0x90;
  ptr[2] = 0x90;
  ptr[3] = 0x90;
#endif

  /* extra read for time, but isn't it optimized away? */
  qry[0] = ptr[0];
  qry[0] = ptr[0];
  qry[1] = ptr[1];
  qry[2] = ptr[2];
  qry[3] = ptr[3];
  qry[4] = ptr[4];
  qry[5] = ptr[5];
  qry[6] = ptr[6];
  qry[7] = ptr[7];

  for (i=0; i<8; i++) {
    if (readqry[i] != qry[i])
      break;
  }

  if (i < 8) {
    if ((qry[0] == 0x90) &&
	(qry[1] == 0x90) &&
	(qry[2] == 0x90) &&
	(qry[3] == 0x90)) {

      /* OOPS! THIS IS RAM */
      DEBUG("Found RAM\n");
      return 0;

#ifdef  __BIG_ENDIAN
    } else if ((qry[0] == 0x00)   && (qry[1] != 0x00)   &&
	       (qry[2] == 0x00)   && (qry[3] != 0x00)   &&
	       (qry[4] == qry[0]) && (qry[5] == qry[1]) &&
	       (qry[6] == qry[2]) && (qry[7] == qry[3])) {
#else
    } else if ((qry[0] != 0x00)   && (qry[1] == 0x00)   &&
	       (qry[2] != 0x00)   && (qry[3] == 0x00)   &&
	       (qry[4] == qry[0]) && (qry[5] == qry[1]) &&
	       (qry[6] == qry[2]) && (qry[7] == qry[3])) {
#endif
      DEBUG("Found 64/16 JEDEC Flash\n");
      found_flash = 1;
      bit_width = 3;
      chip_cfg = 2;
#ifdef  __BIG_ENDIAN
    } else if ((qry[0] == 0x00) && (qry[1] == 0x00) &&
	       (qry[2] == 0x00) && (qry[3] != 0x00)) {
#else
    } else if ((qry[0] != 0x00) && (qry[1] == 0x00) &&
	       (qry[2] == 0x00) && (qry[3] == 0x00)) {
#endif
      DEBUG("Found 32/32 JEDEC Flash\n");
      found_flash = 1;
      bit_width = 2;
      chip_cfg = 0;
#ifdef  __BIG_ENDIAN
    } else if ((qry[0] == 0x00) && (qry[1] != 0x00) &&
	       (qry[2] == 0x00) && (qry[3] == qry[1])) {
#else
    } else if ((qry[0] != 0x00) && (qry[1] == 0x00) &&
	       (qry[2] == qry[0]) && (qry[3] == 0x00)) {
#endif
      DEBUG("Found 32/16 JEDEC Flash\n");
      found_flash = 1;
      bit_width = 2;
      chip_cfg = 1;
#ifdef  __BIG_ENDIAN
    } else if ((qry[0] == 0x00) && (qry[1] != 0x00)) {
#else
    } else if ((qry[0] != 0x00) && (qry[1] == 0x00)) {
#endif
      DEBUG("Found 16/16 JEDEC Flash\n");
      found_flash = 1;
      bit_width = 1;
      chip_cfg = 0;
    } else if ((qry[0] == qry[1]) &&
	       (qry[0] == qry[2]) &&
	       (qry[0] == qry[3]) &&
	       (qry[0] == qry[4])) {
      DEBUG("Found 32/8 JEDEC Flash\n");
      found_flash = 1;
      bit_width = 2;
      chip_cfg = 2;
    } else if ((qry[0] == qry[1]) &&
	       (qry[2] == qry[3])) {
      DEBUG("Found 16/8 JEDEC Flash\n");
      found_flash = 1;
      bit_width = 1;
      chip_cfg = 1;
    } else {
      DEBUG("Found 8/8 JEDEC Flash\n");
      found_flash = 1;
      bit_width = 0;
      chip_cfg = 0;
    }

    if (found_flash) {
      flash->ptr = ptr;
      flash->bit_width = bit_width;
      flash->addr_shft = bit_width;
      flash->chip_cfg  = chip_cfg;
      primary_vendor = read_with_chip_width(flash, 0);
      device_id = read_with_chip_width(flash, 1);
      part = jedec_part_found(primary_vendor, device_id);
      DEBUG("Vendor = 0x%x, Device = 0x%x, part = 0x%x\n",
	primary_vendor, device_id, part);
    }
  }

  /*
   * Probe for AMD JEDEC part
   */
#if defined(CONFIG_AMD_FLASH)
  if (!found_flash) {
    flash->ptr = ptr;
    DEBUG("Probing for AMD JEDEC FLASH at %lx\n", phys_addr);
    bit_width = 2;
    while(part == -1) {
      chip_cfg = bit_width;
      while(part == -1) {
	/* Return to read array mode: */
	ptr[0] = 0xff;
	ptr[1] = 0xff;
	ptr[2] = 0xff;
	ptr[3] = 0xff;

	flash->bit_width = bit_width;
	flash->addr_shft = bit_width;
	flash->chip_cfg  = chip_cfg;
	DEBUG("Probing with bit_width %d, addr_shft %d, chip_cfg %d\n",
		flash->bit_width, flash->addr_shft, flash->chip_cfg);
	write_amd_command(flash, 0x555, 0xAA);
	write_amd_command(flash, 0x2AA, 0x55);
	write_amd_command(flash, 0x555, 0x90);
	for (i=0; i<8; i++) {
	  if (readqry[i] != ptr[i])
	    break;
	}
	if (i < 8) {
	  /* Something happened */
	  primary_vendor = read_with_chip_width(flash, 0);
	  device_id = read_with_chip_width(flash, 1);
	  part = jedec_part_found(primary_vendor, device_id);
	  DEBUG("Vendor = %d, Device = %d, part = %d\n",
	    primary_vendor, device_id, part);
	  if (part >= 0) {
	    DEBUG("Found AMD JEDEC Flash, bit_width=%d, chip_cfg=%d\n",
	      bit_width, chip_cfg);
	    break;
	  }
	}
	if (chip_cfg == 0) {
	  break;
	} else {
	  chip_cfg--;
	}
      }
      if (part >= 0) {
	break;
      }
      if (bit_width == 0) {
	break;
      } else {
	bit_width--;
      }
    }
  }
#endif

  /*
   * If a part was found, read the regions
   */
  if (part >= 0) {
    struct flash_device_info *fdi;
    struct region_definition *region_map;
    ulong xsize;
    char c = 'k';

    size  = known_parts[part].size << chip_cfg;
    xsize = size  >> 10;	/* in kB */
    if (xsize >= 1024) {
	xsize >>= 10;		/* in MB */
	c = 'M';
    }

    printk("Found %dx%dbit %lu%cbyte JEDEC flash device of type %s at %lx\n",
	   (1<<(chip_cfg)),
	   (8<<(bit_width - chip_cfg)),
	   xsize, c,
	   known_parts[part].name, phys_addr);


    region_map = known_parts[part].region;
    /* We allocate memory for all partitions,
     * but initialize only partition 0 = the whole device
     */
    fdi = kmalloc(FLASH_PART_MAX*sizeof(struct flash_device_info), GFP_KERNEL);
    if (!fdi) {
      kfree(flash);
      return 0;
    }
    /* Count number of regions */
    for (i=0; region_map[i].size != 0; i++)
	;
    fdi->num_regions = i;
    DEBUG("Num regions: %d\n", fdi->num_regions);
    fdi->region = kmalloc(sizeof(flash_region_info_t)*fdi->num_regions, GFP_KERNEL);
    if (!fdi->region) {
      kfree(fdi);
      kfree(flash);
      return 0;
    }
    region = fdi->region;
    offset = 0;
    for (i=0; i<fdi->num_regions; i++) {
      region->sector_size = region_map[i].size << chip_cfg;
      region->num_sectors = region_map[i].count;
      region->offset = offset;
      DEBUG("region %d offset: 0x%08lx size: %6d, num_sect: %d, total size: %6d\n",
	   i,
	   fdi->region[i].offset,
	   fdi->region[i].sector_size , fdi->region[i].num_sectors,
	   fdi->region[i].sector_size * fdi->region[i].num_sectors);
      offset += (region->sector_size * region->num_sectors);
      region++;
    }

    /*
     * There are several places in the code that assume that the list
     * of erase regions is sorted by increasing address offset. So
     * far, this has always been true for all flash chips. Except for
     * the Am29DL322D/323D/324D, which break with this rule. Silly,
     * isn't it? Well, the easiest way to fix this problem is to
     * _make_ the list sorted. Since it's always a very short list,
     * we use simple bubble sort. - wd
     */
    for (i=0; i<fdi->num_regions; i++) {
  	int j;

	for (j=i+1; j<fdi->num_regions; j++) {
		if (fdi->region[i].offset > fdi->region[j].offset) {
			flash_region_info_t tmp;

			tmp	       = fdi->region[i];
			fdi->region[i] = fdi->region[j];
			fdi->region[j] = tmp;
		}
	}
    }

#ifdef	DEBUGFLASH
    printk ("Sorted region list:\n");
    for (i=0; i<fdi->num_regions; i++) {
	printk ("sect %i off %08lX sz: %d, n_sect: %d, total size: %d\n",
		i, fdi->region[i].offset,
		fdi->region[i].sector_size , fdi->region[i].num_sectors,
		fdi->region[i].sector_size * fdi->region[i].num_sectors);
    }
#endif

    /* Return the part to read array mode */
    ptr[0] = 0xff;
    ptr[1] = 0xff;
    ptr[2] = 0xff;
    ptr[3] = 0xff;

    iounmap ((void *)ptr);
    ptr = ioremap_nocache(phys_addr, size);

    fdi->handle = flash;
    flash->ptr = ptr;
    flash->size = size;
    flash->phys_addr = phys_addr;
    flash->bit_width = bit_width;
    flash->addr_shft = bit_width;
    flash->chip_cfg = chip_cfg;
    init_MUTEX (&flash->mutex);

    fdi->next = NULL;
    fdi->handle = flash;
    fdi->dev = 0;
    known_parts[part].init_ftn(fdi);
    register_flash(fdi);
  } else {
    /* Return the part to read array mode */
    ptr[0] = 0xff;
    ptr[1] = 0xff;
    ptr[2] = 0xff;
    ptr[3] = 0xff;
  }
  return size;
}

