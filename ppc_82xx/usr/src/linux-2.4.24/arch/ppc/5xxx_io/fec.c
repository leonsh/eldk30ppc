/*
 * arch/ppc/5xxx_io/fec.c
 *
 * Driver for the MPC5200 Fast Ethernet Controller
 * Support for MPC5100 FEC has been removed, contact the author if you need it
 *
 * Author: Dale Farnsworth <dfarnsworth@mvista.com>
 *
 * 2003 (c) MontaVista, Software, Inc.  This file is licensed under the terms
 * of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */

static const char *version = "fec.c v0.2\n";

#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/crc32.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/skbuff.h>
#include <asm/delay.h>
#include "fec.h"
#ifdef CONFIG_UBOOT
#include <asm/ppcboot.h>
#endif

#ifdef CONFIG_MPC5100
#undef CONFIG_BESTCOMM_API
#define MPC5100_FIX10HDX
#endif

#ifdef CONFIG_NET_FASTROUTE
#error "Fast Routing on MPC5200 ethernet not supported"
#endif

extern int mpc5xxx_sdma_fec_rx_task_setup(int num_bufs, int maxbufsize);
extern int mpc5xxx_sdma_fec_tx_task_setup(int num_bufs);
extern struct mpc5xxx_fec_bd * mpc5xxx_fec_get_bd_ring(int tasknum);
extern void mpc5xxx_sdma_enable_task(int tasknum);
extern void mpc5xxx_sdma_disable_task(int tasknum);
#ifndef CONFIG_BESTCOMM_API
extern u32 *mpc5xxx_sdma_var_addr(int tasknum);
extern u32 *mpc5xxx_sdma_sram_alloc(int size, int align_shift);
extern u32 scEthernetRecv_TDT;
extern u32 scEthernetXmit_TDT;
extern int mpc5xxx_sdma_load_task(u32 *task_start);
#endif

static struct net_device *mpc5xxx_fec_dev;
static void mpc5xxx_fec_interrupt(int, void *, struct pt_regs *);
static void mpc5xxx_sdma_receive_interrupt(int, void *, struct pt_regs *);
static void mpc5xxx_sdma_transmit_interrupt(int, void *, struct pt_regs *);
static struct net_device_stats *mpc5xxx_fec_get_stats(struct net_device *);
static void mpc5xxx_fec_set_multicast_list(struct net_device *dev);

#ifdef CONFIG_USE_MDIO
static void mpc5xxx_fec_mii(struct net_device *dev);
static int mpc5xxx_fec_ioctl(struct net_device *, struct ifreq *rq, int cmd);
static int mpc5xxx_netdev_ethtool_ioctl(struct net_device *dev, void *useraddr);
#ifdef CONFIG_USE_MDIO_NOT_YET
static void mpc5xxx_mdio_callback(uint regval, struct net_device *dev, uint data);
static int mpc5xxx_mdio_read(struct net_device *dev, int phy_id, int location);
#endif

/* MII processing.  We keep this as simple as possible.  Requests are
 * placed on the list (if there is room).  When the request is finished
 * by the MII, an optional function may be called.
 */
typedef struct mii_list {
	uint    mii_regval;
	void    (*mii_func)(uint val, struct net_device *dev, uint data);
	struct  mii_list *mii_next;
	uint    mii_data;
} mii_list_t;

#define		NMII	20
mii_list_t      mii_cmds[NMII];
mii_list_t      *mii_free;
mii_list_t      *mii_head;
mii_list_t      *mii_tail;

typedef struct mdio_read_data {
	u16 regval;
	struct task_struct *sleeping_task;
} mdio_read_data_t;

static int mii_queue(struct net_device *dev, int request,
		void (*func)(uint, struct net_device *, uint), uint data);

/* Make MII read/write commands for the FEC.
 * */
#define mk_mii_read(REG)	(0x60020000 | ((REG & 0x1f) << 18))
#define mk_mii_write(REG, VAL)	(0x50020000 | ((REG & 0x1f) << 18) | \
							(VAL & 0xffff))
#define mk_mii_end	0

/* Register definitions for the PHY.
*/

#define MII_REG_CR	 0	/* Control Register */
#define MII_REG_SR	 1	/* Status Register */
#define MII_REG_PHYIR1	 2	/* PHY Identification Register 1 */
#define MII_REG_PHYIR2	 3	/* PHY Identification Register 2 */
#define MII_REG_ANAR	 4	/* A-N Advertisement Register */
#define MII_REG_ANLPAR	 5	/* A-N Link Partner Ability Register */
#define MII_REG_ANER	 6	/* A-N Expansion Register */
#define MII_REG_ANNPTR	 7	/* A-N Next Page Transmit Register */
#define MII_REG_ANLPRNPR 8	/* A-N Link Partner Received Next Page Reg. */

/* values for phy_status */

#define PHY_CONF_ANE	0x0001	/* 1 auto-negotiation enabled */
#define PHY_CONF_LOOP	0x0002	/* 1 loopback mode enabled */
#define PHY_CONF_SPMASK	0x00f0	/* mask for speed */
#define PHY_CONF_10HDX	0x0010	/* 10 Mbit half duplex supported */
#define PHY_CONF_10FDX	0x0020	/* 10 Mbit full duplex supported */
#define PHY_CONF_100HDX	0x0040	/* 100 Mbit half duplex supported */
#define PHY_CONF_100FDX	0x0080	/* 100 Mbit full duplex supported */

#define PHY_STAT_LINK	0x0100	/* 1 up - 0 down */
#define PHY_STAT_FAULT	0x0200	/* 1 remote fault */
#define PHY_STAT_ANC	0x0400	/* 1 auto-negotiation complete	*/
#define PHY_STAT_SPMASK	0xf000	/* mask for speed */
#define PHY_STAT_10HDX	0x1000	/* 10 Mbit half duplex selected	*/
#define PHY_STAT_10FDX	0x2000	/* 10 Mbit full duplex selected	*/
#define PHY_STAT_100HDX	0x4000	/* 100 Mbit half duplex selected */
#define PHY_STAT_100FDX	0x8000	/* 100 Mbit full duplex selected */

#endif	/* CONFIG_USE_MDIO */

u8 mpc5xxx_fec_mac_addr[6];
u8 null_mac[6];

static void mpc5xxx_fec_tx_timeout(struct net_device *dev)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;

	priv->stats.tx_errors++;

	if (!priv->tx_full)
		netif_wake_queue(dev);
}

static void
mpc5xxx_fec_set_paddr(struct net_device *dev, u8 *mac)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	struct mpc5xxx_fec *fec = priv->fec;

	out_be32(&fec->paddr1, (mac[0]<<24) | (mac[1]<<16)
			| (mac[2]<<8) | (mac[3]<<0));
	out_be32(&fec->paddr2, (mac[4]<<24) | (mac[5]<<16) | 0x8808);
}

static int
mpc5xxx_fec_set_mac_address(struct net_device *dev, void *addr)
{
	struct sockaddr *sock = (struct sockaddr *)addr;

	mpc5xxx_fec_set_paddr(dev, sock->sa_data);
	return 0;
}

/* This function is called to start or restart the FEC during a link
 * change.  This happens on fifo errors or when switching between half
 * and full duplex.
 */
static void
mpc5xxx_fec_restart(struct net_device *dev, int duplex)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	struct mpc5xxx_fec *fec = priv->fec;
	u32 rcntrl;
	u32 tcntrl;
	int i;

#if MPC5xxx_FEC_DEBUG > 1
	printk("mpc5xxx_fec_restart\n");
