/*
 *  drivers/video/lynxfb.c -- frame buffer device for
 *  Silicon Motion Lynx3DM chip.
 *
 *  This was written to drive the Lynx board in the Embedded Planet
 *  Icebox (embedded PPC machine) and is only tested on that machine.
 *  It should really be generalised quite a bit - for one thing it is
 *  hard-wired to 640x480.
 *
 *  Copyright (C) 2002 David Gibson, IBM Corporation.
 *
 *  Based on chipsfb.c:
 *  Copyright (C) 1998 Paul Mackerras,
 *  which in turn is derived from the Powermac "chips" driver:
 *  Copyright (C) 1997 Fabio Riccardi.
 *  And from the frame buffer device for Open Firmware-initialized devices:
 *  Copyright (C) 1997 Geert Uytterhoeven.
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.  */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fb.h>
#include <linux/selection.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <asm/io.h>

#include <video/fbcon.h>
#include <video/fbcon-cfb8.h>
#include <video/fbcon-cfb16.h>
#include <video/fbcon-cfb24.h>
#include <video/macmodes.h>

MODULE_LICENSE("GPL");

struct fb_info_lynx {
	struct fb_info info;
	struct fb_fix_screeninfo fix;
	struct fb_var_screeninfo var;
	struct display disp;
	struct {
		u8 red, green, blue;
	} palette[256];
	struct pci_dev *pdev;

	u8 chiprev;
	unsigned long ramsize;

	unsigned long aperture_phys;
	u8 *aperture;
	unsigned long frame_buffer_phys;
	u8 *frame_buffer;

	struct fb_info_lynx *next;
#ifdef FBCON_HAS_CFB16
	u16 fbcon_cfb16_cmap[16];
#endif
#ifdef FBCON_HAS_CFB16
	u32 fbcon_cfb24_cmap[16];
#endif
};
static struct fb_info_lynx *all_lynx;
static int currcon; /* = 0 */

#define MMIO_DPR_OFFSET	(0x000)
#define MMIO_VPR_OFFSET	(0x800)
#define MMIO_IO_OFFSET	(0xc0000)

#if 0
#define write_ind(lp, num, val, ap, dp)	do { \
		writeb((num), (lp)->aperture + MMIO_IO_OFFSET + (ap)); \
		writeb((val), (lp)->aperture + MMIO_IO_OFFSET + (dp)); \
	} while (0)
#define read_ind(lp, num, ap, dp)	({ writeb((num), (lp)->aperture + MMIO_IO_OFFSET + (ap)); \
	readb((lp)->aperture + MMIO_IO_OFFSET + (dp));})
#else
#define write_ind(lp, num, val, ap, dp) do { \
		outb((num), (ap)); outb((val), (dp)); \
	} while (0)
#define read_ind(lp, num, ap, dp)	({ outb((num), (ap)); inb((dp)); })
#endif

/* CRTC registers */
#define write_crt(lp, num, val)	write_ind(lp, num, val, 0x3d4, 0x3d5)
#define read_crt(lp, num)	read_ind(lp, num, 0x3d4, 0x3d5)
/* graphics registers */
#define write_grx(lp, num, val)	write_ind(lp, num, val, 0x3ce, 0x3cf)
#define read_grx(lp, num)	read_ind(lp, num, 0x3ce, 0x3cf)
/* sequencer registers */
#define write_seq(lp, num, val)	write_ind(lp, num, val, 0x3c4, 0x3c5)
#define read_seq(lp, num)	read_ind(lp, num, 0x3c4, 0x3c5)
/* system control registers */
#define write_scr write_seq
#define read_scr read_seq
/* flat panel registers */
#define write_fpr write_seq
#define read_fpr read_fpr
/* memory control registers */
#define write_mcr write_seq
#define read_mcr read_seq

/* attribute controller */
#define write_atr(lp, num, val) do { \
		inb(0x3da); write_ind(lp, num, val, 0x3c0, 0x3c0); \
	} while (0)
#define read_atr(lp, num) ({inb(0x3da); read_ind(lp, num, 0x3c0, 0x3c1); })

