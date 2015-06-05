/*
 * include/asm-ppc/mpc5xxx.h
 * 
 * Prototypes, etc. for the Motorola MPC5xxx embedded cpu chips
 *
 * Author: Dale Farnsworth <dfarnsworth@mvista.com>
 *
 * 2003 (c) MontaVista, Software, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#ifndef __ASM_MPC5XXX_H
#define __ASM_MPC5XXX_H

#define MPC5xxx_FEC_DEBUG 0
#define MPC5xxx_SDMA_DEBUG 0

#ifndef __ASSEMBLY__
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/config.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/machdep.h>
#endif /* __ASSEMBLY */

#define MPC5xxx_MBAR		0xf0000000
#define MPC5xxx_SDRAM_START	(MPC5xxx_MBAR + 0x0034)
#define MPC5xxx_SDRAM_CONFIG_0	(MPC5xxx_MBAR + 0x0034)
#define MPC5xxx_SDRAM_STOP	(MPC5xxx_MBAR + 0x0038)
#define MPC5xxx_SDRAM_CONFIG_1	(MPC5xxx_MBAR + 0x0038)
#define MPC5xxx_CDM		(MPC5xxx_MBAR + 0x0200)
#define MPC5xxx_SFTRST		(MPC5xxx_MBAR + 0x0220)
#define MPC5xxx_SFTRST_BIT	0x01000000
#define MPC5xxx_INTR		(MPC5xxx_MBAR + 0x0500)
#define MPC5xxx_GPT		(MPC5xxx_MBAR + 0x0600)
#define MPC5xxx_MSCAN1		(MPC5xxx_MBAR + 0x0900)	/* MSCAN Module 1 */
#define MPC5xxx_MSCAN2		(MPC5xxx_MBAR + 0x0980)	/* MSCAN Module 2 */
#define MPC5xxx_GPIO		(MPC5xxx_MBAR + 0x0b00)
#define MPC5xxx_PCI		(MPC5xxx_MBAR + 0x0d00)
#define MPC5xxx_USB_OHCI	(MPC5xxx_MBAR + 0x1000)
#define MPC5xxx_SDMA		(MPC5xxx_MBAR + 0x1200)
#define MPC5xxx_XLB		(MPC5xxx_MBAR + 0x1f00)
#define MPC5xxx_PSC1		(MPC5xxx_MBAR + 0x2000)
#define MPC5xxx_PSC2		(MPC5xxx_MBAR + 0x2200)
#define MPC5xxx_PSC3		(MPC5xxx_MBAR + 0x2400)
#define MPC5xxx_PSC4		(MPC5xxx_MBAR + 0x2600)
#define MPC5xxx_PSC5		(MPC5xxx_MBAR + 0x2800)
#define MPC5xxx_PSC6		(MPC5xxx_MBAR + 0x2C00)
#define MPC5xxx_FEC		(MPC5xxx_MBAR + 0x3000)
#define MPC5xxx_ATA		(MPC5xxx_MBAR + 0x3a00)
#define MPC5xxx_I2C1		(MPC5xxx_MBAR + 0x3d00)
#define MPC5xxx_I2C_MICR	(MPC5xxx_MBAR + 0x3d20)
#define MPC5xxx_I2C2		(MPC5xxx_MBAR + 0x3d40)
#ifdef CONFIG_MPC5100
#define MPC5xxx_SRAM		(MPC5xxx_MBAR + 0x4000)
#define MPC5xxx_SRAM_SIZE	(8*1024)
#define MPC5xxx_SDMA_MAX_TASKS	8
#elif defined(CONFIG_MPC5200)
#define MPC5xxx_SRAM		(MPC5xxx_MBAR + 0x8000)
#define MPC5xxx_SRAM_SIZE	(16*1024)
#define MPC5xxx_SDMA_MAX_TASKS	16
#endif

#define	MPC5xxx_INIT_WINDOW_DISABLE		0x0
#define	MPC5xxx_INIT_WINDOW_ENABLE		0x1
#define	MPC5xxx_INIT_WINDOW_READ		0x0
#define	MPC5xxx_INIT_WINDOW_READ_LINE		0x2
#define	MPC5xxx_INIT_WINDOW_READ_MULTIPLE	0x4
#define MPC5xxx_INIT_WINDOW_MEM			0x0
#define MPC5xxx_INIT_WINDOW_IO			0x8

/* General Purpose Timers registers */
#define MPC5xxx_GPT0_ENABLE	(MPC5xxx_GPT + 0x0)
#define MPC5xxx_GPT0_COUNTER	(MPC5xxx_GPT + 0x4)

/* MSCAN control register 0 (CANCTL0) bits */
#define MPC5xxx_MSCAN_RXFRM	0x80
#define MPC5xxx_MSCAN_RXACT	0x40
#define MPC5xxx_MSCAN_CSWAI	0x20
#define MPC5xxx_MSCAN_SYNCH	0x10
#define MPC5xxx_MSCAN_TIME	0x08
#define MPC5xxx_MSCAN_WUPE	0x04
#define MPC5xxx_MSCAN_SLPRQ	0x02
#define MPC5xxx_MSCAN_INITRQ	0x01

/* MSCAN control register 1 (CANCTL1) bits */
#define MPC5xxx_MSCAN_CANE	0x80
#define MPC5xxx_MSCAN_CLKSRC	0x40
#define MPC5xxx_MSCAN_LOOPB	0x20
#define MPC5xxx_MSCAN_LISTEN	0x10
#define MPC5xxx_MSCAN_WUPM	0x04
#define MPC5xxx_MSCAN_SLPAK	0x02
#define MPC5xxx_MSCAN_INITAK	0x01