#endif
	out_be32(&fec->rfifo_status, in_be32(&fec->rfifo_status) & 0x700000);
	out_be32(&fec->tfifo_status, in_be32(&fec->tfifo_status) & 0x700000);
	out_be32(&fec->reset_cntrl, 0x1000000);

	/* Whack a reset.  We should wait for this. */
	out_be32(&fec->ecntrl, MPC5xxx_FEC_ECNTRL_RESET);
	for (i = 0; i < MPC5xxx_FEC_RESET_DELAY; ++i) {
		if ((in_be32(&fec->ecntrl) & MPC5xxx_FEC_ECNTRL_RESET) == 0)
			break;
		udelay(1);
	}
	if (i == MPC5xxx_FEC_RESET_DELAY)
		printk ("FEC Reset timeout!\n");

	/* Set station address. */
	out_be32(&fec->paddr1, *(u32 *)&dev->dev_addr[0]);
	out_be32(&fec->paddr2,
		((*(u16 *)&dev->dev_addr[4]) << 16) | 0x8808);

	mpc5xxx_fec_set_multicast_list(dev);

	rcntrl = MPC5xxx_FEC_RECV_BUFFER_SIZE << 16;	/* max frame length */
	rcntrl |= MPC5xxx_FEC_RCNTRL_FCE;
#ifdef	CONFIG_USE_MDIO
	rcntrl |= MPC5xxx_FEC_RCNTRL_MII_MODE;
#endif
	if (duplex)
		tcntrl = MPC5xxx_FEC_TCNTRL_FDEN;		/* FD enable */
	else {
		rcntrl |= MPC5xxx_FEC_RCNTRL_DRT;
		tcntrl = 0;
	}
	out_be32(&fec->r_cntrl, rcntrl);
	out_be32(&fec->x_cntrl, tcntrl);

#ifdef CONFIG_USE_MDIO
	/* Set MII speed. */
	out_be32(&fec->mii_speed, priv->phy_speed);
#endif

	priv->full_duplex = duplex;

	/* Clear any outstanding interrupt. */
	out_be32(&fec->ievent, 0xffffffff);	/* clear intr events */

	/* Enable interrupts we wish to service.
	*/
#ifdef CONFIG_USE_MDIO
	out_be32(&fec->imask, 0xf0fe0000);	/* enable all intr but tfint */
#else
	out_be32(&fec->imask, 0xf07e0000);	/* enable all intr but tfint */
#endif

	/* And last, enable the transmit and receive processing.
	*/
	out_be32(&fec->ecntrl, MPC5xxx_FEC_ECNTRL_ETHER_EN);
	out_be32(&fec->r_des_active, 0x01000000);

	/* The tx ring is no longer full. */
	if (priv->tx_full)
	{
		priv->tx_full = 0;
		netif_wake_queue(dev);
	}
}

#ifdef	CONFIG_USE_MDIO
static void
mpc5xxx_fec_mii(struct net_device *dev)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	struct mpc5xxx_fec *fec = priv->fec;
	mii_list_t	*mip;
	uint		mii_reg;

	mii_reg = in_be32(&fec->mii_data);

	if ((mip = mii_head) == NULL) {
		printk("MII and no head!\n");
		return;
	}
#if MPC5xxx_FEC_DEBUG > 4
	printk("mpc5xxx_fec_mii %08x %08x %08x\n",
		mii_reg, (u32)mip->mii_func, mip->mii_data);
#endif

	if (mip->mii_func != NULL)
		(*(mip->mii_func))(mii_reg, dev, mip->mii_data);

	mii_head = mip->mii_next;
	mip->mii_next = mii_free;
	mii_free = mip;

	if ((mip = mii_head) != NULL)
		out_be32(&fec->mii_data, mip->mii_regval);
}

static int
mii_queue(struct net_device *dev, int regval, void (*func)(uint, struct net_device *, uint), uint data)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	struct mpc5xxx_fec *fec = priv->fec;
	unsigned long	flags;
	mii_list_t	*mip;
	int		retval;

#if MPC5xxx_FEC_DEBUG > 4
	printk("mii_queue: %08x %08x %08x\n", regval, (u32)func, data);
#endif

	/* Add PHY address to register command.
	*/
	regval |= priv->phy_addr << 23;

	retval = 0;

	save_flags(flags);
	cli();

	if ((mip = mii_free) != NULL) {
		mii_free = mip->mii_next;
		mip->mii_regval = regval;
		mip->mii_func = func;
		mip->mii_next = NULL;
		mip->mii_data = data;
		if (mii_head) {
			mii_tail->mii_next = mip;
			mii_tail = mip;
		} else {
			mii_head = mii_tail = mip;
			out_be32(&fec->mii_data, regval);
		}
	} else
		retval = 1;

	restore_flags(flags);

	return retval;
}

static void mii_do_cmd(struct net_device *dev, const phy_cmd_t *c)
{
	int k;

	if (!c)
		return;

	for (k = 0; (c+k)->mii_data != mk_mii_end; k++)
		mii_queue(dev, (c+k)->mii_data, (c+k)->funct, 0);
}

static void mii_parse_sr(uint mii_reg, struct net_device *dev, uint data)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	uint s = priv->phy_status;

	s &= ~(PHY_STAT_LINK | PHY_STAT_FAULT | PHY_STAT_ANC);

	if (mii_reg & 0x0004)
		s |= PHY_STAT_LINK;
	if (mii_reg & 0x0010)
		s |= PHY_STAT_FAULT;
	if (mii_reg & 0x0020)
		s |= PHY_STAT_ANC;

	priv->phy_status = s;
#if MPC5xxx_FEC_DEBUG > 4
	printk("mii_parse_sr: mii_reg %08x\n", mii_reg);
#endif
}

static void mii_parse_cr(uint mii_reg, struct net_device *dev, uint data)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	uint s = priv->phy_status;

	s &= ~(PHY_CONF_ANE | PHY_CONF_LOOP);

	if (mii_reg & 0x1000)
		s |= PHY_CONF_ANE;
	if (mii_reg & 0x4000)
		s |= PHY_CONF_LOOP;

	priv->phy_status = s;
}

static void mii_parse_anar(uint mii_reg, struct net_device *dev, uint data)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	uint s = priv->phy_status;

	s &= ~(PHY_CONF_SPMASK);

	if (mii_reg & 0x0020)
		s |= PHY_CONF_10HDX;
	if (mii_reg & 0x0040)
		s |= PHY_CONF_10FDX;
	if (mii_reg & 0x0080)
		s |= PHY_CONF_100HDX;
	if (mii_reg & 0x0100)
		s |= PHY_CONF_100FDX;

	priv->phy_status = s;
}

/* ------------------------------------------------------------------------- */
/* Generic PHY support.  Should work for all PHYs, but does not support link
 * change interrupts.
 */
#ifdef CONFIG_FEC_GENERIC_PHY

static phy_info_t phy_info_generic = {
	0x00000000, /* 0-->match any PHY */
	"GENERIC",

	(const phy_cmd_t []) {  /* config */
		/* advertise only half-duplex capabilities */
		{ mk_mii_write(MII_ADVERTISE, MII_ADVERTISE_HALF),
			mii_parse_anar },

		/* enable auto-negotiation */
		{ mk_mii_write(MII_BMCR, BMCR_ANENABLE), mii_parse_cr },
		{ mk_mii_end, }
	},
	(const phy_cmd_t []) {  /* startup */
		/* restart auto-negotiation */
		{ mk_mii_write(MII_BMCR, (BMCR_ANENABLE | BMCR_ANRESTART)),
			NULL },
		{ mk_mii_end, }
	},
	(const phy_cmd_t []) { /* ack_int */
		/* We don't actually use the ack_int table with a generic
		 * PHY, but putting a reference to mii_parse_sr here keeps
		 * us from getting a compiler warning about unused static
		 * functions in the case where we only compile in generic
		 * PHY support.
		 */
		{ mk_mii_read(MII_BMSR), mii_parse_sr },
		{ mk_mii_end, }
	},
	(const phy_cmd_t []) {  /* shutdown */
		{ mk_mii_end, }
	},
};
#endif	/* CONFIG_FEC_GENERIC_PHY */

