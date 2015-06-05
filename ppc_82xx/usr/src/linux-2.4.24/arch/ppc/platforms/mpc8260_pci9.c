/*
 * arch/ppc/platforms/mpc8260_pci9.c
 *
 * Workaround for device erratum PCI 9.
 * See Motorola's "XPC826xA Family Device Errata Reference."
 * The erratum applies to all 8260 family Hip4 processors.  It is scheduled 
 * to be fixed in HiP4 Rev C.  Erratum PCI 9 states that a simultaneous PCI 
 * inbound write transaction and PCI outbound read transaction can result in a 
 * bus deadlock.  The suggested workaround is to use the IDMA controller to 
 * perform all reads from PCI configuration, memory, and I/O space.
 *
 * Author:  andy_lowe@mvista.com
 *
 * 2003 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/string.h>

#include <asm/io.h>
#include <asm/pci-bridge.h>
#include <asm/machdep.h>
#include <asm/byteorder.h>
#include <asm/mpc8260.h>
#include <asm/immap_8260.h>
#include <asm/cpm_8260.h>

#ifdef CONFIG_8260_PCI9

#define IDMA_XFER_BUF_SIZE 64	/* size of the IDMA transfer buffer */

/* define a structure for the IDMA dpram usage */
typedef struct idma_dpram_s {
	idma_t pram;				/* IDMA parameter RAM */
	u_char xfer_buf[IDMA_XFER_BUF_SIZE];	/* IDMA transfer buffer */
	idma_bd_t bd;				/* buffer descriptor */
} idma_dpram_t;

/* define offsets relative to start of IDMA dpram */
#define IDMA_XFER_BUF_OFFSET (sizeof(idma_t))
#define IDMA_BD_OFFSET (sizeof(idma_t) + IDMA_XFER_BUF_SIZE)

/* define globals */
static volatile idma_dpram_t *idma_dpram;

/* Exactly one of CONFIG_8260_PCI9_IDMAn must be defined, 
 * where n is 1, 2, 3, or 4.  This selects the IDMA channel used for 
 * the PCI9 workaround.
 */
#ifdef CONFIG_8260_PCI9_IDMA1
#define IDMA_CHAN 0
#define PROFF_IDMA PROFF_IDMA1_BASE
#define IDMA_PAGE CPM_CR_IDMA1_PAGE
#define IDMA_SBLOCK CPM_CR_IDMA1_SBLOCK
#endif
#ifdef CONFIG_8260_PCI9_IDMA2
#define IDMA_CHAN 1
#define PROFF_IDMA PROFF_IDMA2_BASE
#define IDMA_PAGE CPM_CR_IDMA2_PAGE
#define IDMA_SBLOCK CPM_CR_IDMA2_SBLOCK
#endif
#ifdef CONFIG_8260_PCI9_IDMA3
#define IDMA_CHAN 2
#define PROFF_IDMA PROFF_IDMA3_BASE
#define IDMA_PAGE CPM_CR_IDMA3_PAGE
#define IDMA_SBLOCK CPM_CR_IDMA3_SBLOCK
#endif
#ifdef CONFIG_8260_PCI9_IDMA4
#define IDMA_CHAN 3
#define PROFF_IDMA PROFF_IDMA4_BASE
#define IDMA_PAGE CPM_CR_IDMA4_PAGE
#define IDMA_SBLOCK CPM_CR_IDMA4_SBLOCK
#endif

void idma_pci9_init(void)
{
	uint dpram_offset;
	volatile idma_t *pram;
	volatile im_idma_t *idma_reg;
	volatile immap_t *immap = (immap_t *)IMAP_ADDR;

	/* allocate IDMA dpram */
	dpram_offset = m8260_cpm_dpalloc(sizeof(idma_dpram_t), 64);
	idma_dpram = 
		(volatile idma_dpram_t *)&immap->im_dprambase[dpram_offset];

	/* initialize the IDMA parameter RAM */
	memset((void *)idma_dpram, 0, sizeof(idma_dpram_t));
	pram = &idma_dpram->pram;
	pram->ibase = dpram_offset + IDMA_BD_OFFSET;
	pram->dpr_buf = dpram_offset + IDMA_XFER_BUF_OFFSET;
	pram->ss_max = 32;
	pram->dts = 32;

	/* initialize the IDMA_BASE pointer to the IDMA parameter RAM */
	*((ushort *) &immap->im_dprambase[PROFF_IDMA]) = dpram_offset;

	/* initialize the IDMA registers */
	idma_reg = (volatile im_idma_t *) &immap->im_sdma.sdma_idsr1;
	idma_reg[IDMA_CHAN].idmr = 0;		/* mask all IDMA interrupts */
	idma_reg[IDMA_CHAN].idsr = 0xff;	/* clear all event flags */

	printk("Using IDMA%d for MPC8260 device erratum PCI 9 workaround\n",
		IDMA_CHAN + 1);

	return;
}

