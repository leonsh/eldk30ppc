/*
 * linux/drivers/char/flash.c  Version 0.1
 *
 * Copyright (C) 1999 Alexander Larsson (alla@lysator.liu.se or alex@signum.se)
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
#include <linux/module.h>
#include <linux/devfs_fs_kernel.h>
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

#undef	DEBUGFLASH

#ifdef DEBUGFLASH
#define DEBUG(fmt,args...) printk(fmt ,##args)
#else
#define DEBUG(fmt,args...)
#endif

#define MAKE_TWO(byte) (((u_short)byte)<<8 | byte)
#define MAKE_FOUR(byte) (((u_int)byte)<<24 | byte<<16 | byte<<8 | byte)

flash_device_info_t *flash_devices = NULL;
static int next_minor = 0;

/*
 * We are called for partition 0 only (full device),
 * but we have to initialize all partitions
 */
int register_flash(struct flash_device_info *fdi)
{
  int i, n, dev;

  fdi->dev = MKDEV(FLASH_MAJOR, next_minor);
  fdi->next = flash_devices;
  flash_devices = fdi;
  dev = MINOR(fdi->dev)>>FLASH_PART_BITS;

  next_minor += FLASH_PART_MAX;

  for (n=0, i=1; i<FLASH_PART_MAX; ++i) {
	fdi[i] = fdi[0];		/* copy initialization from device data */
	if ((dev < flash_part_num) && flash_partition_table[dev][i-1].size) {
		fdi[i].dev += i;	/* adjust minor number */
		++n;
	} else {
		fdi[i].dev = 0;		/* disable dev */
	}
  }

  printk("Registered flash device /dev/flash%c (minor %d, %d partitions)\n",
	dev+'a', MINOR(fdi->dev), n);

  return 0;
}

int unregister_flash(struct flash_device_info *unreg)
{
  struct flash_device_info *fdi, *prev;

  prev = NULL;
  fdi = flash_devices;
  while (fdi != NULL && fdi != unreg) {
    struct flash_device_info *next = fdi->next;

    prev = fdi;
    kfree (fdi);
    fdi = next;
  }

  if (fdi == NULL)
    return -1;

  if (prev)
    prev->next = fdi->next;
  else
    flash_devices = fdi->next;

  printk("unregistered flash device\n");

  return 0;
}

static loff_t flash_lseek(struct file * file, loff_t offset, int orig)
{
  switch (orig) {
  case 0:
    file->f_pos = offset;
    return file->f_pos;
  case 1:
    file->f_pos += offset;
    return file->f_pos;
  default:
    return -EINVAL;
  }
}

static u_long end_of_flash(struct flash_device_info *fdi)
{
  flash_region_info_t *last_region;

  last_region = &fdi->region[fdi->num_regions-1];

  return last_region->offset +
    last_region->sector_size*last_region->num_sectors;
}

/*
 * This funcion reads the flash memory.
 */
static ssize_t flash_read(struct file * file, char * buf,
                          size_t count, loff_t *ppos)
{
  struct flash_device_info *fdi = file->private_data;
  unsigned long p = *ppos;
  unsigned long end_flash;
  int rc;
  int dev0 = MINOR(fdi->dev) >> FLASH_PART_BITS;
  int part = MINOR(fdi->dev) &  FLASH_PART_MASK;

  DEBUG("flash_read: minor=0x%x (dev %d, part %d) off=0x%lx cnt=%d\n",
	MINOR(fdi->dev), dev0, part, p, count);

  if (part) {
	/* No need to check index - already done in open() */
	DEBUG("part offset 0x%08lx size %d\n",
		flash_partition_table[dev0][part-1].start,
		flash_partition_table[dev0][part-1].size);
	p += flash_partition_table[dev0][part-1].start;
	end_flash = flash_partition_table[dev0][part-1].start
		  + flash_partition_table[dev0][part-1].size;
  } else {
	end_flash = end_of_flash(fdi);
  }

  DEBUG("NEW off=0x%lx end=0x%lx cnt=%d\n", p, end_flash, count);

  if (p >= end_flash)
    return 0;

  if (count > end_flash - p)
    count = end_flash - p;

  rc = (*fdi->ops->read)(fdi, buf, count, p, 1);

  if (rc)
    return rc;

  *ppos += count;
  return count;
}