/* ------------------------------------------------------------------------- */
/* The Level one LXT971 is used on some of my custom boards		     */

#ifdef CONFIG_FEC_LXT971

/* register definitions for the 971 */

#define MII_LXT971_PCR	16	/* Port Control Register	*/
#define MII_LXT971_SR2	17	/* Status Register 2		*/
#define MII_LXT971_IER	18	/* Interrupt Enable Register	*/
#define MII_LXT971_ISR	19	/* Interrupt Status Register	*/
#define MII_LXT971_LCR	20	/* LED Control Register		*/
#define MII_LXT971_TCR	30	/* Transmit Control Register	*/

static void mii_parse_lxt971_sr2(uint mii_reg, struct net_device *dev, uint data)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	uint s = priv->phy_status;

	s &= ~(PHY_STAT_SPMASK);

	if (mii_reg & 0x4000) {
		if (mii_reg & 0x0200)
			s |= PHY_STAT_100FDX;
		else
			s |= PHY_STAT_100HDX;
	}
	else {
		if (mii_reg & 0x0200)
			s |= PHY_STAT_10FDX;
		else
			s |= PHY_STAT_10HDX;
	}
	if (mii_reg & 0x0008)
		s |= PHY_STAT_FAULT;

	priv->phy_status = s;
}

static phy_info_t phy_info_lxt971 = {
	0x0001378e,
	"LXT971",

	(const phy_cmd_t []) {	/* config */
#ifdef MPC5100_FIX10HDX
		{ mk_mii_write(MII_REG_ANAR, 0x021), NULL }, /* 10 Mbps, HD */
#else
		{ mk_mii_write(MII_REG_ANAR, 0x0A1), NULL }, /* 10/100, HD */
#endif
		{ mk_mii_read(MII_REG_CR), mii_parse_cr },
		{ mk_mii_read(MII_REG_ANAR), mii_parse_anar },
		{ mk_mii_end, }
	},
	(const phy_cmd_t []) {	/* startup - enable interrupts */
		{ mk_mii_write(MII_LXT971_IER, 0x00f2), NULL },
		{ mk_mii_write(MII_REG_CR, 0x1200), NULL }, /* autonegotiate */

		/* Somehow does the 971 tell me that the link is down
		 * the first read after power-up.
		 * read here to get a valid value in ack_int */

		{ mk_mii_read(MII_REG_SR), mii_parse_sr },
		{ mk_mii_end, }
	},
	(const phy_cmd_t []) { /* ack_int */
		/* find out the current status */

		{ mk_mii_read(MII_REG_SR), mii_parse_sr },
		{ mk_mii_read(MII_LXT971_SR2), mii_parse_lxt971_sr2 },

		/* we only need to read ISR to acknowledge */

		{ mk_mii_read(MII_LXT971_ISR), NULL },
		{ mk_mii_end, }
	},
	(const phy_cmd_t []) {	/* shutdown - disable interrupts */
		{ mk_mii_write(MII_LXT971_IER, 0x0000), NULL },
		{ mk_mii_end, }
	},
};

#endif /* CONFIG_FEC_LXT971 */

static phy_info_t *phy_info[] = {

#ifdef CONFIG_FEC_LXT971
	&phy_info_lxt971,
#endif /* CONFIG_FEC_LXT971 */

#ifdef CONFIG_FEC_GENERIC_PHY
	/* Generic PHY support.  This must be the last PHY in the table.
	 * It will be used to support any PHY that doesn't match a previous
	 * entry in the table.
	 */
	&phy_info_generic,
#endif /* CONFIG_FEC_GENERIC_PHY */

	NULL
};

static void mii_display_config(struct net_device *dev)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	uint s = priv->phy_status;

	printk("%s: config: auto-negotiation ", dev->name);

	if (s & PHY_CONF_ANE)
		printk("on");
	else
		printk("off");

	if (s & PHY_CONF_100FDX)
		printk(", 100FDX");
	if (s & PHY_CONF_100HDX)
		printk(", 100HDX");
	if (s & PHY_CONF_10FDX)
		printk(", 10FDX");
	if (s & PHY_CONF_10HDX)
		printk(", 10HDX");
	if (!(s & PHY_CONF_SPMASK))
		printk(", No speed/duplex selected?");

	if (s & PHY_CONF_LOOP)
		printk(", loopback enabled");

	printk(".\n");

	priv->sequence_done = 1;
}

static void mii_queue_config(uint mii_reg, struct net_device *dev, uint data)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;

	priv->phy_task.routine = (void *)mii_display_config;
	priv->phy_task.data = dev;
	schedule_task(&priv->phy_task);
}


phy_cmd_t phy_cmd_config[] = { { mk_mii_read(MII_REG_CR), mii_queue_config },
			       { mk_mii_end, } };


/* Read remainder of PHY ID.
*/
static void
mii_discover_phy3(uint mii_reg, struct net_device *dev, uint data)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	int	i;

	priv->phy_id |= (mii_reg & 0xffff);

	for (i = 0; phy_info[i]; i++) {
		if (phy_info[i]->id == (priv->phy_id >> 4) || !phy_info[i]->id)
			break;
		if (phy_info[i]->id == 0)	/* check generic entry */
			break;
	}

	if (!phy_info[i])
		panic("%s: PHY id 0x%08x is not supported!\n",
			dev->name, priv->phy_id);

	priv->phy = phy_info[i];
	priv->phy_id_done = 1;

	printk("%s: Phy @ 0x%x, type %s (0x%08x)\n",
		dev->name, priv->phy_addr, priv->phy->name, priv->phy_id);
}

/* Scan all of the MII PHY addresses looking for someone to respond
 * with a valid ID.  This usually happens quickly.
 */
static void
mii_discover_phy(uint mii_reg, struct net_device *dev, uint data)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	uint	phytype;

#if MPC5xxx_FEC_DEBUG > 4
	printk("mii_discover_phy\n");
#endif

	if ((phytype = (mii_reg & 0xffff)) != 0xffff) {
		/* Got first part of ID, now get remainder.
		*/
		priv->phy_id = phytype << 16;
		mii_queue(dev, mk_mii_read(MII_REG_PHYIR2), mii_discover_phy3, 0);
	} else {
		priv->phy_addr++;
		if (priv->phy_addr < 32)
			mii_queue(dev, mk_mii_read(MII_REG_PHYIR1),
							mii_discover_phy, 0);
		else
			printk("fec: No PHY device found.\n");
	}
}
#endif	/* CONFIG_USE_MDIO */

static int
mpc5xxx_fec_open(struct net_device *dev)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
#ifndef CONFIG_BESTCOMM_API
	struct mpc5xxx_sram_fec *sram= priv->sram;
	struct mpc5xxx_sdma *sdma = priv->sdma;
#endif
	struct sk_buff *skb;
	int i;
	struct mpc5xxx_rbuf *rbuf;
#if MPC5xxx_FEC_DEBUG > 1
	int firstfew = 4;
#endif

#if MPC5xxx_FEC_DEBUG > 1
	printk("mpc5xxx_fec_open\n");
#endif

	/*
	 * Initialize receive queue
	 */
#ifdef CONFIG_BESTCOMM_API
	fec_init_queue(&priv->r_queue,
		mpc5xxx_fec_get_bd_ring(priv->r_tasknum),
		&priv->rskb[0], MPC5xxx_FEC_RBD_NUM);