/* Use the IDMA controller to transfer data from I/O memory to local RAM.
 * The src address must be a physical address suitable for use by the DMA 
 * controller with no translation.  The dst address must be a kernel virtual 
 * address.  The dst address is translated to a physical address via 
 * virt_to_phys().
 * The sinc argument specifies whether or not the source address is incremented
 * by the DMA controller.  The source address is incremented if and only if sinc
 * is non-zero.  The destination address is always incremented since the 
 * destination is always host RAM.
 */
static void 
idma_pci9_read(u8 *dst, u8 *src, int bytes, int unit_size, int sinc)
{
	unsigned long flags;
	volatile idma_t *pram = &idma_dpram->pram;
	volatile idma_bd_t *bd = &idma_dpram->bd;
	volatile immap_t *immap = (immap_t *)IMAP_ADDR;

	local_irq_save(flags);

	/* initialize IDMA parameter RAM for this transfer */
	if (sinc)
		pram->dcm = IDMA_DCM_DMA_WRAP_64 | IDMA_DCM_SINC
			  | IDMA_DCM_DINC | IDMA_DCM_SD_MEM2MEM;
	else
		pram->dcm = IDMA_DCM_DMA_WRAP_64 | IDMA_DCM_DINC 
			  | IDMA_DCM_SD_MEM2MEM;
	pram->ibdptr = pram->ibase;
	pram->sts = unit_size;
	pram->istate = 0;

	/* initialize the buffer descriptor */
	bd->dst = virt_to_phys(dst);
	bd->src = (uint) src;
	bd->len = bytes;
	bd->flags = IDMA_BD_V | IDMA_BD_W | IDMA_BD_I | IDMA_BD_L | IDMA_BD_DGBL
		  | IDMA_BD_DBO_BE | IDMA_BD_SBO_BE | IDMA_BD_SDTB;

	/* issue the START_IDMA command to the CP */
	while (immap->im_cpm.cp_cpcr & CPM_CR_FLG);
	immap->im_cpm.cp_cpcr = mk_cr_cmd(IDMA_PAGE, IDMA_SBLOCK, 0,
					 CPM_CR_START_IDMA) | CPM_CR_FLG;
	while (immap->im_cpm.cp_cpcr & CPM_CR_FLG);

	/* wait for transfer to complete */
	while(bd->flags & IDMA_BD_V);

	local_irq_restore(flags);

	return;
}

/* Same as idma_pci9_read, but 16-bit little-endian byte swapping is performed
 * if the unit_size is 2, and 32-bit little-endian byte swapping is performed if
 * the unit_size is 4.
 */
static void 
idma_pci9_read_le(u8 *dst, u8 *src, int bytes, int unit_size, int sinc)
{
	int i;
	u8 *p;

	idma_pci9_read(dst, src, bytes, unit_size, sinc);
	switch(unit_size) {
		case 2:
			for (i = 0, p = dst; i < bytes; i += 2, p += 2)
				swab16s((u16 *) p);
			break;
		case 4:
			for (i = 0, p = dst; i < bytes; i += 4, p += 4)
				swab32s((u32 *) p);
			break;
		default:
			break;
	}
}

int readb(volatile unsigned char *addr)
{
	u8 val;
	unsigned long pa = iopa((unsigned long) addr);

	idma_pci9_read((u8 *)&val, (u8 *)pa, sizeof(val), sizeof(val), 0);
	return val;
}

int readw(volatile unsigned short *addr)
{
	u16 val;
	unsigned long pa = iopa((unsigned long) addr);

	idma_pci9_read((u8 *)&val, (u8 *)pa, sizeof(val), sizeof(val), 0);
	return swab16(val);
}

unsigned readl(volatile unsigned *addr)
{
	u32 val;
	unsigned long pa = iopa((unsigned long) addr);

	idma_pci9_read((u8 *)&val, (u8 *)pa, sizeof(val), sizeof(val), 0);
	return swab32(val);
}

int inb(unsigned port)
{
	u8 val;
	u8 *addr = (u8 *)(port + _IO_BASE);

	idma_pci9_read((u8 *)&val, (u8 *)addr, sizeof(val), sizeof(val), 0);
	return val;
}

int inw(unsigned port)
{
	u16 val;
	u8 *addr = (u8 *)(port + _IO_BASE);

	idma_pci9_read((u8 *)&val, (u8 *)addr, sizeof(val), sizeof(val), 0);
	return swab16(val);
}

unsigned inl(unsigned port)
{
	u32 val;
	u8 *addr = (u8 *)(port + _IO_BASE);

	idma_pci9_read((u8 *)&val, (u8 *)addr, sizeof(val), sizeof(val), 0);
	return swab32(val);
}