/* Drawing processor */
#define write_dprw(lp, off, val) writew((val), (lp)->aperture + MMIO_DPR_OFFSET + (off))
#define read_dprw(lp, off) readw((lp)->aperture + MMIO_DPR_OFFSET + (off))
#define write_dprl(lp, off, val) writel((val), (lp)->aperture + MMIO_DPR_OFFSET + (off))
#define read_dprl(lp, off) readl((lp)->aperture + MMIO_DPR_OFFSET + (off))
/* Video processor */
#define write_vprw(lp, off, val) writew((val), (lp)->aperture + MMIO_VPR_OFFSET + (off))
#define read_vprw(lp, off) readw((lp)->aperture + MMIO_VPR_OFFSET + (off))
#define write_vprl(lp, off, val) writel((val), (lp)->aperture + MMIO_VPR_OFFSET + (off))
#define read_vprl(lp, off) readl((lp)->aperture + MMIO_VPR_OFFSET + (off))


static int __init lynxfb_init_one(struct pci_dev *dp, const struct pci_device_id *ent);
static void lynx_init_hw(struct fb_info_lynx *lynx);
static int lynx_get_fix(struct fb_fix_screeninfo *fix, int con, struct fb_info *info);
static int lynx_get_var(struct fb_var_screeninfo *var, int con, struct fb_info *info);
static int lynx_set_var(struct fb_var_screeninfo *var, int con, struct fb_info *info);
static int lynxfb_switch_con(int con, struct fb_info *info);
static int lynxfb_updatevar(int con, struct fb_info *info);
static void lynxfb_blank(int blank, struct fb_info *info);
static int lynx_get_cmap(struct fb_cmap *cmap, int kspc, int con,
			 struct fb_info *info);
static int lynx_set_cmap(struct fb_cmap *cmap, int kspc, int con,
			 struct fb_info *info);
static void lynx_set_bitdepth(struct fb_info_lynx *p, struct display *disp, int con, int bpp);
static void do_install_cmap(int con, struct fb_info *info);


static struct fb_ops lynxfb_ops = {
	owner:		THIS_MODULE,
	fb_get_fix:	lynx_get_fix,
	fb_get_var:	lynx_get_var,
	fb_set_var:	lynx_set_var,
	fb_get_cmap:	lynx_get_cmap,
	fb_set_cmap:	lynx_set_cmap,
};

static struct pci_device_id lynxfb_pci_tbl[] __devinitdata = {
	{PCI_VENDOR_ID_SILICONMOTION, PCI_DEVICE_ID_SILICONMOTION_SM720,
	 	PCI_ANY_ID, PCI_ANY_ID,},
	{0,},
};

static struct pci_driver lynxfb_pci_driver = {
	name:		"lynxfb",
	id_table:	lynxfb_pci_tbl,
	probe:		lynxfb_init_one,
	remove:		NULL,
	suspend:	NULL,
	resume:		NULL,
};

int __init lynx_init(void)
{
	printk(KERN_DEBUG "lynxfb.c: lynx_init()\n");
	return pci_module_init(&lynxfb_pci_driver);
}

module_init(lynx_init);

