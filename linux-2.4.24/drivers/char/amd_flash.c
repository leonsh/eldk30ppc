/*
 * linux/drivers/char/amd_flash.c  Version 0.3
 *
 **********************************************************
 * NOTE:
 *
 *  This driver has been verified with the following configurations:
 *
 *  - AMD AM29LV160B [1 x 16 bit on a  8 bit bus, 1 bank, System: HERMES-PRO]
 *  - AMD AM29LV160B [2 x 16 bit on a 32 bit bus, 1 bank, System: TQM850L]
 *  - STM M29W800AB  [2 x 16 bit on a 32 bit bus, 1 bank, System: ETX_094]
 *  - AMD AM29LV640U [4 x 16 bit on a 64 bit bus, 1 bank, System: TQM8260]
 *
 **********************************************************
 *
 * Copyright (C) 1999 Alexander Larsson (alla@lysator.liu.se or alex@signum.se)
 * Copyright (C) 2000 Harris Corporation (Dave Brown dbrown03@harris.com)
 * Copyright (C) 2000 DENX Software Engineering (Wolfgang Denk, wd@denx.de)
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
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/flash.h>

#include <asm/uaccess.h>

typedef	volatile unsigned int	vu_int;
typedef	volatile unsigned short	vu_short;
typedef	volatile unsigned char	vu_char;

#undef	 DEBUGFLASH

#ifdef DEBUGFLASH
#define DEBUG(fmt,args...) printk(fmt ,##args)
#else
#define DEBUG(fmt,args...)
#endif

static int amd_read(struct flash_device_info *, char *, size_t, off_t, int );
static int amd_write(struct flash_device_info *, const char *, size_t, off_t, int);
static int amd_write_compat(struct flash_device_info *, const char *, size_t, off_t, int);
static int amd_erase_sector(struct flash_device_info *, off_t, size_t);
static int amd_protect_sector(struct flash_device_info *, off_t, size_t, int);
static int amd_get_info(struct flash_device_info *, flash_info_t *);

#define individual_bit_width(flash) ((flash)->bit_width-(flash)->chip_cfg)

static void flash_delay (void)
{
  current->state = TASK_INTERRUPTIBLE;
  schedule_timeout(10*100/HZ); /* Time out 0.1 sec. */
}

struct flash_device_ops amd_ops = {
  amd_read,
  amd_write,
  amd_erase_sector,
  amd_protect_sector,
  amd_get_info
};

/* Options for non bypass unlock mode*/
struct flash_device_ops amd_compat_ops = {
  amd_read,
  amd_write_compat,
  amd_erase_sector,
  amd_protect_sector,
  amd_get_info
};

void __init amd_flash_init(struct flash_device_info *fdi)
{
  fdi->ops = &amd_ops;
}

void __init amd_flash_compat_init(struct flash_device_info *fdi)
{
  fdi->ops = &amd_compat_ops;
}

