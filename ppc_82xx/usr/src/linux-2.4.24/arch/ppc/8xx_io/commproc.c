/*
 * General Purpose functions for the global management of the
 * Communication Processor Module.
 * Copyright (c) 1997 Dan Malek (dmalek@jlc.net)
 *
 * In addition to the individual control of the communication
 * channels, there are a few functions that globally affect the
 * communication processor.
 *
 * Buffer descriptors must be allocated from the dual ported memory
 * space.  The allocator for that is here.  When the communication
 * process is reset, we reclaim the memory available.  There is
 * currently no deallocator for this memory.
 * The amount of space available is platform dependent.  On the
 * MBX, the EPPC software loads additional microcode into the
 * communication processor, and uses some of the DP ram for this
 * purpose.  Current, the first 512 bytes and the last 256 bytes of
 * memory are used.  Right now I am conservative and only use the
 * memory that can never be used for microcode.  If there are
 * applications that require more DP ram, we can expand the boundaries
 * but then we have to be careful of any downloaded microcode.
 */

#include <linux/config.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/param.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/bootmem.h>
#include <linux/irq.h>
#include <asm/mpc8xx.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/8xx_immap.h>
#include <asm/commproc.h>

#include <asm/io.h>

extern int get_pteptr(struct mm_struct *mm, unsigned long addr, pte_t **ptep);

typedef struct	cpm_dpalloc_entry_s {
	u_short				start_addr;
	u_short				size;
	struct	cpm_dpalloc_entry_s *	next_entr;
} cpm_dpalloc_entry_t;

static	void	cpm_dpalloc_init(void);
static	void	cpm_dpalloc_data_init(cpm_dpalloc_entry_t  *);
static	int	cpm_dpalloc_cl(cpm_dpalloc_entry_t *);
static	int	cpm_dpalloc_rem(cpm_dpalloc_entry_t **, cpm_dpalloc_entry_t *);
static	void	cpm_dpalloc_ins(cpm_dpalloc_entry_t **, cpm_dpalloc_entry_t *);

#define CPM_DPALLOC_COUNT	  (CPM_DATAONLY_SIZE / sizeof(cbd_t) + 1)  

/* Dynamic lists of allocated and free DPRAM 
*/
static	cpm_dpalloc_entry_t  *	cpm_dpalloc_free;
static	cpm_dpalloc_entry_t  *	cpm_dpalloc_alloc;
static	cpm_dpalloc_entry_t	cpm_dpalloc_list[CPM_DPALLOC_COUNT];

static	uint	host_buffer;	/* Start of the host buffer */
static	uint	host_end;	/* end + 1 */
cpm8xx_t	*cpmp;		/* Pointer to comm processor space */
				/* Number of pages in the host buffer */
 static	uint	host_page_num = (8
#ifdef CONFIG_SCC1_UART_RX_BDNUM
		+ CONFIG_SCC1_UART_RX_BDNUM * CONFIG_SCC1_UART_RX_BDSIZE
		+ CONFIG_SCC1_UART_TX_BDNUM * CONFIG_SCC1_UART_TX_BDSIZE
#endif
#ifdef CONFIG_SCC2_UART_RX_BDNUM
		+ CONFIG_SCC2_UART_RX_BDNUM * CONFIG_SCC2_UART_RX_BDSIZE
		+ CONFIG_SCC2_UART_TX_BDNUM * CONFIG_SCC2_UART_TX_BDSIZE
#endif
#ifdef CONFIG_SCC3_UART_RX_BDNUM
		+ CONFIG_SCC3_UART_RX_BDNUM * CONFIG_SCC3_UART_RX_BDSIZE
		+ CONFIG_SCC3_UART_TX_BDNUM * CONFIG_SCC3_UART_TX_BDSIZE
#endif
#ifdef CONFIG_SCC4_UART_RX_BDNUM
		+ CONFIG_SCC4_UART_RX_BDNUM * CONFIG_SCC4_UART_RX_BDSIZE
		+ CONFIG_SCC4_UART_TX_BDNUM * CONFIG_SCC4_UART_TX_BDSIZE
#endif
#ifdef CONFIG_SMC1_UART_RX_BDNUM
		+ CONFIG_SMC1_UART_RX_BDNUM * CONFIG_SMC1_UART_RX_BDSIZE
		+ CONFIG_SMC1_UART_TX_BDNUM * CONFIG_SMC1_UART_TX_BDSIZE
#endif
#ifdef CONFIG_SMC2_UART_RX_BDNUM
		+ CONFIG_SMC2_UART_RX_BDNUM * CONFIG_SMC2_UART_RX_BDSIZE
		+ CONFIG_SMC2_UART_TX_BDNUM * CONFIG_SMC2_UART_TX_BDSIZE
#endif
		- 1) / PAGE_SIZE + 1;

