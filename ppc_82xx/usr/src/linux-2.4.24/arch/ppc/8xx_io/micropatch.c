
/* Microcode patches for the CPM as supplied by Motorola.
 * This is the one for IIC/SPI.  There is a newer one that
 * also relocates SMC2, but this would require additional changes
 * to uart.c, so I am holding off on that for a moment.
 */
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/param.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <asm/mpc8xx.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/8xx_immap.h>
#include <asm/commproc.h>

/* Define this to get SMC patches as well.  You need to modify the uart
 * driver as well......
#define USE_SMC_PATCH 1
 */

#ifdef CONFIG_USB_MPC8xx
#define USE_USB_SOF_PATCH
#undef USE_IIC_PATCH
#undef USE_SMC_PATCH
#include "micropatch_usb_8xx.c"
#else
#ifdef CONFIG_MPC850_UCODE_PATCH
#include "micropatch_850.c"
#else
#include "micropatch_8xx.c"
#endif
#endif

/* Load the microcode patch.  This is called early in the CPM initialization
 * with the controller in the reset state.  We enable the processor after
 * we load the patch.
 */
void
cpm_load_patch(volatile immap_t *immr)
{
#ifdef PATCH_DEFINED
	volatile uint		*dp;
	volatile cpm8xx_t	*commproc;
	volatile iic_t		*iip;
	volatile spi_t		*spp;
	int	i;

	commproc = (cpm8xx_t *)&immr->im_cpm;

	/* We work closely with commproc.c.  We know it only allocates
	 * from data only space.
	 * For this particular patch, we only use the bottom 512 bytes
	 * and the upper 256 byte extension.  We will use the space
	 * starting at 1K for the relocated parameters, as the general
	 * CPM allocation won't come from that area.
	 */
	commproc->cp_rccr = 0;

	/* Copy the patch into DPRAM.
	*/
	dp = (uint *)(commproc->cp_dpmem);
	for (i=0; i<(sizeof(patch_2000)/4); i++)
		*dp++ = patch_2000[i];

	dp = (uint *)&(commproc->cp_dpmem[0x0f00]);
	for (i=0; i<(sizeof(patch_2f00)/4); i++)
		*dp++ = patch_2f00[i];

#ifdef USE_USB_SOF_PATCH
#if 0 /* usb patch should not relocate iic */
	iip = (iic_t *)&commproc->cp_dparam[PROFF_IIC];
#define RPBASE 0x0030
	iip->iic_rpbase = RPBASE;

	/* Put SPI above the IIC, also 32-byte aligned.
	*/
	i = (RPBASE + sizeof(iic_t) + 31) & ~31;
	spp = (spi_t *)&commproc->cp_dparam[PROFF_SPI];
	spp->spi_rpbase = i;
#endif

	/* Enable uCode fetches from DPRAM. */
	commproc->cp_rccr = 0x0009;

	printk("USB uCode patch installed\n");
#endif /* USE_USB_SOF_PATCH */

#if defined(USE_SMC_PATCH) || defined(USE_IIC_PATCH)

	iip = (iic_t *)&commproc->cp_dparam[PROFF_IIC];
#define RPBASE 0x0400
	iip->iic_rpbase = RPBASE;

	/* Put SPI above the IIC, also 32-byte aligned.
	*/
	i = (RPBASE + sizeof(iic_t) + 31) & ~31;
	spp = (spi_t *)&commproc->cp_dparam[PROFF_SPI];
	spp->spi_rpbase = i;

#ifdef USE_SMC_PATCH
	dp = (uint *)&(commproc->cp_dpmem[0x0e00]);
	for (i=0; i<(sizeof(patch_2e00)/4); i++)
		*dp++ = patch_2e00[i];

	/* Enable the traps to get to it.
	*/
	commproc->cp_cpmcr1 = 0x8080;
	commproc->cp_cpmcr2 = 0x808a;
	commproc->cp_cpmcr3 = 0x8028;
	commproc->cp_cpmcr4 = 0x802a;

	/* Enable uCode fetches from DPRAM.
	*/
	commproc->cp_rccr = 3;
#endif

#ifdef USE_IIC_PATCH
	/* Enable the traps to get to it.
	*/
	commproc->cp_cpmcr1 = 0x802a;
	commproc->cp_cpmcr2 = 0x8028;
	commproc->cp_cpmcr3 = 0x802e;
	commproc->cp_cpmcr4 = 0x802c;

	/* Enable uCode fetches from DPRAM.
	*/
	commproc->cp_rccr = 1;

	printk("I2C uCode patch installed\n");
#endif

	/* Relocate the IIC and SPI parameter areas.  These have to
	 * aligned on 32-byte boundaries.
	 */
	iip = (iic_t *)&commproc->cp_dparam[PROFF_IIC];
	iip->iic_rpbase = RPBASE;

	/* Put SPI above the IIC, also 32-byte aligned.
	*/
	i = (RPBASE + sizeof(iic_t) + 31) & ~31;
	spp = (spi_t *)&commproc->cp_dparam[PROFF_SPI];
	spp->spi_rpbase = i;

#endif /* USE_SMC_PATCH || USE_IIC_PATCH */
#endif /* PATCH_DEFINED */
}

void
verify_patch(volatile immap_t *immr)
{
#ifdef PATCH_DEFINED
	volatile uint		*dp;
	volatile cpm8xx_t	*commproc;
	int i;

	commproc = (cpm8xx_t *)&immr->im_cpm;

	printk("cp_rccr %x\n", commproc->cp_rccr);
	commproc->cp_rccr = 0;

	dp = (uint *)(commproc->cp_dpmem);
	for (i=0; i<(sizeof(patch_2000)/4); i++)
		if (*dp++ != patch_2000[i]) {
			printk("patch_2000 bad at %d\n", i);
			dp--;
			printk("found 0x%X, wanted 0x%X\n", *dp, patch_2000[i]);
			break;
		}

	dp = (uint *)&(commproc->cp_dpmem[0x0f00]);
	for (i=0; i<(sizeof(patch_2f00)/4); i++)
		if (*dp++ != patch_2f00[i]) {
			printk("patch_2f00 bad at %d\n", i);
			dp--;
			printk("found 0x%X, wanted 0x%X\n", *dp, patch_2f00[i]);
			break;
		}

	commproc->cp_rccr = 0x0009;
#endif /* PATCH_DEFINED */
}