static ssize_t flash_write(struct file * file, const char * buf,
                           size_t count, loff_t *ppos)
{
  struct flash_device_info *fdi = file->private_data;
  unsigned long p = *ppos;
  unsigned long end_flash;
  int rc;
  int dev0 = MINOR(fdi->dev) >> FLASH_PART_BITS;
  int part = MINOR(fdi->dev) &  FLASH_PART_MASK;

  DEBUG("flash_write: minor=0x%x (dev %d, part %d) off=0x%lx cnt=%d\n",
	MINOR(fdi->dev), dev0, part, p, count);

  if (part) {
	/* No need to check index - already done in open() */
	DEBUG("part offset 0x%08lx size %d\n",
		flash_partition_table[dev0][part-1].start,
		flash_partition_table[dev0][part-1].size);
	p += flash_partition_table[dev0][part-1].start;
	end_flash = flash_partition_table[dev0][part-1].start
		  + flash_partition_table[dev0][part-1].size;
  } else {
	end_flash = end_of_flash(fdi);
  }

  DEBUG("NEW off=0x%lx end=0x%lx cnt=%d\n", p, end_flash, count);

  if (p >= end_flash)
    return (-ENOSPC);

  if (count > end_flash - p)
    count = end_flash - p;


  DEBUG("Writing flash @ %lx, count = %d\n", p, count);

  if (flash_on_sector_boundaries(fdi, p, count)) {
    /* On a sector boundary => Erase the sectors */
    rc = (*fdi->ops->erase_sector)(fdi, p, count);
    if (rc)
      return rc;
  }

  rc = (*fdi->ops->write)(fdi, buf, count, p, 1);
  if (rc) {
    return rc;
  }

  *ppos += count;
  return count;
}

static int flash_ioctl(struct inode * inode, struct file *filp,
                       unsigned int cmd, unsigned long arg)
{
  struct flash_device_info *fdi = filp->private_data;
  erase_info_t erase_info;
  int i, n_part, rc;
  int dev0 = MINOR(fdi->dev) >> FLASH_PART_BITS;
  int part = MINOR(fdi->dev) &  FLASH_PART_MASK;

  for (n_part=0, i=1; (dev0<flash_part_num) && (i<FLASH_PART_MAX); ++i) {
	if (flash_partition_table[dev0][i-1].size) {
		++n_part;
	}
  }

  DEBUG("flash_ioctl: minor=0x%x (dev %d, part %d) - total %d partitions\n",
	MINOR(fdi->dev), dev0, part, n_part);

  switch (cmd) {
  case FLASHGETINFO:
    rc = (*fdi->ops->get_info)(fdi, (flash_info_t *)arg);
    return rc;
    break;
  case FLASHGETREGIONINFO:
    rc = copy_to_user((char *)arg, fdi->region,
                       sizeof(flash_region_info_t)*fdi->num_regions);
    if (rc)
	return -EFAULT;
    return 0;
    break;
  case FLASHGETPARTINFO:
    rc = copy_to_user((char *)arg, &flash_partition_table[dev0][0],
			sizeof(flash_partition_t) * n_part);
    if (rc)
	return -EFAULT;
    return (0);
    break;
  case FLASHERASE:
    rc = copy_from_user(&erase_info, (erase_info_t *)arg, sizeof(erase_info_t));
    if (rc)
      return rc;

    DEBUG("Erase off=0x%lx size=%ld\n", erase_info.offset, erase_info.size);

    if (part) {
	unsigned long erase_end, part_end;

	/* No need to check index - already done in open() */
	DEBUG("part offset 0x%08lx size %d\n",
		flash_partition_table[dev0][part-1].start,
		flash_partition_table[dev0][part-1].size);
	erase_info.offset += flash_partition_table[dev0][part-1].start;

	erase_end = erase_info.offset + erase_info.size;
	part_end = flash_partition_table[dev0][part-1].start
		 + flash_partition_table[dev0][part-1].size;

	if (erase_end > part_end)
		return -ENOSPC;
    }

    DEBUG("NEW off=0x%lx size=%ld\n", erase_info.offset, erase_info.size);

    rc = (*fdi->ops->erase_sector)(fdi, erase_info.offset, erase_info.size);
    return rc;
    break;
  default:
    return -EINVAL;
  }
  return -EINVAL;
}

static int flash_mmap (struct file *file, struct vm_area_struct *vma)
{
	struct flash_device_info *fdi = file->private_data;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

	/* map flash always readonly */
	if (pgprot_val (vma->vm_page_prot) & _PAGE_RW) {
		DEBUG ("flash can only be mapped readonly!\n");
		return (-EINVAL);
	}

	vma->vm_flags |= VM_RESERVED;
	vma->vm_flags |= VM_IO;

	pgprot_val (vma->vm_page_prot) |= _PAGE_NO_CACHE | _PAGE_GUARDED;

	if (remap_page_range (vma->vm_start,
			((embedded_flash_t *)(fdi->handle))->phys_addr + offset,
			vma->vm_end - vma->vm_start,
			vma->vm_page_prot)) {
		return (-EAGAIN);
	}
	return 0;
}

static
struct flash_device_info *flash_find_device (kdev_t dev)
{
  struct flash_device_info *fdi;
  int dev0 = dev & ~FLASH_PART_MASK;
  int part = dev &  FLASH_PART_MASK;

  DEBUG ("flash_find_device: dev=0x%x dev0=0x%x part=%d\n", dev, dev0, part);