/* MSCAN receiver flag register (CANRFLG) bits */
#define MPC5xxx_MSCAN_WUPIF	0x80
#define MPC5xxx_MSCAN_CSCIF	0x40
#define MPC5xxx_MSCAN_RSTAT1	0x20
#define MPC5xxx_MSCAN_RSTAT0	0x10
#define MPC5xxx_MSCAN_TSTAT1	0x08
#define MPC5xxx_MSCAN_TSTAT0	0x04
#define MPC5xxx_MSCAN_OVRIF	0x02
#define MPC5xxx_MSCAN_RXF		0x01

/* MSCAN receiver interrupt enable register (CANRIER) bits */
#define MPC5xxx_MSCAN_WUPIE	0x80
#define MPC5xxx_MSCAN_CSCIE	0x40
#define MPC5xxx_MSCAN_RSTATE1	0x20
#define MPC5xxx_MSCAN_RSTATE0	0x10
#define MPC5xxx_MSCAN_TSTATE1	0x08
#define MPC5xxx_MSCAN_TSTATE0	0x04
#define MPC5xxx_MSCAN_OVRIE	0x02
#define MPC5xxx_MSCAN_RXFIE	0x01

/* MSCAN transmitter flag register (CANTFLG) bits */
#define MPC5xxx_MSCAN_TXE2	0x04
#define MPC5xxx_MSCAN_TXE1	0x02
#define MPC5xxx_MSCAN_TXE0	0x01
#define MPC5xxx_MSCAN_TXE	(MPC5xxx_MSCAN_TXE2 | MPC5xxx_MSCAN_TXE1 | \
				 MPC5xxx_MSCAN_TXE0)

/* MSCAN transmitter interrupt enable register (CANTIER) bits */
#define MPC5xxx_MSCAN_TXIE2	0x04
#define MPC5xxx_MSCAN_TXIE1	0x02
#define MPC5xxx_MSCAN_TXIE0	0x01
#define MPC5xxx_MSCAN_TXIE	(MPC5xxx_MSCAN_TXIE2 | MPC5xxx_MSCAN_TXIE1 | \
				 MPC5xxx_MSCAN_TXIE0)

/* MSCAN transmitter message abort request (CANTARQ) bits */
#define MPC5xxx_MSCAN_ABTRQ2	0x04
#define MPC5xxx_MSCAN_ABTRQ1	0x02
#define MPC5xxx_MSCAN_ABTRQ0	0x01

/* MSCAN transmitter message abort ack (CANTAAK) bits */
#define MPC5xxx_MSCAN_ABTAK2	0x04
#define MPC5xxx_MSCAN_ABTAK1	0x02
#define MPC5xxx_MSCAN_ABTAK0	0x01

/* MSCAN transmit buffer selection (CANTBSEL) bits */
#define MPC5xxx_MSCAN_TX2	0x04
#define MPC5xxx_MSCAN_TX1	0x02
#define MPC5xxx_MSCAN_TX0	0x01

/* MSCAN ID acceptance control register (CANIDAC) bits */
#define MPC5xxx_MSCAN_IDAM1	0x20
#define MPC5xxx_MSCAN_IDAM0	0x10
#define MPC5xxx_MSCAN_IDHIT2	0x04
#define MPC5xxx_MSCAN_IDHIT1	0x02
#define MPC5xxx_MSCAN_IDHIT0	0x01

/* I2Cn frequency divider register possible values */
#define MPC5xxx_I2C_100HZ	0x1F	/* 100HZ Clock */

/* I2Cn control register bits */
#define MPC5xxx_I2C_EN		0x80	/* Enable I2Cn module */
#define MPC5xxx_I2C_IEN		0x40	/* Enable interrupt from I2Cn module */
#define MPC5xxx_I2C_STA		0x20	/* Master/Slave mode select */
#define MPC5xxx_I2C_TX		0x10	/* Transmit/Receive mode select */
#define MPC5xxx_I2C_TXAK	0x08	/* Transmit ack enable */
#define MPC5xxx_I2C_RSTA	0x04	/* Repeat start */
#define MPC5xxx_I2C_INIT_MASK	(MPC5xxx_I2C_EN | MPC5xxx_I2C_STA | \
				 MPC5xxx_I2C_TX | MPC5xxx_I2C_RSTA)

/* I2Cn status register bits */
#define MPC5xxx_I2C_CF		0x80	/* Data transferring */
#define MPC5xxx_I2C_AAS		0x40	/* Addressed as slave */
#define MPC5xxx_I2C_BB		0x20	/* Bus busy */
#define MPC5xxx_I2C_AL		0x10	/* Arbitration lost */
#define MPC5xxx_I2C_SRW		0x04	/* Slave Read/Write */
#define MPC5xxx_I2C_IF		0x02	/* I2Cn Interrupt */
#define MPC5xxx_I2C_RXAK	0x01	/* Receive ack */

/* Programmable Serial Controller (PSC) status register bits */
#define MPC5xxx_PSC_SR_CDE	0x0080
#define MPC5xxx_PSC_SR_RXRDY	0x0100
#define MPC5xxx_PSC_SR_RXFULL	0x0200
#define MPC5xxx_PSC_SR_TXRDY	0x0400
#define MPC5xxx_PSC_SR_TXEMP	0x0800
#define MPC5xxx_PSC_SR_OE	0x1000
#define MPC5xxx_PSC_SR_PE	0x2000
#define MPC5xxx_PSC_SR_FE	0x4000
#define MPC5xxx_PSC_SR_RB	0x8000