void insb(unsigned port, void *buf, int ns)
{
	u8 *addr = (u8 *)(port + _IO_BASE);

	idma_pci9_read((u8 *)buf, (u8 *)addr, ns*sizeof(u8), sizeof(u8), 0);
}

void insw(unsigned port, void *buf, int ns)
{
	u8 *addr = (u8 *)(port + _IO_BASE);

	idma_pci9_read((u8 *)buf, (u8 *)addr, ns*sizeof(u16), sizeof(u16), 0);
}

void insl(unsigned port, void *buf, int nl)
{
	u8 *addr = (u8 *)(port + _IO_BASE);

	idma_pci9_read((u8 *)buf, (u8 *)addr, nl*sizeof(u32), sizeof(u32), 0);
}

void insw_ns(unsigned port, void *buf, int ns)
{
	u8 *addr = (u8 *)(port + _IO_BASE);

	idma_pci9_read((u8 *)buf, (u8 *)addr, ns*sizeof(u16), sizeof(u16), 0);
}

void insl_ns(unsigned port, void *buf, int nl)
{
	u8 *addr = (u8 *)(port + _IO_BASE);

	idma_pci9_read((u8 *)buf, (u8 *)addr, nl*sizeof(u32), sizeof(u32), 0);
}

void *memcpy_fromio(void *dest, unsigned src, size_t count)
{
	idma_pci9_read((u8 *)dest, (u8 *)src, count, 32, 1);
	return dest;
}

EXPORT_SYMBOL(readb);
EXPORT_SYMBOL(readw);
EXPORT_SYMBOL(readl);
EXPORT_SYMBOL(inb);
EXPORT_SYMBOL(inw);
EXPORT_SYMBOL(inl);
EXPORT_SYMBOL(insb);
EXPORT_SYMBOL(insw);
EXPORT_SYMBOL(insl);
EXPORT_SYMBOL(insw_ns);
EXPORT_SYMBOL(insl_ns);
EXPORT_SYMBOL(memcpy_fromio);

#endif	/* ifdef CONFIG_8260_PCI9 */

/* Indirect PCI routines adapted from arch/ppc/kernel/indirect_pci.c.
 * Copyright (C) 1998 Gabriel Paubert.
 */
#ifndef CONFIG_8260_PCI9
#define cfg_read(val, addr, type, op)	*val = op((type)(addr))
#else
#define cfg_read(val, addr, type, op) \
	idma_pci9_read_le((u8*)(val),(u8*)(addr),sizeof(*(val)),sizeof(*(val)),0)
#endif
#define cfg_write(val, addr, type, op)	op((type *)(addr), (val))

#define INDIRECT_PCI_OP(rw, size, type, op, mask)			 \
static int								 \
indirect_##rw##_config_##size(struct pci_dev *dev, int offset, type val) \
{									 \
	struct pci_controller *hose = dev->sysdata;			 \
	u8 cfg_type = 0;						 \
									 \
	if (ppc_md.pci_exclude_device)					 \
		if (ppc_md.pci_exclude_device(dev->bus->number, dev->devfn)) \
			return PCIBIOS_DEVICE_NOT_FOUND;		 \
									 \
	if (hose->set_cfg_type)					 	 \
		if (dev->bus->number != hose->first_busno)		 \
			cfg_type = 1;					 \
									 \
	out_be32(hose->cfg_addr, 					 \
		 (((offset & 0xfc) | cfg_type) << 24) | (dev->devfn << 16) \
		 | ((dev->bus->number - hose->bus_offset) << 8) | 0x80); \
	cfg_##rw(val, hose->cfg_data + (offset & mask), type, op);	 \
	return PCIBIOS_SUCCESSFUL;    					 \
}

INDIRECT_PCI_OP(read, byte, u8 *, in_8, 3)
INDIRECT_PCI_OP(read, word, u16 *, in_le16, 2)
INDIRECT_PCI_OP(read, dword, u32 *, in_le32, 0)
INDIRECT_PCI_OP(write, byte, u8, out_8, 3)
INDIRECT_PCI_OP(write, word, u16, out_le16, 2)
INDIRECT_PCI_OP(write, dword, u32, out_le32, 0)

static struct pci_ops indirect_pci_ops =
{
	indirect_read_config_byte,
	indirect_read_config_word,
	indirect_read_config_dword,
	indirect_write_config_byte,
	indirect_write_config_word,
	indirect_write_config_dword
};

void
setup_m8260_indirect_pci(struct pci_controller* hose, u32 cfg_addr, u32 cfg_data)
{
	hose->ops = &indirect_pci_ops;
	hose->cfg_addr = (unsigned int *) ioremap(cfg_addr, 4);
	hose->cfg_data = (unsigned char *) ioremap(cfg_data, 4);
}