void write_amd_command(struct embedded_flash *flash, off_t offset, u_char data)
{
  vu_char *ptr = flash->ptr + (offset << flash->addr_shft);

  switch (flash->bit_width)
  {
  case 0:
    DEBUG("%s[%d] %p <- %02x\n",__FUNCTION__,__LINE__, ptr, data);
    *(vu_char *)ptr = (u_char)data;
    break;
  case 1:
    if (flash->chip_cfg == 0) {
      DEBUG("%s[%d] %p <- %04x\n",__FUNCTION__,__LINE__, ptr, data);
      *(vu_short *)ptr = (u_short)(data);
    } else {
      DEBUG("%s[%d] %p <- %04x\n",__FUNCTION__,__LINE__, ptr, (data << 8) | data);
      *(vu_short *)ptr = (u_short)((data << 8) | data);
    }
    break;
  case 2:
    if (flash->chip_cfg == 0) {
      DEBUG("%s[%d] %p <- %08x\n",__FUNCTION__,__LINE__, ptr, data);
      *(vu_int *)ptr = (u_int)(data);
    } else if (flash->chip_cfg == 1) {
      DEBUG("%s[%d] %p <- %08x\n",__FUNCTION__,__LINE__, ptr, (data << 16) | data);
      *(vu_int *)ptr = (u_int)((data << 16) | data);
    } else {
      DEBUG("%s[%d] %p <- %08x\n",__FUNCTION__,__LINE__,
	    ptr, (data << 24) | (data << 16) | (data << 8) | data);
      *(vu_int *)ptr = (u_int)((data << 24) | (data << 16) | (data << 8) | data);
    }
    break;
  case 3:
    if (flash->chip_cfg == 0) {
#ifdef __BIG_ENDIAN
      DEBUG("%s[%d] %p <- %08x\n",__FUNCTION__,__LINE__, ptr+4, data);
      *(vu_int *)(ptr+4) = (u_int)(data);
#else
      DEBUG("%s[%d] %p <- %08x\n",__FUNCTION__,__LINE__, ptr, data);
      *(vu_int *)ptr = (u_int)(data);
#endif
    } else if (flash->chip_cfg == 1) {
      DEBUG("%s[%d] (%p,%p) <- %08x\n",__FUNCTION__,__LINE__, ptr, ptr+4, data);
      *(vu_int *)ptr = (u_int)(data);
      *(vu_int *)(ptr+4) = (u_int)(data);
    } else {
      DEBUG("%s[%d] (%p,%p) <- %08x\n",__FUNCTION__,__LINE__,
	ptr, ptr+4, (data << 16) | data);
      *(vu_int *)ptr = (u_int)((data << 16) | data);
      *(vu_int *)(ptr+4) = (u_int)((data << 16) | data);
    }
    break;
  }
}


static u_int read_with_chip_width(struct embedded_flash *flash, u_long offs)
{
  u_int data;
  vu_char *ptr = flash->ptr + (offs << flash->addr_shft);

  switch(individual_bit_width(flash)) {
  case 0: /* 8bit */
    data = *ptr;
    break;
  case 1: /* 16bit */
    data = *(u_short *)ptr;
    break;
  case 2: /* 32bit */
    data = *(u_int *)ptr;
    break;
  }
  return data;
}

#define PROGRAM_WRITE 1
#define PROGRAM_ERASE 2

static int program_in_progress(struct embedded_flash *flash)
{
  /* This function doesn't work reliably if erase_suspend is used. */
  vu_char *ptr;
  int i;
  int num_chip;
  u_int data1, data2, data;

  num_chip = 1<<flash->chip_cfg;

  ptr = flash->ptr;

  for (i=0;i<num_chip;i++) {
    data1 = read_with_chip_width(flash, 0) & 0x44; /* mask out DQ6 and DQ2 */
    data2 = read_with_chip_width(flash, 0) & 0x44 ;/* mask out DQ6 and DQ2 */
    data = data1 ^ data2; /* xor */
    if (data&0x40) {
      /* DQ6 toggled, some program is running (might be in erase suspend) */
      DEBUG("dq6 on chip %d toggled, embedded program running.\n", i);
      if (data&0x04) {
        DEBUG("dq2 on chip %d toggled, erase runnung.\n", i);
        return PROGRAM_ERASE;
      } else {
        DEBUG("dq2 on chip %d not toggled, write runnung.\n", i);
        return PROGRAM_WRITE;
      }
    }
  }
  return 0; /* No program in progress. */
}

#define	MAX_ERASE_TIME	512	/* in 0.1s steps = 51 sec. */

