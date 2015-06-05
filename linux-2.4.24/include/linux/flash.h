#ifndef _LINUX_FLASH_H
#define _LINUX_FLASH_H

/*
 * /usr/include/linux/flash.h  Version 0.2
 *
 * Copyright (C) 1999 Alexander Larsson (alla@lysator.liu.se or alex@signum.se)
 * Copyright (C) 2000, 2001 Wolfgang Denk, wd@denx.de
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

#include <asm/ioctl.h>
#include <asm/byteorder.h>

typedef struct flash_info_t {
  u_int size;        /* Total size of flash array in bytes. */
  u_int bitwidth;    /* Bit width of individual chips. */
  u_int chip_config; /* What chip configuration is used, 1 chip, 2 chips etc. */
  u_int addr_shift;  /* Number of Bits address needs to be shifted */
  u_int num_regions; /* Number of regions with equal sized sectors */
  u_int num_parts;   /* Number of partitions (minor devices) */
} flash_info_t;

typedef struct flash_region_info_t {
  u_long offset;
  u_int sector_size; /* What size are the erase zones (in bytes) */
  u_int num_sectors;
} flash_region_info_t;

typedef struct erase_info_t {
  u_long offset;
  u_long size;
} erase_info_t;

/*
 * Splitting flash devices into "partitions"
 * using the N lower bits of the minor number;
 * minor number mod 0 is always reserved for the whole flash bank,
 * so with 3 bits you can get 8-1 = 7 partitions
 */

#define FLASH_PART_BITS		3	/* Max. 7 partitions per flash device */

#define FLASH_PART_MAX		(1 << FLASH_PART_BITS)
#define FLASH_PART_MASK		(FLASH_PART_MAX - 1)

typedef struct flash_partition {
	off_t	start;
	size_t	size;
} flash_partition_t;

extern flash_partition_t flash_partition_table[][FLASH_PART_MAX-1];
extern int flash_part_num;

/* IOCTL:s */
#define FLASHGETINFO		_IOR('F', 1, flash_info_t)
#define FLASHGETREGIONINFO	_IOR('F', 2, flash_region_info_t)
#define FLASHERASE		_IOW('F', 3, erase_info_t)
#define FLASHGETPARTINFO	_IOW('F', 4, flash_partition_t)

#ifdef __KERNEL__

/* Defines used by CFI (Common Flash Interface) */
#define CFI_QRY_OFFSET				0x10
#define CFI_PRIMARY_VENDOR_ID_OFFSET		0x13
#define CFI_PRIMARY_QUERY_TABLE_OFFSET		0x15
#define CFI_ALT_VENDOR_ID_OFFSET		0x17
#define CFI_ALT_QUERY_TABLE_OFFSET		0x19

#define CFI_DEVICE_SIZE_OFFSET			0x27
#define CFI_NUM_ERASE_BLOCKS_OFFSET		0x2C
/*
 * These are defined as one 32 bit field in the spec,
 * but its easier to treat as two 16 bit fields
 */
#define CFI_ERASE_REGION_INFO_NBLOCKS_OFFSET	0x2D
#define CFI_ERASE_REGION_INFO_SIZE_OFFSET	0x2F

#define CFI_QUERY_CMD				0x98
#define CFI_QUERY_OFFSET			0x55

#define CFI_VENDOR_NONE				0x0000
#define CFI_VENDOR_INTEL_SHARP_EXTENDED		0x0001
#define CFI_VENDOR_AMD_FUJITSU_STANDARD		0x0002
#define CFI_VENDOR_INTEL_STANDARD		0x0003
#define CFI_VENDOR_AMD_FUJITSU_EXTENDED		0x0004
#define CFI_VENDOR_MITSUBISHI_STANDARD		0x0100
#define CFI_VENDOR_MITSUBISHI_EXTENDED		0x0101
#define CFI_VENDOR_RESERVED			0xffff