#else
	fec_init_queue(&priv->r_queue, (struct mpc5xxx_fec_bd *)&sram->rbd[0],
		&priv->rskb[0], MPC5xxx_FEC_RBD_NUM);
#endif

#if MPC5xxx_FEC_DEBUG > 1
	printk("FEC receive queue: %d entries at %08x\n", MPC5xxx_FEC_RBD_NUM,
		(u32)priv->r_queue.bd_base);
#endif

	for (i=0; i<MPC5xxx_FEC_RBD_NUM; i++) {
		skb = dev_alloc_skb(sizeof *rbuf);
		if (skb == 0)
			goto eagain;
		skb->dev = dev;
		rbuf = (struct mpc5xxx_rbuf *)skb_put(skb, sizeof *rbuf);
#if MPC5xxx_FEC_DEBUG > 0
		memset(rbuf, 0, sizeof *rbuf);
		flush_dcache_range((u32)rbuf, (u32)rbuf + sizeof *rbuf);
#endif
#if MPC5xxx_FEC_DEBUG > 1
		if (firstfew) {
			--firstfew;
			printk("receive skb %08x, rbuf %08x\n",
				(u32)skb, (u32)rbuf);
		}
#endif
		invalidate_dcache_range((u32)rbuf, (u32)rbuf + sizeof *rbuf);

		fec_set_start_skb(&priv->r_queue, skb);
		fec_set_start_data(&priv->r_queue,
				(u32)virt_to_phys((void *)&rbuf->data));
		fec_set_start_status(&priv->r_queue,
					MPC5xxx_FEC_RBD_INIT | sizeof *rbuf);
		fec_next_start(&priv->r_queue);
	}

	/*
	 * Initialize transmit queue
	 */
#ifdef CONFIG_BESTCOMM_API
	fec_init_queue(&priv->t_queue,
		mpc5xxx_fec_get_bd_ring(priv->t_tasknum),
		&priv->tskb[0], MPC5xxx_FEC_TBD_NUM);
#else
	fec_init_queue(&priv->t_queue, (struct mpc5xxx_fec_bd *)&sram->tbd[0],
		&priv->tskb[0], MPC5xxx_FEC_TBD_NUM);
#endif

#if MPC5xxx_FEC_DEBUG > 1
	printk("FEC transmit queue: %d entries at %08x\n", MPC5xxx_FEC_TBD_NUM,
		(u32)priv->t_queue.bd_base);
#endif

	for (i=0; i<MPC5xxx_FEC_TBD_NUM; i++) {
		fec_set_start_data(&priv->t_queue, 0);
		fec_set_start_status(&priv->t_queue, 0);
		fec_next_start(&priv->t_queue);
	}

#ifndef CONFIG_BESTCOMM_API
#if MPC5xxx_FEC_DEBUG > 1
	printk("rbd %08x, tbd %08x, rskb %08x\n",
			(u32)&sram->rbd[0], (u32)&sram->tbd[0],
			(u32)&priv->rskb[0]);
#endif
#endif

	mpc5xxx_fec_set_paddr(dev, dev->dev_addr);

#ifndef CONFIG_BESTCOMM_API
	out_8(&sdma->IPR0, 7);			/* always */
	out_8(&sdma->IPR3, 6);			/* eth rx */
	out_8(&sdma->IPR4, 5);			/* eth tx */

	/* clear pending interrupt bits */
	out_be32(&sdma->IntPend, 1<<priv->r_tasknum);
	out_be32(&sdma->IntPend, 1<<priv->t_tasknum);

	out_be32((u32*)&sram->tbd_base, (u32)sram->tbd);
	out_be32((u32*)&sram->tbd_next, (u32)sram->tbd);
	out_be32((u32*)&sram->rbd_base, (u32)sram->rbd);
	out_be32((u32*)&sram->rbd_next, (u32)sram->rbd);
#endif

#ifdef CONFIG_USE_MDIO
	if (!priv->sequence_done) {
		if (!priv->phy) {
			printk("mpc5xxx_fec_open: PHY not configured\n");
			return -ENODEV;		/* No PHY we understand */
		}

		mii_do_cmd(dev, priv->phy->config);
		mii_do_cmd(dev, phy_cmd_config);  /* display configuration */
		while(!priv->sequence_done)
			schedule();

		mii_do_cmd(dev, priv->phy->startup);
	}
#endif /* CONFIG_USE_MDIO */

	mpc5xxx_sdma_enable_task(priv->r_tasknum);
#ifdef CONFIG_BESTCOMM_API
	mpc5xxx_sdma_enable_task(priv->t_tasknum);
#endif

	netif_start_queue(dev);

	return 0;

eagain:
	printk("mpc5xxx_fec_open: failed\n");
	for (i=0; i<MPC5xxx_FEC_RBD_NUM; i++) {
		skb = fec_finish_skb(&priv->r_queue);
		if (skb == 0)
			break;
		dev_kfree_skb(skb);
	}

	return -EAGAIN;
}

/* This will only be invoked if your driver is _not_ in XOFF state.
 * What this means is that you need not check it, and that this
 * invariant will hold if you make sure that the netif_*_queue()
 * calls are done at the proper times.
 */
static int
mpc5xxx_fec_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	int pad;
	short length;

#if MPC5xxx_FEC_DEBUG > 4
	printk("mpc5xxx_fec_hard_start_xmit:\n");
	printk("dev %08x, priv %08x, skb %08x\n",
			(u32)dev, (u32)priv, (u32)skb);
#endif
#if MPC5xxx_FEC_DEBUG > 0
	if (fec_start_status(&priv->t_queue) & MPC5xxx_FEC_TBD_TFD)
		panic("MPC5xxx transmit queue overrun\n");
#endif

	length = skb->len;
#define MUST_ALIGN_TRANSMIT_DATA
#ifdef	MUST_ALIGN_TRANSMIT_DATA
	pad = (int)skb->data & 0x3;
	if (pad) {
		void *old_data = skb->data;
		skb_push(skb, pad);
		memcpy(skb->data, old_data, length);
		skb_trim(skb, length);
	}
#endif
	/* Zero out up to the minimum length ethernet packet size,
	 * so we don't inadvertently expose sensitive data
	 */ 
	pad = ETH_ZLEN - skb->len;
	if (pad > 0) {
		skb = skb_pad(skb, pad);
		if (skb == 0)
			return 0;
		length += pad;
	}
#if MPC5xxx_FEC_DEBUG > 4
	{
		int i;
		int *p;
		printk("Outgoing data @%08x, length %08x:\n",
			(u32)skb->data, length);
		for (i=0; i<length; i+=16) {
			p = (int *)((int)skb->data + i);
			printk("%08x: %08x %08x %08x %08x\n",
				i, p[0], p[1], p[2], p[3]);
		}
	}
#endif

	flush_dcache_range((u32)skb->data, (u32)skb->data + length);
	fec_set_start_skb(&priv->t_queue, skb);
	fec_set_start_data(&priv->t_queue, virt_to_phys((void *)skb->data));

	spin_lock_irq(&priv->lock);
#ifdef CONFIG_BESTCOMM_API
	fec_set_start_status(&priv->t_queue, MPC5xxx_FEC_TBD_INIT | length);
#else
	fec_set_start_status(&priv->t_queue, MPC5xxx_FEC_TBD_INIT);
	fec_set_start_len(&priv->t_queue, length);
#endif

	dev->trans_start = jiffies;

#ifndef CONFIG_BESTCOMM_API
	mpc5xxx_sdma_enable_task(priv->t_tasknum);