static void do_erase_sector(struct embedded_flash *flash, off_t offset)
{
  int i;
  u_long j;
  vu_char *ptr;

  j = jiffies;
  ptr = flash->ptr;

  DEBUG("Erasing @ 0x%lX\n", offset);

  write_amd_command(flash, 0x555, 0xAA);
  write_amd_command(flash, 0x2AA, 0x55);
  write_amd_command(flash, 0x555, 0x80);
  write_amd_command(flash, 0x555, 0xAA);
  write_amd_command(flash, 0x2AA, 0x55);

  write_amd_command(flash, offset >> flash->addr_shft, 0x30);
  flash_delay();

  switch(flash->bit_width) {
  case 0:
	{	vu_char *addr = (vu_char *)(&ptr[offset]);

		for (i=0; (i<MAX_ERASE_TIME) && (*addr != 0xFF); ++i) {
			flash_delay();
		}
	}
	break;
  case 1:
	{	vu_short *addr = (vu_short *)(&ptr[offset]);

		for (i=0; (i<MAX_ERASE_TIME) && (*addr != 0xFFFF); ++i) {
			flash_delay();
		}
	}
	break;
  case 2:
	{	vu_int *addr = (vu_int *)(&ptr[offset]);

		for (i=0; (i<MAX_ERASE_TIME) && (*addr != 0xFFFFFFFF); ++i) {
			flash_delay();
		}
	}
	break;
  case 3:
	{	vu_int *addr = (vu_int *)(&ptr[offset]);

		for (i=0; (i<MAX_ERASE_TIME) && (*addr != 0xFFFFFFFF)
					 && (*(addr+1) != 0xFFFFFFFF); ++i) {
			flash_delay();
		}
	}
	break;
  }

  j = jiffies - j;

  DEBUG("Erasing @ 0x%lx %s %ld jiffies\n",
  	offset,
	(i < MAX_ERASE_TIME) ? "took" : "TIMED OUT AFTER",
	j);
}

static int amd_read(struct flash_device_info *fdi,
                    char *buf, size_t count, off_t offset, int userspace_ptr)
{
  struct embedded_flash *flash = fdi->handle;
  int ret;

  ret = 0;

/********
  DEBUG("amd_read buf=0x%lx, ptr=0x%x, count=%d, offset=0x%lx, us_ptr=0x%ld\n",
      (u_long)buf, (int)flash->ptr, (int)count, offset, (long) userspace_ptr);
********/

  down(&flash->mutex);

  if (program_in_progress(flash)) {
    printk("Error, embedded program in progress during flash read\n");
    ret = -EIO;
    goto error;
  }

  if (userspace_ptr) {
    if (copy_to_user(buf, (char *)flash->ptr+offset, count) > 0) {
      ret = -EFAULT;
    }
  } else {
    memcpy(buf, (char *)flash->ptr+offset, count);
  }

 error:

  up(&flash->mutex);

  return ret;
}

static void unlock_bypass(struct embedded_flash *flash)
{
  write_amd_command(flash, 0x555, 0xAA);
  write_amd_command(flash, 0x2AA, 0x55);
  write_amd_command(flash, 0x555, 0x20);
}

static void unlock_bypass_reset(struct embedded_flash *flash)
{
  /* Address is "don't care" for unlock_bypass_reset */
  write_amd_command(flash, 0x00, 0x90);
  write_amd_command(flash, 0x00, 0x00);
}

static int do_write_int(struct embedded_flash *flash, off_t offset, u_int data, int bypass)
{
  vu_char *ptr;
  int wait, wait1;
  u_int old;

  ptr = flash->ptr;

  if ( ((old = *(vu_int *)&ptr[offset]) & data) != data) {
    printk("Flash write error, not fully erased at 0x%p+0x%lx: "
		"old 0x%08x  new 0x%08x\n",
	   ptr, offset, old, data);
    return (-1);
  }

#ifdef DEBUGFLASH
  if ((offset & (~0x3)) != offset) {
    printk("do_write_int: incorrectly aligned offset 0x%lx\n", offset);
    return -1;
  }
#endif

  if (bypass) {
    /*
     * Program byte:
     * Address is "don't care" in unlock_bypass mode
     */
    write_amd_command(flash, 0, 0xA0);
  } else {
    write_amd_command(flash, 0x555, 0xAA);
    write_amd_command(flash, 0x2AA, 0x55);
    write_amd_command(flash, 0x555, 0xA0);
  }

  *(vu_int *)&ptr[offset] = data;
  if (flash->bit_width == 3)	/* satisfy the remaining chips */
  	*(vu_int *)&ptr[offset^4] = *(vu_int *)&ptr[offset^4];

  wait1 = wait = 0;
  while ((old = *(vu_int *)&ptr[offset]) != data) {
    wait++;
    if ((wait%10000)==0) {
      DEBUG("do_write_int, wait=%d\n", wait);
      DEBUG("addr = 0x%x, offset = 0x%lx, data = 0x%08x, flash: 0x%08x\n",
              (int)&ptr[offset], offset, data, old);
      if (++wait1 > 32) {
	return (-1);
      }
    }
    /* Busy wait */
  }
  return (0);
}


