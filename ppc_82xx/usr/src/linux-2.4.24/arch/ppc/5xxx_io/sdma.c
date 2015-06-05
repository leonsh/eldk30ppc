/*
 *  arch/ppc/mpc5xxx_io/sdma.c
 *
 *  Driver for MPC5xxx processor SmartComm peripheral controller
 *
 *  Author: Dale Farnsworth <dfarnsworth@mvista.com>
 *
 * 2003 (c) MontaVista, Software, Inc.  This file is licensed under the terms
 * of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */

#include <asm/mpc5xxx.h>
#include <asm/delay.h>
#include <bestcomm_api.h>
#include <mgt5200/sdma.h>

#define RFIFO_DATA	0xf0003184
#define TFIFO_DATA	0xf00031a4


#ifdef CONFIG_BESTCOMM_API

/*
 * returns 1 on success
 * returns 0 if already loaded
 * returns -1 on failure
 */
int
mpc5xxx_sdma_load_tasks_image(void)
{
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5xxx_SDMA;
	int task;
	static int done = 0;

	if (done)
		return 0;

	/* zero some registers before we start */
	for (task=0; task<MPC5xxx_SDMA_MAX_TASKS; task++) {
		out_be16(&sdma->tcr_0 + task, 0);
		out_8(&sdma->IPR0 + task, 0);
	}

	out_8(&sdma->IPR0, 7);	/* always */
	out_8(&sdma->IPR3, 6);	/* eth rx */
	out_8(&sdma->IPR4, 5);	/* eth tx */

	TasksInitAPI((uint8 *) MPC5xxx_MBAR);
	TasksLoadImage((sdma_regs *)MPC5xxx_SDMA);
	done = 1;
	return 1;
}

#else /* !CONFIG_BESTCOMM_API */

struct sdma_task_info {
	u32 start;
	u32 stop;
	u32 var;
	u32 fdt;
	u32 exec_status;	/* used internally by SmartComm engine */
	u32 mvtp;		/* used internally by SmartComm engine */
	u32 context;
	u32 litbase;
};

extern u32 taskTableBytes;
extern u32 taskTableTasks;
extern u32 offsetEntry;
extern struct sdma_task_info taskTable;

static struct sdma_task_info *task_info_base;
static u32 free_addr;
static u32 end_free_addr;

int
mpc5xxx_sdma_init(void)
{
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5xxx_SDMA;
	int sram_addr = MPC5xxx_SRAM;

#if MPC5xxx_SDMA_DEBUG > 0
	memset_io(sram_addr, 0, MPC5xxx_SRAM_SIZE);
#endif

	free_addr = sram_addr;
	end_free_addr = sram_addr + MPC5xxx_SRAM_SIZE;

	out_be32(&sdma->taskBar, sram_addr);
	return 0;
}

u32 *
mpc5xxx_sdma_sram_alloc(int size, int align_shift)
{
	u32 rv;
	u32 align_mask;

	if (free_addr == 0)
		(void) mpc5xxx_sdma_init();

	if (align_shift < 4) align_shift = 4;	/* minimum 16-byte alignment */

	align_mask = (1 << align_shift) - 1;
	rv = (free_addr + align_mask) & ~align_mask;

	if ((rv + size) <= end_free_addr)
		free_addr = rv + size;
	else {

		printk(KERN_WARNING "mpc5xxx_sdma_sram_alloc: out of memory\n");
		rv = 0;
	}

	return (u32 *)rv;
}

/*
 * returns 1 on success
 * returns 0 if already loaded
 * returns -1 on failure
 */
int
mpc5xxx_sdma_load_tasks_image(void)
{
	struct sdma_task_info *task_info;
	u32 task_code_base;
	int i;
	static int done = 0;

	if (done)
		return 0;

	done = 1;

	task_code_base = (u32)mpc5xxx_sdma_sram_alloc(taskTableBytes, 8);

#if MPC5xxx_SDMA_DEBUG > 0
	printk("task_code_base: %08x\n", task_code_base);
#endif

	if (!task_code_base)
		return -1;

	memcpy_toio((u32*)task_code_base, (u32 *)&taskTable, taskTableBytes);

	task_info_base = (struct sdma_task_info *)(task_code_base+offsetEntry);

#if MPC5xxx_SDMA_DEBUG > 0
	printk("task_info_base: %08x\n", (u32)task_info_base);
#endif
	for (i=0; i<taskTableTasks; i++) {
		task_info = task_info_base + i;
		task_info->start += task_code_base;
		task_info->stop += task_code_base;
		task_info->var += task_code_base;
		task_info->fdt += task_code_base;
		task_info->context += task_code_base;
	}

	return 1;
}

u32 *
mpc5xxx_sdma_var_addr(int task)
{
#if MPC5xxx_SDMA_DEBUG > 0
	printk("mpc5xxx_sdma_var_addr[%d]: %08x\n", task, (u32)task_info_base[task].var);
#endif
	return (u32 *)task_info_base[task].var;
}

#endif /* CONFIG_BESTCOMM_API */

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

/*
 * Initialize FEC receive task.
 * Returns task number of FEC receive task.
 * Returns -1 on failure
 */
int
mpc5xxx_sdma_fec_rx_task_setup(int num_bufs, int maxbufsize)
{
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5xxx_SDMA;
	static TaskSetupParamSet_t params;
	int tasknum;

	if (mpc5xxx_sdma_load_tasks_image() < 0)
		return -1;

	params.NumBD = num_bufs;
	params.Size.MaxBuf = maxbufsize;
	params.StartAddrSrc = RFIFO_DATA;
	params.IncrSrc = 0;
	params.SzSrc = 4;
	params.IncrDst = 4;
	params.SzDst = 4;

	tasknum = TaskSetup(TASK_FEC_RX, &params);

	/* clear pending interrupt bits */
	out_be32(&sdma->IntPend, 1<<tasknum);

	return tasknum;
}

/*
 * Initialize FEC transmit task.
 * Returns task number of FEC transmit task.
 * Returns -1 on failure
 */
int
mpc5xxx_sdma_fec_tx_task_setup(int num_bufs)
{
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5xxx_SDMA;
	static TaskSetupParamSet_t params;
	int tasknum;

	if (mpc5xxx_sdma_load_tasks_image() < 0)
		return -1;

	params.NumBD = num_bufs;
	params.IncrSrc = 4;
	params.SzSrc = 4;
	params.StartAddrDst = TFIFO_DATA;
	params.IncrDst = 0;
	params.SzDst = 4;

	tasknum = TaskSetup(TASK_FEC_TX, &params);

	/* clear pending interrupt bits */
	out_be32(&sdma->IntPend, 1<<tasknum);

	return tasknum;
}