/* CPM interrupt vector functions.
 *
 * The cpm_vecs structure is only needed to support the cpm_install_handler()
 * mechanism of installing CPM interrupt handlers.
 */
struct	cpm_action {
	void	(*handler)(void *, struct pt_regs * regs);
	void	*dev_id;
};
static	struct	cpm_action cpm_vecs[CPMVEC_NR];
static	void	cpm_interrupt(int irq, void * dev, struct pt_regs * regs);
static	void	cpm_error_interrupt(int irq, void *dev, struct pt_regs *regs);
static	void	alloc_host_memory(void);

/* Define a table of names to identify CPM interrupt handlers in
 * /proc/interrupts.
 */
const char *cpm_int_name[] =
	{ "error",	"PC4",		"PC5",		"SMC2",
	  "SMC1",	"SPI",		"PC6",		"Timer 4",
	  "",		"PC7",		"PC8",		"PC9",
	  "Timer 3",	"",		"PC10",		"PC11",
	  "I2C",	"RISC Timer",	"Timer 2",	"",
	  "IDMA2",	"IDMA1",	"SDMA error",	"PC12",
	  "PC13",	"Timer 1",	"PC14",		"SCC4",
	  "SCC3",	"SCC2",		"SCC1",		"PC15"
	};

static void
cpm_mask_irq(unsigned int irq)
{
	int cpm_vec = irq - CPM_IRQ_OFFSET;

	((immap_t *)IMAP_ADDR)->im_cpic.cpic_cimr &= ~(1 << cpm_vec);
}

static void
cpm_unmask_irq(unsigned int irq)
{
	int cpm_vec = irq - CPM_IRQ_OFFSET;

	((immap_t *)IMAP_ADDR)->im_cpic.cpic_cimr |= (1 << cpm_vec);
}

static void
cpm_eoi(unsigned int irq)
{
	int cpm_vec = irq - CPM_IRQ_OFFSET;

	((immap_t *)IMAP_ADDR)->im_cpic.cpic_cisr = (1 << cpm_vec);
}

struct hw_interrupt_type cpm_pic = {
	" CPM      ",
	NULL,
	NULL,
	cpm_unmask_irq,
	cpm_mask_irq,
	NULL,
	cpm_eoi
};

void
m8xx_cpm_reset(uint host_page_addr)
{
	volatile immap_t	*imp;
	volatile cpm8xx_t	*commproc;
	pte_t			*pte;
	int			i;

	imp = (immap_t *)IMAP_ADDR;
	commproc = (cpm8xx_t *)&imp->im_cpm;

#ifdef CONFIG_UCODE_PATCH
	/* Perform a reset.
	*/
	commproc->cp_cpcr = (CPM_CR_RST | CPM_CR_FLG);

	/* Wait for it.
	*/
	while (commproc->cp_cpcr & CPM_CR_FLG);

	cpm_load_patch(imp);
#endif

	/* Set SDMA Bus Request priority 5.
	 * On 860T, this also enables FEC priority 6.  I am not sure
	 * this is what we realy want for some applications, but the
	 * manual recommends it.
	 * Bit 25, FAM can also be set to use FEC aggressive mode (860T).
	*/
	imp->im_siu_conf.sc_sdcr = 1;

	/* Reclaim the DP memory for our use.
	*/
	cpm_dpalloc_init();        

	/* Set the host page for allocation.
	*/
	host_buffer = (uint) alloc_bootmem_pages(PAGE_SIZE * host_page_num);
	host_end = host_buffer + PAGE_SIZE * host_page_num;

	/* We need to get these pages early, so I have to do it the
	 * hard way.
	 */
	for (i = 0; i < host_page_num; i++) {
		if (get_pteptr(&init_mm, host_buffer + i * PAGE_SIZE, &pte)) {
			pte_val(*pte) |= _PAGE_NO_CACHE;
			flush_tlb_page(init_mm.mmap, host_buffer + i * PAGE_SIZE);
		} else {
			panic("Huh?  No CPM host page?");
		}
	}

	/* Tell everyone where the comm processor resides.
	*/
	cpmp = (cpm8xx_t *)commproc;
}
/* We used to do this earlier, but have to postpone as long as possible
 * to ensure the kernel VM is now running.
 */
