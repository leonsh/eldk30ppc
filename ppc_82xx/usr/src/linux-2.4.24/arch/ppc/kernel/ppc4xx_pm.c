/*
 *	file: ppc4xx_pm.c
 *
 *	This an attempt to get Power Management going for the IBM 4xx processor.
 *	This was derived from the ppc4xx._setup.c file
 *
 *      Armin Kuster akuster@mvista.com
 *      Jan  2002
 *
 *
 * Copyright 2002 MontaVista Softare Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR   IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT,  INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *	Version 1.0 (02/14/01) - A. Kuster
 *	Initial version	 - moved pm code from ppc4xx_setup.c
 *
 *	1.1 02/21/01 - A. Kuster
 *		minor fixes, init value to 0 & += to &=
 *		added stb03 ifdef for 2nd i2c device
 *
 *	1.2 02/23/01 - A. Kuster
 *		Added 4xx version of apm support
 *
 *	1.3 05/25/02 - Armin
 *	   name change *_driver *_dev
 *
 *	1.4 06/18/02 - Armin
 *	fix IIC config name
 *
 *	1.5 07/24/02 - Armin
 *	fixed ppc4xx_pm_init so it inits now
 *	and am using a default Power Managment bitmap
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <asm/ibm4xx.h>
#include <platforms/ibm_ocp.h>
#include <asm/system.h>

#undef DEBUG

#ifdef DEBUG
#define DBG(x...) printk(x)
#else
#define DBG(x...)
#endif

LIST_HEAD(ocp_devices);

#ifdef CONFIG_APM
/*
 * OCP Power management..
 *
 * This needs to be done centralized, so that we power manage PCI
 * devices in the right order: we should not shut down PCI bridges
 * before we've shut down the devices behind them, and we should
 * not wake up devices before we've woken up the bridge to the
 * device.. Eh?
 *
 * We do not touch devices that don't have a driver that exports
 * a suspend/resume function. That is just too dangerous. If the default
 * PCI suspend/resume functions work for a device, the driver can
 * easily implement them (ie just have a suspend function that calls
 * the pci_set_power_state() function).
 */

static int ocp_pm_save_state_device(struct ocp_dev *dev, u32 state)
{
	int error = 0;
	if (dev) {
		struct ocp_dev *driver = dev->driver;
		if (driver && driver->save_state)
			error = driver->save_state(dev,state);
	}
	return error;
}

static int ocp_pm_suspend_device(struct ocp_dev *dev, u32 state)
{
	int error = 0;
	if (dev) {
		struct ocp_dev *driver = dev->driver;
		if (driver && driver->suspend)
			error = driver->suspend(dev,state);
	}
	return error;
}

static int ocp_pm_resume_device(struct ocp_dev *dev)
{
	int error = 0;
	if (dev) {
		struct ocp_dev *driver = dev->driver;
		if (driver && driver->resume)
			error = driver->resume(dev);
	}
	return error;
}

static int
ocp_pm_callback(struct pm_dev *pm_device, pm_request_t rqst, void *data)
{
	int error = 0;

	switch (rqst) {
	case PM_SAVE_STATE:
		error = ocp_pm_save_state_device((u32)data);
		break;
	case PM_SUSPEND:
		error = ocp_pm_suspend_device((u32)data);
		break;
	case PM_RESUME:
		error = ocp_pm_resume_device((u32)data);
		break;
	default: break;
	}
	return error;
}
/**
 * ocp_register_driver - register a new ocp driver
 * @drv: the driver structure to register
 *
 * Adds the driver structure to the list of registered drivers
 * Returns the number of ocp devices which were claimed by the driver
 * during registration.  The driver remains registered even if the
 * return value is zero.
 */
int
ocp_register_driver(struct ocp_dev *drv)
{
	struct ocp_dev *dev;
	struct ocp_
	list_add_tail(&drv->node, &ocp_devs);
	return 0;
}

EXPORT_SYMBOL(ocp_register_driver);
#endif

/* When bits are "1" then the given clock is
 * stopped therefore saving power 
 *
 * The objected is to turn off all unneccessary 
 * clocks and have the drivers enable/disable
 * them when in use.  We set the default
 * in the <core>.h file
 */

int __init
ppc4xx_pm_init(void)
{
	
	mtdcr(DCRN_CPMFR, 0);

	/* turn off unused hardware to save power */

	printk(KERN_INFO "OCP 4xx power management enabled\n");
	mtdcr(DCRN_CPMFR, DFLT_IBM4xx_PM);

#ifdef CONFIG_APM
	pm_gpio = pm_register(PM_SYS_DEV, 0, ocp_pm_callback);
#endif

	return 0;
}
__initcall(ppc4xx_pm_init);

/* Force/unforce power down for CPM Class 1 devices */

void
ppc4xx_cpm_fr(u32 bits, int val)
{
	unsigned long flags;

	save_flags(flags);
	cli();

	if (val)
		mtdcr(DCRN_CPMFR, mfdcr(DCRN_CPMFR) | bits);
	else
		mtdcr(DCRN_CPMFR, mfdcr(DCRN_CPMFR) & ~bits);

	restore_flags(flags);
}
