/*
 * arch/ppc/kernel/mpc5xxx_pic.c
 *
 * Interupt Controller functions for the Motorola MPC5xxx processors.
 *
 * Author: Dale Farnsworth	<dfarnsworth@mvista.com>
 * Derived from code developed by Kent Borg
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of
 * any kind, whether express or implied.
 */

#include <linux/stddef.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/stddef.h>
#include <linux/delay.h>
#include <linux/irq.h>

#include <asm/io.h>
#include <asm/processor.h>
#include <asm/system.h>
#include <asm/irq.h>
#include <asm/mpc5xxx.h>

static void
mpc5xxx_ic_disable(unsigned int irq)
{
	struct mpc5xxx_intr *intr = (struct mpc5xxx_intr *)MPC5xxx_INTR;
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5xxx_SDMA;
	u32 val;

	if (irq == MPC5xxx_IRQ0) {
		val = in_be32(&intr->ctrl);
		val &= ~(1 << 11);
		out_be32(&intr->ctrl, val);
	}
	else if (irq < MPC5xxx_IRQ1) {
		BUG();
	}
	else if (irq <= MPC5xxx_IRQ3) {
		val = in_be32(&intr->ctrl);
		val &= ~(1 << (10 - (irq - MPC5xxx_IRQ1)));
		out_be32(&intr->ctrl, val);
	}
	else if (irq < MPC5xxx_SDMA_IRQ_BASE) {
		val = in_be32(&intr->main_mask);
		val |= 1 << (16 - (irq - MPC5xxx_MAIN_IRQ_BASE));
		out_be32(&intr->main_mask, val);
	}
	else if (irq < MPC5xxx_PERP_IRQ_BASE) {
		val = in_be32(&sdma->IntMask);
		val |= 1 << (irq - MPC5xxx_SDMA_IRQ_BASE);
		out_be32(&sdma->IntMask, val);
	}
	else {
		val = in_be32(&intr->per_mask);
		val |= 1 << (31 - (irq - MPC5xxx_PERP_IRQ_BASE));
		out_be32(&intr->per_mask, val);
	}
}

static void
mpc5xxx_ic_enable(unsigned int irq)
{
	struct mpc5xxx_intr *intr = (struct mpc5xxx_intr *)MPC5xxx_INTR;
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5xxx_SDMA;
	u32 val;

	if (irq == MPC5xxx_IRQ0) {
		val = in_be32(&intr->ctrl);
		val |= 1 << 11;
		out_be32(&intr->ctrl, val);
	}
	else if (irq < MPC5xxx_IRQ1) {
		BUG();
	}
	else if (irq <= MPC5xxx_IRQ3) {
		val = in_be32(&intr->ctrl);
		val |= 1 << (10 - (irq - MPC5xxx_IRQ1));
		out_be32(&intr->ctrl, val);
	}
	else if (irq < MPC5xxx_SDMA_IRQ_BASE) {
		val = in_be32(&intr->main_mask);
		val &= ~(1 << (16 - (irq - MPC5xxx_MAIN_IRQ_BASE)));
		out_be32(&intr->main_mask, val);
	}
	else if (irq < MPC5xxx_PERP_IRQ_BASE) {
		val = in_be32(&sdma->IntMask);
		val &= ~(1 << (irq - MPC5xxx_SDMA_IRQ_BASE));
		out_be32(&sdma->IntMask, val);
	}
	else {
		val = in_be32(&intr->per_mask);
		val &= ~(1 << (31 - (irq - MPC5xxx_PERP_IRQ_BASE)));
		out_be32(&intr->per_mask, val);
	}
}