static void
alloc_host_memory()
{
	uint	physaddr;

	/* Set the host page for allocation.
	*/
	host_buffer = (uint)consistent_alloc(GFP_KERNEL, PAGE_SIZE, &physaddr);
	host_end = host_buffer + PAGE_SIZE;
}

/* This is called during init_IRQ.  We used to do it above, but this
 * was too early since init_IRQ was not yet called.
 */
void
cpm_interrupt_init(void)
{
	int i;

	/* Initialize the CPM interrupt controller.
	*/
	((immap_t *)IMAP_ADDR)->im_cpic.cpic_cicr =
	    (CICR_SCD_SCC4 | CICR_SCC_SCC3 | CICR_SCB_SCC2 | CICR_SCA_SCC1) |
		((CPM_INTERRUPT/2) << 13) | CICR_HP_MASK;
	((immap_t *)IMAP_ADDR)->im_cpic.cpic_cimr = 0;

        /* install the CPM interrupt controller routines for the CPM
         * interrupt vectors
         */
        for ( i = CPM_IRQ_OFFSET ; i < CPM_IRQ_OFFSET + NR_CPM_INTS ; i++ )
                irq_desc[i].handler = &cpm_pic;

	/* Set our interrupt handler with the core CPU.
	*/
	if (request_irq(CPM_INTERRUPT, cpm_interrupt, 0, "CPM cascade",
				NULL) != 0)
		panic("Could not allocate CPM IRQ!");

	/* Install our own error handler.
	*/
	if (request_irq(CPM_IRQ_OFFSET + CPMVEC_ERROR, cpm_error_interrupt,
				0, cpm_int_name[CPMVEC_ERROR], NULL) != 0)
		panic("Could not allocate CPM error IRQ!");

	((immap_t *)IMAP_ADDR)->im_cpic.cpic_cicr |= CICR_IEN;
}

/*
 * Get the CPM interrupt vector.
 */
int
cpm_get_irq(struct pt_regs *regs)
{
	int cpm_vec;

	/* Get the vector by setting the ACK bit and then reading
	 * the register.
	 */
	((volatile immap_t *)IMAP_ADDR)->im_cpic.cpic_civr = 1;
	cpm_vec = ((volatile immap_t *)IMAP_ADDR)->im_cpic.cpic_civr;
	cpm_vec >>= 11;

	return cpm_vec;
}

/* CPM interrupt controller cascade interrupt.
*/
static	void
cpm_interrupt(int irq, void * dev, struct pt_regs * regs)
{
	/* This interrupt handler never actually gets called.  It is
	 * installed only to unmask the CPM cascade interrupt in the SIU
	 * and to make the CPM cascade interrupt visible in /proc/interrupts.
 	 */
}

/* The CPM can generate the error interrupt when there is a race condition
 * between generating and masking interrupts.  All we have to do is ACK it
 * and return.  This is a no-op function so we don't need any special
 * tests in the interrupt handler.
 */
static	void
cpm_error_interrupt(int irq, void *dev, struct pt_regs *regs)
{
}

/* A helper function to translate the handler prototype required by
 * request_irq() to the handler prototype required by cpm_install_handler().
 */
static void
cpm_handler_helper(int irq, void *dev_id, struct pt_regs *regs)
{
	int cpm_vec = irq - CPM_IRQ_OFFSET;

	(*cpm_vecs[cpm_vec].handler)(dev_id, regs);
}

/* Install a CPM interrupt handler.
 * This routine accepts a CPM interrupt vector in the range 0 to 31.
 * This routine is retained for backward compatibility.  Rather than using
 * this routine to install a CPM interrupt handler, you can now use
 * request_irq() with an IRQ in the range CPM_IRQ_OFFSET to
 * CPM_IRQ_OFFSET + NR_CPM_INTS - 1 (16 to 47).
 *
 * Notice that the prototype of the interrupt handler function must be
 * different depending on whether you install the handler with
 * request_irq() or cpm_install_handler().
 */