  fdi = flash_devices;
  while (fdi != NULL) {
    DEBUG ("flash_find_device: at %p fdi->dev=0x%x\n", fdi, fdi->dev);
    if (fdi->dev == dev0) {
	DEBUG ("==> found, use part %d at %p: fdi->dev=0x%x\n",
		part, &fdi[part], fdi->dev);
	if (fdi->dev) {
		return (&fdi[part]);
	} else {
		return NULL;
	}
    }
    fdi = fdi->next;
  }
  return fdi;
}


static int flash_open(struct inode * inode, struct file * filp)
{
  kdev_t dev = inode->i_rdev;
  struct flash_device_info *fdi = flash_find_device(dev);

  DEBUG("entering flash_open, minor=%d [%d]: bank %d, partition %d\n",
	MINOR(dev), fdi?MINOR(fdi->dev):-1,
	MINOR(dev)>>FLASH_PART_BITS, MINOR(dev)&FLASH_PART_MASK);

  if ((fdi == NULL) || (fdi->dev == 0))
    return -ENODEV;

  filp->private_data = fdi;

  DEBUG("opened device /dev/flash%c%d\n",
	(MINOR(dev)>>FLASH_PART_BITS)+'a', MINOR(dev)&FLASH_PART_MASK);

  return 0;
}


static struct file_operations flash_fops = {
	llseek:		flash_lseek,
	read:		flash_read,
	write:		flash_write,
	mmap:		flash_mmap,
	ioctl:		flash_ioctl,
	open:		flash_open,
};

int flash_on_sector_boundaries(struct flash_device_info *fdi,
                              off_t offset, size_t count)
{
  flash_region_info_t *region = NULL;
  int i;
  u_long region_size;

  /* find start and check that it is on sector boundary */
  for (i = 0; i<fdi->num_regions; i++) {
    region = &fdi->region[i];
    region_size = region->sector_size*region->num_sectors;
    if  ( (offset >= region->offset) &&
          (offset < region->offset+region_size) ) {
      if ((offset % region->sector_size) != 0) {
	DEBUG("in region %d, sector %ld, %ld bytes in\n",
		i,
		(offset - region->offset) / region->sector_size,
		(offset - region->offset) % region->sector_size);
	return 0;	/* Not on sector boundaries */
      } else {
	DEBUG("in region %d, sector %ld\n",
		i,
		(offset - region->offset) / region->sector_size);
        break;		/* OK: start is on sector boundary */
      }
    }
  }
  /* find end and check that it is on sector boundary */
  offset += count;
  for ( ; i<fdi->num_regions; i++) {
    region = &fdi->region[i];
    region_size = region->sector_size*region->num_sectors;
    if  ( (offset >= region->offset) &&
          (offset <= region->offset+region_size) ) {	/* include end here, too */
      if ((offset % region->sector_size) != 0) {
	return 0;	/* Not on sector boundaries */
      } else {
        return 1;	/* OK: end is on sector boundary, too */
      }
    }
  }
  /* we fell trough: end is beyond device limit */
  return 0;
}


unsigned long __init
	cfi_flash_probe(volatile unsigned char *ptr, unsigned long phys_addr);
unsigned long __init
	jedec_flash_probe(volatile unsigned char *ptr, unsigned long phys_addr);


/********** Init stuff: ********/

int __init flash_init(void)
{
  unsigned long probe_addr, end_addr;
  unsigned long size;
  volatile unsigned char *ptr;
  bd_t *bd = (bd_t *)__res;  /* pointer to board info data */

  if (devfs_register_chrdev(FLASH_MAJOR,"flash",&flash_fops)) {
    printk("unable to get major %d for flash devs\n", FLASH_MAJOR);
    return -EIO;
  }

  end_addr = bd->bi_flashstart + bd->bi_flashsize - 1;

  for ( probe_addr = bd->bi_flashstart ;
        (probe_addr <= end_addr) && (probe_addr >= bd->bi_flashstart) ;
	/* empty */
      ) {

    DEBUG("Probing for FLASH memory at 0x%lx ...\n", probe_addr);
    /*
     * Map only 16k now; remap for real size in the probe routine
     */
    ptr = ioremap_nocache(probe_addr, 16<<10);

    DEBUG("Probing for CFI FLASH memory at 0x%lx (%p)...\n", probe_addr, ptr);
    size = cfi_flash_probe(ptr, probe_addr);

    if (size == 0) {
      DEBUG("Probing for JEDEC FLASH memory at 0x%lx (%p)...\n", probe_addr, ptr);
      size = jedec_flash_probe(ptr, probe_addr);
    }

    if (size) {
      probe_addr += size;
    } else {
      iounmap((void *)ptr);
      printk("Couldn't find flash device at %lx\n", probe_addr);
      break;
    }
  }
  
  return 0;
}

module_init(flash_init);