static int __init lynxfb_init_one(struct pci_dev *dp, const struct pci_device_id *ent)
{
	struct fb_info_lynx *p = NULL;
	int err = 0;
	unsigned long addr, size;
	int i;
	u16 cmd;

	printk(KERN_DEBUG "lynxfb.c: lynxfb_init_one() slot=%s\n", dp->slot_name);

	if ((dp->resource[0].flags & IORESOURCE_MEM) == 0)
		return -EBUSY; /* FIXME: not sure what error codes we should use here */
	addr = dp->resource[0].start;
	size = dp->resource[0].end + 1 - addr;
	if (addr == 0)
		return -EBUSY;

	err = pci_request_regions(dp, "lynxfb");
	if (err)
		return err;

	p = kmalloc(sizeof(*p), GFP_ATOMIC);
	if (! p) {
		err = -ENOMEM;
		goto fail;
	}
	memset(p, 0, sizeof(*p));

	pci_set_drvdata(dp, p);

//#ifdef __BIG_ENDIAN
//	addr += 0x2000000;
//#endif
	p->pdev = dp;
	p->aperture_phys = addr;
	p->aperture = ioremap(addr, 0x2000000);

#if 0
	/* we should use pci_enable_device here, but,
	   the device doesn't declare its I/O ports in its BARs
	   so pci_enable_device won't turn on I/O responses */
	pci_read_config_word(dp, PCI_COMMAND, &cmd);
	cmd |= 3;	/* enable memory and IO space */
	pci_write_config_word(dp, PCI_COMMAND, cmd);
#else
	err = pci_enable_device(dp);
	if (err) {
		err = -EBUSY;
		goto fail;
	}
#endif

	p->frame_buffer_phys = p->aperture_phys + 0x200000;
	p->frame_buffer = p->aperture + 0x200000;

	pci_read_config_byte(dp, PCI_REVISION_ID, &p->chiprev);
	printk(KERN_DEBUG "lynxfb.c: Chip Revision %x\n", (unsigned) p->chiprev);

	lynx_init_hw(p);

	printk(KERN_DEBUG "lynxfb.c: aperture physical addr 0x%08lx, virtual addr 0x%p\n",
	       p->aperture_phys, p->aperture);

	strcpy(p->fix.id, "Lynx3DM");
	p->fix.smem_start = p->frame_buffer_phys;
	p->fix.smem_len = p->ramsize;
	p->fix.type = FB_TYPE_PACKED_PIXELS;
	p->fix.visual = FB_VISUAL_PSEUDOCOLOR;
	p->fix.line_length = 640*3;

	p->var.xres = 640;
	p->var.yres = 480;
	p->var.xres_virtual = 640;
	p->var.yres_virtual = 480;
	p->var.bits_per_pixel = 24;
	p->var.red.offset = 0;
	p->var.green.offset = 8;
	p->var.blue.offset = 16;
	p->var.red.length = p->var.green.length = p->var.blue.length = 8;
	p->var.height = p->var.width = -1;
	p->var.vmode = FB_VMODE_NONINTERLACED;
	p->var.pixclock = 10000;
	p->var.left_margin = p->var.right_margin = 16;
	p->var.upper_margin = p->var.lower_margin = 16;
	p->var.hsync_len = p->var.vsync_len = 8;

	p->disp.var = p->var;
	p->disp.cmap.red = NULL;
	p->disp.cmap.green = NULL;
	p->disp.cmap.blue = NULL;
	p->disp.cmap.transp = NULL;
	p->disp.screen_base = p->frame_buffer;
	p->disp.visual = p->fix.visual;
	p->disp.type = p->fix.type;
	p->disp.type_aux = p->fix.type_aux;
	p->disp.line_length = p->fix.line_length;
	p->disp.can_soft_blank = 1;
#ifdef FBCON_HAS_CFB24
	p->disp.dispsw = &fbcon_cfb24;
	p->disp.dispsw_data = p->fbcon_cfb24_cmap;
#else
	p->disp.dispsw = &fbcon_dummy;
#endif
	p->disp.scrollmode = SCROLL_YREDRAW;

	strcpy(p->info.modename, p->fix.id);
	p->info.node = -1;
	p->info.fbops = &lynxfb_ops;
	p->info.disp = &p->disp;
	p->info.fontname[0] = 0;
	p->info.changevar = NULL;
	p->info.switch_con = &lynxfb_switch_con;
	p->info.updatevar = &lynxfb_updatevar;
	p->info.blank = &lynxfb_blank;
	p->info.flags = FBINFO_FLAG_DEFAULT;

	for (i = 0; i < 16; ++i) {
		int j = color_table[i];
		p->palette[i].red = default_red[j];
		p->palette[i].green = default_grn[j];
		p->palette[i].blue = default_blu[j];
	}

	err = register_framebuffer(&p->info);
	if (err < 0)
		goto fail;

	p->next = all_lynx;
	all_lynx = p;

	return 0;

 fail:
	if (p) {
		if (p->aperture)
			iounmap(p->aperture);
		kfree(p);
	}

	pci_release_regions(dp);
	return err;
}

#if 0
static void lynxfb_remove_one(struct pci_dev *pdev)
{
	struct fb_info_lynx *lynx;

	lynx = (struct fb_info_lynx *)pci_get_drvdata(pdev); 

	/* FIXME: unregister framebuffer */

	if (lynx->aperture) {
		iounmap(lynx->aperture);
	}
	kfree(lynx);
	pci_release_regions(pdev);
}
#endif

/* The memory initialization and detection code is inspired by some
   code provided by Silicon Motion Inc. */
#define MEM_BLOCK_SIZE		(512 * 1024)	/* check memory in 512kB blocks */