void
cpm_install_handler(int cpm_vec, void (*handler)(void *, struct pt_regs *regs),
		    void *dev_id)
{
	int err;

	/* If null handler, assume we are trying to free the IRQ.
	*/
	if (!handler) {
		free_irq(CPM_IRQ_OFFSET + cpm_vec, dev_id);
		return;
	}

	if (cpm_vecs[cpm_vec].handler != 0)
		printk(KERN_INFO "CPM interrupt %x replacing %x\n",
			(uint)handler, (uint)cpm_vecs[cpm_vec].handler);
	cpm_vecs[cpm_vec].handler = handler;
	cpm_vecs[cpm_vec].dev_id = dev_id;

	if ((err = request_irq(CPM_IRQ_OFFSET + cpm_vec, cpm_handler_helper,
					0, cpm_int_name[cpm_vec], dev_id)))
		printk(KERN_ERR "request_irq() returned %d for CPM vector %d\n",
				err, cpm_vec);
}

/* Free a CPM interrupt handler.
 * This routine accepts a CPM interrupt vector in the range 0 to 31.
 * This routine is retained for backward compatibility.
 */
void
cpm_free_handler(int cpm_vec)
{
	request_irq(CPM_IRQ_OFFSET + cpm_vec, NULL, 0, 0,
		cpm_vecs[cpm_vec].dev_id);

	cpm_vecs[cpm_vec].handler = NULL;
	cpm_vecs[cpm_vec].dev_id = NULL;
}

/* Allocate some memory from the dual ported ram.  We may want to
 * enforce alignment restrictions, but right now everyone is a good
 * citizen.
 */
uint
m8xx_cpm_dpalloc(uint size)
{
	cpm_dpalloc_entry_t   * new_el;
	cpm_dpalloc_entry_t   * p;
	cpm_dpalloc_entry_t   * p1;
	uint			retloc;
	u_short			max;
	unsigned long		flags;
	int			i;

	size = (size + 7) & ~7;
	max = CPM_DATAONLY_SIZE;
	retloc = 0;
	new_el = NULL;

	if (size == 0) goto DONE;

	save_flags(flags);
	cli();

	/* Find free area in DPRAM
	*/
	for (p = cpm_dpalloc_free; p != NULL; p = p->next_entr) {
		if (p->size <= max && p->size >= size) {
			new_el = p;
			max = p->size;
		}
	}

	if (new_el == NULL) goto DONE1;

	/* Insert new element in the list of allocated DPRAM
	*/
	retloc = new_el->start_addr;
	p1 = cpm_dpalloc_list;
	p = NULL;
	i = 0;
	while (i < CPM_DPALLOC_COUNT) {
		if (p1->start_addr == 0 && !p1->size && !p1->next_entr) {
			p = p1;
			break;
		}
		i ++;
		p1 ++;
	}

	if (p == NULL) {
		panic ("m8xx_cpm_dpalloc: INTERNAL ERROR\n");
	}

	p->start_addr = new_el->start_addr;
	p->size = size;

	if (new_el->size > size) {
		new_el->size -= size;
		new_el->start_addr += size;
		cpm_dpalloc_ins(&cpm_dpalloc_alloc, p);
	} else {
		cpm_dpalloc_ins(&cpm_dpalloc_alloc, p);
		i = cpm_dpalloc_rem(&cpm_dpalloc_free, new_el);

		if ( i == -1) {
			panic ("m8xx_cpm_dpalloc: INTERNAL ERROR\n");
		}

		cpm_dpalloc_data_init(new_el);
	}

DONE1:
	restore_flags(flags);
DONE:
	return retloc;
}

int
m8xx_cpm_dpfree(uint start_addr)
{
	cpm_dpalloc_entry_t   * r;
	int			retloc;
	unsigned long		flags;

	retloc = -1;

	if ((start_addr < CPM_DATAONLY_BASE) ||
	    (start_addr > CPM_DATAONLY_SIZE + CPM_DATAONLY_BASE)) {
		goto DONE;
	}

	save_flags(flags);
	cli();

	for (r = cpm_dpalloc_alloc;
	     (r != NULL) && (r->start_addr != start_addr);
	     r = r->next_entr)
		/* EMPTY */ ;

	if (r) {
		retloc = cpm_dpalloc_rem(&cpm_dpalloc_alloc, r);

		if (retloc == -1) {
			panic("m8xx_cpm_dpfree: INTERNAL ERROR\n");
		}
		cpm_dpalloc_ins(&cpm_dpalloc_free, r);
		retloc = cpm_dpalloc_cl(cpm_dpalloc_free);

		if (retloc == -1) {
			panic("m8xx_cpm_dpfree: INTERNAL ERROR\n");
		}
		retloc = 0;
	}

	restore_flags(flags);
DONE:
	return retloc;
}