static void
mpc5xxx_ic_ack(unsigned int irq)
{
	struct mpc5xxx_intr *intr = (struct mpc5xxx_intr *)MPC5xxx_INTR;
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5xxx_SDMA;
	u32 val;

	/*
	 * Only some irqs are reset here, others in interrupting hardware.
	 */
			
	switch (irq) {
	case MPC5xxx_IRQ0:
		val = in_be32(&intr->ctrl);
		val |= 0x08000000;
		out_be32(&intr->ctrl, val);
		break;
	case MPC5xxx_CCS_IRQ:
		val = in_be32(&intr->enc_status);
		val |= 0x00000400;
		out_be32(&intr->enc_status, val);
		break;
	case MPC5xxx_IRQ1:
		val = in_be32(&intr->ctrl);
		val |= 0x04000000;
		out_be32(&intr->ctrl, val);
		break;
	case MPC5xxx_IRQ2:
		val = in_be32(&intr->ctrl);
		val |= 0x02000000;
		out_be32(&intr->ctrl, val);
		break;
	case MPC5xxx_IRQ3:
		val = in_be32(&intr->ctrl);
		val |= 0x01000000;
		out_be32(&intr->ctrl, val);
		break;
	default:
		if (irq >= MPC5xxx_SDMA_IRQ_BASE
		    && irq < (MPC5xxx_SDMA_IRQ_BASE + MPC5xxx_SDMA_IRQ_NUM)) {
			out_be32(&sdma->IntPend,
				 1 << (irq - MPC5xxx_SDMA_IRQ_BASE));
		}
		break;
	}
}

static void
mpc5xxx_ic_disable_and_ack(unsigned int irq)
{
	mpc5xxx_ic_disable(irq);
	mpc5xxx_ic_ack(irq);
}

static void
mpc5xxx_ic_end(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED | IRQ_INPROGRESS)))
		mpc5xxx_ic_enable(irq);
}

static struct hw_interrupt_type mpc5xxx_ic = {
	"MPC5xxx",
	NULL,				/* startup(irq) */
	NULL,				/* shutdown(irq) */
	mpc5xxx_ic_enable,		/* enable(irq) */
	mpc5xxx_ic_disable,		/* disable(irq) */
	mpc5xxx_ic_disable_and_ack,	/* disable_and_ack(irq) */
	mpc5xxx_ic_end,			/* end(irq) */
	0				/* set_affinity(irq, cpumask) SMP. */
};

void __init
mpc5xxx_init_irq(void)
{
	struct mpc5xxx_intr *intr = (struct mpc5xxx_intr *)MPC5xxx_INTR;
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5xxx_SDMA;
	int i;

	/* Disable all interrupt sources. */
	out_be32(&sdma->IntPend, 0xffffffff);	/* 1 means clear pending */
	out_be32(&sdma->IntMask, 0xffffffff);	/* 1 means disabled */
	out_be32(&intr->per_mask, 0x7ffffc00);	/* 1 means disabled */
	out_be32(&intr->main_mask, 0x00010fff);	/* 1 means disabled */
	out_be32(&intr->ctrl,
			0x0f000000 |	/* clear IRQ 0-3 */
			0x00c00000 |	/* IRQ0: level-sensitive, active low */
			0x00001000 |	/* MEE master external enable */
			0x00000000 |	/* 0 means disable IRQ 0-3 */
			0x00000001);	/* CEb route critical normally */

	/* Zero a bunch of the priority settings.  */
	out_be32(&intr->per_pri1, 0);
	out_be32(&intr->per_pri2, 0);
	out_be32(&intr->per_pri3, 0);
	out_be32(&intr->main_pri1, 0);
	out_be32(&intr->main_pri2, 0);

	/* Initialize irq_desc[i].handler's with mpc5xxx_ic. */
	for (i = 0; i < NR_IRQS; i++)
		irq_desc[i].handler = &mpc5xxx_ic;  
}

int
mpc5xxx_get_irq(struct pt_regs *regs)
{
	struct mpc5xxx_intr *intr = (struct mpc5xxx_intr *)MPC5xxx_INTR;
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5xxx_SDMA;
	u32 status;
	int irq = -1;

	status = in_be32(&intr->enc_status);

	if (status & 0x00000400) {		/* critical */
		irq = (status >> 8) & 0x3;
		if (irq == 2)			/* high priority peripheral */
			goto peripheral;
		irq += MPC5xxx_CRIT_IRQ_BASE;
	}
	else if (status & 0x00200000) {		/* main */
		irq = (status >> 16) & 0x1f;
		if (irq == 4)			/* low priority peripheral */
			goto peripheral;
		irq += MPC5xxx_MAIN_IRQ_BASE;
	}
	else if (status & 0x20000000) {		/* peripheral */
peripheral:
		irq = (status >> 24) & 0x1f;
		if (irq == 0) {			/* bestcomm */
			status = in_be32(&sdma->IntPend);
			irq = ffs(status) + MPC5xxx_SDMA_IRQ_BASE-1;
		}
		else
			irq += MPC5xxx_PERP_IRQ_BASE;
	}

	return irq;
}
