/*
 * linux/drivers/char/cfi_flash.c  Version 0.1
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
#include <asm/residual.h>

#undef	DEBUGFLASH

#ifdef DEBUGFLASH
#define DEBUG(fmt,args...) printk(fmt ,##args)
#else
#define DEBUG(fmt,args...)
#endif

#define MAKE_TWO(byte) (((u_short)byte)<<8 | byte)
#define MAKE_FOUR(byte) (((u_int)byte)<<24 | byte<<16 | byte<<8 | byte)


static struct flash_device_info * cfi_flash_region_probe (
			volatile unsigned char *ptr, u_long size,
			int bit_width, int chip_cfg, int addr_shft,
			int top_boot) __init;
#ifdef CONFIG_AMD_FLASH
extern void amd_flash_init(struct flash_device_info *fdi) __init;
#endif
#ifdef CONFIG_INTEL_FLASH
extern void intel_flash_init(struct flash_device_info *fdi) __init;
#endif

static char *vendor_name_table[] __initdata = {
  "Unknown",
  "None",
  "Intel/Sharp extended",
  "AMD/Fujitsu standard",
  "Intel standard",
  "AMD/Fujitsu extended",
  "Mitsubishi standard",
  "Mitsubishi extended"
  /* Add new at end */
};

unsigned long __init cfi_flash_probe(volatile unsigned char *ptr, unsigned long phys_addr)
{
  struct flash_device_info *fdi;
  struct embedded_flash *flash;
  unsigned char  qry[8];
  int total_buswidth=0;
  int chip_buswidth=0;
  int found_flash;
  int bit_width;
  int addr_shft;
  int chip_cfg;
  int n_chip;
  u_short primary_vendor;
  unsigned char *vendor_name;
  u_long size;
  unsigned char i, val, ext_off;
  unsigned char top_boot = 0;
  unsigned char id_string[] = { 'P', 'R', 'I', 0x31, };


  size = 0;
  found_flash = 0;

  /*
   * The early query attempts seem to confuse the
   * AM29LV640D flash chips on the TQM8260 board
   */
#ifndef CONFIG_TQM8260

  /*
   * Probe at 0x10 for 8/8 bit device chip
   */
  DEBUG("Write qry cmd (%02x) at addr 0x%8p\n",
	CFI_QUERY_CMD, ptr + CFI_QUERY_OFFSET);
  ptr[CFI_QUERY_OFFSET] = CFI_QUERY_CMD;  __asm__("eieio");
  qry[0] = ptr[0x10]; /* Extra read cycle */
  qry[0] = ptr[0x10];
  qry[1] = ptr[0x11];
  qry[2] = ptr[0x12];
  if ((qry[0]=='Q') &&
      (qry[1]=='R') &&
      (qry[2]=='Y')) {
    DEBUG("Found 8/8 bit device\n");
    found_flash = 1;
    total_buswidth=8;
    chip_buswidth=8;
  }
  else
  {
    DEBUG(" 8/ 8 Probing @ 0x10 yields %02X %02X %02X\n", qry[0], qry[1], qry[2]);
  }

  /*
   * Probe at 0x20 for 8/16 bit device chip
   */
  if (!found_flash) {
    DEBUG("Write qry cmd (%02x) at addr 0x%8p\n",
	CFI_QUERY_CMD, ptr + (CFI_QUERY_OFFSET<<1));
    ptr[CFI_QUERY_OFFSET<<1] = CFI_QUERY_CMD; __asm__("eieio");
    qry[0] = ptr[0x20]; /* Extra read cycle */
    qry[0] = ptr[0x20];
    qry[1] = ptr[0x21];
    qry[2] = ptr[0x22];
    qry[3] = ptr[0x23];
    qry[4] = ptr[0x24];
    qry[5] = ptr[0x25];
    if ((qry[0]=='Q') && (qry[1]=='Q') &&
	(qry[2]=='R') && (qry[3]=='R') &&
	(qry[4]=='Y') && (qry[5]=='Y')) {
      DEBUG("Found 8/16 bit device\n");
      found_flash = 1;
      total_buswidth=8;		/*  8 bit bus */
      chip_buswidth=16;		/* 16 bit device */
    }
    else
    {
      DEBUG(" 8/16 Probing @ 0x20 yields %02X %02X %02X %02X %02X %02X\n", 
	qry[0], qry[1], qry[2], qry[3], qry[4], qry[5]);
    }
  }

  /*
   * Probe at 0x20 for 16/8 bit device chip
   */
  if (!found_flash) {
    u_short qry_cmd = (u_short)MAKE_TWO(CFI_QUERY_CMD);
    u_short *s_ptr  = (u_short *)(&ptr[CFI_QUERY_OFFSET<<1]);
    DEBUG("Write qry cmd (%04x) at addr 0x%8p\n", qry_cmd, s_ptr);
    *s_ptr = qry_cmd; __asm__("eieio");
    qry[0] = ptr[0x20]; /* Extra read cycle */
    qry[0] = ptr[0x20];
    qry[1] = ptr[0x21];
    qry[2] = ptr[0x22];
    qry[3] = ptr[0x23];
    qry[4] = ptr[0x24];
#ifdef DEBUGFLASH
    qry[6] = ptr[0x26];
    qry[7] = ptr[0x27];
#endif
    if ((qry[0]=='Q') && (qry[1]=='Q') &&
	(qry[2]=='R') && (qry[3]=='R') &&
	(qry[4]=='Y') && (qry[5]=='Y')) {
      DEBUG("Found 16/8 bit device\n");
      found_flash = 1;
      total_buswidth=16;
      chip_buswidth=8;
    }
    else
    {
      DEBUG("16/ 8 Probing @ 0x20 yields %02X %02X %02X %02X %02X %02X %02X %02X\n",
	qry[0], qry[1], qry[2], qry[3], qry[4], qry[5], qry[6], qry[7]);
    }
  }

  /*
   * Probe at 0x20 for 16/16 bit device chip
   */
  if (!found_flash) {
    u_short qry_cmd = (u_short)CFI_QUERY_CMD;
    u_short *s_ptr  = (u_short *)(&ptr[CFI_QUERY_OFFSET<<1]);
    DEBUG("Write qry cmd (%04x) at addr 0x%8p\n", qry_cmd, s_ptr);
    *s_ptr = qry_cmd; __asm__("eieio");
    qry[0] = ptr[0x20]; /* Extra read cycle */
    qry[0] = ptr[0x20];
    qry[1] = ptr[0x21];
    qry[2] = ptr[0x22];
    qry[3] = ptr[0x23];
    qry[4] = ptr[0x24];
    qry[5] = ptr[0x25];
#ifdef DEBUGFLASH
    qry[6] = ptr[0x26];
    qry[7] = ptr[0x27];
#endif
#ifdef  __BIG_ENDIAN
    if ((qry[0]== 0 ) && (qry[1]=='Q') &&
	(qry[2]== 0 ) && (qry[3]=='R') &&
	(qry[4]== 0 ) && (qry[5]=='Y')) 
#else  /* little_endian */
    if ((qry[0]=='Q') && (qry[1]== 0 ) &&
	(qry[2]=='R') && (qry[3]== 0 ) &&
	(qry[4]=='Y') && (qry[5]== 0 )) 
#endif
    {
      DEBUG("Found 16/16 bit device\n");
      found_flash = 1;
      total_buswidth=16;
      chip_buswidth=16;
    }
    else
    {
      DEBUG("16/16 Probing @ 0x20 yields %02X %02X %02X %02X %02X %02X %02X %02X\n",
	qry[0], qry[1], qry[2], qry[3], qry[4], qry[5], qry[6], qry[7]);
    }
  }

 /*
  * Probe at 0x40 for 32/8 bit device chip
  */
  if (!found_flash) {
    u_int qry_cmd = MAKE_FOUR(CFI_QUERY_CMD);
    u_int *l_ptr  = (u_int *)(&ptr[CFI_QUERY_OFFSET<<2]);
    DEBUG("Write qry cmd (%08x) at addr 0x%8p\n", qry_cmd, l_ptr);
    *l_ptr = qry_cmd; __asm__("eieio");
    qry[0] = ptr[0x40]; /* Extra read cycle */
    qry[0] = ptr[0x40];
    qry[1] = ptr[0x41];
    qry[2] = ptr[0x42];
    qry[3] = ptr[0x43];
    qry[4] = ptr[0x44];
    qry[5] = ptr[0x45];
    qry[6] = ptr[0x46];
    qry[7] = ptr[0x47];
    if ((qry[0]=='Q') && (qry[1]=='Q') &&
	(qry[2]=='Q') && (qry[3]=='Q') &&
	(qry[4]=='R') && (qry[5]=='R') &&
	(qry[6]=='R') && (qry[7]=='R')) {
      DEBUG("Found 32/8 bit device\n");
      found_flash = 1;
      total_buswidth=32;
      chip_buswidth=8;
    }
    else
    {
      DEBUG("32/ 8 Probing @ 0x40 yields %02X %02X %02X %02X %02X %02X %02X %02X\n", 
	qry[0], qry[1], qry[2], qry[3], qry[4], qry[5], qry[6], qry[7]);
    }
  }

  /*
   * Probe at 0x40 for 32/16 bit device chip
   */
  if (!found_flash) {
    u_int qry_cmd = ((u_int)CFI_QUERY_CMD)<<16 | ((u_int)CFI_QUERY_CMD);
    u_int *l_ptr  = (u_int *)(&ptr[CFI_QUERY_OFFSET<<2]);
    DEBUG("Write qry cmd (%08x) at addr 0x%8p\n", qry_cmd, l_ptr);
    *l_ptr = qry_cmd; __asm__("eieio");
    qry[0] = ptr[0x40]; /* Extra read cycle */
    qry[0] = ptr[0x40];
    qry[1] = ptr[0x41];
    qry[2] = ptr[0x42];
    qry[3] = ptr[0x43];
    qry[4] = ptr[0x44];
    qry[5] = ptr[0x45];
    qry[6] = ptr[0x46];
    qry[7] = ptr[0x47];
#ifdef  __BIG_ENDIAN
    if ((qry[0]== 0 ) && (qry[1]== 'Q' ) &&
	(qry[2]== 0 ) && (qry[3]== 'Q' ) &&
	(qry[4]== 0 ) && (qry[5]== 'R' ) &&
	(qry[6]== 0 ) && (qry[7]== 'R' )) 
#else  /* little_endian */
    if ((qry[0]=='Q') && (qry[1]==  0  ) &&
	(qry[2]=='Q') && (qry[3]==  0  ) &&
	(qry[4]=='R') && (qry[5]==  0  ) &&
	(qry[6]=='R') && (qry[7]==  0  )) 
#endif
    {
      DEBUG("Found 32/16 bit device\n");
      found_flash = 1;
      total_buswidth=32;
      chip_buswidth=16;
    }
    else
    {
      DEBUG("32/16 Probing @ 0x40 yields %02X %02X %02X %02X %02X %02X %02X %02X\n", 
	qry[0], qry[1], qry[2], qry[3], qry[4], qry[5], qry[6], qry[7]);
    }
  }

  /*
  * Probe at 0x40 for 32/32 bit device chip
  */
  if (!found_flash) {
    u_int qry_cmd = (u_int)CFI_QUERY_CMD;
    u_int *l_ptr  = (u_int *)(&ptr[CFI_QUERY_OFFSET<<2]);
    DEBUG("Write qry cmd (%08x) at addr 0x%8p\n", qry_cmd, l_ptr);
    *l_ptr = qry_cmd; __asm__("eieio");
    qry[0] = ptr[0x40]; /* Extra read cycle */
    qry[0] = ptr[0x40];
    qry[1] = ptr[0x41];
    qry[2] = ptr[0x42];
    qry[3] = ptr[0x43];
    qry[4] = ptr[0x44];
    qry[5] = ptr[0x45];
    qry[6] = ptr[0x46];
    qry[7] = ptr[0x47];
#ifdef  __BIG_ENDIAN
    if ((qry[0]== 0 ) && (qry[1]== 0 ) &&
	(qry[2]== 0 ) && (qry[3]=='Q') &&
	(qry[4]== 0 ) && (qry[5]== 0 ) &&
	(qry[6]== 0 ) && (qry[7]=='R')) 
#else  /* little_endian */
    if ((qry[0]== 0 ) && (qry[1]== 0 ) &&
	(qry[2]=='Q') && (qry[3]== 0 ) &&
	(qry[4]== 0 ) && (qry[5]== 0 ) &&
	(qry[6]=='R') && (qry[7]== 0 )) 
#endif
    {
      DEBUG("Found 32/32 bit device\n");
      found_flash = 1;
      total_buswidth=32;
      chip_buswidth=32;
    }
    else
    {
      DEBUG("32/32 Probing @ 0x40 yields %02X %02X %02X %02X %02X %02X %02X %02X\n", 
	qry[0], qry[1], qry[2], qry[3], qry[4], qry[5], qry[6], qry[7]);
    }
  }

#endif /* CONFIG_TQM8260 */

  /*
  * Probe at 0x80 for 64/16 bit device chip
  */
  if (!found_flash) {
    u_int qry_cmd = ((u_int)CFI_QUERY_CMD)<<16 | ((u_int)CFI_QUERY_CMD);
    u_int *l_ptr  = (u_int *)(&ptr[CFI_QUERY_OFFSET<<3]);
    DEBUG("Write qry cmd (%08x) at addr (0x%8p,0x%8p)\n", qry_cmd,
			l_ptr, l_ptr+1);
    *l_ptr = qry_cmd; *(l_ptr+1) = qry_cmd; __asm__("eieio");
    qry[0] = ptr[0x80]; /* Extra read cycle */
    qry[0] = ptr[0x80];
    qry[1] = ptr[0x81];
    qry[2] = ptr[0x82];
    qry[3] = ptr[0x83];
    qry[4] = ptr[0x84];
    qry[5] = ptr[0x85];
    qry[6] = ptr[0x86];
    qry[7] = ptr[0x87];
#ifdef  __BIG_ENDIAN
    if ((qry[0]== 0 ) && (qry[1]== 'Q' ) &&
	(qry[2]== 0 ) && (qry[3]== 'Q' ) &&
	(qry[4]== 0 ) && (qry[5]== 'Q' ) &&
	(qry[6]== 0 ) && (qry[7]== 'Q' )) 
#else  /* little_endian */
    if ((qry[0]=='Q') && (qry[1]==  0  ) &&
	(qry[2]=='Q') && (qry[3]==  0  ) &&
	(qry[4]=='Q') && (qry[5]==  0  ) &&
	(qry[6]=='Q') && (qry[7]==  0  )) 
#endif
    {
      DEBUG("Found 64/16 bit device\n");
      found_flash = 1;
      total_buswidth=64;
      chip_buswidth=16;
    }
    else
    {
      DEBUG("64/16 Probing @ 0x80 yields %02X %02X %02X %02X %02X %02X %02X %02X\n", 
	qry[0], qry[1], qry[2], qry[3], qry[4], qry[5], qry[6], qry[7]);
    }
  }

  /* combinations:
   * Bus Width	Chip Width	# of chips	chip_cfg	addr_shft
   *	64	64		1	not implemented yet
   *	64	32		2	not implemented yet
   *	64	16		4		2		3
   *	64	8		8	not implemented yet
   *-----------------------------------------------------
   *	32	32		1		0		2
   *	32	16		2		1		2
   *	32	 8		4		2		2
   *-----------------------------------------------------
   *	16	32			not implemented yet
   *	16	16		1		0		1
   *	16	 8		2		1		1
   *-----------------------------------------------------
   *	 8	32			not implemented yet
   *	 8	16		1		0		1
   *	 8	 8		1		0		0
   */

  if (found_flash) {
    ulong xsize;
    char c;

    switch (total_buswidth) {
      case 64:	addr_shft = bit_width = 3;
		switch (chip_buswidth) {
		case 16: n_chip = 4; chip_cfg =  2; break;
		default: n_chip = 0; chip_cfg = -1; break;
		}
		break;
      case 32:	addr_shft = bit_width = 2;
		switch (chip_buswidth) {
		case 32: n_chip = 1; chip_cfg =  0; break;
		case 16: n_chip = 2; chip_cfg =  1; break;
		case  8: n_chip = 4; chip_cfg =  2; break;
		default: n_chip = 0; chip_cfg = -1; break;
		}
		break;
      case 16:	addr_shft = bit_width = 1;
      		switch (chip_buswidth) {
		case 32: n_chip = 0; chip_cfg = -1; break;
		case 16: n_chip = 1; chip_cfg =  0; break;
		case  8: n_chip = 2; chip_cfg =  1; break;
		default: n_chip = 0; chip_cfg = -1; break;
		}
		break;
      case  8: 	addr_shft = bit_width = 0;
		switch (chip_buswidth) {
		case 32: n_chip = 0; chip_cfg = -1; break;
		case 16: n_chip = 1; chip_cfg =  0; addr_shft = 1; break;
		case  8: n_chip = 1; chip_cfg =  0; break;
		default: n_chip = 0; chip_cfg = -1; break;
		}
		break;
      default:	chip_cfg = -1; break;
      }
      if (chip_cfg < 0) {
	printk ("Unknown CFI Flash Configuration: "
		"total buswidth=%d chip buswidth=%d address shift=%d\n",
		total_buswidth, chip_buswidth, addr_shft);
	found_flash = 0;
    }

    DEBUG ("bit_width: %d, chip_cfg: %d addr shft: %d\n",
	bit_width, chip_cfg, addr_shft);

    primary_vendor = cfi_read_short(ptr, CFI_PRIMARY_VENDOR_ID_OFFSET, 0,
				    bit_width, chip_cfg, addr_shft);

    DEBUG ("Primary Vendor = 0x%04x\n", primary_vendor);

    switch(primary_vendor) {
      case CFI_VENDOR_NONE:
	vendor_name = vendor_name_table[1];
      case CFI_VENDOR_INTEL_SHARP_EXTENDED:
	vendor_name = vendor_name_table[2];
	break;
      case CFI_VENDOR_AMD_FUJITSU_STANDARD:
	vendor_name = vendor_name_table[3];
	break;
      case CFI_VENDOR_INTEL_STANDARD:
	vendor_name = vendor_name_table[4];
	break;
      case CFI_VENDOR_AMD_FUJITSU_EXTENDED:
	vendor_name = vendor_name_table[5];
	break;
      case CFI_VENDOR_MITSUBISHI_STANDARD:
	vendor_name = vendor_name_table[6];
	break;
      case CFI_VENDOR_MITSUBISHI_EXTENDED:
	vendor_name = vendor_name_table[7];
	break;
      default:
	vendor_name = vendor_name_table[0];
	break;
    }

    size = cfi_read_char(ptr, CFI_DEVICE_SIZE_OFFSET,
    			0, bit_width, chip_cfg, addr_shft);
    size = (1<<size) * n_chip;

    xsize = size  >> 10;	/* in kB */
    c = 'k';

    if (xsize >= 1024) {
	xsize >>= 10;		/* in MB */
	c = 'M';
    }

    printk("Found %dx%dbit %lu%cByte CFI flash device of type %s at %lX\n",
	   n_chip, chip_buswidth,
           xsize, c,
	   vendor_name, phys_addr);

    /*
     * Support for AM29DL322D/323D/324D chips which announce bottom
     * or top boot sectors just by a bit in the Primary Extended CFI
     * Query Table
     */
    ext_off = cfi_read_char (ptr, CFI_PRIMARY_QUERY_TABLE_OFFSET,
				0, bit_width, chip_cfg, addr_shft);
    DEBUG("Primary Extended CFI Query Table found at offset 0x%X\n", ext_off);

    /* verify that we have a valid Primary Vendor-Specific Extended Query Table */
    for (i=0; ext_off && i<4; ++i) {
    	val = cfi_read_char (ptr, ext_off + i, 0, bit_width, chip_cfg, addr_shft);
	if (val != id_string[i]) {
		DEBUG("EXT+0x%02x: bad ID - want %02x, got %02x\n",
			i, id_string[i], val);
		ext_off = 0;
	}
    }

    if (ext_off) {
	val = cfi_read_char  (ptr, ext_off+0x0F, 0, bit_width, chip_cfg, addr_shft);
	DEBUG("EXT+0x0F: 0x%02X ==> %s Boot Device\n", val,
		(val == 2) ? "Bottom" :
		(val == 3) ? "Top" : "*** unknown ***");
	if (val == 3)
		top_boot = 1;
    }

    /* Now probe for regions */
    fdi = cfi_flash_region_probe (ptr, size,
    				  bit_width, chip_cfg, addr_shft, top_boot);
    if (fdi == 0) {
	DEBUG("Unable to allocate memory for flash regions\n");
	size = 0;
    }
#ifdef CONFIG_AMD_FLASH
    else if (primary_vendor == CFI_VENDOR_AMD_FUJITSU_STANDARD) {
	amd_flash_init(fdi);
    }
#endif
#ifdef CONFIG_INTEL_FLASH
    else if ((primary_vendor == CFI_VENDOR_INTEL_SHARP_EXTENDED) ||
    	     (primary_vendor == CFI_VENDOR_INTEL_STANDARD) ) {
	intel_flash_init(fdi);
    }
#endif
    else {
	DEBUG("Flash type %s not supported\n", vendor_name);
	kfree(fdi->region);
	kfree(fdi);
	size = 0;
    }
  }
  if (size) {
    flash = kmalloc(sizeof(struct embedded_flash), GFP_KERNEL);
    if (!flash) {
      kfree(fdi->region);
      kfree(fdi);
      size = 0;
    }
  }

  /* Return to read array mode: */
#ifndef CONFIG_TQM8260
#ifdef	CONFIG_PM826
  ((u_int *)ptr)[0] = 0xff;
  ((u_int *)ptr)[1] = 0xff;
  ((u_int *)ptr)[2] = 0xff;
  ((u_int *)ptr)[3] = 0xff;
#else
  ptr[0] = 0xff;
  ptr[1] = 0xff;
  ptr[2] = 0xff;
  ptr[3] = 0xff;
#endif /* CONFIG_PM826 */
#else
  /*
   * Reset sequence for TQM8260 (must reset all 4 chips)
   */
  ((u_int *)ptr)[0] = 0xf0;
  ((u_int *)ptr)[1] = 0xf0;
  ((u_int *)ptr)[2] = 0xf0;
  ((u_int *)ptr)[3] = 0xf0;
#endif /* CONFIG_TQM8260 */
  
  if (size) {
    iounmap ((void *)ptr);
    ptr = ioremap_nocache(phys_addr, size);

    fdi->handle = flash;
    flash->ptr = ptr;
    flash->size = size;
    flash->phys_addr = phys_addr;
    flash->bit_width = bit_width;
    flash->addr_shft = addr_shft;
    flash->chip_cfg = chip_cfg;
    init_MUTEX (&flash->mutex);

    fdi->next = NULL;
    fdi->handle = flash;
    fdi->dev = 0;
    register_flash(fdi);
  }
  return size;
}

