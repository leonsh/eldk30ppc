/*
 * linux/drivers/char/intel_flash.c  Version 0.1
 *
 **********************************************************
 * NOTE:
 *
 *  This driver has been verified with the following configurations:
 *
 *  - Micron 28F400B3 [1 x 16 Mbit on a 16 bi bus, 1 bank, System: IVMS8]
 *  - INTEL  28F160[CF]3B [4 x 16 Mbit on a 64 bit bus, 1 bank, System: PM826]
 *
 **********************************************************
 *
 * Copyright (C) 1999 Alexander Larsson (alla@lysator.liu.se or alex@signum.se)
 * Copyright (C) 2000 Harris Corporation (Dave Brown dbrown03@harris.com)
 * Copyright (C) 2000, 2001 DENX Software Engineering (Wolfgang Denk, wd@denx.de)
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

#if 0
extern void m8xx_set_flash_vpp(void);
extern void m8xx_clr_flash_vpp(void);
#endif

#undef	DEBUGFLASH

#ifdef DEBUGFLASH
#define DEBUG(fmt,args...) printk(fmt ,##args)
#else
#define DEBUG(fmt,args...)
#endif

static int intel_read(struct flash_device_info *, char *, size_t, off_t, int );
static int intel_write(struct flash_device_info *, const char *, size_t, off_t, int);
static int intel_erase_sector(struct flash_device_info *, off_t, size_t);
static int intel_protect_sector(struct flash_device_info *, off_t, size_t, int);
static int intel_get_info(struct flash_device_info *, flash_info_t *);

#define individual_bit_width(flash) ((flash)->bit_width-(flash)->chip_cfg)

static void flash_delay (void)
{
  current->state = TASK_INTERRUPTIBLE;
  schedule_timeout(10*100/HZ); /* Time out 0.1 sec. */
}

struct flash_device_ops intel_ops = {
  intel_read,
  intel_write,
  intel_erase_sector,
  intel_protect_sector,
  intel_get_info
};

void __init intel_flash_init(struct flash_device_info *fdi)
{
  fdi->ops = &intel_ops;
}

#define PROGRAM_WRITE 1
#define PROGRAM_ERASE 2

static u_int make_command(struct embedded_flash *flash, u_char data)
{
  u_int result;
  switch (flash->bit_width)
  {
  case 0:
    result = (u_char)data;
    break;
  case 1:
    if (flash->chip_cfg == 0)
      result = (u_short)(data);
    else
      result = (u_short)((data << 8) | data);
    break;
  case 2:
    if (flash->chip_cfg == 0)
      result = (u_int)(data);
    else if (flash->chip_cfg == 1)
      result = (u_int)((data << 16) | data);
    else
      result = (u_int)((data << 24) | (data << 16) | (data << 8) | data);
    break;
  case 3:
    if (flash->chip_cfg == 0) {
      /* unsupported */
    }
    else if (flash->chip_cfg == 1)
      result = (u_int)data;
    else
      result = (u_int)((data << 16) | data);
    break;
  }
  return result;  
}

static u_int read_status(struct embedded_flash *flash, u_long offs)
{
  u_int data;
  vu_char *ptr = flash->ptr;

  switch(flash->bit_width) {
  case 0: /* 8bit bus */
    data = (ptr[offs]);
    break;
  case 1: /* 16bit */
    data = (*(u_short *)&ptr[offs]);
    break;
  case 2: /* 32bit */
  case 3: /* 64bit */
    data = (*(u_int *)&ptr[offs]);
    break;
  }
  return data;
}