/* PSC Command values */
#define MPC5xxx_PSC_RX_ENABLE		0x0001
#define MPC5xxx_PSC_RX_DISABLE		0x0002
#define MPC5xxx_PSC_TX_ENABLE		0x0004
#define MPC5xxx_PSC_TX_DISABLE		0x0008
#define MPC5xxx_PSC_SEL_MODE_REG_1	0x0010
#define MPC5xxx_PSC_RST_RX		0x0020
#define MPC5xxx_PSC_RST_TX		0x0030
#define MPC5xxx_PSC_RST_ERR_STAT	0x0040
#define MPC5xxx_PSC_RST_BRK_CHG_INT	0x0050
#define MPC5xxx_PSC_START_BRK		0x0060
#define MPC5xxx_PSC_STOP_BRK		0x0070

/* PSC Rx FIFO status bits */
#define MPC5xxx_PSC_RX_FIFO_ERR		0x0040
#define MPC5xxx_PSC_RX_FIFO_UF		0x0020
#define MPC5xxx_PSC_RX_FIFO_OF		0x0010
#define MPC5xxx_PSC_RX_FIFO_FR		0x0008
#define MPC5xxx_PSC_RX_FIFO_FULL	0x0004
#define MPC5xxx_PSC_RX_FIFO_ALARM	0x0002
#define MPC5xxx_PSC_RX_FIFO_EMPTY	0x0001

/* PSC interrupt mask bits */
#define MPC5xxx_PSC_IMR_TXRDY		0x0100
#define MPC5xxx_PSC_IMR_RXRDY		0x0200
#define MPC5xxx_PSC_IMR_DB		0x0400
#define MPC5xxx_PSC_IMR_IPC		0x8000

/* PSC input port change bit */
#define MPC5xxx_PSC_CTS			0x01
#define MPC5xxx_PSC_DCD			0x02
#define MPC5xxx_PSC_D_CTS		0x10
#define MPC5xxx_PSC_D_DCD		0x20

/* PSC mode fields */
#define MPC5xxx_PSC_MODE_5_BITS			0x00
#define MPC5xxx_PSC_MODE_6_BITS			0x01
#define MPC5xxx_PSC_MODE_7_BITS			0x02
#define MPC5xxx_PSC_MODE_8_BITS			0x03
#define MPC5xxx_PSC_MODE_PAREVEN		0x00
#define MPC5xxx_PSC_MODE_PARODD			0x04
#define MPC5xxx_PSC_MODE_PARFORCE		0x08
#define MPC5xxx_PSC_MODE_PARNONE		0x10
#define MPC5xxx_PSC_MODE_ERR			0x20
#define MPC5xxx_PSC_MODE_FFULL			0x40
#define MPC5xxx_PSC_MODE_RXRTS			0x80

#define MPC5xxx_PSC_MODE_ONE_STOP_5_BITS	0x00
#define MPC5xxx_PSC_MODE_ONE_STOP		0x07
#define MPC5xxx_PSC_MODE_TWO_STOP		0x0f

#define MPC5xxx_PSC_RFNUM_MASK	0x01ff

/* Memory allocation block size */
#define MPC5xxx_SDRAM_UNIT	0x8000		/* 32K byte */

#define MPC5xxx_CRIT_IRQ_NUM	4
#define MPC5xxx_MAIN_IRQ_NUM	17
#define MPC5xxx_SDMA_IRQ_NUM	17
#define MPC5xxx_PERP_IRQ_NUM	22

#define MPC5xxx_CRIT_IRQ_BASE	0
#define MPC5xxx_MAIN_IRQ_BASE	(MPC5xxx_CRIT_IRQ_BASE + MPC5xxx_CRIT_IRQ_NUM)
#define MPC5xxx_SDMA_IRQ_BASE	(MPC5xxx_MAIN_IRQ_BASE + MPC5xxx_MAIN_IRQ_NUM)
#define MPC5xxx_PERP_IRQ_BASE	(MPC5xxx_SDMA_IRQ_BASE + MPC5xxx_SDMA_IRQ_NUM)

#define MPC5xxx_IRQ0			(MPC5xxx_CRIT_IRQ_BASE + 0)
#define MPC5xxx_SLICE_TIMER_0_IRQ	(MPC5xxx_CRIT_IRQ_BASE + 1)
#define MPC5xxx_HI_INT_IRQ		(MPC5xxx_CRIT_IRQ_BASE + 2)
#define MPC5xxx_CCS_IRQ			(MPC5xxx_CRIT_IRQ_BASE + 3)

#define MPC5xxx_IRQ1			(MPC5xxx_MAIN_IRQ_BASE + 1)
#define MPC5xxx_IRQ2			(MPC5xxx_MAIN_IRQ_BASE + 2)
#define MPC5xxx_IRQ3			(MPC5xxx_MAIN_IRQ_BASE + 3)