static int do_write_short(struct embedded_flash *flash, off_t offset, u_short data, int bypass)
{
  vu_char *ptr;
  int wait, wait1;
  u_short old;

  ptr = flash->ptr;

  if ( ((old = *(vu_short *)&ptr[offset]) & data) != data) {
    printk("Flash write error:"
	   " not fully erased at %p+0x%lx: old 0x%08x  new 0x%08x\n",
	   ptr, offset, old, data);
    return (-1);
  }

#ifdef DEBUGFLASH
  if ((offset & (~1)) != offset) {
    printk("do_write_short: incorrectly aligned offset 0x%lx\n", offset);
    return -1;
  }
#endif

  if (bypass) {
    /*
     * Program byte:
     * Address is "don't care" in unlock_bypass mode
     */
    write_amd_command(flash, 0, 0xA0);
  } else {
    write_amd_command(flash, 0x555, 0xAA);
    write_amd_command(flash, 0x2AA, 0x55);
    write_amd_command(flash, 0x555, 0xA0);
  }

  DEBUG("do_write_short: addr = %p (%p + 0x%lx), data = 0x%04x\n",
  	(vu_short *)&ptr[offset], ptr, offset, data);
  *(vu_short *)&ptr[offset] = data;
  if (flash->bit_width == 2) {	/* satisfy the remaining chips */
  	*(vu_short *)&ptr[offset^2] = *(vu_short *)&ptr[offset^2];
  }
  if (flash->bit_width == 3) {	/* satisfy the remaining chips */
  	*(vu_short *)&ptr[offset^2] = *(vu_short *)&ptr[offset^2];
	*(vu_short *)&ptr[offset^4] = *(vu_short *)&ptr[offset^4];
	*(vu_short *)&ptr[offset^6] = *(vu_short *)&ptr[offset^6];
  }

  wait1 = wait = 0;
  while ((old = *(vu_short *)&ptr[offset]) != data) {
    wait++;
    if ((wait%10000)==0) {
      DEBUG("do_write_short, wait=%d\n"
	    "addr = 0x%x, offset = 0x%lx, data = 0x%08x, flash: 0x%08x\n",
	    wait,
            (int)&ptr[offset], offset, data, old);
      if (++wait1 > 32) {
	return (-1);
      }
    }
    /* Busy wait */
  }
  return (0);
}

static int do_write_char(struct embedded_flash *flash, off_t offset, u_char data, int bypass)
{
  vu_char *ptr;
  int wait, wait1;
  u_char c, old;

  ptr = flash->ptr;

  old = ptr[offset];

  if ( ((c = ptr[offset]) & data) != data) {
    printk("Flash write error, not fully erased at 0x%p+0x%lx: old 0x%02x  new 0x%02x\n",
	ptr, offset, c, data);
    return (-1);
  }

  if (bypass) {
    /*
     * Program byte:
     * Address is "don't care" in unlock_bypass mode
     */
    write_amd_command(flash, 0, 0xA0);
  } else {
    write_amd_command(flash, 0x555, 0xAA);
    write_amd_command(flash, 0x2AA, 0x55);
    write_amd_command(flash, 0x555, 0xA0);
  }

/* XXX __BIG_ENDIAN only !!! */

  switch (flash->bit_width) {
  case 0:	/* 8 bit */
	{
		ptr[offset] = data;
	}
  	break;
  case 1:	/* 16 bit */
	{	u_short value;
		off_t s_offset = offset & (~1);
		vu_short *s_ptr = (vu_short *)(ptr + s_offset);

#ifdef __BIG_ENDIAN
		if (s_offset == offset) {		/* even address	*/
			value = (*s_ptr & 0x00FF) | (data << 8);
		} else {				/* off address	*/
			value = (*s_ptr & 0xFF00) | data;
		}
#else	/* little endian */
		if (s_offset == offset) {		/* even address	*/
			value = (*s_ptr & 0xFF00) | data;
		} else {				/* off address	*/
			value = (*s_ptr & 0x00FF) | (data << 8);
		}
#endif	/* __BIG_ENDIAN */
		*s_ptr = value;
	}
	break;
  case 2:	/* 32 bit */
  case 3:	/* 64 bit */
	{	u_int value;
		int pos = 8 * (offset & 3);
		u_int  i_offset = offset & (~3);
		vu_int *i_ptr = (vu_int *)(ptr + i_offset);

#ifdef __BIG_ENDIAN
		value = (*i_ptr & ~(0xFF000000 >> pos)) | (data << (24-pos));
#else   /* little endian */
		value = (*i_ptr & ~(0x000000FF << (24-pos))) | (data << pos);
#endif  /* __BIG_ENDIAN */
		*i_ptr = value;
		if (flash->bit_width == 3)	/* satisfy the remaining chips */
			*(vu_int *)(4^(u_int)i_ptr) = *(vu_int *)(4^(u_int)i_ptr);
  	}
	break;
  default:
printk ("BIT_WIDTH %d not implemented\n",flash->bit_width);return(-1);
  	break;
  }

  wait1 = wait = 0;
  while ((c = ptr[offset]) != data) {
    wait++;
    if ((wait%1000)==0) {
      DEBUG("do_write_char, wait=%d, "
      	    "offset = %lx, data = %02x, flash: %02x [0x%02x]\n",
              wait, offset, data, c, old);
      if (++wait1 > 32) {
	return (-1);
      }
    }
    /* Busy wait */
  }
  return (0);
}