unsigned long __init test_memory(u8 *fb, unsigned long range)
{
	unsigned long offset;
	unsigned long memsize = 0;

	for (offset = range; offset != 0; offset -= MEM_BLOCK_SIZE) {
		*(u32 *) (fb + offset - 4) = offset ^ 0x01234567;
		*(u32 *) (fb + offset - 8) = offset ^ 0x456789AB;
		*(u32 *) (fb + offset - 12) = offset ^ 0x89ABCDEF;
		*(u32 *) (fb + offset - 16) = offset ^ 0xCDEF0123;
	}

	for (offset = MEM_BLOCK_SIZE; offset <= range; offset += MEM_BLOCK_SIZE) {
		// In each block, verify the 128 bits.
		if ((*(u32 *) (fb + offset - 4) == (offset ^ 0x01234567))
		    && (*(u32 *) (fb + offset - 8) == (offset ^ 0x456789AB))
		    && (*(u32 *) (fb + offset - 12) == (offset ^ 0x89ABCDEF))
		    && (*(u32 *) (fb + offset - 16) == (offset ^ 0xCDEF0123))) {
			memsize += MEM_BLOCK_SIZE;
		} else
			break;
	}

	return memsize;
}

void __init lynx_ram_init(struct fb_info_lynx *lynx)
{
	unsigned long range;
	u8 *fb;

	lynx->ramsize = 0;

	range = 0x1000000;	// 16MB

	write_scr(lynx, 0x18, read_scr(lynx, 0x18) | 0x41);

	fb = lynx->frame_buffer;

	write_mcr(lynx, 0x62, 0xff);
	write_mcr(lynx, 0x76, 0x7f);
	write_crt(lynx, 0xff, 0x00);
	
	if (lynx->chiprev >= 0xc0) {
		*((u16 *)(fb + 4)) = 0x5AA5;
		if (*((u16 *)(fb + 4)) != 0x5AA5)
			write_mcr(lynx, 0x62, 0xfe);
	}
		
	write_mcr(lynx, 0x76, 0x7b);

	if ( (test_memory(fb, 0x100000) != 0) && (lynx->chiprev < 0xc0) )
		write_mcr(lynx, 0x62, 0xbb);
	else
		write_crt(lynx, 0xff, 0x80);

	write_mcr(lynx, 0x76, 0x7f);

	lynx->ramsize = test_memory(fb, 0x1000000); /* Test a 16MB range */
		
	if (lynx->ramsize == 0x400000)
		write_mcr(lynx, 0x76, 0xff);
	else
		write_mcr(lynx, 0x76, 0x3f);

	
}

struct regval {
	u8 index, val;
};

/* FIXME: these tables can probably be trimmed rather a lot */
static struct regval crt_init_tbl[] = {
	{0x00, 0x5f},
	{0x01, 0x4f},
	{0x02, 0x4f},
	{0x03, 0x00},
	{0x04, 0x52},
	{0x05, 0x1e},
	{0x06, 0x0b},
	{0x07, 0x3e},
	{0x08, 0x00},
	{0x09, 0x40},
	{0x0a, 0x00},
	{0x0b, 0x00},
	{0x0c, 0x00},
	{0x0d, 0x00},
	{0x0e, 0x00},
	{0x0f, 0x00},

	{0x10, 0xea},
	{0x11, 0x2c},
	{0x12, 0xdf},
	{0x13, 0x28},
	{0x14, 0x00},
	{0x15, 0xdf},
	{0x16, 0x0b},
	{0x17, 0xc3},
	{0x18, 0xff},

