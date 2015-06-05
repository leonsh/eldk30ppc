/*
 *  arch/ppc/5xxx_io/sdma.c
 *
 *  Driver for MPC5xxx processor SmartComm/SmartDMA peripheral controller
 *
 *  Author: Dale Farnsworth <dfarnsworth@mvista.com>
 *
 * 2003 (c) MontaVista, Software, Inc.  This file is licensed under the terms
 * of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */

#include <asm/mpc5xxx.h>
#include <asm/delay.h>

struct sdma_task_info {
	u32 task_start;
	u32 task_end;
	u32 vartab;
	u32 fdt;
	u32 exec_status;	/* used internally by SmartComm engine */
	u32 mvtp;		/* used internally by SmartComm engine */
	u32 csave;
	u32 flags;
};

extern struct sdma_task_info taskTable;

static int task_index;
static struct sdma_task_info *task_info_base;
static u32 free_addr;
static u32 end_free_addr;

int
mpc5xxx_sdma_init()
{
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5xxx_SDMA;
	int sram_addr = MPC5xxx_SRAM;

#if MPC5xxx_SDMA_DEBUG > 0
	memset_io(sram_addr, 0, MPC5xxx_SRAM_SIZE);
#endif

	task_info_base = (struct sdma_task_info *)sram_addr;
	end_free_addr = sram_addr + MPC5xxx_SRAM_SIZE;
	free_addr = (u32)&task_info_base[MPC5xxx_SDMA_MAX_TASKS];

	out_be32(&sdma->taskBar, sram_addr);
	return 0;
}

u32 *
mpc5xxx_sdma_sram_alloc(int size, int align_shift)
{
	u32 rv;
	u32 align_mask;

	if (free_addr == 0)
		(void) mpc5xxx_sdma_init(MPC5xxx_SDMA_MAX_TASKS);

	if (align_shift < 4) align_shift = 4;	/* minimum 16-byte alignment */

	align_mask = (1 << align_shift) - 1;

	switch (align_shift) {
	case 4:
		free_addr = (free_addr + align_mask) & ~align_mask;
		rv = free_addr;
		free_addr += size;
		break;
	case 8:
		end_free_addr -= size;
		end_free_addr &= ~align_mask;
		rv = end_free_addr;
		break;
	default:
		panic("mpc5xxx_sdma_sram_alloc: bad alignment shift %d\n",
				align_shift);
		rv = 0;
		break;
	}
	if (free_addr > end_free_addr) {
		printk(KERN_WARNING "mpc5xxx_sdma_sram_alloc: out of memory\n");
		rv = 0;
	}

	return (u32 *)rv;
}

int
mpc5xxx_sdma_load_task(u32 *task_start)
{
	static u32 previous_src_fdt;
	static u32 previous_fdt;
	struct sdma_task_info *src_task_info;
	struct sdma_task_info *end_task_info;
	struct sdma_task_info task_info;
	u32 task_size;
	u32 src_fdt;
	u32 fdt_flags;
	int fdt_alloc = 0;

	if (free_addr == 0)
		(void) mpc5xxx_sdma_init(MPC5xxx_SDMA_MAX_TASKS);

	if (task_index >= MPC5xxx_SDMA_MAX_TASKS)
		return -1;

	end_task_info = (struct sdma_task_info *) taskTable.task_start;
	for (src_task_info = &taskTable; ; src_task_info++) {
#if MPC5xxx_SDMA_DEBUG > 1
		printk("src_task_info %08x, end_task_info %08x\n",
				(u32)src_task_info, (u32)end_task_info);
#endif
		if (src_task_info >= end_task_info)
			return -1;
		if (src_task_info->task_start == (u32)task_start)
			break;
	}

	task_size = src_task_info->task_end - (u32)task_start + 4;

	task_info.task_start = mpc5xxx_sdma_sram_alloc(task_size, 2);
	if (!task_info.task_start)
		return -1;
	task_info.task_end = task_info.task_start + task_size - 4;
	memcpy_toio(task_info.task_start, task_start, task_size);

	task_info.csave = mpc5xxx_sdma_sram_alloc(MPC5xxx_SDMA_CSAVE_SIZE,
						MPC5xxx_SDMA_CSAVE_ALIGN_SHIFT);
	if (!task_info.csave)
		return -1;

#if MPC5xxx_SDMA_DEBUG > 1
	printk("comparing fdt and last_fdt %08x %08x\n",
		(u32)src_task_info->fdt & ~0x3, (u32)previous_src_fdt);
#endif
	src_fdt = src_task_info->fdt & ~0x3;
	fdt_flags = src_task_info->fdt & 0x3;

	if (previous_fdt && memcmp((u32 *)src_fdt,
			(u32 *)previous_src_fdt, MPC5xxx_SDMA_FDT_SIZE) == 0)
		task_info.fdt = previous_fdt | fdt_flags;
	else {
		task_info.fdt = mpc5xxx_sdma_sram_alloc(MPC5xxx_SDMA_FDT_SIZE,
						MPC5xxx_SDMA_FDT_ALIGN_SHIFT);
		if (!task_info.fdt)
			return -1;
		previous_src_fdt = src_fdt;
		previous_fdt = task_info.fdt;
		task_info.fdt |= fdt_flags;
		memcpy_toio(previous_fdt, (u32 *)src_fdt, MPC5xxx_SDMA_FDT_SIZE);
	}

	task_info.vartab = mpc5xxx_sdma_sram_alloc(MPC5xxx_SDMA_VAR_SIZE,
						MPC5xxx_SDMA_VAR_ALIGN_SHIFT);
	if (!task_info.vartab)
		return -1;
	memcpy_toio(task_info.vartab, (u32 *)src_task_info->vartab,
			MPC5xxx_SDMA_VAR_SIZE);

	task_info.exec_status = src_task_info->exec_status;
	task_info.mvtp = src_task_info->mvtp;
	task_info.flags = src_task_info->flags;

	memcpy_toio(&task_info_base[task_index], &task_info, sizeof task_info);

#if MPC5xxx_SDMA_DEBUG > 1
	printk("loading task %d:\n", task_index);
	printk("task_info_addr %08x, task_addr %08x, end_task_addr %08x\n",
			(u32)&task_info_base[task_index],
			task_info.task_start, task_info.task_end);
	printk("var_addr %08x, fdt_addr %08x, csave_addr %08x\n",
			task_info.vartab,
			task_info.fdt & ~0x3,
			task_info.csave);
#endif

	return task_index++;
}

u32 *
mpc5xxx_sdma_task_addr(int task)
{
	return (u32 *)task_info_base[task].task_start;
}

u32 *
mpc5xxx_sdma_var_addr(int task)
{
	return (u32 *)task_info_base[task].vartab;
}

u32 *
mpc5xxx_sdma_csave_addr(int task)
{
	return (u32 *)task_info_base[task].csave;
}

void
mpc5xxx_sdma_enable_task(int task)
{
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5xxx_SDMA;
	u16 val16;

	val16 = in_be16(&sdma->tcr_0 + task);
	val16 |= MPC5xxx_SDMA_TASK_ENABLE;
	out_be16(&sdma->tcr_0 + task, val16);
}

void
mpc5xxx_sdma_disable_task(int task)
{
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5xxx_SDMA;
	u16 val16;

	val16 = in_be16(&sdma->tcr_0 + task);
	val16 &= ~MPC5xxx_SDMA_TASK_ENABLE;
	out_be16(&sdma->tcr_0 + task, val16);
}