static int do_amd_write(struct flash_device_info *fdi, const char *buf,
		     size_t count, off_t offset, int userspace_ptr,
		     int bypass)
{
  struct embedded_flash *flash = fdi->handle;
  struct timeval time1, time2;
  vu_char *ptr;
  u_short bit_width, chip_cfg;
  int ret;
  u_long end_offset;
  u_int data;
  int loops;

  ret = 0;
  ptr = flash->ptr;
  bit_width = flash->bit_width;
  chip_cfg = flash->chip_cfg;

/***/
  DEBUG("amd_write buf=0x%x, count=%d=0x%x, offset=0x%x, us_ptr=%d\n",
    (int)buf, (int)count, (int)count, (int)offset, (int) userspace_ptr);
/***/

  down(&flash->mutex);

  if (program_in_progress(flash)) {
    printk("Error, embedded program in progress during flash write\n");
    ret = -EIO;
    goto ERROR;
  }

  end_offset = offset + count;

  if (userspace_ptr && (!access_ok(VERIFY_READ, buf, count)) ) {
    ret = -EFAULT;
    goto ERROR;
  }

  if (bypass) {
    unlock_bypass(flash);
  }

  do_gettimeofday(&time1);

  loops = 0;
  while (offset<end_offset) {
    if ((offset%4 == 0) && (end_offset-offset >= 4) && (bit_width >= 2)) {
      if (userspace_ptr) {
        if (__get_user(data, (int *)buf)) {
          ret = -EFAULT;
          goto UNLOCK_BYPASS_RESET_ERROR;
        }
      } else {
        data = *(int *)buf;
      }

      if (do_write_int(flash, offset, data, bypass)) {
	ret = -EFAULT;
	goto UNLOCK_BYPASS_RESET_ERROR;
      }
      buf += 4;
      offset += 4;
    } else if ((offset%2 == 0) && (end_offset-offset >= 2) && (bit_width >= 1)){
      if (userspace_ptr) {
        if (__get_user(data, (short *)buf)) {
          ret = -EFAULT;
          goto UNLOCK_BYPASS_RESET_ERROR;
        }
      } else {
        data = *(short *)buf;
      }

      if (do_write_short(flash, offset, data, bypass)) {
        ret = -EFAULT;
	goto UNLOCK_BYPASS_RESET_ERROR;
      }
      buf += 2;
      offset += 2;
    } else {
      if (userspace_ptr) {
        if (__get_user(data, (char *)buf)) {
          ret = -EFAULT;
          goto UNLOCK_BYPASS_RESET_ERROR;
        }
      } else {
        data = *(char *)buf;
      }
      if (do_write_char(flash, offset, data, bypass)) {
        ret = -EFAULT;
	goto UNLOCK_BYPASS_RESET_ERROR;
      }
      buf += 1;
      offset += 1;
    }

    /* every 32 writes we check if >1msec has passed. then we schedule */
    if ((++loops & 0x1f) == 0) {
      do_gettimeofday(&time2);
      time2.tv_sec -= time1.tv_sec;
      time2.tv_usec -= time1.tv_usec;

      if ((time2.tv_sec>0) || (time2.tv_usec>1000)) {
        /* May wait to short if tv_sec wraps. Who cares... */
        current->state = TASK_RUNNING;
        schedule();
        if (userspace_ptr && (!access_ok(VERIFY_READ, buf, end_offset-offset))) {
          ret = -EFAULT;
          goto UNLOCK_BYPASS_RESET_ERROR;
        }
        do_gettimeofday(&time1);
      }
    }
  }

UNLOCK_BYPASS_RESET_ERROR:
  if (bypass) {
    unlock_bypass_reset(flash);
  }

ERROR:
  up(&flash->mutex);

  return ret;
}