/*
 * "top_boot" is a flag needed for newer flash chips like the
 * Am29DL322D/323D/324D, where the CFI data contain no indication if
 * it's a top or bottom boot sector device. We have to check and read
 * the Primary Vendor-Specific Extended Query table to find out...
 */

static struct flash_device_info * __init
cfi_flash_region_probe (volatile unsigned char *ptr, u_long size,
			int bit_width, int chip_cfg, int addr_shft,
			int top_boot)
{
  struct flash_device_info *fdi;
  int i;
  u_long offset;
  u_short reg_info;
  int sector_size;

  /* We allocate memory for all partitions,
   * but initialize only partition 0 = the whole device
   */
  fdi = kmalloc(FLASH_PART_MAX*sizeof(struct flash_device_info), GFP_KERNEL);
  if (!fdi) {
    return 0;
  }

  fdi->num_regions = cfi_read_char(ptr, CFI_NUM_ERASE_BLOCKS_OFFSET,
				     0, bit_width, chip_cfg, addr_shft);
  DEBUG("Num regions: %d\n", fdi->num_regions);

  fdi->region = kmalloc(sizeof(flash_region_info_t)*fdi->num_regions, GFP_KERNEL);
  if (!fdi->region) {
    kfree(fdi);
    return 0;
  }

  fdi->next = NULL;
  fdi->handle = 0;
  fdi->dev = 0;

  if (top_boot) {
	offset = size;
  } else {
	offset = 0;
  }

  for (i=0;i<fdi->num_regions;i++) {

    reg_info = cfi_read_short(ptr, CFI_ERASE_REGION_INFO_NBLOCKS_OFFSET+4*i,
			      0, bit_width, chip_cfg, addr_shft);
    fdi->region[i].num_sectors = reg_info + 1;

    reg_info = cfi_read_short(ptr, CFI_ERASE_REGION_INFO_SIZE_OFFSET+4*i,
			      0, bit_width, chip_cfg, addr_shft);
    sector_size = reg_info;
    if (sector_size==0)
      fdi->region[i].sector_size = 128 << chip_cfg;
    else
      fdi->region[i].sector_size = (sector_size*256) << chip_cfg;

    if (top_boot) {
	offset -= fdi->region[i].sector_size * fdi->region[i].num_sectors;
	fdi->region[i].offset = offset;
    } else {
	fdi->region[i].offset = offset;
	offset += fdi->region[i].sector_size * fdi->region[i].num_sectors;
    }

    DEBUG("sector %i offset %08lX size: %d, num_sectors: %d, total size: %d\n",
	i, fdi->region[i].offset,
	fdi->region[i].sector_size , fdi->region[i].num_sectors,
	fdi->region[i].sector_size * fdi->region[i].num_sectors);
  }

  /*
   * There are several places in the code that assume that the list of
   * erase regions is sorted by increasing address offset. So far,
   * this has always been true for all flash chips. Except for the
   * Am29DL322D/323D/324D, which break with this rule. Silly, isn't
   * it? Well, the easiest way to fix this problem is to _make_ the
   * list sorted. Since it's always a very short list, we use simple
   * bubble sort. - wd
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

  return fdi;
}