#define MPC5xxx_SDMA_IRQ		(MPC5xxx_PERP_IRQ_BASE + 0)
#define MPC5xxx_PSC1_IRQ		(MPC5xxx_PERP_IRQ_BASE + 1)
#define MPC5xxx_PSC2_IRQ		(MPC5xxx_PERP_IRQ_BASE + 2)
#define MPC5xxx_PSC3_IRQ		(MPC5xxx_PERP_IRQ_BASE + 3)
#define MPC5xxx_IRDA_IRQ		(MPC5xxx_PERP_IRQ_BASE + 4)
#define MPC5xxx_FEC_IRQ			(MPC5xxx_PERP_IRQ_BASE + 5)
#define MPC5xxx_USB_IRQ			(MPC5xxx_PERP_IRQ_BASE + 6)
#define MPC5xxx_ATA_IRQ			(MPC5xxx_PERP_IRQ_BASE + 7)
#define MPC5xxx_PCI_CNTRL_IRQ		(MPC5xxx_PERP_IRQ_BASE + 8)
#define MPC5xxx_PCI_SCIRX_IRQ		(MPC5xxx_PERP_IRQ_BASE + 9)
#define MPC5xxx_PCI_SCITX_IRQ		(MPC5xxx_PERP_IRQ_BASE + 10)
#define MPC5xxx_SPI_MODF_IRQ		(MPC5xxx_PERP_IRQ_BASE + 13)
#define MPC5xxx_SPI_SPIF_IRQ		(MPC5xxx_PERP_IRQ_BASE + 14)
#define MPC5xxx_I2C1_IRQ		(MPC5xxx_PERP_IRQ_BASE + 15)
#define MPC5xxx_I2C2_IRQ		(MPC5xxx_PERP_IRQ_BASE + 16)
#define MPC5xxx_CAN1_IRQ		(MPC5xxx_PERP_IRQ_BASE + 17)
#define MPC5xxx_CAN2_IRQ		(MPC5xxx_PERP_IRQ_BASE + 18)
#define MPC5xxx_IR_RX_IRQ		(MPC5xxx_PERP_IRQ_BASE + 19)
#define MPC5xxx_IR_TX_IRQ		(MPC5xxx_PERP_IRQ_BASE + 20)
#define MPC5xxx_XLB_ARB_IRQ		(MPC5xxx_PERP_IRQ_BASE + 21)

#define PCI_WINDOW_TRANSLATION(proc_start, proc_end, pci_start, pci_end) \
	((((proc_start >> 24) & 0xff) << 24) | \
	((((proc_end - proc_start) >> 24) & 0xff) << 16) | \
	(((pci_start >> 24) & 0xff) << 8))

#define PCI_WINDOW_CONTROL(win0, win1, win2)	((win0 << 24) | \
						 (win1 << 16) | \
						 (win2 << 8))

#ifndef __ASSEMBLY__

struct mpc5xxx_pci {
	volatile u32		idr;		/* PCI + 0x00 */
	volatile u32		scr;		/* PCI + 0x04 */
	volatile u32		ccrir;		/* PCI + 0x08 */
	volatile u32		cr1;		/* PCI + 0x0C */
	volatile u32		bar0;		/* PCI + 0x10 */
	volatile u32		bar1;		/* PCI + 0x14 */
	volatile u8		reserved1[16];	/* PCI + 0x18 */
	volatile u32		ccpr;		/* PCI + 0x28 */
	volatile u32		sid;		/* PCI + 0x2C */
	volatile u32		erbar;		/* PCI + 0x30 */
	volatile u32		cpr;		/* PCI + 0x34 */
	volatile u8		reserved2[4];	/* PCI + 0x38 */
	volatile u32		cr2;		/* PCI + 0x3C */
	volatile u8		reserved3[32];	/* PCI + 0x40 */
	volatile u32		gscr;		/* PCI + 0x60 */
	volatile u32		tbatr0;		/* PCI + 0x64 */
	volatile u32		tbatr1;		/* PCI + 0x68 */
	volatile u32		tcr;		/* PCI + 0x6C */
	volatile u32		iw0btar;	/* PCI + 0x70 */
	volatile u32		iw1btar;	/* PCI + 0x74 */
	volatile u32		iw2btar;	/* PCI + 0x78 */
	volatile u8		reserved4[4];	/* PCI + 0x7C */
	volatile u32		iwcr;		/* PCI + 0x80 */
	volatile u32		icr;		/* PCI + 0x84 */
	volatile u32		isr;		/* PCI + 0x88 */
	volatile u32		arb;		/* PCI + 0x8C */
	volatile u8		reserved5[104];	/* PCI + 0x90 */
	volatile u32		car;		/* PCI + 0xF8 */
	volatile u8		reserved6[4];	/* PCI + 0xFC */
};