#endif

	fec_next_start(&priv->t_queue);

	if (fec_start_status(&priv->t_queue) & MPC5xxx_FEC_TBD_TFD) {
		priv->tx_full = 1;
		netif_stop_queue(dev);
	}
	spin_unlock_irq(&priv->lock);

	return 0;
}

/* This handles SDMA transmit task interrupts
 */
static void
mpc5xxx_sdma_transmit_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{
	struct net_device *dev = dev_id;
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;

	spin_lock(&priv->lock);

#if MPC5xxx_FEC_DEBUG > 4
	printk("mpc5xxx_sdma_transmit_interrupt:\n");
#endif
	while (!fec_queue_empty(&priv->t_queue) || priv->tx_full) {
		if (fec_finish_status(&priv->t_queue) & MPC5xxx_FEC_TBD_TFD)
			break;
#if MPC5xxx_FEC_DEBUG > 4
		printk("dev %08x, priv %08x\n", (u32)dev, (u32)priv);
#endif
#if MPC5xxx_FEC_DEBUG > 0
		if (fec_finish_data(&priv->t_queue) !=
		    virt_to_phys((void *)(fec_finish_skb(&priv->t_queue)->data)))
			panic("bd->data != &skb->data\n");

		fec_set_finish_status(&priv->t_queue, 0);
		fec_set_finish_data(&priv->t_queue, 0);
#endif
		dev_kfree_skb_irq(fec_finish_skb(&priv->t_queue));
		fec_next_finish(&priv->t_queue);

		priv->tx_full = 0;
	}

	if (netif_queue_stopped(dev) && !priv->tx_full)
		netif_wake_queue(dev);

	spin_unlock(&priv->lock);
}

static void
mpc5xxx_sdma_receive_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{
	struct net_device *dev = dev_id;
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	struct sk_buff *skb;
	struct sk_buff *nskb;
	struct mpc5xxx_rbuf *rbuf;
	struct mpc5xxx_rbuf *nrbuf;
	u32 status;
	int length;

#if MPC5xxx_FEC_DEBUG > 4
	printk("mpc5xxx_sdma_receive_interrupt:\n");
#endif

	for (;;) {
		status = fec_finish_status(&priv->r_queue);
#ifdef CONFIG_BESTCOMM_API
		if (!(status & MPC5xxx_FEC_RBD_RFD))
			break;
#else
		if (status & MPC5xxx_FEC_RBD_EMPTY)
			break;
#endif
		length = status & 0xffff;
		skb = fec_finish_skb(&priv->r_queue);
		rbuf = (struct mpc5xxx_rbuf *)skb->data;
#if MPC5xxx_FEC_DEBUG > 4
		printk("status %08x, skb %08x, rbuf %08x\n",
				status, (u32)skb, (u32)rbuf);
#endif
#if MPC5xxx_FEC_DEBUG > 0
		{
			u32 tmp;
			tmp = fec_finish_data(&priv->r_queue);
			if (tmp != (u32)virt_to_phys((void *)&rbuf->data)) {
				printk("bd->data %08x, &rbuf->data %08x\n",
					tmp, (u32)&rbuf->data);
				panic("MPC5xxx FEC rbd vs rskb mismatch\n");
			}
		}
#endif
		/* allocate replacement skb */
		nskb = dev_alloc_skb(sizeof *nrbuf);
		if (nskb == NULL) {
			printk(KERN_NOTICE
			"%s: Memory squeeze, dropping packet.\n",
				dev->name);
			priv->stats.rx_dropped++;
			nrbuf = (struct mpc5xxx_rbuf *)skb->data;
		}
		else {
			nskb->dev = dev;
			nrbuf = (struct mpc5xxx_rbuf *)skb_put(nskb,
					sizeof *nrbuf);
#if MPC5xxx_FEC_DEBUG > 0
			memset(nrbuf, 0, sizeof *nrbuf);
			flush_dcache_range((u32)nrbuf,
				(u32)nrbuf + sizeof *nrbuf);
#endif
			invalidate_dcache_range((u32)nrbuf,
					(u32)nrbuf + sizeof *nrbuf);
#if MPC5xxx_FEC_DEBUG > 4
			{
				int i;
				printk("Incoming rbuf, length: %08x\n",
						length);
				for (i=0; i<length; i+=16) {
					printk("%08x: %08x %08x %08x %08x\n",
						i,
						*(int *)((int)rbuf + i),
						*(int *)((int)rbuf + i + 4),
						*(int *)((int)rbuf + i + 8),
						*(int *)((int)rbuf + i + 12));
				}
			}
#endif
			skb_trim(skb, length);
			skb->protocol = eth_type_trans(skb, dev);
			netif_rx(skb);
			dev->last_rx = jiffies;
			fec_set_finish_skb(&priv->r_queue, nskb);
		}

		fec_set_finish_data(&priv->r_queue,
				(u32)virt_to_phys((void *)&nrbuf->data));
		fec_set_finish_status(&priv->r_queue,
					MPC5xxx_FEC_RBD_INIT | sizeof *rbuf);
		fec_next_finish(&priv->r_queue);

		if (!nskb)
			break;
	}

	return;
}

static void mpc5xxx_fec_reinit(struct net_device *dev)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	struct mpc5xxx_fec *fec = priv->fec;
#ifndef CONFIG_BESTCOMM_API
	u32 *var_tbl;
#endif
#ifdef CONFIG_MPC5200
	u16 u16_value;
#endif
	int i;
	u32 u32_value;
	static void mpc5xxx_fec_update_stat(struct net_device *);

	netif_stop_queue(dev);
	out_be32(&fec->imask, 0x0); 

	/* Disable the rx and tx queues. */
	mpc5xxx_sdma_disable_task(priv->r_tasknum);
	mpc5xxx_sdma_disable_task(priv->t_tasknum);

	/* Stop FEC */
	out_be32(&fec->ecntrl, in_be32(&fec->ecntrl) & ~0x2);
	
	/* Restart the DMA tasks */
#ifdef CONFIG_BESTCOMM_API
	priv->r_tasknum = mpc5xxx_sdma_fec_rx_task_setup(MPC5xxx_FEC_RBD_NUM,
						MPC5xxx_FEC_RECV_BUFFER_SIZE);
	priv->t_tasknum = mpc5xxx_sdma_fec_tx_task_setup(MPC5xxx_FEC_TBD_NUM);
#else
	priv->r_tasknum = mpc5xxx_sdma_load_task(&scEthernetRecv_TDT);
	priv->t_tasknum = mpc5xxx_sdma_load_task(&scEthernetXmit_TDT);
	var_tbl = mpc5xxx_sdma_var_addr(priv->r_tasknum);
	out_be32(&var_tbl[0], (u32)priv->sram);
	var_tbl = mpc5xxx_sdma_var_addr(priv->t_tasknum);
	out_be32(&var_tbl[0], (u32)priv->sram);
#endif

	/* Reconfigure FEC */
	u32_value = in_be32(&priv->gpio->port_config);
#ifdef CONFIG_USE_MDIO
	u32_value |= 0x00050000;	/* 100MBit with MD */
#else
	u32_value |= 0x00020000;	/* 10MBit with 7-wire */
#endif
	out_be32(&priv->gpio->port_config, u32_value);

	out_be32(&fec->op_pause, 0x00010020);	/* change to 0xffff0020 ??? */
	out_be32(&fec->rfifo_cntrl, 0x0f000000);
	out_be32(&fec->rfifo_alarm, 0x0000030c);
	out_be32(&fec->tfifo_cntrl, 0x0f000000);
	out_be32(&fec->tfifo_alarm, 0x00000100);
	out_be32(&fec->x_wmrk, 0x3);		/* xmit fifo watermark = 256 */
	out_be32(&fec->xmit_fsm, 0x03000000);	/* enable crc generation */
	out_be32(&fec->iaddr1, 0x00000000);	/* No individual filter */
	out_be32(&fec->iaddr2, 0x00000000);	/* No individual filter */