	/* extended */
	{0x30, 0x00},
	{0x31, 0x00},
	{0x32, 0x00},
	{0x33, 0x00},
	{0x34, 0x00},
	{0x35, 0x55},
	{0x36, 0x03},
	{0x37, 0x10},
	{0x38, 0x00},
	{0x39, 0x00},
	{0x3a, 0x00},
	{0x3b, 0x40},
	{0x3c, 0x00},
	{0x3d, 0x20},
	{0x3e, 0x00},
	{0x3f, 0x00},
	{0x9e, 0x03},
	{0x9f, 0x00},
	{0x90, 0x56},
	{0x91, 0xdd},
	{0x92, 0x5e},
	{0x93, 0xea},
	{0x94, 0x87},
	{0x95, 0x44},
	{0x96, 0x8f},
	{0x97, 0x55},
	{0x98, 0xdf},
	{0x99, 0xff},
	{0x9a, 0x55},
	{0x9b, 0x0a},
	{0xa0, 0x00},
	{0xa1, 0x00},
	{0xa2, 0x00},
	{0xa3, 0x00},
	{0xa4, 0x00},
	{0xa5, 0x00},
	{0xa6, 0x00},
	{0xa7, 0x00}, 
	{0xa8, 0x00},
	{0xa9, 0x00},
	{0xaa, 0x00},
	{0xab, 0x00},
	{0xac, 0x00},
	{0xad, 0x00},
	/* SVRs */
	{0x40, 0x5f},
	{0x41, 0x4f},
	{0x42, 0x00},
	{0x43, 0x52},
	{0x44, 0x1e},
	{0x45, 0x0b},
	{0x46, 0xdf},
	{0x47, 0x00},
	{0x48, 0xe9},
	{0x49, 0x0b},
	{0x4a, 0x2e},
	{0x4b, 0x00},
	{0x4c, 0x4f},
	{0x4d, 0xdf},
};

static struct regval grx_init_tbl[] = {
	{0x00, 0x00},
	{0x01, 0x00},
	{0x02, 0x00},
	{0x03, 0x00},
	{0x04, 0x00},
	{0x05, 0x40},
	{0x06, 0x05},
	{0x07, 0x0f},
	{0x08, 0xff},
};

static struct regval atr_init_tbl[] = {
	{0x00, 0x3f},
	{0x01, 0x01},
	{0x02, 0x02},
	{0x03, 0x03},
	{0x04, 0x04},
	{0x05, 0x05},
	{0x06, 0x06},
	{0x07, 0x07},
	{0x08, 0x08},
	{0x09, 0x09},
	{0x0a, 0x0a},
	{0x0b, 0x0b},
	{0x0c, 0x0c},
	{0x0e, 0x0e},
	{0x0f, 0x0f},

	{0x10, 0x41},
	{0x11, 0x00},
	{0x12, 0x0f},
	{0x13, 0x00},
	{0x14, 0x00},
};

static struct regval seq_init_tbl[] = {
	{0x00, 0x01},
	{0x01, 0x01},
	{0x02, 0x0f},
	{0x03, 0x00},
	{0x04, 0x0e},
	/* SCRs */
	{0x17, 0x0c},
	{0x18, 0x51},
	{0x19, 0x00},
	/* PDRs */
	{0x20, 0x84},
	{0x21, 0xb0},
	{0x22, 0x32},
	{0x23, 0x00},
	{0x24, 0x01},
	/* FPRs */
	{0x30, 0x22},
	{0x31, 0x01},
	{0x32, 0x24},
	{0x33, 0x00},
	{0x34, 0xc0},
	{0x3e, 0x03},
	{0x3f, 0xff},
	{0x40, 0x00},
	{0x41, 0xfc},
	{0x42, 0x00},
	{0x43, 0x00},
	{0x44, 0x20},
	{0x45, 0xf0},
	{0x46, 0x00},
	{0x47, 0xfc},
	{0x48, 0x20},
	{0x49, 0x3c},
	{0x4a, 0x44},
	{0x4b, 0x20},
	{0x4c, 0x00},
	{0x4d, 0x00},
	{0x50, 0x04},
	{0x51, 0x24},
	{0x52, 0x63},
	{0x53, 0x4f},
	{0x54, 0x52},
	{0x55, 0x0b},
	{0x56, 0xdf},
	{0x57, 0xe9},
	{0x58, 0x04},
	{0x59, 0x03},
	{0x5a, 0x19},
	{0xa0, 0x00},
	{0xa1, 0x00},
	{0xa2, 0x00},
	{0xa3, 0x01},
	{0xa4, 0x00},
	{0xa5, 0xed},
	{0xa6, 0xed},
	{0xa7, 0xed},
	{0xa8, 0x00},
	{0xa9, 0x00},
	{0xaa, 0x00},
	{0xab, 0x00},
	{0xac, 0x00},
	{0xad, 0x02},
	{0xae, 0x00},
	{0xaf, 0x00},
	/* MCRs */
	{0x60, 0x00},
	{0x61, 0x00},
	{0x62, 0xff},
	{0x76, 0x3f},
	/* CCRs */
	{0x65, 0x00},
	{0x66, 0x03},
	{0x68, 0x54},
	{0x69, 0x04},
	{0x6a, 0x0d},
	{0x6b, 0x02},
	{0x6c, 0x07},
	{0x6d, 0x82},
	{0x6e, 0x07},
	{0x6f, 0x82},
	{0x7a, 0x30},
	{0x7b, 0x30},
	{0x7c, 0x30},
	{0x7d, 0x00},
	/* GPRs */
	{0x70, 0x40},
	{0x71, 0xdd},
	{0x72, 0x0c},
	{0x73, 0x0c},
	{0x74, 0x24},
	{0x75, 0xa0},
	/* PHRs */
	{0x80, 0xff},
	{0x81, 0x00},
	/* POPs */
	{0x82, 0x00},
	{0x84, 0x24},
	{0x85, 0xa0},
	{0x86, 0xc7},
	{0x90, 0x00},
	{0x91, 0x00},
	{0x92, 0x00},
	{0x93, 0x01},
	/* HCRs */
	{0x88, 0x00},
	{0x89, 0x00},
	{0x8a, 0x00},
	{0x8b, 0x00},
	{0x8c, 0xff},
	{0x8d, 0xff},
};

