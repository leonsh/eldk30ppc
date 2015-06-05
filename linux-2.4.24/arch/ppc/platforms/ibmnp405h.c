/*
 *
 *    Copyright 2000-2002 MontaVista Software Inc.
 *      Completed implementation.
 *	Current maintainer
 *      Armin Kuster akuster@mvista.com
 *
 *    Module name: ibmnp405h.c
 *
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
 *      History: 01/02/2002 - armin
 *		initial release
 *
 *	       : 04/07/02 - Armin
 *	         removed eth1 & 2, Duh! ratio not 1:1 its 1:4 
 *
 *	       : 04/18/02 - Armin
 *	         removed mal1 define
 *
 *		: 05/06/02 - Armin
 *		converted to core_ocp[];
 *
 *		: 07/06/02 - Armin
 *		added OCP_IRQ_MUL to define EMAC
 *		and addes PM reg
 *		removed irq_resource
 *		added BL_MAC_ETHx to core_ocp
 *
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/threads.h>
#include <linux/param.h>
#include <linux/string.h>
#include <platforms/ibmnp405h.h>
#include <asm/ocp.h>


struct ocp_def core_ocp[] = {
	{UART, UART0_IO_BASE, UART0_INT, IBM_CPM_UART0},
	{UART, UART1_IO_BASE, UART1_INT, IBM_CPM_UART1},
	{IIC, IIC0_BASE, IIC0_IRQ, IBM_CPM_IIC0},
	{GPIO, GPIO0_BASE, OCP_IRQ_NA, IBM_CPM_GPIO0},
	{PCI, PCIL0_BASE, OCP_IRQ_NA, IBM_CPM_PCI},
	{OPB, OPB0_BASE, OCP_IRQ_NA, IBM_CPM_OPB},
	{EMAC, EMAC0_BASE, BL_MAC_ETH0, IBM_CPM_EMAC0},
	{EMAC, EMAC1_BASE, BL_MAC_ETH1, IBM_CPM_EMAC1},
	{EMAC, EMAC2_BASE, BL_MAC_ETH2, IBM_CPM_EMAC2},
	{EMAC, EMAC3_BASE, BL_MAC_ETH3, IBM_CPM_EMAC3},
	{ZMII, ZMII0_BASE, OCP_IRQ_NA, OCP_CPM_NA},
	{OCP_NULL_TYPE, 0x0, OCP_IRQ_NA, OCP_CPM_NA},

};