#ifdef CONFIG_MPC5200
	/* Disable COMM Bus Prefetch */
	/* This should be done by the Motorola's BestComm Layer */
	u16_value = in_be16(&priv->sdma->PtdCntrl);
	u16_value |= 1;
	out_be16(&priv->sdma->PtdCntrl, u16_value);
#endif

	mpc5xxx_fec_restart(dev, 0);

	/*
	* Read MIB counters in order to reset them,
	* then zero all the stats fields in memory
	*/
	mpc5xxx_fec_update_stat(dev);

	if (priv->sequence_done) { /* redo the mpc5xxx_fec_open() */
		/* Free rx buffers */
		for (i=0; i<MPC5xxx_FEC_RBD_NUM; i++) {
			dev_kfree_skb(fec_finish_skb(&priv->r_queue));
			fec_next_finish(&priv->r_queue);
		}
		mpc5xxx_fec_open(dev);
	}
	return;
}

static void
mpc5xxx_fec_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device *)dev_id;
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	struct mpc5xxx_fec *fec = priv->fec;
	int ievent;

#if MPC5xxx_FEC_DEBUG > 4
	printk("mpc5xxx_fec_interrupt:\n");
#endif

	ievent = in_be32(&fec->ievent);
	out_be32(&fec->ievent, ievent);		/* clear pending events */

	if (ievent & (MPC5xxx_FEC_IEVENT_RFIFO_ERROR |
		      MPC5xxx_FEC_IEVENT_XFIFO_ERROR)) {
		if (ievent & MPC5xxx_FEC_IEVENT_RFIFO_ERROR)
			printk(KERN_WARNING "MPC5xxx_FEC_IEVENT_RFIFO_ERROR\n");
		if (ievent & MPC5xxx_FEC_IEVENT_XFIFO_ERROR)
			printk(KERN_WARNING "MPC5xxx_FEC_IEVENT_XFIFO_ERROR\n");
		mpc5xxx_fec_reinit(dev);
	}
	else if (ievent & MPC5xxx_FEC_IEVENT_MII) {
#ifdef CONFIG_USE_MDIO
		mpc5xxx_fec_mii(dev);
#else
		printk("%s[%d] %s: unexpected MPC5xxx_FEC_IEVENT_MII\n"
			__FILE__, __LINE__, __FUNCTION__);
#endif /* CONFIG_USE_MDIO */
	}
}

static int
mpc5xxx_fec_close(struct net_device *dev)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	unsigned long timeout;
	int i;

	priv->open_time = 0;
	priv->sequence_done = 0;

	netif_stop_queue(dev);

	/* Wait for rx queue to drain */
	timeout = jiffies + 2*HZ;
	while (!fec_queue_empty(&priv->t_queue) && jiffies < timeout) {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(HZ/10);
	}

	/* Disable the rx and tx queues. */
	mpc5xxx_sdma_disable_task(priv->r_tasknum);
	mpc5xxx_sdma_disable_task(priv->t_tasknum);

	/* Free rx Buffers */
	for (i=0; i<MPC5xxx_FEC_RBD_NUM; i++) {
		dev_kfree_skb(fec_finish_skb(&priv->r_queue));
		fec_next_finish(&priv->r_queue);
	}

	mpc5xxx_fec_get_stats(dev);

	return 0;
}

/*
 * Get the current statistics.
 * This may be called with the card open or closed.
 */
static struct net_device_stats *
mpc5xxx_fec_get_stats(struct net_device *dev)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	struct net_device_stats *stats = &priv->stats;
	struct mpc5xxx_fec *fec = priv->fec;

	stats->rx_bytes = in_be32(&fec->rmon_r_octets);
	stats->rx_packets = in_be32(&fec->rmon_r_packets);
	stats->rx_errors = stats->rx_packets - in_be32(&fec->ieee_r_frame_ok);
	stats->tx_bytes = in_be32(&fec->rmon_t_octets);
	stats->tx_packets = in_be32(&fec->rmon_t_packets);
	stats->tx_errors = stats->tx_packets - (
					in_be32(&fec->ieee_t_frame_ok) +
					in_be32(&fec->rmon_t_col) +
					in_be32(&fec->ieee_t_1col) +
					in_be32(&fec->ieee_t_mcol) +
					in_be32(&fec->ieee_t_def));
	stats->multicast = in_be32(&fec->rmon_r_mc_pkt);
	stats->collisions = in_be32(&fec->rmon_t_col);

	/* detailed rx_errors: */
	stats->rx_length_errors = in_be32(&fec->rmon_r_undersize)
			+ in_be32(&fec->rmon_r_oversize)
			+ in_be32(&fec->rmon_r_frag)
			+ in_be32(&fec->rmon_r_jab);
	stats->rx_over_errors = in_be32(&fec->r_macerr);
	stats->rx_crc_errors = in_be32(&fec->ieee_r_crc);
	stats->rx_frame_errors = in_be32(&fec->ieee_r_align);
	stats->rx_fifo_errors = in_be32(&fec->rmon_r_drop);
	stats->rx_missed_errors = in_be32(&fec->rmon_r_drop);

	/* detailed tx_errors: */
	stats->tx_aborted_errors = 0;
	stats->tx_carrier_errors = in_be32(&fec->ieee_t_cserr);
	stats->tx_fifo_errors = in_be32(&fec->rmon_t_drop);
	stats->tx_heartbeat_errors = in_be32(&fec->ieee_t_sqe);
	stats->tx_window_errors = in_be32(&fec->ieee_t_lcol);

	return stats;
}

static void
mpc5xxx_fec_update_stat(struct net_device *dev)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	struct net_device_stats *stats = &priv->stats;
	struct mpc5xxx_fec *fec = priv->fec;

	out_be32(&fec->mib_control, MPC5xxx_FEC_MIB_DISABLE);
	memset_io(&fec->rmon_t_drop, 0,
			(u32)&fec->reserved10 - (u32)&fec->rmon_t_drop);
	out_be32(&fec->mib_control, 0);
	memset(stats, 0, sizeof *stats);
	mpc5xxx_fec_get_stats(dev);
}

/*
 * Set or clear the multicast filter for this adaptor.
 */
static void
mpc5xxx_fec_set_multicast_list(struct net_device *dev)
{
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;
	struct mpc5xxx_fec *fec = priv->fec;
	u32 u32_value;

	if (dev->flags & IFF_PROMISC) {
		printk("%s: Promiscuous mode enabled.\n", dev->name);
		u32_value = in_be32(&fec->r_cntrl);
		u32_value |= MPC5xxx_FEC_RCNTRL_PROM;
		out_be32(&fec->r_cntrl, u32_value);
	}
	else if (dev->flags & IFF_ALLMULTI) {
		u32_value = in_be32(&fec->r_cntrl);
		u32_value &= ~MPC5xxx_FEC_RCNTRL_PROM;
		out_be32(&fec->r_cntrl, u32_value);
		out_be32(&fec->gaddr1, 0xffffffff);
		out_be32(&fec->gaddr2, 0xffffffff);
	}
	else {
		u32 crc;
		int i;
		struct dev_mc_list *dmi;
		u32 gaddr1 = 0x00000000;
		u32 gaddr2 = 0x00000000;

		dmi = dev->mc_list;
		for (i=0; i<dev->mc_count; i++) {
			crc = ether_crc_le(6, dmi->dmi_addr) >> 26;
			if (crc >= 32)
				gaddr1 |= 1 << (crc-32);
			else
				gaddr2 |= 1 << crc;
			dmi = dmi->next;
		}
		out_be32(&fec->gaddr1, gaddr1);
		out_be32(&fec->gaddr2, gaddr2);
	}
}