void write_intel_command(struct embedded_flash *flash, off_t offset, u_char data, int both)
{
  vu_char *ptr;
  short bit_width;

  ptr = flash->ptr;
  /* bit_width: 0 => 8 bit total width, 1 => 16 bit, 2 => 32 bit, 3 => 64 bit */
  bit_width = flash->bit_width;

#ifdef CONFIG_CU824
  both = 0;
#endif

/* TODO - any concerns with chip_cfg here? */
  /* chip_config: 0 => 1 chip, 1 => 2 chip, 2 => 4 chip */

  switch (bit_width)
  {
  case 0:
    *(vu_char *)(&ptr[offset]) = (u_char)data;
    break;
  case 1:
    if (flash->chip_cfg == 0) {
      *(vu_short *)(&ptr[offset]) = (u_short)(data);
    } else {
      *(vu_short *)(&ptr[offset]) = (u_short)((data << 8) | data);
    }
    break;
  case 2:
    if (flash->chip_cfg == 0) {
      *(vu_int *)(&ptr[offset]) =
	(u_int)(data);
    } else if (flash->chip_cfg == 1) {
      *(vu_int *)(&ptr[offset]) =
	(u_int)((data << 16) | data);
    } else {
      *(vu_int *)(&ptr[offset]) =
	(u_int)((data << 24) | (data << 16) | (data << 8) | data);
    }
    break;
  case 3:
    if (flash->chip_cfg == 0) {
      /* unsupported */
    } else if (flash->chip_cfg == 1) {
      *(vu_int *)(&ptr[offset & ~3]) =
	(u_int)(data);
      if (!both)
	break;
      *(vu_int *)(&ptr[(offset & ~3) + 4]) =
	(u_int)(data);
    } else {
      *(vu_int *)(&ptr[offset & ~3]) =
	(u_int)((data << 16) | data);
      if (!both)
	break;
      *(vu_int *)(&ptr[(offset & ~3) + 4]) =
	(u_int)((data << 16) | data);
    }
    break;
  }
}

static int program_in_progress(struct embedded_flash *flash)
{
  int busy;
  u_int desired = make_command(flash, 0x80);
  /* Issue the status command */
  write_intel_command(flash, 0, 0x70, 1);
  if ((read_status(flash, 0) & desired) == desired
     && (flash->bit_width < 3
        || (read_status(flash, 4) & desired) == desired)) {
    busy = 0;
  } else {
    busy = 1;
  }
  write_intel_command(flash, 0, 0xFF, 1);
  return busy;
}

static void do_erase_sector(struct embedded_flash *flash, off_t offset)
{
  u_long j;
  vu_char *ptr;
  u_int desired = make_command(flash, 0x80);

  j = jiffies;
  ptr = flash->ptr;

  write_intel_command(flash, offset, 0x20, 1);
  current->state = TASK_INTERRUPTIBLE;
  schedule_timeout(2); /* Time out soon. */
  write_intel_command(flash, offset, 0xd0, 1);

  flash_delay ();
  while((read_status(flash, offset) & desired) != desired
	|| (flash->bit_width == 3
           && (read_status(flash, offset + 4) & desired) != desired)) {
    flash_delay ();
  }
  write_intel_command(flash, offset, 0xFF, 1);

  j = jiffies - j;
  DEBUG("Erasing @ 0x%lx took %ld jiffies\n", offset, j);
}