#define N_ELTS(x)	(sizeof(x) / sizeof(x[0]))

static void __init lynx_init_hw(struct fb_info_lynx *lynx)
{
	int i, j;

	outb(0xe3, 0x3c2); /* set misc output reg */

	for (i = 0; i < N_ELTS(seq_init_tbl); ++i)
		write_seq(lynx, seq_init_tbl[i].index, seq_init_tbl[i].val);
	for (i = 0; i < N_ELTS(grx_init_tbl); ++i)
		write_grx(lynx, grx_init_tbl[i].index, grx_init_tbl[i].val);
	for (i = 0; i < N_ELTS(atr_init_tbl); ++i)
		write_atr(lynx, atr_init_tbl[i].index, atr_init_tbl[i].val);
	for (i = 0; i < N_ELTS(crt_init_tbl); ++i)
		write_crt(lynx, crt_init_tbl[i].index, crt_init_tbl[i].val);

#if 0
	for (i = 0; i < 0x100; i += 0x10) {
		printk("CRT %02X  ", i);
		for (j = 0; j < 0x10; j++)
			printk(" %02x", read_crt(lynx, i+j));
		printk("\n");
	}
	for (i = 0; i < 0x100; i += 0x10) {
		printk("SEQ %02X  ", i);
		for (j = 0; j < 0x10; j++)
			printk(" %02x", read_seq(lynx, i+j));
		printk("\n");
	}
#endif
	/* Initialize the RAM - this should be unecessary on machines
           with a BIOS, but it's needed on the Icebox */
	lynx_ram_init(lynx);

	write_dprw(lynx, 0x1e, 0x0);
	write_vprl(lynx, 0x00, 0x0);

	printk(KERN_DEBUG "lynxfb.c: Found %ldk of video memory.\n",
	       lynx->ramsize / 1024);
	memset(lynx->frame_buffer, 0, lynx->ramsize);

	/* FIXME: poke GPR71 in the way that X expects */
}

static int lynx_get_fix(struct fb_fix_screeninfo *fix, int con,
			 struct fb_info *info)
{
	struct fb_info_lynx *lp = (struct fb_info_lynx *) info;

	*fix = lp->fix;
	return 0;
}

static int lynx_get_var(struct fb_var_screeninfo *var, int con,
			struct fb_info *info)
{

	struct fb_info_lynx *lp = (struct fb_info_lynx *) info;

	*var = lp->var;
	return 0;
}

static int lynx_set_var(struct fb_var_screeninfo *var, int con,
			 struct fb_info *info)
{
	struct fb_info_lynx *lp = (struct fb_info_lynx *) info;
	struct display *disp = (con >= 0)? &fb_display[con] : &lp->disp;

	if (var->xres != 640 || var->yres != 480
	    || var->xres_virtual != 640 || var->yres_virtual != 480
	    || (var->bits_per_pixel != 8 && var->bits_per_pixel != 16)
	    || var->nonstd
	    || (var->vmode & FB_VMODE_MASK) != FB_VMODE_NONINTERLACED)
		return -EINVAL;