#ifdef CONFIG_USE_MDIO

#ifdef CONFIG_USE_MDIO_NOT_YET
static void mpc5xxx_mdio_callback(uint regval, struct net_device *dev, uint data)
{
	mdio_read_data_t* mrd = (mdio_read_data_t *)data;
	mrd->regval = 0xFFFF & regval;
	wake_up_process(mrd->sleeping_task);
}

static int mpc5xxx_mdio_read(struct net_device *dev, int phy_id, int location)
{
	uint retval;
	mdio_read_data_t* mrd = (mdio_read_data_t *)kmalloc(sizeof(*mrd),
			GFP_KERNEL);

	mrd->sleeping_task = current;
	set_current_state(TASK_INTERRUPTIBLE);
	mii_queue(dev, mk_mii_read(location),
		mpc5xxx_mdio_callback, (unsigned int) mrd);
	schedule();

	retval = mrd->regval;

	kfree(mrd);

	return retval;
}
#endif

void mdio_write(struct net_device *dev, int phy_id, int location, int value)
{
	mii_queue(dev, mk_mii_write(location, value), NULL, 0);
}
#endif	/* CONFIG_USE_MDIO */

static int
mpc5xxx_netdev_ethtool_ioctl(struct net_device *dev, void *useraddr)
{
#ifdef CONFIG_USE_MDIO_NOT_YET_XXX
	struct mpc5xxx_fec_priv *private = (struct mpc5xxx_fec_priv *)dev->priv;
#endif
	u32 ethcmd;

	if (copy_from_user(&ethcmd, useraddr, sizeof ethcmd))
		return -EFAULT;

	switch (ethcmd) {

		/* Get driver info */
	case ETHTOOL_GDRVINFO:{
			struct ethtool_drvinfo info = { ETHTOOL_GDRVINFO };
			strncpy(info.driver, "gt64260",
				sizeof info.driver - 1);
			strncpy(info.version, version,
				sizeof info.version - 1);
			if (copy_to_user(useraddr, &info, sizeof info))
				return -EFAULT;
			return 0;
		}
		/* get settings */
#ifdef CONFIG_USE_MDIO_NOT_YET_XXX
	case ETHTOOL_GSET:{
			struct ethtool_cmd ecmd = { ETHTOOL_GSET };
			spin_lock_irq(&private->lock);
			mii_ethtool_gset(&private->mii_if, &ecmd);
			spin_unlock_irq(&private->lock);
			if (copy_to_user(useraddr, &ecmd, sizeof ecmd))
				return -EFAULT;
			return 0;
		}
		/* set settings */
	case ETHTOOL_SSET:{
			int r;
			struct ethtool_cmd ecmd;
			if (copy_from_user(&ecmd, useraddr, sizeof ecmd))
				return -EFAULT;
			spin_lock_irq(&private->lock);
			r = mii_ethtool_sset(&private->mii_if, &ecmd);
			spin_unlock_irq(&private->lock);
			return r;
		}
		/* restart autonegotiation */
	case ETHTOOL_NWAY_RST:{
			return mii_nway_restart(&private->mii_if);
		}
		/* get link status */
	case ETHTOOL_GLINK:{
			struct ethtool_value edata = { ETHTOOL_GLINK };
			edata.data = mii_link_ok(&private->mii_if);
			if (copy_to_user(useraddr, &edata, sizeof edata))
				return -EFAULT;
			return 0;
		}
#endif
		/* get message-level */
	case ETHTOOL_GMSGLVL:{
			struct ethtool_value edata = { ETHTOOL_GMSGLVL };
			edata.data = 0;	/* XXX */
			if (copy_to_user(useraddr, &edata, sizeof edata))
				return -EFAULT;
			return 0;
		}
		/* set message-level */
	case ETHTOOL_SMSGLVL:{
			struct ethtool_value edata;
			if (copy_from_user(&edata, useraddr, sizeof edata))
				return -EFAULT;
/* debug = edata.data; *//* XXX */
			return 0;
		}
	}
	return -EOPNOTSUPP;
}

static int
mpc5xxx_fec_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
#ifdef CONFIG_USE_MDIO_NOT_YET_XXX
	struct mii_ioctl_data *data = (struct mii_ioctl_data *) &rq->ifr_data;
	int phy = dev->base_addr & 0x1f;
#endif
	int retval;

	switch (cmd) {
	case SIOCETHTOOL:
		retval = mpc5xxx_netdev_ethtool_ioctl(
					dev, (void *) rq->ifr_data);
		break;

#ifdef CONFIG_USE_MDIO_NOT_YET_XXX
	case SIOCGMIIPHY:	/* Get address of MII PHY in use. */
	case SIOCDEVPRIVATE:	/* for binary compat, remove in 2.5 */
		data->phy_id = phy;
		/* Fall through */

	case SIOCGMIIREG:	/* Read MII PHY register. */
	case SIOCDEVPRIVATE + 1:	/* for binary compat, remove in 2.5 */
		data->val_out =
			mpc5xxx_mdio_read(dev, data->phy_id&0x1f,
				data->reg_num&0x1f);
		retval = 0;
		break;

	case SIOCSMIIREG:	/* Write MII PHY register. */
	case SIOCDEVPRIVATE + 2:	/* for binary compat, remove in 2.5 */
		if (!capable(CAP_NET_ADMIN)) {
			retval = -EPERM;
		} else {
			mdio_write(dev, data->phy_id & 0x1f,
				data->reg_num & 0x1f, data->val_in);
			retval = 0;
		}
		break;
#endif

	default:
		retval = -EOPNOTSUPP;
		break;
	}
	return retval;
}

static void __init
mpc5xxx_fec_str2mac(char *str, unsigned char *mac)
{
	int i;
	u64 val64;

	val64 = simple_strtoull(str, NULL, 16);

	for (i = 0; i < 6; i++)
		mac[5-i] = val64 >> (i*8);
}

static int __init
mpc5xxx_fec_mac_setup(char *mac_address)
{
	mpc5xxx_fec_str2mac(mac_address, mpc5xxx_fec_mac_addr);
	return 0;
}

__setup("mpc5xxx_mac=", mpc5xxx_fec_mac_setup);

