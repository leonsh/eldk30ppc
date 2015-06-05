/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <linux/config.h>
#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/reboot.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/major.h>
#include <linux/blk.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/ide.h>
#include <linux/irq.h>
#include <linux/seq_file.h>
#include <linux/serial.h>
#include <linux/serialP.h>
#include <linux/serial_reg.h>

#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/time.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/machdep.h>
#include <asm/prom.h>
#include <asm/open_pic.h>
#include <asm/bootinfo.h>
#include <asm/mpc10x.h>
#include <asm/pci-bridge.h>

#include "sl8245.h"

extern struct serial_state rs_table[];

unsigned char __res[sizeof(bd_t)];

/*
 * Define all of the IRQ senses and polarities.
 */
static u_char sl8245_openpic_initsenses[] __initdata = {
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* EPIC IRQ 0 */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* EPIC IRQ 1 */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* EPIC IRQ 2 */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE),	/* DUART CH1  */
};

void __init
sl8245_calibrate_decr(void)
{
	ulong	freq;
 
        freq = 133333333 / 4;

	tb_ticks_per_jiffy = freq / HZ;
	tb_to_us = mulhwu_scale_factor(freq, 1000000);
}

static void __init
sl8245_setup_arch(void)
{
	loops_per_jiffy = 100000000 / HZ;

#ifdef CONFIG_BLK_DEV_INITRD
	if (initrd_start)
		ROOT_DEV = MKDEV(RAMDISK_MAJOR, 0);
	else
#endif
	ROOT_DEV = to_kdev_t(0x00FF);	/* /dev/nfs pseudo device */

	/* Lookup PCI host bridges */
	sl8245_find_bridges();
	
	rs_table[0].port = (ulong) ioremap (rs_table[0].port, PAGE_SIZE);
	rs_table[0].iomem_base = (u8 *) rs_table[0].port;
}

void sl8245_fix_uart (void)
{
	volatile u_char * u = (volatile u_char *) rs_table[0].port;
	u_char lcr_saved, dll_saved, dlm_saved;

	lcr_saved = u[UART_LCR];
	u[UART_LCR] = UART_LCR_DLAB;
	dll_saved = u[UART_DLL];
	dlm_saved = u[UART_DLM];

	open_pic.enable(0);
	open_pic.disable(0);

	u[UART_LCR] = UART_LCR_DLAB;
	u[UART_DLL] = dll_saved;
	u[UART_DLM] = dlm_saved;
	u[UART_LCR] = lcr_saved;
}

static void __init
sl8245_init_IRQ(void)
{
	OpenPIC_InitSenses = sl8245_openpic_initsenses;
	OpenPIC_NumInitSenses = sizeof(sl8245_openpic_initsenses);

	/*
	 * We need to tell openpic_set_sources where things actually are.
	 * mpc10x_common will setup OpenPIC_Addr at ioremap(EUMB phys base +
	 * EPIC offset (0x40000));  The EPIC IRQ Register Address Map -
	 * Interrupt Source Configuration Registers gives these numbers
	 * as offsets starting at 0x50200, we need to adjust occordinly.
	 */

	openpic_set_sources(0, 3, OpenPIC_Addr + 0x10200);
	openpic_set_sources(3, 1, OpenPIC_Addr + 0x11120);
	
	openpic_init (0);
}

static int
sl8245_get_irq(struct pt_regs *regs)
{
        int irq;

	irq = openpic_irq();

	if (irq == OPENPIC_VEC_SPURIOUS)
		irq = -1;

	return irq;
}

static unsigned long __init
sl8245_find_end_of_memory(void)
{
	bd_t *bp = (bd_t *)__res;

	return bp->bi_memsize;
}

static void __init
sl8245_map_io(void)
{
	io_block_mapping(0xfe000000, 0xfe000000, 0x02000000, _PAGE_IO);
}

static void
sl8245_power_off(void)
{
	__cli();
	for(;;);  /* No way to shut power off with software */
	/* NOTREACHED */
}

static void
sl8245_restart(char *cmd)
{
	sl8245_power_off();
	/* NOTREACHED */
}

static void
sl8245_halt(void)
{
	sl8245_power_off();
	/* NOTREACHED */
}

void __init
platform_init(unsigned long r3, unsigned long r4, unsigned long r5,
	      unsigned long r6, unsigned long r7)
{
	parse_bootinfo(find_bootinfo());

	/* ASSUMPTION:  If both r3 (bd_t pointer) and r6 (cmdline pointer)
	 * are non-zero, then we should use the board info from the bd_t
	 * structure and the cmdline pointed to by r6 instead of the
	 * information from birecs, if any.  Otherwise, use the information
	 * from birecs as discovered by the preceeding call to
	 * parse_bootinfo().  This rule should work with both PPCBoot, which
	 * uses a bd_t board info structure, and the kernel boot wrapper,
	 * which uses birecs.
	 */
	if ( r3 && r6) {
		/* copy board info structure */
		memcpy( (void *)__res,(void *)(r3+KERNELBASE), sizeof(bd_t) );
		/* copy command line */
		*(char *)(r7+KERNELBASE) = 0;
		strcpy(cmd_line, (char *)(r6+KERNELBASE));
	}

#ifdef CONFIG_BLK_DEV_INITRD
	/* take care of initrd if we have one */
	if ( r4 )
	{
		initrd_start = r4 + KERNELBASE;
		initrd_end = r5 + KERNELBASE;
	}
#endif /* CONFIG_BLK_DEV_INITRD */

	isa_io_base = MPC10X_MAPB_ISA_IO_BASE;
	isa_mem_base = MPC10X_MAPB_ISA_MEM_BASE;
	pci_dram_offset = MPC10X_MAPB_DRAM_OFFSET;

	ppc_md.setup_arch = sl8245_setup_arch;
	ppc_md.init_IRQ = sl8245_init_IRQ;
	ppc_md.get_irq = sl8245_get_irq;

	ppc_md.restart = sl8245_restart;
	ppc_md.power_off = sl8245_power_off;
	ppc_md.halt = sl8245_halt;

	ppc_md.find_end_of_memory = sl8245_find_end_of_memory;
	ppc_md.setup_io_mappings = sl8245_map_io;

	ppc_md.calibrate_decr = sl8245_calibrate_decr;

	return;
}