static int amd_write_compat(struct flash_device_info *fdi, const char *buf,
		     size_t count, off_t offset, int userspace_ptr)
{
  return do_amd_write(fdi, buf, count, offset, userspace_ptr,0);
}

static int amd_write(struct flash_device_info *fdi, const char *buf,
		     size_t count, off_t offset, int userspace_ptr)
{
  return do_amd_write(fdi, buf, count, offset, userspace_ptr,1);
}

static int amd_erase_sector(struct flash_device_info *fdi,
                            off_t offset, size_t count)
{
  struct embedded_flash *flash = fdi->handle;
  flash_region_info_t *region = NULL;
  u_long region_size, region_end;
  u_long end;
  int ret;
  int i;

  ret = 0;

  end = offset + count;

  DEBUG("%s[%d]: amd_erase count=%d, offset=0x%x, end=0x%x\n",__FILE__,__LINE__,
    (int)count, (int)offset,(int)end);

  down(&flash->mutex);

  if (!flash_on_sector_boundaries(fdi, offset, count)) {
    DEBUG("Trying to erase, but not at sector boundaries. offs=%x, count=%x\n",
            (int)offset, (int)count);
    ret = -EINVAL;
    goto error;
  }

  if (program_in_progress(flash)) {
    printk("Error, embedded program in progress during flash erase\n");
    ret = -EIO;
    goto error;
  }

  /* erase all sectors in mem region */
  /* Now guaranteed to be on sector boundaries */
  for (i = 0; i<fdi->num_regions; i++) {
    region = &fdi->region[i];

    region_size = region->sector_size * region->num_sectors;
    region_end  = region->offset + region_size;

    if (region->offset >= end) {
      break;
    }

    if  ( (offset >= region->offset) && (offset < region_end) ) {
      u_long erase_end = (end > region_end) ? region_end : end;

      while (offset < erase_end) {
	DEBUG("%s[%d]: do_erase_sector region %d offset 0x%lx\n",
		__FILE__, __LINE__, i, offset);
	do_erase_sector(flash, offset);
	offset += region->sector_size;
      }
    }
  }



 error:

  up(&flash->mutex);

  return ret;
}

static int amd_protect_sector(struct flash_device_info *fdi,
                              off_t offset, size_t count, int protected)
{
  return -ENOSYS;
}

static int amd_get_info(struct flash_device_info *fdi,
                         flash_info_t *flash_info)
{
  struct embedded_flash *flash = fdi->handle;
  flash_info_t t;
  int i, n;
  int dev0 = MINOR(fdi->dev) >> FLASH_PART_BITS;
  
  for (n=0, i=1; (dev0<flash_part_num) && (i<FLASH_PART_MAX); ++i) {
	if (flash_partition_table[dev0][i-1].size) {
		++n;
	}
  }
 
  t.size	= flash->size;
  t.bitwidth	= 8<<individual_bit_width(flash);
  t.addr_shift	= flash->addr_shft;
  t.chip_config	= 1<<flash->chip_cfg;
  t.num_regions	= fdi->num_regions;
  t.num_parts	= n;

  if (copy_to_user(flash_info, &t, sizeof(flash_info_t))) {
    return -EFAULT;
  }

  return 0;
}