	if ((var->activate & FB_ACTIVATE_MASK) == FB_ACTIVATE_NOW &&
	    var->bits_per_pixel != disp->var.bits_per_pixel) {
		lynx_set_bitdepth(lp, disp, con, var->bits_per_pixel);
	}
	
	return 0;
}

static int lynxfb_getcolreg(u_int regno, u_int *red, u_int *green,
			     u_int *blue, u_int *transp, struct fb_info *info)
{
	struct fb_info_lynx *p = (struct fb_info_lynx *) info;

	if (regno > 255)
		return 1;
	*red = (p->palette[regno].red<<8) | p->palette[regno].red;
	*green = (p->palette[regno].green<<8) | p->palette[regno].green;
	*blue = (p->palette[regno].blue<<8) | p->palette[regno].blue;
	*transp = 0;
	return 0;
}

static int lynxfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			     u_int transp, struct fb_info *info)
{
	struct fb_info_lynx *p = (struct fb_info_lynx *) info;

	if (regno > 255)
		return 1;
	red >>= 8;
	green >>= 8;
	blue >>= 8;
	p->palette[regno].red = red;
	p->palette[regno].green = green;
	p->palette[regno].blue = blue;
	/* FIXME: allow mmio */
	outb(regno, 0x3c8);
	udelay(1);
	outb(red, 0x3c9);
	outb(green, 0x3c9);
	outb(blue, 0x3c9);

#ifdef FBCON_HAS_CFB16
	if (regno < 16)
		p->fbcon_cfb16_cmap[regno] = ((red & 0xf8) << 7)
			| ((green & 0xf8) << 2) | ((blue & 0xf8) >> 3);
#endif
#ifdef FBCON_HAS_CFB24
	if (regno < 16)
		p->fbcon_cfb24_cmap[regno] = (red << 16)
			| (green << 8) | (blue);
#endif

	return 0;
}

static int lynx_get_cmap(struct fb_cmap *cmap, int kspc, int con,
			  struct fb_info *info)
{
	if (con == currcon)		/* current console? */
		return fb_get_cmap(cmap, kspc, lynxfb_getcolreg, info);
	if (fb_display[con].cmap.len)	/* non default colormap? */
		fb_copy_cmap(&fb_display[con].cmap, cmap, kspc ? 0 : 2);
	else {
		int size = fb_display[con].var.bits_per_pixel == 16 ? 32 : 256;
		fb_copy_cmap(fb_default_cmap(size), cmap, kspc ? 0 : 2);
	}
	return 0;
}

static int lynx_set_cmap(struct fb_cmap *cmap, int kspc, int con,
			 struct fb_info *info)
{
	int err;
	if (!fb_display[con].cmap.len) {	/* no colormap allocated? */
		int size = fb_display[con].var.bits_per_pixel == 16 ? 32 : 256;
		if ((err = fb_alloc_cmap(&fb_display[con].cmap, size, 0)))
			return err;
	}
	if (con == currcon)			/* current console? */
		return fb_set_cmap(cmap, kspc, lynxfb_setcolreg, info);
	else
		fb_copy_cmap(cmap, &fb_display[con].cmap, kspc ? 0 : 1);
	return 0;
}

static int lynxfb_switch_con(int con, struct fb_info *info)
{
	struct fb_info_lynx *p = (struct fb_info_lynx *) info;
	int new_bpp, old_bpp;

	/* Do we have to save the colormap? */
	if (fb_display[currcon].cmap.len)
		fb_get_cmap(&fb_display[currcon].cmap, 1, lynxfb_getcolreg, info);

	new_bpp = fb_display[con].var.bits_per_pixel;
	old_bpp = fb_display[currcon].var.bits_per_pixel;
	currcon = con;

	if (new_bpp != old_bpp)
		lynx_set_bitdepth(p, &fb_display[con], con, new_bpp);
	
	do_install_cmap(con, info);
	return 0;
}

static int lynxfb_updatevar(int con, struct fb_info *info)
{
	return 0;
}