static int __init
mpc5xxx_fec_init(void)
{
	struct mpc5xxx_fec *fec;
	struct net_device *dev;
	struct mpc5xxx_fec_priv *priv;
	int i;
#ifndef CONFIG_BESTCOMM_API
	struct mpc5xxx_sram_fec *sram;
	u32 *var_tbl;
#endif
#ifdef CONFIG_MPC5200
	u16 u16_value;
#endif
	u32 u32_value;
#ifdef CONFIG_UBOOT
	extern unsigned char __res[];
	bd_t *bd = (bd_t *)__res;
#define MPC5xxx_IPBFREQ bd->bi_ipbfreq
#else
#define MPC5xxx_IPBFREQ CONFIG_PPC_5xxx_IPBFREQ
#endif

#if MPC5xxx_FEC_DEBUG > 1
	printk("mpc5xxx_fec_init\n");
#endif

	dev = init_etherdev(NULL, sizeof *priv);
	if (!dev)
		return -EIO;

	mpc5xxx_fec_dev = dev;
	priv = (struct mpc5xxx_fec_priv *)dev->priv;
#if MPC5xxx_FEC_DEBUG > 1
	printk("fec_priv %08x\n", (u32)priv);
#endif
	priv->fec = fec = (struct mpc5xxx_fec *)MPC5xxx_FEC;
	priv->gpio = (struct mpc5xxx_gpio *)MPC5xxx_GPIO;
	priv->sdma = (struct mpc5xxx_sdma *)MPC5xxx_SDMA;
	spin_lock_init(&priv->lock);

	dev->open		= mpc5xxx_fec_open;
	dev->stop		= mpc5xxx_fec_close;
	dev->hard_start_xmit	= mpc5xxx_fec_hard_start_xmit;
	dev->do_ioctl		= mpc5xxx_fec_ioctl;
	dev->get_stats		= mpc5xxx_fec_get_stats;
	dev->set_mac_address	= mpc5xxx_fec_set_mac_address;
	dev->set_multicast_list = mpc5xxx_fec_set_multicast_list;

	dev->tx_timeout		= mpc5xxx_fec_tx_timeout;
	dev->watchdog_timeo	= MPC5xxx_FEC_WATCHDOG_TIMEOUT; 
	dev->flags &= ~IFF_RUNNING;

#ifdef CONFIG_NET_FASTROUTE
	dev->accept_fastpath = mpc5xxx_fec_accept_fastpath;
#endif
	if (memcmp(mpc5xxx_fec_mac_addr, null_mac, 6) != 0)
		memcpy(dev->dev_addr, mpc5xxx_fec_mac_addr, 6);
	else {
		*(u32 *)&dev->dev_addr[0] = in_be32(&fec->paddr1);
		*(u16 *)&dev->dev_addr[4] = in_be16((u16*)&fec->paddr2);
	}

#if MPC5xxx_FEC_DEBUG > 1
	printk("loading sdma tasks\n");
#endif
#ifdef CONFIG_BESTCOMM_API
	priv->r_tasknum = mpc5xxx_sdma_fec_rx_task_setup(MPC5xxx_FEC_RBD_NUM,
						MPC5xxx_FEC_RECV_BUFFER_SIZE);
	priv->t_tasknum = mpc5xxx_sdma_fec_tx_task_setup(MPC5xxx_FEC_TBD_NUM);
#else
	priv->r_tasknum = mpc5xxx_sdma_load_task(&scEthernetRecv_TDT);
	priv->t_tasknum = mpc5xxx_sdma_load_task(&scEthernetXmit_TDT);
#endif

#if MPC5xxx_FEC_DEBUG > 1
	printk("eth receive task: %d, transmit task: %d\n",
			priv->r_tasknum, priv->t_tasknum);
#endif

#ifndef CONFIG_BESTCOMM_API
	sram = (struct mpc5xxx_sram_fec *)
			mpc5xxx_sdma_sram_alloc(sizeof *sram, 2);
	priv->sram = sram;

	var_tbl = mpc5xxx_sdma_var_addr(priv->r_tasknum);
	out_be32(&var_tbl[0], (u32)priv->sram);
	var_tbl = mpc5xxx_sdma_var_addr(priv->t_tasknum);
	out_be32(&var_tbl[0], (u32)priv->sram);
#endif	/* CONFIG_BESTCOMM_API */

	priv->r_irq = MPC5xxx_SDMA_IRQ_BASE + priv->r_tasknum;
	priv->t_irq = MPC5xxx_SDMA_IRQ_BASE + priv->t_tasknum;

	if (request_irq(MPC5xxx_FEC_IRQ, &mpc5xxx_fec_interrupt,
			SA_INTERRUPT | SA_SAMPLE_RANDOM, "eth_err", dev)) {
		panic("FEC interrupt allocation failed\n");
	}
	dev->irq = MPC5xxx_FEC_IRQ;

	if (request_irq(priv->r_irq, &mpc5xxx_sdma_receive_interrupt,
			SA_INTERRUPT | SA_SAMPLE_RANDOM, "eth_recv", dev)) {
		panic("FEC receive task interrupt allocation failed\n");
	}

	if (request_irq(priv->t_irq, &mpc5xxx_sdma_transmit_interrupt,
			SA_INTERRUPT | SA_SAMPLE_RANDOM, "eth_xmit", dev)) {
		panic("FEC transmit task interrupt allocation failed\n");
	}

#if MPC5xxx_FEC_DEBUG > 1
	printk("fec_irq %d, r_irq %d, t_irq %d\n",
			dev->irq, priv->r_irq, priv->t_irq);
#endif

	u32_value = in_be32(&priv->gpio->port_config);
#ifdef CONFIG_USE_MDIO
	u32_value |= 0x00050000;	/* 100MBit with MD	*/
#else
	u32_value |= 0x00020000;	/* 10MBit with 7-wire	*/
#endif
	out_be32(&priv->gpio->port_config, u32_value);

	out_be32(&fec->op_pause, 0x00010020);	/* change to 0xffff0020 ??? */
	out_be32(&fec->rfifo_cntrl, 0x0f000000);
	out_be32(&fec->rfifo_alarm, 0x0000030c);
	out_be32(&fec->tfifo_cntrl, 0x0f000000);
	out_be32(&fec->tfifo_alarm, 0x00000100);
	out_be32(&fec->x_wmrk, 0x3);		/* xmit fifo watermark = 256 */
	out_be32(&fec->xmit_fsm, 0x03000000);	/* enable crc generation */
	out_be32(&fec->iaddr1, 0x00000000);	/* No individual filter */
	out_be32(&fec->iaddr2, 0x00000000);	/* No individual filter */

#ifdef CONFIG_MPC5200
	/* Disable COMM Bus Prefetch */
	u16_value = in_be16(&priv->sdma->PtdCntrl);
	u16_value |= 1;
	out_be16(&priv->sdma->PtdCntrl, u16_value);
#endif

#if !defined(CONFIG_USE_MDIO)
	mpc5xxx_fec_restart (dev, 0);	/* always use half duplex mode only */
#else
	for (i=0; i<NMII-1; i++)
		mii_cmds[i].mii_next = &mii_cmds[i+1];
	mii_free = mii_cmds;

	priv->phy_speed = (((MPC5xxx_IPBFREQ >> 20) / 5) << 1);

	mpc5xxx_fec_restart (dev, 0);	/* half duplex, negotiate speed */
//	mpc5xxx_fec_restart (dev, 1);	/* full duplex, negotiate speed */

	/* Queue up command to detect the PHY and initialize the
	 * remainder of the interface.
	 */
	priv->phy_id_done = 0;
	priv->phy_addr = 0;
	mii_queue(dev, mk_mii_read(MII_REG_PHYIR1), mii_discover_phy, 0);

	priv->old_status = 0;
#endif

	/*
	 * Read MIB counters in order to reset them,
	 * then zero all the stats fields in memory
	 */
	mpc5xxx_fec_update_stat(dev);

	return 0;
}

static void __exit
mpc5xxx_fec_uninit(void)
{
	struct net_device *dev = mpc5xxx_fec_dev;
	struct mpc5xxx_fec_priv *priv = (struct mpc5xxx_fec_priv *)dev->priv;

	unregister_netdev(dev);
	free_irq(dev->irq, dev);
	free_irq(priv->r_irq, dev);
	free_irq(priv->t_irq, dev);
	kfree(dev->priv);
	dev->priv = NULL;
}

static int __init
mpc5xxx_fec_module_init(void)
{
	return mpc5xxx_fec_init();
}

static void __exit
mpc5xxx_fec_module_exit(void)
{
	mpc5xxx_fec_uninit();
}

module_init(mpc5xxx_fec_module_init);
module_exit(mpc5xxx_fec_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dale Farnsworth");
MODULE_DESCRIPTION("Ethernet driver for Motorola MPC5xxx FEC");
