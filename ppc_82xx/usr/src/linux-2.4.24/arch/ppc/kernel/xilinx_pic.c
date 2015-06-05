/*
 * xilinx_pic.c
 *
 * Interrupt controller driver for Xilinx Virtex-II Pro.
 *
 * Author: MontaVista Software, Inc. <source@mvista.com>
 *
 * 2002-2003 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/init.h>
#include <linux/irq.h>
#include <asm/io.h>
#include <platforms/xilinx_ocp/xparameters.h>
#include <asm/ibm4xx.h>

/* No one else should require these constants, so define them locally here. */
#define ISR 0			/* Interrupt Status Register */
#define IPR 1			/* Interrupt Pending Register */
#define IER 2			/* Interrupt Enable Register */
#define IAR 3			/* Interrupt Acknowledge Register */
#define SIE 4			/* Set Interrupt Enable bits */
#define CIE 5			/* Clear Interrupt Enable bits */
#define IVR 6			/* Interrupt Vector Register */
#define MER 7			/* Master Enable Register */

#if XPAR_XINTC_USE_DCR == 0
static volatile u32 *intc;
#define intc_out_be32(addr, mask)     out_be32((addr), (mask))
#define intc_in_be32(addr)            in_be32((addr))
#else
#define intc    XPAR_INTC_0_BASEADDR
#define intc_out_be32(addr, mask)     mtdcr((addr), (mask))
#define intc_in_be32(addr)            mfdcr((addr))
#endif

/* Global Variables */
struct hw_interrupt_type *ppc4xx_pic;

/*
 *  Returns:
 *    0         - the interrupt is level sensitive
 *    non-zero  - the interrupt is edge triggered
 */
static inline unsigned long
xilinx_intc_kind_of_intr(unsigned long mask)
{
	return (XPAR_INTC_0_KIND_OF_INTR & mask);
}
	
static void
xilinx_intc_enable(unsigned int irq)
{
	unsigned long mask = (0x80000000 >> (irq & 31));
	// printk(KERN_INFO "enable: %d\n", irq);
	intc_out_be32(intc + SIE, mask);
}

static void
xilinx_intc_disable(unsigned int irq)
{
	unsigned long mask = (0x80000000 >> (irq & 31));
	// printk(KERN_INFO "disable: %d\n", irq);
	intc_out_be32(intc + CIE, mask);
}

static void
xilinx_intc_disable_and_ack(unsigned int irq)
{
	unsigned long mask = (0x80000000 >> (irq & 31));
	// printk(KERN_INFO "disable_and_ack: %d\n", irq);
	intc_out_be32(intc + CIE, mask);
	if (xilinx_intc_kind_of_intr(mask) != 0)
		intc_out_be32(intc + IAR, mask); /* ack edge triggered intr */
}

static void
xilinx_intc_end(unsigned int irq)
{
	unsigned long mask = (0x80000000 >> (irq & 31));

	// printk(KERN_INFO "end: %d\n", irq);
	if (!(irq_desc[irq].status & (IRQ_DISABLED | IRQ_INPROGRESS))) {
		intc_out_be32(intc + SIE, mask);
		/* ack level sensitive intr */
		if(xilinx_intc_kind_of_intr(mask) == 0)
			intc_out_be32(intc + IAR, mask);
	}
}

static struct hw_interrupt_type xilinx_intc = {
	"Xilinx Interrupt Controller",
	NULL,
	NULL,
	xilinx_intc_enable,
	xilinx_intc_disable,
	xilinx_intc_disable_and_ack,
	xilinx_intc_end,
	0
};

int
xilinx_pic_get_irq(struct pt_regs *regs)
{
	int irq;

	/*
	 * NOTE: This function is the one that needs to be improved in
	 * order to handle multiple interrupt controllers.  It currently
	 * is hardcoded to check for interrupts only on the first INTC.
	 */

	/*
	 * The Interrupt Vector Register returns the vector
	 * corresponding to the highest priority (most significant or
	 * lowest numbered bit) interrupt pending (enabled and active).
	 * To convert this to an interrupt number, we just need to shift
	 * it two places to the right.
	 */
	irq = intc_in_be32(intc + IVR);
	if (irq != -1)
		irq = 31 - irq;

	// printk(KERN_INFO "get_irq: %d\n", irq);

	return (irq);
}

void __init
ppc4xx_pic_init(void)
{
#if XPAR_XINTC_USE_DCR == 0
	intc = ioremap(XPAR_INTC_0_BASEADDR, 32);

	printk(KERN_INFO "Xilinx INTC #0 at 0x%08lX mapped to 0x%08lX\n",
	       (unsigned long) XPAR_INTC_0_BASEADDR, (unsigned long) intc);
#else
	printk(KERN_INFO "Xilinx INTC #0 at 0x%08lX (DCR)\n",
	       (unsigned long) XPAR_INTC_0_BASEADDR);
#endif

	/*
	 * Disable all external interrupts until they are
	 * explicity requested.
	 */
	intc_out_be32(intc + IER, 0);

	/* Acknowledge any pending interrupts just in case. */
	intc_out_be32(intc + IAR, ~(u32) 0);

	/* Turn on the Master Enable. */
	intc_out_be32(intc + MER, 0x3UL);

	ppc4xx_pic = &xilinx_intc;
	ppc_md.get_irq = xilinx_pic_get_irq;
}