static void lynxfb_blank(int blank, struct fb_info *info)
{
	struct fb_info_lynx *p = (struct fb_info_lynx *) info;
	int i;

	if (blank) {
		/* get the palette from the chip */
		for (i = 0; i < 256; ++i) {
			outb(i, 0x3c7);
			udelay(1);
			p->palette[i].red = inb(0x3c9);
			p->palette[i].green = inb(0x3c9);
			p->palette[i].blue = inb(0x3c9);
		}
		for (i = 0; i < 256; ++i) {
			outb(i, 0x3c8);
			udelay(1);
			outb(0, 0x3c9);
			outb(0, 0x3c9);
			outb(0, 0x3c9);
		}
	} else {
		for (i = 0; i < 256; ++i) {
			outb(i, 0x3c8);
			udelay(1);
			outb(p->palette[i].red, 0x3c9);
			outb(p->palette[i].green, 0x3c9);
			outb(p->palette[i].blue, 0x3c9);
		}
	}
}

/*
 * Exported functions
 */
static void do_install_cmap(int con, struct fb_info *info)
{
	if (con != currcon)
		return;
	if (fb_display[con].cmap.len)
		fb_set_cmap(&fb_display[con].cmap, 1, lynxfb_setcolreg, info);
	else {
		int size = fb_display[con].var.bits_per_pixel == 16 ? 32 : 256;
		fb_set_cmap(fb_default_cmap(size), 1, lynxfb_setcolreg, info);
	}
}

static void lynx_set_bitdepth(struct fb_info_lynx *p, struct display *disp, int con, int bpp)
{
	int err;
	struct fb_fix_screeninfo* fix = &p->fix;
	struct fb_var_screeninfo* var = &p->var;

	if (bpp == 24) {
		if (con == currcon) {
			write_crt(p, 0x13, 240);	// Set line length (doublewords)
			write_dprw(p, 0x1e,
				   (read_dprw(p, 0x1e) & 0xffcf) | 0x30);
			write_vprl(p, 0x00,
				   (read_vprl(p, 0x00) & 0xfff8ffff) | 0x40000);
		}

		fix->line_length = 640*3;
		fix->visual = FB_VISUAL_TRUECOLOR;

		var->red.offset = 16;
		var->green.offset = 8;
		var->blue.offset = 0;
		var->red.length = var->green.length = var->blue.length = 8;
		
#ifdef FBCON_HAS_CFB24
		disp->dispsw = &fbcon_cfb24;
		disp->dispsw_data = p->fbcon_cfb24_cmap;
#else
		disp->dispsw = &fbcon_dummy;
#endif
	} else if (bpp == 16) {
		if (con == currcon) {
			write_crt(p, 0x13, 160);	// Set line length (doublewords)
			write_dprw(p, 0x1e,
				   (read_dprw(p, 0x1e) & 0xffcf) | 0x10);
			write_vprl(p, 0x00,
				   (read_vprl(p, 0x00) & 0xfff8ffff) | 0x20000);
		}

		fix->line_length = 640*2;
		fix->visual = FB_VISUAL_TRUECOLOR;

		var->red.offset = 10;
		var->green.offset = 5;
		var->blue.offset = 0;
		var->red.length = var->blue.length = 5;
		var->green.length = 6;
		
#ifdef FBCON_HAS_CFB16
		disp->dispsw = &fbcon_cfb16;
		disp->dispsw_data = p->fbcon_cfb16_cmap;
#else
		disp->dispsw = &fbcon_dummy;
#endif
	} else if (bpp == 8) {
		if (con == currcon) {
			write_crt(p, 0x13, 80);		// Set line length (doublewords)
			write_dprw(p, 0x1e,
				   (read_dprw(p, 0x1e) & 0xffcf) | 0x0);
			write_vprl(p, 0x00,
				   (read_vprl(p, 0x00) & 0xfff8ffff) | 0x0);
		}

		fix->line_length = 640;
		fix->visual = FB_VISUAL_PSEUDOCOLOR;		

 		var->red.offset = var->green.offset = var->blue.offset = 0;
		var->red.length = var->green.length = var->blue.length = 8;
		
#ifdef FBCON_HAS_CFB8
		disp->dispsw = &fbcon_cfb8;
#else
		disp->dispsw = &fbcon_dummy;
#endif
	}

	var->bits_per_pixel = bpp;
	disp->line_length = p->fix.line_length;
	disp->visual = fix->visual;
	disp->var = *var;

	if (p->info.changevar)
		(*p->info.changevar)(con);

	if ((err = fb_alloc_cmap(&disp->cmap, 0, 0)))
		return;
	do_install_cmap(con, (struct fb_info *)p);
}