extern inline u_short cfi_read_short(volatile unsigned char *baseptr,
				     int offset, int chip,
				     int bit_width, int chip_cfg, int addr_shft)
{
  /* bit_width: 0 => 8 bit total width, 1 => 16 bit, 2 => 32 bit, 3 => 64 bit */
  /* chip_config: 0 => 1 chip, 1 => 2 chip, 2 => 4 chip */
  u_short res;
  volatile u_char *ptr;

  ptr = baseptr + (offset<<addr_shft);

  ptr -= chip << (addr_shft-chip_cfg); /* chip offset */

#ifdef __BIG_ENDIAN
  switch (addr_shft) {
  case 0:
    res = (ptr[ 1] << 8) | ptr[0]; break;
  case 1:
    res = (ptr[ 3] << 8) | ptr[1]; break;
  case 2:
    res = (ptr[ 7] << 8) | ptr[3]; break;
  case 3:
    res = (ptr[15] << 8) | ptr[7]; break;
  }
#else
  switch (addr_shft) {
  case 0:
    res = (ptr[0] << 8) | ptr[ 1]; break;
  case 1:
    res = (ptr[1] << 8) | ptr[ 3]; break;
  case 2:
    res = (ptr[3] << 8) | ptr[ 7]; break;
  case 3:
    res = (ptr[7] << 8) | ptr[15]; break;
  }
#endif

  return res;
}

extern inline u_char cfi_read_char(volatile unsigned char *baseptr,
				   int offset, int chip,
				   int bit_width, int chip_cfg, int addr_shft)
{
  volatile u_char *ptr;

  ptr = baseptr + (offset<<addr_shft);
  ptr -= chip << (addr_shft-chip_cfg); /* chip offset */

  switch (bit_width) {
  case 0:
    return ptr[0];
  case 1:
    return ptr[1];
  case 2:
    return ptr[3];
  default:
  case 3:
    return ptr[7];
  }
}

/* Interface for generic flash chips */
typedef struct flash_device_info {
        struct flash_device_ops  *ops;  /* link to device_ops */
        struct flash_device_info *next; /* next device_info for this major */
        void *handle;                   /* driver-dependent data */
        kdev_t dev;                     /* device number for char device */

        int num_regions;
        flash_region_info_t *region;    /* Always stored in order */
} flash_device_info_t;

/* Interface for CFI or JEDEC flash chips in an embedded system  */
/* where the flash is memory mapped.                             */
/* This structure contains fields common to all or most drivers. */
typedef struct embedded_flash {
	volatile u_char	*ptr;
	u_long		size;
	u_long		phys_addr;
	short		bit_width;	/* 0: 8 bit,   1: 16 bit,   2: 32 bit	*/
	short		chip_cfg;	/* 0: 1 chip,  1:  2 chip,  2:  4 chip	*/
	short		addr_shft;	/* address bit shift			*/
	char		erase_suspend;	/* 0: unsupported, 1: read only, 2: r/w */
	/* etc.. sector protect etc... */
	struct semaphore mutex;
} embedded_flash_t;

struct flash_device_ops {
        int (*read) (struct flash_device_info *,
		     char *, size_t, off_t, int);
        int (*write) (struct flash_device_info *,
		      const char *, size_t, off_t, int);
        int (*erase_sector) (struct flash_device_info *,
			      off_t, size_t);
			     /* Can erase several consecutive sectors. */
        int (*protect_sector) (struct flash_device_info *,
				off_t, size_t, int);
        int (*get_info) (struct flash_device_info *, flash_info_t *);
};


extern int register_flash(struct flash_device_info *fdi);
extern int unregister_flash(struct flash_device_info *fdi);

extern int flash_init(void);
extern flash_device_info_t *flash_devices;

/* Some utility routines: */
extern int flash_on_sector_boundaries(struct flash_device_info *fdi,
				     off_t offset, size_t count);

#endif  /* End of kernel only stuff */

#endif  /* _LINUX_FLASH_H */