struct mpc5xxx_psc {
	volatile u8		mode;		/* PSC + 0x00 */
	volatile u8		reserved0[3];
	union {					/* PSC + 0x04 */
		volatile u16	status;
		volatile u16	clock_select;
	} sr_csr;
#define mpc5xxx_psc_status	sr_csr.status
#define mpc5xxx_psc_clock_select	sr_csr.clock_select
	volatile u16		reserved1;
	volatile u8		command;	/* PSC + 0x08 */
	volatile u8		reserved2[3];
	union {					/* PSC + 0x0c */
		volatile u8	buffer_8;
		volatile u16	buffer_16;
		volatile u32	buffer_32;
	} buffer;
#define mpc5xxx_psc_buffer_8	buffer.buffer_8
#define mpc5xxx_psc_buffer_16	buffer.buffer_16
#define mpc5xxx_psc_buffer_32	buffer.buffer_32
	union {					/* PSC + 0x10 */
		volatile u8	ipcr;
		volatile u8	acr;
	} ipcr_acr;
#define mpc5xxx_psc_ipcr		ipcr_acr.ipcr
#define mpc5xxx_psc_acr		ipcr_acr.acr
	volatile u8		reserved3[3];
	union {					/* PSC + 0x14 */
		volatile u16	isr;
		volatile u16	imr;
	} isr_imr;
#define mpc5xxx_psc_isr		isr_imr.isr
#define mpc5xxx_psc_imr		isr_imr.imr
	volatile u16		reserved4;
	volatile u8		ctur;		/* PSC + 0x18 */
	volatile u8		reserved5[3];
	volatile u8		ctlr;		/* PSC + 0x1c */
	volatile u8		reserved6[3];
	volatile u16		ccr;		/* PSC + 0x20 */
	volatile u8		reserved7[14];
	volatile u8		ivr;		/* PSC + 0x30 */
	volatile u8		reserved8[3];
	volatile u8		ip;		/* PSC + 0x34 */
	volatile u8		reserved9[3];
	volatile u8		op1;		/* PSC + 0x38 */
	volatile u8		reserved10[3];
	volatile u8		op0;		/* PSC + 0x3c */
	volatile u8		reserved11[3];
	volatile u32		sicr;		/* PSC + 0x40 */
	volatile u8		ircr1;		/* PSC + 0x44 */
	volatile u8		reserved13[3];
	volatile u8		ircr2;		/* PSC + 0x44 */
	volatile u8		reserved14[3];
	volatile u8		irsdr;		/* PSC + 0x4c */
	volatile u8		reserved15[3];
	volatile u8		irmdr;		/* PSC + 0x50 */
	volatile u8		reserved16[3];
	volatile u8		irfdr;		/* PSC + 0x54 */
	volatile u8		reserved17[3];
	volatile u16		rfnum;		/* PSC + 0x58 */
	volatile u16		reserved18;
	volatile u16		tfnum;		/* PSC + 0x5c */
	volatile u16		reserved19;
	volatile u32		rfdata;		/* PSC + 0x60 */
	volatile u16		rfstat;		/* PSC + 0x64 */
	volatile u16		reserved20;
	volatile u8		rfcntl;		/* PSC + 0x68 */
	volatile u8		reserved21[5];
	volatile u16		rfalarm;	/* PSC + 0x6e */
	volatile u16		reserved22;
	volatile u16		rfrptr;		/* PSC + 0x72 */
	volatile u16		reserved23;
	volatile u16		rfwptr;		/* PSC + 0x76 */
	volatile u16		reserved24;
	volatile u16		rflrfptr;	/* PSC + 0x7a */
	volatile u16		reserved25;
	volatile u16		rflwfptr;	/* PSC + 0x7e */
	volatile u32		tfdata;		/* PSC + 0x80 */
	volatile u16		tfstat;		/* PSC + 0x84 */
	volatile u16		reserved26;
	volatile u8		tfcntl;		/* PSC + 0x88 */
	volatile u8		reserved27[5];
	volatile u16		tfalarm;	/* PSC + 0x8e */
	volatile u16		reserved28;
	volatile u16		tfrptr;		/* PSC + 0x92 */
	volatile u16		reserved29;
	volatile u16		tfwptr;		/* PSC + 0x96 */
	volatile u16		reserved30;
	volatile u16		tflrfptr;	/* PSC + 0x9a */
	volatile u16		reserved31;
	volatile u16		tflwfptr;	/* PSC + 0x9e */
};

struct mpc5xxx_intr {
	volatile u32		per_mask;	/* INTR + 0x00 */
	volatile u32		per_pri1;	/* INTR + 0x04 */
	volatile u32		per_pri2;	/* INTR + 0x08 */
	volatile u32		per_pri3;	/* INTR + 0x0c */
	volatile u32		ctrl;		/* INTR + 0x10 */
	volatile u32		main_mask;	/* INTR + 0x14 */
	volatile u32		main_pri1;	/* INTR + 0x18 */
	volatile u32		main_pri2;	/* INTR + 0x1c */
	volatile u32		reserved1;	/* INTR + 0x20 */
	volatile u32		enc_status;	/* INTR + 0x24 */
	volatile u32		crit_status;	/* INTR + 0x28 */
	volatile u32		main_status;	/* INTR + 0x2c */
	volatile u32		per_status;	/* INTR + 0x30 */
	volatile u32		reserved2;	/* INTR + 0x34 */
	volatile u32		per_error;	/* INTR + 0x38 */
};

struct mpc5xxx_gpio {
	volatile u32		port_config;	/* GPIO + 0x00 */
	volatile u32		simple_gpioe;	/* GPIO + 0x04 */
	volatile u32		simple_ode;	/* GPIO + 0x08 */
	volatile u32		simple_ddr;	/* GPIO + 0x0c */
	volatile u32		simple_dvo;	/* GPIO + 0x10 */
	volatile u32		simple_ival;	/* GPIO + 0x14 */
	volatile u8		outo_gpioe;	/* GPIO + 0x18 */
	volatile u8		reserved1[3];	/* GPIO + 0x19 */
	volatile u8		outo_dvo;	/* GPIO + 0x1c */
	volatile u8		reserved2[3];	/* GPIO + 0x1d */
	volatile u8		sint_gpioe;	/* GPIO + 0x20 */
	volatile u8		reserved3[3];	/* GPIO + 0x21 */
	volatile u8		sint_ode;	/* GPIO + 0x24 */
	volatile u8		reserved4[3];	/* GPIO + 0x25 */
	volatile u8		sint_ddr;	/* GPIO + 0x28 */
	volatile u8		reserved5[3];	/* GPIO + 0x29 */
	volatile u8		sint_dvo;	/* GPIO + 0x2c */
	volatile u8		reserved6[3];	/* GPIO + 0x2d */
	volatile u8		sint_inten;	/* GPIO + 0x30 */
	volatile u8		reserved7[3];	/* GPIO + 0x31 */
	volatile u16		sint_itype;	/* GPIO + 0x34 */
	volatile u16		reserved8;	/* GPIO + 0x36 */
	volatile u8		gpio_control;	/* GPIO + 0x38 */
	volatile u8		reserved9[3];	/* GPIO + 0x39 */
	volatile u8		sint_istat;	/* GPIO + 0x3c */
	volatile u8		sint_ival;	/* GPIO + 0x3d */
	volatile u8		bus_errs;	/* GPIO + 0x3e */
	volatile u8		reserved10;	/* GPIO + 0x3f */
};