/* We also own one page of host buffer space for the allocation of
 * UART "fifos" and the like.
 */
uint
m8xx_cpm_hostalloc(uint size)
{
	uint	retloc;

#if 1
	if (host_buffer == 0)
		alloc_host_memory();
#endif

	if ((host_buffer + size) >= host_end)
		return(0);

	retloc = host_buffer;
	host_buffer += size;

	return(retloc);
}

/* Set a baud rate generator.  This needs lots of work.  There are
 * four BRGs, any of which can be wired to any channel.
 * The internal baud rate clock is the system clock divided by 16.
 * This assumes the baudrate is 16x oversampled by the uart.
 */
#define BRG_INT_CLK		(((bd_t *)__res)->bi_intfreq)
#define BRG_UART_CLK		(BRG_INT_CLK/16)
#define BRG_UART_CLK_DIV16	(BRG_UART_CLK/16)

void
m8xx_cpm_setbrg(uint brg, uint rate)
{
	volatile uint	*bp;

	/* This is good enough to get SMCs running.....
	*/
	bp = (uint *)&cpmp->cp_brgc1;
	bp += brg;
	/* The BRG has a 12-bit counter.  For really slow baud rates (or
	 * really fast processors), we may have to further divide by 16.
	 */
	if (((BRG_UART_CLK / rate) - 1) < 4096)
		*bp = (((BRG_UART_CLK / rate) - 1) << 1) | CPM_BRG_EN;
	else
		*bp = (((BRG_UART_CLK_DIV16 / rate) - 1) << 1) |
						CPM_BRG_EN | CPM_BRG_DIV16;
}

/* Initialize dynamic lists
 */
static void
cpm_dpalloc_init(void)
{
	int i;

	cpm_dpalloc_free = cpm_dpalloc_list;
	cpm_dpalloc_free->start_addr = CPM_DATAONLY_BASE;
	cpm_dpalloc_free->size = CPM_DATAONLY_SIZE;
	cpm_dpalloc_free->next_entr = NULL;
	cpm_dpalloc_alloc = NULL;

	for(i = 1; i <= CPM_DPALLOC_COUNT; i++) {
		cpm_dpalloc_data_init (cpm_dpalloc_list + i);
	}
}

/* Initialize element in list of DPRAM
 */
static void
cpm_dpalloc_data_init(cpm_dpalloc_entry_t * ptr)
{
	ptr->start_addr = 0;
	ptr->size = 0;
	ptr->next_entr = NULL;
}

static int
cpm_dpalloc_cl(cpm_dpalloc_entry_t * head)
{
	cpm_dpalloc_entry_t	* curr;
	cpm_dpalloc_entry_t	* next;
	int			  retloc;

	curr = head;
	retloc = -1;

	if (curr) {
		retloc = 0;

		while ((next = curr->next_entr)) {
			if (curr->start_addr + curr->size == next->start_addr) {
				curr->size = curr->size + next->size;
				curr->next_entr = next->next_entr;
				cpm_dpalloc_data_init(next);
			} else {
				curr = next;
			}
		 }
	}

	return retloc;
}

/* Remove element from dynamic list of DPRAM
 */
static int
cpm_dpalloc_rem(cpm_dpalloc_entry_t ** head, cpm_dpalloc_entry_t * ptr)
{
	cpm_dpalloc_entry_t	* prev;
	cpm_dpalloc_entry_t	* curr;
	int			  retloc;

	retloc = -1;
	for (prev = NULL , curr = *head;
	     curr != NULL && ptr != curr;
	     prev = curr , curr = curr->next_entr)
		/* EMPTY */ ;

	if (curr) {
		if (prev) {
			prev->next_entr = curr->next_entr;
		} else {
			*head = curr->next_entr;
		}
		retloc = 0;
	}

	return retloc;
}

/* Insert element in dynamic list of DPRAM
 */
static void
cpm_dpalloc_ins(cpm_dpalloc_entry_t ** head, cpm_dpalloc_entry_t * ptr)
{
	cpm_dpalloc_entry_t	* prev;
	cpm_dpalloc_entry_t	* curr;

	for (prev = NULL , curr = *head;
	     curr != NULL && ptr->start_addr >= curr->start_addr;
	     prev = curr , curr = curr->next_entr)
		/* EMPTY */ ;

	ptr->next_entr = curr;

	if (prev) {
		prev->next_entr = ptr;
	} else {
		*head = ptr;
	}
}
