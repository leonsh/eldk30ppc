/*
 * arch/ppc/kernel/mpc5xxx_pci.c
 *
 * PCI Support for MPC5xxx Processors
 *
 * 2003 (c) Wolfgang Denk, DENX Software Engineering, <wd@denx.de>.  This
 * file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of
 * any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/machdep.h>
#include <asm/pci-bridge.h>
#include <asm/mpc5xxx.h>

#define MPC5xxx_PCI_CAR		0xf0000df8
#define CONFIG_PCI_IO_PHYS	0x50000000

static int mpc5xxx_read_config_dword(struct pci_dev *dev,
					int offset, u32* value)
{
	*(volatile u32 *)MPC5xxx_PCI_CAR = (1 << 31) | (dev->devfn << 8) |
		(dev->bus->number << 16) | (offset & 0xfc);
	udelay(10);
	eieio();
	*value = in_le32((volatile u32 *)CONFIG_PCI_IO_PHYS);
	eieio();
	*(volatile u32 *)MPC5xxx_PCI_CAR = 0;
	udelay(10);
	return 0;
}

static int mpc5xxx_write_config_dword(struct pci_dev *dev,
					int offset, u32 value)
{
	*(volatile u32 *)MPC5xxx_PCI_CAR = (1 << 31) | (dev->devfn << 8) |
			(dev->bus->number << 16) | (offset & 0xfc);
	udelay(10);
	eieio();
	out_le32((volatile u32 *)CONFIG_PCI_IO_PHYS, value);
	eieio();
	*(volatile u32 *)MPC5xxx_PCI_CAR = 0;
	udelay(10);
	return 0;
}

#define PCI_READ_VIA_DWORD_OP(size, type, off_mask)			\
int mpc5xxx_read_config_##size##_via_dword(struct pci_dev *dev,		\
					int offset, type val)		\
{									\
	u32 val32;							\
									\
	if (mpc5xxx_read_config_dword(dev, offset & 0xfc, &val32) < 0)	\
		return -1;						\
									\
	*val = (val32 >> ((offset & (int)off_mask) * 8));		\
									\
	return 0;							\
}

#define PCI_WRITE_VIA_DWORD_OP(size, type, off_mask, val_mask)		\
int mpc5xxx_write_config_##size##_via_dword(struct pci_dev *dev,	\
						int offset, type val)	\
{									\
	u32 val32, mask, ldata;						\
									\
	if (mpc5xxx_read_config_dword(dev, offset & 0xfc, &val32) < 0)	\
		return -1;						\
									\
	mask = val_mask;						\
	ldata = (((unsigned long)val) & mask) <<			\
					((offset & (int)off_mask) * 8);	\
	mask <<= ((mask & (int)off_mask) * 8);				\
	val32 = (val32 & ~mask) | ldata;				\
									\
	if (mpc5xxx_write_config_dword(dev, offset & 0xfc, val32) < 0)	\
		return -1;						\
									\
	return 0;							\
}

PCI_READ_VIA_DWORD_OP(byte, u8 *, 0x03)
PCI_READ_VIA_DWORD_OP(word, u16 *, 0x02)
PCI_WRITE_VIA_DWORD_OP(byte, u8, 0x03, 0x000000ff)
PCI_WRITE_VIA_DWORD_OP(word, u16, 0x02, 0x0000ffff)

static struct pci_ops mpc5xxx_pci_ops =
{
	mpc5xxx_read_config_byte_via_dword,
	mpc5xxx_read_config_word_via_dword,
	mpc5xxx_read_config_dword,
	mpc5xxx_write_config_byte_via_dword,
	mpc5xxx_write_config_word_via_dword,
	mpc5xxx_write_config_dword
};

static void __init
mpc5xxx_init_pci_windows(void)
{
	struct mpc5xxx_pci *pci_regs = (struct mpc5xxx_pci *) MPC5xxx_PCI;

	pci_regs->iw0btar = PCI_WINDOW_TRANSLATION(
					MPC5xxx_PROC_PCI_MEM_START,
					MPC5xxx_PROC_PCI_MEM_END,
					MPC5xxx_PCI_MEM_START,
					MPC5xxx_PCI_MEM_END);
	pci_regs->iw1btar = PCI_WINDOW_TRANSLATION(
					MPC5xxx_PROC_PCI_IO_START,
					MPC5xxx_PROC_PCI_IO_END,
					MPC5xxx_PCI_IO_START,
					MPC5xxx_PCI_IO_END);
	pci_regs->iw2btar = 0;

	pci_regs->iwcr = PCI_WINDOW_CONTROL(
				(
					MPC5xxx_INIT_WINDOW_ENABLE |
					MPC5xxx_INIT_WINDOW_MEM |
					MPC5xxx_INIT_WINDOW_READ
				),
				(
					MPC5xxx_INIT_WINDOW_ENABLE |
					MPC5xxx_INIT_WINDOW_IO
				),
				MPC5xxx_INIT_WINDOW_DISABLE);
}

void __init
mpc5xxx_find_bridges(void)
{
	extern int pci_assign_all_busses;
	struct pci_controller *hose;

	mpc5xxx_init_pci_windows();

	hose = pcibios_alloc_controller();

	pci_assign_all_busses = 1;

	if (!hose)
		return;

	hose->first_busno = 0;
	hose->last_busno = 0xff;

	pci_init_resource(&hose->io_resource,
		MPC5xxx_PCI_IO_START,
		MPC5xxx_PCI_IO_END,
		IORESOURCE_IO,
		"PCI host bridge");

	pci_init_resource(&hose->mem_resources[0],
		MPC5xxx_PCI_MEM_START,
		MPC5xxx_PCI_MEM_END,
		IORESOURCE_MEM,
		"PCI host bridge");

	hose->io_space.start = MPC5xxx_PCI_IO_START;
	hose->io_space.end = MPC5xxx_PCI_IO_END;
	hose->mem_space.start = MPC5xxx_PCI_MEM_START;
	hose->mem_space.end = MPC5xxx_PCI_MEM_END;
	hose->io_base_virt = (void *)isa_io_base;

	hose->ops = &mpc5xxx_pci_ops;

}