#define MPC5xxx_GPIO_PSC_CONFIG_UART_WITHOUT_CD	4
#define MPC5xxx_GPIO_PSC_CONFIG_UART_WITH_CD	5

struct mpc5xxx_sdma {
	volatile u32		taskBar;	/* SDMA + 0x00 */
	volatile u32		currentPointer;	/* SDMA + 0x04 */
	volatile u32		endPointer;	/* SDMA + 0x08 */
	volatile u32		variablePointer;/* SDMA + 0x0c */

	volatile u8		IntVect1;	/* SDMA + 0x10 */
	volatile u8		IntVect2;	/* SDMA + 0x11 */
	volatile u16		PtdCntrl;	/* SDMA + 0x12 */

	volatile u32		IntPend;	/* SDMA + 0x14 */
	volatile u32		IntMask;	/* SDMA + 0x18 */

	volatile u16		tcr_0;		/* SDMA + 0x1c */
	volatile u16		tcr_1;		/* SDMA + 0x1e */
	volatile u16		tcr_2;		/* SDMA + 0x20 */
	volatile u16		tcr_3;		/* SDMA + 0x22 */
	volatile u16		tcr_4;		/* SDMA + 0x24 */
	volatile u16		tcr_5;		/* SDMA + 0x26 */
	volatile u16		tcr_6;		/* SDMA + 0x28 */
	volatile u16		tcr_7;		/* SDMA + 0x2a */
	volatile u16		tcr_8;		/* SDMA + 0x2c */
	volatile u16		tcr_9;		/* SDMA + 0x2e */
	volatile u16		tcr_a;		/* SDMA + 0x30 */
	volatile u16		tcr_b;		/* SDMA + 0x32 */
	volatile u16		tcr_c;		/* SDMA + 0x34 */
	volatile u16		tcr_d;		/* SDMA + 0x36 */
	volatile u16		tcr_e;		/* SDMA + 0x38 */
	volatile u16		tcr_f;		/* SDMA + 0x3a */

	volatile u8		IPR0;		/* SDMA + 0x3c */
	volatile u8		IPR1;		/* SDMA + 0x3d */
	volatile u8		IPR2;		/* SDMA + 0x3e */
	volatile u8		IPR3;		/* SDMA + 0x3f */
	volatile u8		IPR4;		/* SDMA + 0x40 */
	volatile u8		IPR5;		/* SDMA + 0x41 */
	volatile u8		IPR6;		/* SDMA + 0x42 */
	volatile u8		IPR7;		/* SDMA + 0x43 */
	volatile u8		IPR8;		/* SDMA + 0x44 */
	volatile u8		IPR9;		/* SDMA + 0x45 */
	volatile u8		IPR10;		/* SDMA + 0x46 */
	volatile u8		IPR11;		/* SDMA + 0x47 */
	volatile u8		IPR12;		/* SDMA + 0x48 */
	volatile u8		IPR13;		/* SDMA + 0x49 */
	volatile u8		IPR14;		/* SDMA + 0x4a */
	volatile u8		IPR15;		/* SDMA + 0x4b */
	volatile u8		IPR16;		/* SDMA + 0x4c */
	volatile u8		IPR17;		/* SDMA + 0x4d */
	volatile u8		IPR18;		/* SDMA + 0x4e */
	volatile u8		IPR19;		/* SDMA + 0x4f */
	volatile u8		IPR20;		/* SDMA + 0x50 */
	volatile u8		IPR21;		/* SDMA + 0x51 */
	volatile u8		IPR22;		/* SDMA + 0x52 */
	volatile u8		IPR23;		/* SDMA + 0x53 */
	volatile u8		IPR24;		/* SDMA + 0x54 */
	volatile u8		IPR25;		/* SDMA + 0x55 */
	volatile u8		IPR26;		/* SDMA + 0x56 */
	volatile u8		IPR27;		/* SDMA + 0x57 */
	volatile u8		IPR28;		/* SDMA + 0x58 */
	volatile u8		IPR29;		/* SDMA + 0x59 */
	volatile u8		IPR30;		/* SDMA + 0x5a */
	volatile u8		IPR31;		/* SDMA + 0x5b */

	volatile u32		res1;		/* SDMA + 0x5c */
#ifdef CONFIG_MPC5100
	volatile u32		res2;		/* SDMA + 0x60 */
	volatile u32		res3;		/* SDMA + 0x64 */
#else
	volatile u32		task_size0;	/* SDMA + 0x60 */
	volatile u32		task_size1;	/* SDMA + 0x64 */
#endif
	volatile u32		MDEDebug;	/* SDMA + 0x68 */
	volatile u32		ADSDebug;	/* SDMA + 0x6c */
	volatile u32		Value1;		/* SDMA + 0x70 */
	volatile u32		Value2;		/* SDMA + 0x74 */
	volatile u32		Control;	/* SDMA + 0x78 */
	volatile u32		Status;		/* SDMA + 0x7c */
	volatile u32		EU00;		/* SDMA + 0x80 */
	volatile u32		EU01;		/* SDMA + 0x84 */
	volatile u32		EU02;		/* SDMA + 0x88 */
	volatile u32		EU03;		/* SDMA + 0x8c */
	volatile u32		EU04;		/* SDMA + 0x90 */
	volatile u32		EU05;		/* SDMA + 0x94 */
	volatile u32		EU06;		/* SDMA + 0x98 */
	volatile u32		EU07;		/* SDMA + 0x9c */
	volatile u32		EU10;		/* SDMA + 0xa0 */
	volatile u32		EU11;		/* SDMA + 0xa4 */
	volatile u32		EU12;		/* SDMA + 0xa8 */
	volatile u32		EU13;		/* SDMA + 0xac */
	volatile u32		EU14;		/* SDMA + 0xb0 */
	volatile u32		EU15;		/* SDMA + 0xb4 */
	volatile u32		EU16;		/* SDMA + 0xb8 */
	volatile u32		EU17;		/* SDMA + 0xbc */
	volatile u32		EU20;		/* SDMA + 0xc0 */
	volatile u32		EU21;		/* SDMA + 0xc4 */
	volatile u32		EU22;		/* SDMA + 0xc8 */
	volatile u32		EU23;		/* SDMA + 0xcc */
	volatile u32		EU24;		/* SDMA + 0xd0 */
	volatile u32		EU25;		/* SDMA + 0xd4 */
	volatile u32		EU26;		/* SDMA + 0xd8 */
	volatile u32		EU27;		/* SDMA + 0xdc */
	volatile u32		EU30;		/* SDMA + 0xe0 */
	volatile u32		EU31;		/* SDMA + 0xe4 */
	volatile u32		EU32;		/* SDMA + 0xe8 */
	volatile u32		EU33;		/* SDMA + 0xec */
	volatile u32		EU34;		/* SDMA + 0xf0 */
	volatile u32		EU35;		/* SDMA + 0xf4 */
	volatile u32		EU36;		/* SDMA + 0xf8 */
	volatile u32		EU37;		/* SDMA + 0xfc */
};