static int intel_read(struct flash_device_info *fdi,
                    char *buf, size_t count, off_t offset, int userspace_ptr)
{
  struct embedded_flash *flash = fdi->handle;
  int ret;

  ret = 0;

/********/
  DEBUG("intel_read buf=0x%lx, ptr=0x%x, count=%d, offset=0x%lx, us_ptr=0x%ld\n",
      (u_long)buf, (int)flash->ptr, (int)count, offset, (long) userspace_ptr);
/********/

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

#ifdef CONFIG_CU824
static void write_via_fpu(volatile u_long *addr, u_int *data)
{
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

static int do_write_dword(struct embedded_flash *flash, off_t offset,
                          u_int data_h, u_int data_l)
{
  vu_char *ptr;
  int wait, wait1;
  u_int old_h, old_l;
  u_int data_ptr[2];

  ptr = flash->ptr;

  write_intel_command(flash, offset, 0xFF, 0);
  if ( ((old_h = *(vu_int *)&ptr[offset]) & data_h) != data_h ||
       ((old_l = *(vu_int *)&ptr[offset + 4]) & data_l) != data_l ) {
    printk("Flash write error, not fully erased at 0x%p+0x%lx: old 0x%08x  new 0x%08x\n", ptr, offset, old_h, data_h);
    return (-1);
  }

#ifdef DEBUGFLASH
  if ((offset & (~0x7)) != offset) {
    printk("do_write_dword: incorrectly aligned offset 0x%lx\n", offset);
    return -1;
  }
#endif

  if (old_h != data_h || old_l != data_l) {
    data_ptr[0] = data_h;
    data_ptr[1] = data_l;
    write_intel_command(flash, offset, 0x40, 0);
    wait = wait1 = 0;
    write_via_fpu((long*)&ptr[offset], data_ptr);

    while((read_status(flash, offset) & 0x00800080) != 0x00800080 ||
          (read_status(flash, offset + 4) & 0x00800080) != 0x00800080 ) {
      wait++;
      if ((wait%10000)==0) {
        DEBUG  ("do_write_dword, wait=%d\n"
		"addr = 0x%x, offset = 0x%lx, data = 0x%08x, flash: 0x%08x\n",
		wait,
                (int)&ptr[offset], offset, data_h, old_h);
        if (++wait1 > 32) {
	  return (-1);
        }
      }
      /* Busy wait */
    }
  }
  return (0);
}
#endif

#ifndef CONFIG_CU824
static int do_write_int(struct embedded_flash *flash, off_t offset, u_int data)
{
  vu_char *ptr;
  int wait, wait1;
  u_int old;
  u_int desired;

  ptr = flash->ptr;

  write_intel_command(flash, offset, 0xFF, 0);
  if ( ((old = *(vu_int *)&ptr[offset]) & data) != data) {
    printk("Flash write error, not fully erased at 0x%p+0x%lx: old 0x%08x  new 0x%08x\n", ptr, offset, old, data);
    return (-1);
  }

#ifdef DEBUGFLASH
  if ((offset & (~0x3)) != offset) {
    printk("do_write_int: incorrectly aligned offset 0x%lx\n", offset);
    return -1;
  }
#endif

  if (old != data) {
    /* Program word: */
    write_intel_command(flash, offset, 0x40, 0);
    desired = make_command(flash, 0x80);
    wait = wait1 = 0;
    *(vu_int *)&ptr[offset] = data;

    while((read_status(flash, offset) & desired) != desired) {
      wait++;
      if ((wait%10000)==0) {
        DEBUG  ("do_write_int, wait=%d\n"
		"addr = 0x%x, offset = 0x%lx, data = 0x%08x, flash: 0x%08x\n",
		wait,
                (int)&ptr[offset], offset, data, old);
        if (++wait1 > 32) {
	  return (-1);
        }
      }
      /* Busy wait */
    }
  }
  return (0);
}

static int do_write_short(struct embedded_flash *flash, off_t offset, u_short data)
{
  vu_char *ptr;
  int wait, wait1;
  u_short old;
  u_short desired;

  ptr = flash->ptr;

  write_intel_command(flash, offset, 0xFF, 0);
  if ( ((old = *(vu_short *)&ptr[offset]) & data) != data) {
    printk("Flash write error: "
	   "not fully erased at 0x%p+0x%lx: old 0x%08x  new 0x%08x\n",
	   ptr, offset, old, data);
    return (-1);
  }

#ifdef DEBUGFLASH
  if ((offset & (~1)) != offset) {
    printk("do_write_short: incorrectly aligned offset 0x%lx\n", offset);
    return -1;
  }
#endif

  if (old != data) {
    /* Program word:
     * XXX does not work on 32bit flash chips!
     */
    *(vu_short *)&ptr[offset] = (make_command(flash, 0x40) & 0xFFFF);
    desired = make_command(flash, 0x80) & 0xFFFF;
    wait = wait1 = 0;
    *(vu_short *)&ptr[offset] = data;

    /* We cannot rely on read_status() here because it reads the whole
     * word (on 32/64 bit buses) and the bits we're interested in may
     * come in the upper half-word.
     */
    while((*(vu_short *)&ptr[offset] & desired) != desired) {
      wait++;
      if ((wait%10000)==0) {
        DEBUG  ("do_write_short, wait=%d\n"
		"addr = %p, offset = 0x%lx, data = 0x%08x, flash: 0x%08x\n",
		wait,
                &ptr[offset], offset, data, old);
        if (++wait1 > 32) {
  	return (-1);
        }
      }
      /* Busy wait */
    }
  }

  return (0);
}

static int do_write_char(struct embedded_flash *flash, off_t offset, u_char data)
{
  vu_char *ptr;
  int wait, wait1;
  off_t s_offset;
  u_short value, old;

  ptr = flash->ptr;

  write_intel_command(flash, offset, 0xFF, 0);
  if (ptr[offset] == data)
    return (0);

  if (flash->bit_width > 0) {
    /* 16/32/64 bit bus: use do_write_short
     */
    s_offset = offset & (~1);
    old      = *(vu_short *)&ptr[s_offset];

    if (s_offset == offset) {		/* even address	*/
	value = (old & 0x00FF) | (data << 8);
    } else {				/* off address	*/
  	value = (old & 0xFF00) | data;
    }

    return (do_write_short(flash, s_offset, value));
  } else {
    /* 8bit bus needs special treatment (cannot use do_write_short)
     */
    if ( ((old = ptr[offset]) & data) != data) {
      printk("Flash write error: "
	   "not fully erased at 0x%p+0x%lx: old 0x%08x  new 0x%08x\n",
	   ptr, offset, old, data);
      return (-1);
    }
    ptr[offset] = 0x40;
    wait = wait1 = 0;
    ptr[offset] = data;

    while((ptr[offset] & 0x80) != 0x80) {
      wait++;
      if ((wait%10000)==0) {
        DEBUG  ("do_write_char, wait=%d\n"
		"addr = %p, offset = 0x%lx, data = 0x%08x, flash: 0x%08x\n",
		wait,
                &ptr[offset], offset, data, old);
        if (++wait1 > 32) {
  	return (-1);
        }
      }
      /* Busy wait */
    }

  }

  return (0);
}
#endif

static int intel_write(struct flash_device_info *fdi, const char *buf,
		     size_t count, off_t offset, int userspace_ptr)
{
  struct embedded_flash *flash = fdi->handle;
  struct timeval time1, time2;
  vu_char *ptr;
  u_short bit_width, chip_cfg;
  int ret;
  u_long end_offset;
#ifndef CONFIG_CU824
  u_int data;
#else
  u_int data_h, data_l, wp, cp, l, cnt, i;
#endif
  int loops;

  ret = 0;
  ptr = flash->ptr;
  bit_width = flash->bit_width;
  chip_cfg = flash->chip_cfg;

/***/
  DEBUG("intel_write buf=0x%x, count=%d=0x%x, offset=0x%x, us_ptr=%d\n",
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

#if 0
  m8xx_set_flash_vpp();
#endif

  do_gettimeofday(&time1);

  loops = 0;

#ifdef CONFIG_CU824
#define FLASH_WIDTH 8
  wp = (offset & ~(FLASH_WIDTH - 1));
  if ((l = offset - wp) != 0)
  {
	  data_h = data_l = 0;
    cnt = end_offset - offset;
    for (i = 0, cp = wp; i < l; i++, cp++)
    {
      if (i >= 4)
      {
        data_h = (data_h << 8) | ((data_l & 0xFF000000) >> 24);
      }

      data_l = (data_l << 8) | (*(u_char *)(flash->ptr + cp));
    }
    for (; i < FLASH_WIDTH && cnt > 0; ++i)
    {
      char tmp;

      if (userspace_ptr)
      {
        if (__get_user(tmp, (char *)buf))
        {
          ret = -EFAULT;
          goto UNLOCK_BYPASS_RESET_ERROR;
        }
      }
      else
      {
        tmp = *(char *)buf;
      }

      ((char *)buf)++;

      if (i >= 4)
      {
        data_h = (data_h << 8) | ((data_l & 0xFF000000) >> 24);
      }

      data_l = (data_l << 8) | tmp;

      --cnt; ++cp;
    }

    for (; cnt==0 && i<FLASH_WIDTH; ++i, ++cp)
    {
      if (i >= 4)
      {
        data_h = (data_h << 8) | ((data_l & 0xFF000000) >> 24);
      }

      data_l = (data_h << 8) | (*(u_char *)(flash->ptr + cp));
    }

    if (do_write_dword(flash, wp, data_h, data_l))
    {
      ret = -EFAULT;
      goto UNLOCK_BYPASS_RESET_ERROR;
    }

    offset = wp + FLASH_WIDTH;
  }
#endif

  while (offset<end_offset) {
#ifdef CONFIG_CU824
    if (end_offset - offset < FLASH_WIDTH)
    {
      break;
    }
    if (userspace_ptr)
    {
      if (__get_user(data_h, (int *)buf))
      {
        ret = -EFAULT;
        goto UNLOCK_BYPASS_RESET_ERROR;
      }
      if (__get_user(data_l, (int *)(buf + 4)))
      {
        ret = -EFAULT;
        goto UNLOCK_BYPASS_RESET_ERROR;
      }
    }
    else
    {
      data_h = *(int *)buf;
      data_l = *(int *)(buf + 4);
    }

    if (do_write_dword(flash, offset, data_h, data_l))
    {
      ret = -EFAULT;
      goto UNLOCK_BYPASS_RESET_ERROR;
    }

    buf += FLASH_WIDTH;
    offset += FLASH_WIDTH;
#else
    if ((offset%4 == 0) && (end_offset-offset >= 4) && (bit_width >= 2)) {
      if (userspace_ptr) {
        if (__get_user(data, (int *)buf)) {
          ret = -EFAULT;
          goto UNLOCK_BYPASS_RESET_ERROR;
        }
      } else {
        data = *(int *)buf;
      }

      if (do_write_int(flash, offset, data)) {
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

      if (do_write_short(flash, offset, data)) {
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
      if (do_write_char(flash, offset, data)) {
        ret = -EFAULT;
	goto UNLOCK_BYPASS_RESET_ERROR;
      }
      buf += 1;
      offset += 1;
    }
#endif

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

#ifdef CONFIG_CU824
  data_h = data_l = 0;
  cnt = end_offset - offset;
  wp = offset;
  write_intel_command(flash, offset, 0xFF, 0);
	for (i = 0, cp = wp; i < FLASH_WIDTH && cnt > 0; ++i, ++cp)
  {
    char tmp;

    if (userspace_ptr)
    {
      if (__get_user(tmp, (char *)buf))
      {
        ret = -EFAULT;
        goto UNLOCK_BYPASS_RESET_ERROR;
      }
    }
    else
    {
      tmp = *(char *)buf;
    }

    ((char *)buf)++;

    if (i >= 4)
    {
      data_h = (data_h << 8) | ((data_l & 0xFF000000) >> 24);
    }

    data_l = (data_l << 8) | tmp;

		--cnt;
	}

	for (; i < FLASH_WIDTH; ++i, ++cp)
  {
    if (i >= 4)
    {
      data_h = (data_h << 8) | ((data_l & 0xFF000000) >> 24);
    }

    data_l = (data_l << 8) | (*(u_char *)(flash->ptr + cp));
	}

  if (do_write_dword(flash, wp, data_h, data_l))
  {
    ret = -EFAULT;
    goto UNLOCK_BYPASS_RESET_ERROR;
  }
#endif 

UNLOCK_BYPASS_RESET_ERROR:
  write_intel_command(flash, 0, 0x50, 1);
#if 0
  m8xx_clr_flash_vpp();
#endif
  write_intel_command(flash, 0, 0xFF, 1);
ERROR:
  up(&flash->mutex);

  return ret;
}

static int intel_erase_sector(struct flash_device_info *fdi,
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

  DEBUG("%s[%d]: intel_erase count=%d, offset=0x%x, end=0x%x\n",__FILE__,__LINE__,
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

#if 0
  m8xx_set_flash_vpp();
#endif
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
	do_erase_sector(flash, offset);
	offset += region->sector_size;
      }
    }
  }
  write_intel_command(flash, 0, 0x50, 1);
#if 0
  m8xx_clr_flash_vpp();
#endif
  write_intel_command(flash, 0, 0xFF, 1);

 error:

  up(&flash->mutex);

  return ret;
}

static int intel_protect_sector(struct flash_device_info *fdi,
                              off_t offset, size_t count, int protected)
{
  return -ENOSYS;
}

static int intel_get_info(struct flash_device_info *fdi,
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

  t.size = flash->size;
  t.bitwidth = 8<<individual_bit_width(flash);
  t.chip_config = 1<<flash->chip_cfg;
  t.num_regions = fdi->num_regions;
  t.num_parts   = n;

  if (copy_to_user(flash_info, &t, sizeof(flash_info_t))) {
    return -EFAULT;
  }

  return 0;
}