struct mscan_buffer {
	volatile u8  idr[0x8];		/* 0x00 */
	volatile u8  dsr[0x10];		/* 0x08 */
	volatile u8  dlr;		/* 0x18 */
	volatile u8  tbpr;		/* 0x19 */	/* This register is not applicable for receive buffers */
	volatile u16 rsrv1;		/* 0x1A */
	volatile u8  tsrh;		/* 0x1C */
	volatile u8  tsrl;		/* 0x1D */
	volatile u16 rsrv2;		/* 0x1E */
};

struct mpc5xxx_mscan {
	volatile u8  canctl0;		/* MSCAN + 0x00 */
	volatile u8  canctl1;		/* MSCAN + 0x01 */
	volatile u16 rsrv1;		/* MSCAN + 0x02 */
	volatile u8  canbtr0;		/* MSCAN + 0x04 */
	volatile u8  canbtr1;		/* MSCAN + 0x05 */
	volatile u16 rsrv2;		/* MSCAN + 0x06 */
	volatile u8  canrflg;		/* MSCAN + 0x08 */
	volatile u8  canrier;		/* MSCAN + 0x09 */
	volatile u16 rsrv3;		/* MSCAN + 0x0A */
	volatile u8  cantflg;		/* MSCAN + 0x0C */
	volatile u8  cantier;		/* MSCAN + 0x0D */
	volatile u16 rsrv4;		/* MSCAN + 0x0E */
	volatile u8  cantarq;		/* MSCAN + 0x10 */
	volatile u8  cantaak;		/* MSCAN + 0x11 */
	volatile u16 rsrv5;		/* MSCAN + 0x12 */
	volatile u8  cantbsel;		/* MSCAN + 0x14 */
	volatile u8  canidac;		/* MSCAN + 0x15 */
	volatile u16 rsrv6[3];		/* MSCAN + 0x16 */
	volatile u8  canrxerr;		/* MSCAN + 0x1C */
	volatile u8  cantxerr;		/* MSCAN + 0x1D */
	volatile u16 rsrv7;		/* MSCAN + 0x1E */
	volatile u8  canidar0;		/* MSCAN + 0x20 */
	volatile u8  canidar1;		/* MSCAN + 0x21 */
	volatile u16 rsrv8;		/* MSCAN + 0x22 */
	volatile u8  canidar2;		/* MSCAN + 0x24 */
	volatile u8  canidar3;		/* MSCAN + 0x25 */
	volatile u16 rsrv9;		/* MSCAN + 0x26 */
	volatile u8  canidmr0;		/* MSCAN + 0x28 */
	volatile u8  canidmr1;		/* MSCAN + 0x29 */
	volatile u16 rsrv10;		/* MSCAN + 0x2A */
	volatile u8  canidmr2;		/* MSCAN + 0x2C */
	volatile u8  canidmr3;		/* MSCAN + 0x2D */
	volatile u16 rsrv11;		/* MSCAN + 0x2E */
	volatile u8  canidar4;		/* MSCAN + 0x30 */
	volatile u8  canidar5;		/* MSCAN + 0x31 */
	volatile u16 rsrv12;		/* MSCAN + 0x32 */
	volatile u8  canidar6;		/* MSCAN + 0x34 */
	volatile u8  canidar7;		/* MSCAN + 0x35 */
	volatile u16 rsrv13;		/* MSCAN + 0x36 */
	volatile u8  canidmr4;		/* MSCAN + 0x38 */
	volatile u8  canidmr5;		/* MSCAN + 0x39 */
	volatile u16 rsrv14;		/* MSCAN + 0x3A */
	volatile u8  canidmr6;		/* MSCAN + 0x3C */
	volatile u8  canidmr7;		/* MSCAN + 0x3D */
	volatile u16 rsrv15;		/* MSCAN + 0x3E */
	
	struct mscan_buffer canrxfg;	/* MSCAN + 0x40 */    /* Foreground receive buffer */
	struct mscan_buffer cantxfg;	/* MSCAN + 0x60 */    /* Foreground transmit buffer */
};

struct mpc5xxx_i2c {
	volatile u32 madr;		/* I2Cn + 0x00 */
	volatile u32 mfdr;		/* I2Cn + 0x04 */
	volatile u32 mcr;		/* I2Cn + 0x08 */
	volatile u32 msr;		/* I2Cn + 0x0C */
	volatile u32 mdr;		/* I2Cn + 0x10 */
};

struct mpc5xxx_xlb {
	volatile u8  reserved[0x40];
	volatile u32 config;		/* XLB + 0x40 */
	volatile u32 version;		/* XLB + 0x44 */
	volatile u32 status;		/* XLB + 0x48 */
	volatile u32 int_enable;	/* XLB + 0x4c */
	volatile u32 addr_capture;	/* XLB + 0x50 */
	volatile u32 bus_sig_capture;	/* XLB + 0x54 */
	volatile u32 addr_timeout;	/* XLB + 0x58 */
	volatile u32 data_timeout;	/* XLB + 0x5c */
	volatile u32 bus_act_timeout;	/* XLB + 0x60 */
	volatile u32 master_pri_enable;	/* XLB + 0x64 */
	volatile u32 master_priority;	/* XLB + 0x68 */
	volatile u32 base_address;	/* XLB + 0x6c */
	volatile u32 snoop_window;	/* XLB + 0x70 */
};

struct mpc5xxx_cdm {
	volatile u32 jtag_id;		/* MBAR_CDM + 0x00  reg0 read only */
	volatile u32 rstcfg;		/* MBAR_CDM + 0x04  reg1 read only */
	volatile u32 breadcrumb;	/* MBAR_CDM + 0x08  reg2 */

	volatile u8  mem_clk_sel;	/* MBAR_CDM + 0x0c  reg3 byte0 */
	volatile u8  xlb_clk_sel;	/* MBAR_CDM + 0x0d  reg3 byte1 read only */
	volatile u8  ipg_clk_sel;	/* MBAR_CDM + 0x0e  reg3 byte2 */
	volatile u8  pci_clk_sel;	/* MBAR_CDM + 0x0f  reg3 byte3 */

	volatile u8  ext_48mhz_en;	/* MBAR_CDM + 0x10  reg4 byte0 */
	volatile u8  fd_enable;		/* MBAR_CDM + 0x11  reg4 byte1 */
	volatile u16 fd_counters;	/* MBAR_CDM + 0x12  reg4 byte2,3 */

	volatile u32 clk_enables;	/* MBAR_CDM + 0x14  reg5 */

	volatile u8  osc_disable;	/* MBAR_CDM + 0x18  reg6 byte0 */
	volatile u8  reserved0[3];	/* MBAR_CDM + 0x19  reg6 byte1,2,3 */

	volatile u8  ccs_sleep_enable;	/* MBAR_CDM + 0x1c  reg7 byte0 */
	volatile u8  osc_sleep_enable;	/* MBAR_CDM + 0x1d  reg7 byte1 */
	volatile u8  reserved1;		/* MBAR_CDM + 0x1e  reg7 byte2 */
	volatile u8  ccs_qreq_test;	/* MBAR_CDM + 0x1f  reg7 byte3 */

	volatile u8  soft_reset;	/* MBAR_CDM + 0x20  u8 byte0 */
	volatile u8  no_ckstp;		/* MBAR_CDM + 0x21  u8 byte0 */
	volatile u8  reserved2[2];	/* MBAR_CDM + 0x22  u8 byte1,2,3 */

	volatile u8  pll_lock;		/* MBAR_CDM + 0x24  reg9 byte0 */
	volatile u8  pll_looselock;	/* MBAR_CDM + 0x25  reg9 byte1 */
	volatile u8  pll_sm_lockwin;	/* MBAR_CDM + 0x26  reg9 byte2 */
	volatile u8  reserved3;		/* MBAR_CDM + 0x27  reg9 byte3 */

	volatile u16 reserved4;		/* MBAR_CDM + 0x28  reg10 byte0,1 */
	volatile u16 mclken_div_psc1;	/* MBAR_CDM + 0x2a  reg10 byte2,3 */
    
	volatile u16 reserved5;		/* MBAR_CDM + 0x2c  reg11 byte0,1 */
	volatile u16 mclken_div_psc2;	/* MBAR_CDM + 0x2e  reg11 byte2,3 */
		
	volatile u16 reserved6;		/* MBAR_CDM + 0x30  reg12 byte0,1 */
	volatile u16 mclken_div_psc3;	/* MBAR_CDM + 0x32  reg12 byte2,3 */
    
	volatile u16 reserved7;		/* MBAR_CDM + 0x34  reg13 byte0,1 */
	volatile u16 mclken_div_psc6;	/* MBAR_CDM + 0x36  reg13 byte2,3 */
};

#define MPC5xxx_SDMA_VAR_SIZE		256
#define MPC5xxx_SDMA_VAR_ALIGN_SHIFT	8
#define MPC5xxx_SDMA_FDT_SIZE		128
#define MPC5xxx_SDMA_FDT_ALIGN_SHIFT	8
#define MPC5xxx_SDMA_CSAVE_SIZE		256
#define MPC5xxx_SDMA_CSAVE_ALIGN_SHIFT	8

#define MPC5xxx_SDMA_TASK_ENABLE		0x8000

void mpc5xxx_init_irq(void);
int mpc5xxx_get_irq(struct pt_regs *regs);
void mpc5xxx_pci_init_windows(u8 win0_ctrl, u32 win0_translation,
			    u8 win1_ctrl, u32 win1_translation,
			    u8 win2_ctrl, u32 win2_translation);

#if defined(CONFIG_GLACIER)
#include <platforms/glacier.h>
#elif defined(CONFIG_ICECUBE)
#include <platforms/icecube.h>
#endif

#endif /* __ASSEMBLY__ */

#endif /* __ASM_MPC5XXX_H */
