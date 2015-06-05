/*
 * FILE NAME vitgenio.c
 *
 * BRIEF MODULE DESCRIPTION
 *	Multi-purpose I/O for the VITESSE-Generic board.
 *
 *  Author: Vitesse Semiconductor Corp.
 *          Morten Broerup <mbr@vitesse.com>
 *
 *                   Copyright Notice for Customer Examples
 * 
 * Copyright (c) 2001-2002 Vitesse Semiconductor Corporation. All Rights Reserved.
 * Unpublished rights reserved under the copyright laws of the United States of
 * America, other countries and international treaties. The software is provided
 * without fee. Permission to use, copy, store, modify, disclose, transmit or
 * distribute the software is granted, provided that this copyright notice must
 * appear in any copy, modification, disclosure, transmission or distribution of
 * the software. Vitesse Semiconductor Corporation retains all ownership, 
 * copyright, trade secret and proprietary rights in the software. THIS SOFTWARE
 * HAS BEEN PROVIDED "AS IS", WITHOUT EXPRESS OR IMPLIED WARRANTY INCLUDING, 
 * WITHOUT LIMITATION, IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR USE AND NON-INFRINGEMENT.
 */

#include <linux/module.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h> /* VITGENIO_MINOR */
#include <linux/init.h> /* module_init/module_exit */
#include <linux/delay.h> /* udelay */
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/machdep.h>
#include <linux/vitgenio.h>
#include <asm/semaphore.h>
#include <linux/slab.h>
#include <asm/system.h>
#include <linux/mm.h>
#include <linux/ioport.h>

/*
 * Define NDELAY_USING_UDELAY to use the ordinary udelay().
 * Undefine NDELAY_USING_UDELAY to use __const_udelay() from /include/asm-ppc/delay.h.
 */
#define NDELAY_USING_UDELAY

#define LOOP_LIMIT 1
#ifdef LOOP_LIMIT
#define LOOP_MAX  (unsigned long) 65000
#define LOOP_DEF unsigned long lloop = 0
#define LOOP_NITEST (lloop < (LOOP_MAX))
#define LOOP_DO (++lloop , LOOP_NITEST)
#else
#define LOOP_DEF
#define LOOP_DO 1
#define LOOP_PRINT(dmark, dvl)
#endif

/* ----------------------------------------------------------------- */
/*  Data common for the entire driver */

void *vitgen_cpld_base; /* this is also the base address of the FPGA */
void *vitgen_fpga_base; /* this is the base address of the 32bit FPGA */
void *vitgen_cs2_target_base;
void *vitgen_cs3_target_base;
void *vitgen_cs6_target_base;
void *vitgen_cs7_target_base;

static unsigned int vitgen_cs2_target_buswidth = 0;
static unsigned int vitgen_cs3_target_buswidth = 0;
static unsigned int vitgen_cs6_target_buswidth = 0;
static unsigned int vitgen_cs7_target_buswidth = 16;

const static struct vitgenio_version version = { "$Id: vitgenio.c,v 1.19 2003/03/05 13:21:31 mbr Exp $" };

static struct semaphore sem_spi_readwrite_mutex;


/* ----------------------------------------------------------------- */
/*  Data per open device */

struct vitgenio_fpga_spi_bin_setup {
    unsigned long fpga_bin_ctrl;
};

struct vitgenio_cs_target_setup {
    unsigned int    cs_select;
};

struct vitgenio_dev {
    struct vitgenio_cpld_spi_setup cpld_spi_setup;
    struct vitgenio_fpga_spi_bin_setup fpga_spi_bin_setup;
    struct vitgenio_cs_target_setup cs_target_setup;
};


/* ----------------------------------------------------------------- */
/*  Misc. functions */

static void ndelay(const unsigned int nanoseconds)
{
    if (nanoseconds==0) return;
#ifdef NDELAY_USING_UDELAY
    udelay((nanoseconds+999)/1000);
#else
    /* The parameter to __const_udelay() is a fixed-point number with
     * the binary point to the left of the most-significant bit.
     * Note that 2199/512 ca. == 2^32 / 10^9, multiplying by 2199/512 converts from
     * nanoseconds to a 32-bit fixed-point number of seconds.
     * (See /include/asm-ppc/delay.h for clarification.)
     */
    if (nanoseconds>1000000) {
        /* printk( KERN_WARNING "vitgenio: ndelay(%d) delay too long.\n", nanoseconds ); */
        udelay((nanoseconds+999)/1000);
    } else {
        __const_udelay((nanoseconds*2199+511)/512);
    }
#endif
}


/* ----------------------------------------------------------------- */
/*  SPI using CPLD */

const static struct vitgenio_cpld_spi_setup cpld_spi_setup_default = {
    VITGENIO_SPI_SS_NONE, /* char ss_select; Which of the CPLD_GPIOs is used for Slave Select */
    VITGENIO_SPI_SS_ACTIVE_HIGH, /* char ss_activelow; Slave Select (Chip Select) active low: true, active high: false */
    VITGENIO_SPI_CPOL_0, /* char sck_activelow; CPOL=0: false, CPOL=1: true */
    VITGENIO_SPI_CPHA_0, /* char sck_phase_early; CPHA=0: false, CPHA=1: true */
    VITGENIO_SPI_LSBIT_FIRST, /* char bitorder_msbfirst; */
    VITGENIO_SPI_LSBYTE_FIRST, /* char byteorder_msbfirst; */
    0, /* char reserved1; currently unused, only here for alignment purposes */
    0, /* char reserved2; currently unused, only here for alignment purposes */
    0  /* unsigned int ndelay; minimum delay in nanoseconds, one of these delays are used per clock phase */
};

#define CPLD_SPI_IN_MISO        (1<<0)

#define CPLD_SPI_OUT_SCK        (1<<0)
#define CPLD_SPI_OUT_MOSI       (1<<1)
#define CPLD_SPI_OUT_OE         (1<<2)
#define CPLD_SPI_OUT_CS0        (1<<3)

#define CPLD_SPI_OUT_SCK_ACT    (cpld_spi_setup->sck_activelow?0:CPLD_SPI_OUT_SCK)
#define CPLD_SPI_OUT_SCK_INACT  (cpld_spi_setup->sck_activelow?CPLD_SPI_OUT_SCK:0)

static void cpld_spi_output(const __u8 value, const __u8 mask)
{
    CPLD_SPI_OUT = (CPLD_SPI_OUT & (0xFF^mask)) | (value & mask);
    eieio();
}

static __u8 cpld_spi_input(void)
{
    return (CPLD_SPI_IN & CPLD_SPI_IN_MISO) ? 1 : 0;
}

static void cpld_spi_ss(const int output, const struct vitgenio_cpld_spi_setup *const cpld_spi_setup)
{
    if (cpld_spi_setup->ss_select >= VITGENIO_SPI_SS_CPLD_GPIO0 &&
        cpld_spi_setup->ss_select <= VITGENIO_SPI_SS_CPLD_GPIO10) {
        __u16 mask, value;
        mask = 1 << (cpld_spi_setup->ss_select-VITGENIO_SPI_SS_CPLD_GPIO0);
        value = output ? mask : 0;
        CPLD_GPIO_OUT = (CPLD_GPIO_OUT & (0xFFFF^mask)) | (value & mask);
        eieio();
    }
    else if (cpld_spi_setup->ss_select == VITGENIO_SPI_SS_CS0) {
        __u8 mask, value;
        mask = CPLD_SPI_OUT_CS0;
        value = output ? CPLD_SPI_OUT_CS0 : 0;
        cpld_spi_output(value, mask);
    }
}

static int cpld_spi_transferbit(const int tx, const struct vitgenio_cpld_spi_setup *const cpld_spi_setup)
{
    int result;

    if (cpld_spi_setup->sck_phase_early) {
        cpld_spi_output(CPLD_SPI_OUT_SCK_ACT,CPLD_SPI_OUT_SCK);   /* CPHA=1 -> set CLOCK active */
    }
    cpld_spi_output(tx?CPLD_SPI_OUT_MOSI:0,CPLD_SPI_OUT_MOSI);    /* setup WRITE value */
    ndelay( cpld_spi_setup->ndelay );                             /*   wait t_wph1 */
    result = (cpld_spi_input() & CPLD_SPI_IN_MISO) ? 1 : 0;       /* get READ value */
    if (cpld_spi_setup->sck_phase_early) {
        cpld_spi_output(CPLD_SPI_OUT_SCK_INACT,CPLD_SPI_OUT_SCK); /* CPHA=1 -> set CLOCK inactive */
    } else {
        cpld_spi_output(CPLD_SPI_OUT_SCK_ACT,CPLD_SPI_OUT_SCK);   /* CPHA=0 -> set CLOCK active */
    }
    ndelay( cpld_spi_setup->ndelay );                             /*   wait t_wph2 */
    if (!cpld_spi_setup->sck_phase_early) {
        cpld_spi_output(CPLD_SPI_OUT_SCK_INACT,CPLD_SPI_OUT_SCK); /* CPHA=0 -> set CLOCK inactive */
    }
    return result;
}

static __u8 cpld_spi_transferbyte(const __u8 tx, const struct vitgenio_cpld_spi_setup *const cpld_spi_setup)
{
    int i;
    __u8 incoming, outgoing;

    outgoing = tx;
    for ( i = 0; i < 8; i++ ) {
        if (cpld_spi_setup->bitorder_msbfirst) {
            incoming = (incoming<<1) | (cpld_spi_transferbit(outgoing & 0x80, cpld_spi_setup) ? 0x01 : 0);
            outgoing = outgoing<<1;
        } else {
            incoming = (incoming>>1) | (cpld_spi_transferbit(outgoing & 0x01, cpld_spi_setup) ? 0x80 : 0);
            outgoing = outgoing>>1;
        }
    }
    return incoming;
}


/* ----------------------------------------------------------------- */
/*  SPI using FPGA */

/* defined by JSA 2002-05-30 */
#define HIGH_SPEED_FPGA_ID  0x8001

#define VITGENIO_FPGA_SPI_DONT_START  0
#define VITGENIO_FPGA_SPI_START       1

/* For informational purposes
#define VITGENIO_FPGA_SPI_ADDR_SHIFT         0
#define VITGENIO_FPGA_SPI_SUBB_SHIFT         8
#define VITGENIO_FPGA_SPI_RW_SHIFT          12 
#define VITGENIO_FPGA_SPI_BLOCK_SHIFT       13
#define VITGENIO_FPGA_SPI_SS_SHIFT          16
#define VITGENIO_FPGA_SPI_PADD_SHIFT        24
#define VITGENIO_FPGA_SPI_OE_SHIFT          26
#define VITGENIO_FPGA_SPI_CLOCK_SHIFT       27
*/

/* cpu -> fpga */
#define VITGENIO_FPGA_SPI_CTRL_SS(fss)        ( 1 << ((fss)- VITGENIO_FPGA_SPI_SS_GPIO0 + 16 ))
#define VITGENIO_FPGA_SPI_CTRL_PADD(fpadd)    (  ((fpadd) & 0x03) << 24 )
#define VITGENIO_FPGA_SPI_CTRL_OE(foe)        (  (  (foe) & 0x01) << 26 )
#define VITGENIO_FPGA_SPI_CTRL_CLOCK(fclk)    (  ( (fclk) & 0x01) << 27 )
#define VITGENIO_FPGA_SPI_CTRL_DOUBLE(fdbl)   (  ( (fdbl) & 0x01) << 28 )
#define VITGENIO_FPGA_SPI_CTRL_START(fstrt)   (  ((fstrt) & 0x01) << 29 )

/* fpga -> cpu */
#define VITGENIO_FPGA_SPI_CTRL_DRDY           (1 << 30)
#define VITGENIO_FPGA_SPI_CTRL_RUNN           (1 << 31)


#define VITGENIO_FPGA_SPI_CTRL(fss,fpadd,foe,fclk,fstrt,fdbl) (\
            VITGENIO_FPGA_SPI_CTRL_SS(fss)     | \
            VITGENIO_FPGA_SPI_CTRL_PADD(fpadd) | \
            VITGENIO_FPGA_SPI_CTRL_OE(foe)     | \
            VITGENIO_FPGA_SPI_CTRL_CLOCK(fclk) | \
            VITGENIO_FPGA_SPI_CTRL_START(fstrt)| \
            VITGENIO_FPGA_SPI_CTRL_DOUBLE(fdbl) )
const static struct vitgenio_fpga_spi_setup fpga_spi_setup_default = {
    VITGENIO_SPI_SS_NONE, /* char ss_select; Which of the CPLD_GPIOs is used for Slave Select */
    VITGENIO_FPGA_SPI_SCK_SPEED_SLOW, /* char sck_speed_mode; Clock speed mode */
    VITGENIO_FPGA_SPI_PADDING_0BYTE, /* char sck_pause_mode; Pause/padding length and mode */
    4
};



/*
  FPGA SPI interface
  Data is written to 0x24 MSB clocket out first

  Ctrl is written to 0x28. 

  bit  0 -  7: Address
  bit  8 - 11: Subblock
  bit      12: Read/write (read=0, write=1)
  bit 13 - 15: Block

  bit 16 - 23: CS (max one port enabled) 
  bit 24 - 25: Padding (0: 0byte, 1: 1byte, 2: 2byte, 3: 3byte)
  bit      26: OE - Enable
  bit      27: Clock (0: half->12.5MHz, 1: full->25MHz+)
  bit      28: reserved
  bit      29: Start (clear on write)
  bit      30: Data ready (clear on read)
  bit      31: Running

*/

void copy_fpga_setup(struct vitgenio_fpga_spi_bin_setup *bin_setup, const struct vitgenio_fpga_spi_setup *user_setup) {
    bin_setup->fpga_bin_ctrl = VITGENIO_FPGA_SPI_CTRL(user_setup->ss_select, 
						      user_setup->padding, 
						      1, 
						      user_setup->speed_mode, 
						      1,
						      user_setup->length == 4 ? 0 : 1);
}

/* ----------------------------------------------------------------- */
/*  Target access using Address/Data Bus and Chip Select pins */

const static struct vitgenio_cs_target_setup cs_target_setup_default = {
    VITGENIO_CS7    /* unsigned int    cs_select; */
};


static void *vitgen_cs_target_base(const unsigned int cs)
{
    switch (cs) {
        case VITGENIO_CS2: return vitgen_cs2_target_base;
        case VITGENIO_CS3: return vitgen_cs3_target_base;
        case VITGENIO_CS_FPGA32: return vitgen_fpga_base; /* FPGA 32 bit wide */
        case VITGENIO_CS_CPLD8: return vitgen_cpld_base; /* CPLD/FPGA 8 bit wide */
        case VITGENIO_CS6: return vitgen_cs6_target_base;
        case VITGENIO_CS7: return vitgen_cs7_target_base;
        default: return NULL;
    }
}

static unsigned int vitgen_cs_target_buswidth(const unsigned int cs)
{
    switch (cs) {
        case VITGENIO_CS2: return vitgen_cs2_target_buswidth;
        case VITGENIO_CS3: return vitgen_cs3_target_buswidth;
        case VITGENIO_CS_FPGA32: return 32; /* FPGA 32 bit wide */
        case VITGENIO_CS_CPLD8: return 8;  /* CPLD/FPGA 8 bit wide */
        case VITGENIO_CS6: return vitgen_cs6_target_buswidth;
        case VITGENIO_CS7: return vitgen_cs7_target_buswidth;
        default: return 0;
    }
}

static void *vitgen_cs_target_physical_addr(const unsigned int cs)
{
    switch (cs) {
        case VITGENIO_CS2: return (void*)VITGEN_CS2_TARGET_ADDR;
        case VITGENIO_CS3: return (void*)VITGEN_CS3_TARGET_ADDR;
        case VITGENIO_CS_FPGA32: return (void*)VITGEN_FPGA_ADDR; /* FPGA 32 bit wide */
        case VITGENIO_CS_CPLD8: return (void*)VITGEN_CPLD_ADDR; /* CPLD/FPGA 8 bit wide */
        case VITGENIO_CS6: return (void*)VITGEN_CS6_TARGET_ADDR;
        case VITGENIO_CS7: return (void*)VITGEN_CS7_TARGET_ADDR;
        default: return NULL;
    }
}


/* ----------------------------------------------------------------- */

static int vitgenio_open(struct inode *inode, struct file *file)
{
    struct vitgenio_dev *dev;

    if (!file->private_data) {
        dev = kmalloc( sizeof(struct vitgenio_dev), GFP_KERNEL );
        if (!dev) return -ENOMEM;
        file->private_data = dev;
    } else {
        dev = file->private_data;
    }
    memcpy( &(dev->cpld_spi_setup), &cpld_spi_setup_default, sizeof(struct vitgenio_cpld_spi_setup) );
    copy_fpga_setup(&(dev->fpga_spi_bin_setup), &fpga_spi_setup_default);
    memcpy( &(dev->cs_target_setup), &cs_target_setup_default, sizeof(struct vitgenio_cs_target_setup) );
	return 0;
}

static int vitgenio_release(struct inode *inode, struct file *file)
{
    kfree(file->private_data);
    file->private_data = NULL;
/*	MOD_DEC_USE_COUNT;*/
	return 0;
}

static int vitgenio_ioctl(struct inode *inode, struct file *file,
                          unsigned int cmd, unsigned long arg)
{
    struct vitgenio_dev *const dev = file->private_data;

    /* extract the type and number bitfields, and don't decode
     * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
     */
    if (_IOC_TYPE(cmd)!=VITGENIO_IOC_MAGIC) return -ENOTTY;
/*  if (_IOC_NR(cmd)>VITGENIO_IOC_MAXNR) return -ENOTTY; */

    /* the direction is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. "Type" is user oriented, while
     * access_ok is kernel oriented, so the concept of "read" and
     * "write" is reversed
     */
    if (_IOC_DIR(cmd) & _IOC_READ) {
        if (!access_ok(VERIFY_WRITE,(void *)arg,_IOC_SIZE(cmd))) return -EFAULT;
    }
    else if (_IOC_DIR(cmd)&_IOC_WRITE) {
        if (!access_ok(VERIFY_READ,(void *)arg,_IOC_SIZE(cmd))) return -EFAULT;
    }

    switch (cmd) {
        case VITGENIO_SET_CPLD_CONFIGURATION:
        {
            switch (arg) {
                case VITGENIO_CPLD_CONFIG_GENERIC:
                    CPLD_CONFIG = 0x00;
                    return 0;
                case VITGENIO_CPLD_CONFIG_GPIO2_READY:
                    CPLD_CONFIG = 0x01;
                    return 0;
                default:
                    return -ENOTTY;
            }
        }

        case VITGENIO_CPLD_SPI_SETUP:
        {
            if (__copy_from_user(&(dev->cpld_spi_setup),
                                 (struct vitgenio_cpld_spi_setup*)arg,
                                 sizeof(struct vitgenio_cpld_spi_setup) )) {
                return -EFAULT;
            }
            /* If a CPLD_GPIO pin is selected for SS then set it to Output direction */
            if (dev->cpld_spi_setup.ss_select >= VITGENIO_SPI_SS_CPLD_GPIO0 &&
                dev->cpld_spi_setup.ss_select <= VITGENIO_SPI_SS_CPLD_GPIO10) {
                const __u16 mask = 1 << (dev->cpld_spi_setup.ss_select-VITGENIO_SPI_SS_CPLD_GPIO0);
                /* Set selected CPLD_GPIO pin to Output direction */
                CPLD_GPIO_DIR |= mask;
                eieio();
            }
            /* Set SS inactive */
            if (dev->cpld_spi_setup.ss_select != VITGENIO_SPI_SS_NONE) {
                cpld_spi_ss(dev->cpld_spi_setup.ss_activelow?1:0,&(dev->cpld_spi_setup));
            }
            return 0;
        }

        case VITGENIO_CPLD_SPI_READWRITE:
        {
            unsigned int pos,remain;
            char buffer[16];

            /* Verify that CPLD SPI is enabled */
            if (!(CPLD_SPI_OUT & CPLD_SPI_OUT_OE)) {
                /* OE inactive */
                return -EIO;
            }
            
            /* Enter critical region */
            if (down_interruptible(&sem_spi_readwrite_mutex)) {
                return -ERESTARTSYS;
            }

            /* Set SS active */
            if (dev->cpld_spi_setup.ss_select != VITGENIO_SPI_SS_NONE) {
                ndelay(dev->cpld_spi_setup.ndelay*2);
                cpld_spi_ss(dev->cpld_spi_setup.ss_activelow?0:1,&(dev->cpld_spi_setup));
                ndelay(dev->cpld_spi_setup.ndelay*2);
            }

            /* Do SPI DATA transaction */
            if (__get_user(remain,&(((struct vitgenio_cpld_spi_readwrite*)arg)->length))) {
                /* Leave critical region */
                up(&sem_spi_readwrite_mutex);
                return -EFAULT;
            }
            pos=0;
            while (remain) {
                unsigned int i;
                const unsigned int length = (remain>sizeof(buffer))?sizeof(buffer):remain; /* min(sizeof(buffer),remain) */
                if (__copy_from_user(buffer,
                                     &(((struct vitgenio_cpld_spi_readwrite*)arg)->buffer[pos]),
                                     length)) {
                    /* Leave critical region */
                    up(&sem_spi_readwrite_mutex);
                    return -EFAULT;
                }
                for (i=0; i<length; i++) {
                    buffer[i] = cpld_spi_transferbyte(buffer[i],&(dev->cpld_spi_setup));
                }
                if (__copy_to_user(&(((struct vitgenio_cpld_spi_readwrite*)arg)->buffer[pos]),
                                   buffer,
                                   length)) {
                    /* Leave critical region */
                    up(&sem_spi_readwrite_mutex);
                    return -EFAULT;
                }
                remain -= length;
                pos += length;
            }

            /* Set SS inactive */
            if (dev->cpld_spi_setup.ss_select != VITGENIO_SPI_SS_NONE) {
                ndelay(dev->cpld_spi_setup.ndelay*2);
                cpld_spi_ss(dev->cpld_spi_setup.ss_activelow?1:0,&(dev->cpld_spi_setup));
                ndelay(dev->cpld_spi_setup.ndelay*2);
            }
            /* Leave critical region */
            up(&sem_spi_readwrite_mutex);
            return 0;
        }

        case VITGENIO_ENABLE_CPLD_SPI:
        {
            CPLD_SPI_OUT |= CPLD_SPI_OUT_OE; /* activate OE */
            eieio();
            return 0;
        }

        case VITGENIO_DISABLE_CPLD_SPI:
        {
            CPLD_SPI_OUT &= ~CPLD_SPI_OUT_OE; /* deactivate OE */
            eieio();
            return 0;
        }


        case VITGENIO_GET_HWID:
        {
            return CPLD_HW_ID;
        }

        case VITGENIO_GET_DIPSW:
        {
            const __u8 mask = (arg>>16) & 0xFF;
            return CPLD_DIP_SWITCH & mask;
        }

        case VITGENIO_SET_LED:
        {
            const __u8 mask = (arg>>16) & 0xFF;
            const __u8 value = arg & 0xFF;
            CPLD_LEDS = (CPLD_LEDS & (0xFF^mask)) | (value & mask);
            return 0;
        }

        case VITGENIO_SET_PHY:
        {
            const __u8 mask = (arg>>16) & 0x3F;
            const __u8 value = arg & 0x3F;
            CPLD_PHY_CTRL = (CPLD_PHY_CTRL & (0xFF^mask)) | (value & mask);
            return 0;
        }

        case VITGENIO_SET_EEPROM_WE:
        {
            const __u8 mask = 1 << 7;
            const __u8 value = arg ? 0 : (1<<7); /* arg==true => output 0 to activate EEPROM_nWE */
            CPLD_PHY_CTRL = (CPLD_PHY_CTRL & (0xFF^mask)) | (value & mask);
            return 0;
        }


        case VITGENIO_SET_CPU_LED:
        {
            /* CPU_LED a.k.a. CPU_GPIO[3] */
            const __u32 mask = 1 << (31-3);
            const __u32 value = arg ? 0 : (1<<(31-3)); /* arg==true => output 0 to turn LED on */
            GPIO0_OR = (GPIO0_OR & ~mask) | (value & mask);
            return 0;
        }

        case VITGENIO_GET_CPU_IRQ:
        {
            /* IRQ[0:6] a.k.a. CPU_GPIO[17:23] */
            const __u32 mask = (arg>>16) & 0x7F;
            return (GPIO0_IR >> (31-23)) & mask;
        }
        
        
        case VITGENIO_VERSION:
        {
            if (__copy_to_user((struct vitgenio_version*)arg,
                               &version,
                               sizeof(struct vitgenio_version) )) {
                return -EFAULT;
            }
            return 0;
        }


        case VITGENIO_SET_CPLD_GPIO_DIRECTION:
        {
            /* CPLD_GPIO[10:0] */
            const __u16 mask = (arg>>16) & 0x7FF;
            const __u16 value = arg & 0x7FF;
            CPLD_GPIO_DIR = (CPLD_GPIO_DIR & (0xFFFF^mask)) | (value & mask);
	    eieio();
            return 0;
        }

        case VITGENIO_SET_CPLD_GPIO:
        {
            /* CPLD_GPIO[10:0] */
            const __u16 mask = (arg>>16) & 0x7FF;
            const __u16 value = arg & 0x7FF;
            CPLD_GPIO_OUT = (CPLD_GPIO_OUT & (0xFFFF^mask)) | (value & mask);
	    eieio();
            return 0;
        }

        case VITGENIO_GET_CPLD_GPIO:
        {
            /* CPLD_GPIO[10:0] */
            const __u16 mask = (arg>>16) & 0x7FF;
            return CPLD_GPIO_IN & mask;
        }


        case VITGENIO_SET_CPU_GPIO_1_9_DIRECTION:
        {
            /* CPU_GPIO[1:9], CPU_GPIO9 is LSB */

            __u32 dummy;

            const __u32 mask = ((arg>>16) & 0x1FF) << (31-9);
            const __u32 value = (arg & 0x1FF) << (31-9);
            GPIO0_TCR = (GPIO0_TCR & ~mask) | (value & mask);
            eieio();
            /* ppc405_gpio.c says we must make a dummy read of GPIO0_IR before it is read for real */
            dummy = GPIO0_IR;
            return 0;
        }

        case VITGENIO_SET_CPU_GPIO_1_9:
        {
            /* CPU_GPIO[1:9], CPU_GPIO9 is LSB */
            const __u32 mask = ((arg>>16) & 0x1FF) << (31-9);
            const __u32 value = (arg & 0x1FF) << (31-9);
            GPIO0_OR = (GPIO0_OR & ~mask) | (value & mask);
            return 0;
        }

        case VITGENIO_GET_CPU_GPIO_1_9:
        {
            /* CPU_GPIO[1:9], CPU_GPIO9 is LSB */
            const __u32 mask = (arg>>16) & 0x1FF;
            return (GPIO0_IR >> (31-9)) & mask;
        }


        case VITGENIO_ENABLE_CPLD_MIIM:
        {
            return -ENOTTY;
        }

        case VITGENIO_DISABLE_CPLD_MIIM:
        {
            return -ENOTTY;
        }

        case VITGENIO_SET_CPLD_MIIM:
        {
            return -ENOTTY;
        }

        case VITGENIO_GET_CPLD_MIIM:
        {
            return -ENOTTY;
        }


        case VITGENIO_8BIT_READ:
        {
            const unsigned int cs_select = dev->cs_target_setup.cs_select;
            struct vitgenio_8bit_readwrite readwrite;

            if (vitgen_cs_target_buswidth(cs_select)==0 ||
                vitgen_cs_target_buswidth(cs_select)>8) {
                return -EIO;
            }
            if (__copy_from_user(&readwrite,
                                 (struct vitgenio_8bit_readwrite*)arg,
                                 sizeof(struct vitgenio_8bit_readwrite) )) {
                return -EFAULT;
            }
            readwrite.value = ((__u8 *)(vitgen_cs_target_base(cs_select)))[readwrite.offset];
            if (__put_user(readwrite.value,
                           &(((struct vitgenio_8bit_readwrite*)arg)->value) )) {
                return -EFAULT;
            }
            return 0;
        }

        case VITGENIO_8BIT_WRITE:
        {
            const unsigned int cs_select = dev->cs_target_setup.cs_select;
            struct vitgenio_8bit_readwrite readwrite;

            if (vitgen_cs_target_buswidth(cs_select)==0 ||
                vitgen_cs_target_buswidth(cs_select)>8) {
                return -EIO;
            }
            if (__copy_from_user(&readwrite,
                                 (struct vitgenio_8bit_readwrite*)arg,
                                 sizeof(struct vitgenio_8bit_readwrite) )) {
                return -EFAULT;
            }
            ((__u8 *)(vitgen_cs_target_base(cs_select)))[readwrite.offset] = readwrite.value;
            return 0;
        }

        case VITGENIO_16BIT_READ:
        {
            const unsigned int cs_select = dev->cs_target_setup.cs_select;
            struct vitgenio_16bit_readwrite readwrite;

            if (vitgen_cs_target_buswidth(cs_select)==0 ||
                vitgen_cs_target_buswidth(cs_select)>16) {
                return -EIO;
            }
            if (__copy_from_user(&readwrite,
                                 (struct vitgenio_16bit_readwrite*)arg,
                                 sizeof(struct vitgenio_16bit_readwrite) )) {
                return -EFAULT;
            }
            readwrite.value = ((__u16 *)(vitgen_cs_target_base(cs_select)))[readwrite.offset];
            if (__put_user(readwrite.value,
                           &(((struct vitgenio_16bit_readwrite*)arg)->value) )) {
                return -EFAULT;
            }
            return 0;
        }

        case VITGENIO_16BIT_WRITE:
        {
            const unsigned int cs_select = dev->cs_target_setup.cs_select;
            struct vitgenio_16bit_readwrite readwrite;

            if (vitgen_cs_target_buswidth(cs_select)==0 ||
                vitgen_cs_target_buswidth(cs_select)>16) {
                return -EIO;
            }
            if (__copy_from_user(&readwrite,
                                 (struct vitgenio_16bit_readwrite*)arg,
                                 sizeof(struct vitgenio_16bit_readwrite) )) {
                return -EFAULT;
            }
            ((__u16 *)(vitgen_cs_target_base(cs_select)))[readwrite.offset] = readwrite.value;
            return 0;
        }

        case VITGENIO_32BIT_READ:
        {
            const unsigned int cs_select = dev->cs_target_setup.cs_select;
            struct vitgenio_32bit_readwrite readwrite;

            if (vitgen_cs_target_buswidth(cs_select)==0 ||
                vitgen_cs_target_buswidth(cs_select)>32) {
                return -EIO;
            }
            if (__copy_from_user(&readwrite,
                                 (struct vitgenio_32bit_readwrite*)arg,
                                 sizeof(struct vitgenio_32bit_readwrite) )) {
                return -EFAULT;
            }
            readwrite.value = ((__u32 *)(vitgen_cs_target_base(cs_select)))[readwrite.offset];
            if (__put_user(readwrite.value,
                           &(((struct vitgenio_32bit_readwrite*)arg)->value) )) {
                return -EFAULT;
            }
            return 0;
        }

        case VITGENIO_32BIT_WRITE:
        {
            const unsigned int cs_select = dev->cs_target_setup.cs_select;
            struct vitgenio_32bit_readwrite readwrite;

            if (vitgen_cs_target_buswidth(cs_select)==0 ||
                vitgen_cs_target_buswidth(cs_select)>32) {
                return -EIO;
            }
            if (__copy_from_user(&readwrite,
                                 (struct vitgenio_32bit_readwrite*)arg,
                                 sizeof(struct vitgenio_32bit_readwrite) )) {
                return -EFAULT;
            }
            ((__u32 *)(vitgen_cs_target_base(cs_select)))[readwrite.offset] = readwrite.value;
            if (__put_user(readwrite.value,
                           &(((struct vitgenio_32bit_readwrite*)arg)->value) )) {
                return -EFAULT;
            }
            return 0;
        }

        case VITGENIO_CS_SETUP_TIMING:
        {
            struct vitgenio_cs_setup_timing timing;
            __u32 cr, ap;
            unsigned int buswidth;
            unsigned long flags;
            int errorcode = 0;
            
            if (__copy_from_user(&timing,
                                 (struct vitgenio_cs_setup_timing*)arg,
                                 sizeof(struct vitgenio_cs_setup_timing) )) {
                return -EFAULT;
            }
            if (!vitgen_cs_target_base(timing.cs)) {
                printk( KERN_WARNING "Invalid Chip Select: CS%d\n", timing.cs );
                return -EINVAL;
            }
            switch (timing.bw) {
                case 0: /*fall through intended*/
                case 8:
                    buswidth=8; timing.bw=0;
                    break;
                case 1: /*fall through intended*/
                case 16:
                    buswidth=16; timing.bw=1;
                    break;
                case 2: /*fall through intended*/
                case 32:
                    buswidth=32; timing.bw=2;
                    break;
                default:
                    errorcode = -EINVAL;
            }
            timing.bme &= 1;
            if ( !timing.bme ) {
                if (timing.twt>255) errorcode = -EINVAL;
            } else {
                if (timing.fwt>31) errorcode = -EINVAL;
                if (timing.bwt>7) errorcode = -EINVAL;
                /* Always use TWT, so calculate TWT from FWT and BWT. */
                timing.twt = (timing.fwt<<3) | timing.bwt;
            }
            if (timing.csn>3) errorcode = -EINVAL;
            if (timing.oen>3) errorcode = -EINVAL;
            if (timing.wbn>3) errorcode = -EINVAL;
            if (timing.wbf>3) errorcode = -EINVAL;
            if (timing.th>7) errorcode = -EINVAL;
            timing.re &= 1;
            timing.sor &= 1;
            timing.bem &= 1;
            timing.pen &= 1;

            if (errorcode) {
                printk( KERN_WARNING "Invalid parameters for CS%d\n", timing.cs );
                return errorcode;
            }

            cr = (VITGEN_CS2367_TARGET_BS<<(31-14)) | (3<<(31-16)); /* Bank Size, Bank Usage (Read/Write). */
            switch (timing.cs) {
                case VITGENIO_CS2: cr |= VITGEN_CS2_TARGET_ADDR; vitgen_cs2_target_buswidth=buswidth; break;
                case VITGENIO_CS3: cr |= VITGEN_CS3_TARGET_ADDR; vitgen_cs3_target_buswidth=buswidth; break;
                case VITGENIO_CS6: cr |= VITGEN_CS6_TARGET_ADDR; vitgen_cs6_target_buswidth=buswidth; break;
                case VITGENIO_CS7: cr |= VITGEN_CS7_TARGET_ADDR; vitgen_cs7_target_buswidth=buswidth; break;
            }
            cr |= timing.bw<<(31-18);

            ap = (timing.bme << (31- 0)) |
                 (timing.twt << (31- 8)) |
                 (timing.csn << (31-13)) |
                 (timing.oen << (31-15)) |
                 (timing.wbn << (31-17)) |
                 (timing.wbf << (31-19)) |
                 (timing.th  << (31-22)) |
                 (timing.re  << (31-23)) |
                 (timing.sor << (31-24)) |
                 (timing.bem << (31-25)) |
                 (timing.pen << (31-26)) ;

            /* Set up the bus timing from "ap" and "cr" */
            save_and_cli(flags); /* CLI/STI because we use indirect addressing */
            mtdcr(DCRN_EBCCFGADR,0x10+timing.cs); /* EBC0_BxAP is at offset 0x10+x. */
            mtdcr(DCRN_EBCCFGDATA,ap);
            mtdcr(DCRN_EBCCFGADR,0x00+timing.cs); /* EBC0_BxCR is at offset 0x00+x. */
            mtdcr(DCRN_EBCCFGDATA,cr);
            restore_flags(flags);

            printk( KERN_INFO "vitgenio: CS%d timing changed. "
                    "BW=%d BME=%d TWT=%d FWT=%d BWT=%d CSN=%d OEN=%d WBN=%d WBF=%d TH=%d RE=%d SOR=%d BEM=%d PEN=%d.\n",
                    timing.cs,
                    timing.bw, timing.bme, timing.twt, timing.fwt, timing.bwt,
                    timing.csn, timing.oen, timing.wbn, timing.wbf, timing.th,
                    timing.re, timing.sor, timing.bem, timing.pen );
            return 0;
        }

        case VITGENIO_CS_SELECT:
        {
            if (!vitgen_cs_target_base(arg)) {
                printk( KERN_WARNING "Invalid Chip Select: CS%ud\n", (uint)arg );
                return -EINVAL;
            }
            if (!vitgen_cs_target_buswidth(arg)) {
                printk( KERN_WARNING "Chip Select timing not setup: CS%ud\n", (uint)arg );
                return -EIO;
            }
            dev->cs_target_setup.cs_select = arg;
            return 0;
        }


        case VITGENIO_FPGA_SPI_SETUP:
        {
	    struct vitgenio_fpga_spi_setup fs_setup;
            if (__copy_from_user(&fs_setup,
                                 (struct vitgenio_fpga_spi_setup*)arg,
                                 sizeof(struct vitgenio_fpga_spi_setup) )) {
                return -EFAULT;
            }
	    if (CPLD_HW_ID == 0x03) {
		return -ENODEV; /* Genie HW ID==3 has no FPGA. */
	    }
	    /* currently only 4 or 8 bytes are supported */
	    if (fs_setup.length != 4 && fs_setup.length != 8) {
		printk( KERN_WARNING "Invalid fpga spi block length %d\n", fs_setup.length );
		return -EINVAL;
	    }
	    copy_fpga_setup(&(dev->fpga_spi_bin_setup), &fs_setup);
            return 0;
        }

        case VITGENIO_FPGA_SPI_READWRITE:
        {
            struct vitgenio_fpga_spi_readwrite readwrite;
	    unsigned int read_or_write;
	    uint32_t ctrl;
	    LOOP_DEF;

            if (__copy_from_user(&readwrite,
                                 (struct vitgenio_fpga_spi_readwrite*)arg,
                                 sizeof(struct vitgenio_fpga_spi_readwrite) )) {
                return -EFAULT;
            }
            
	    read_or_write = readwrite.ctrl & VITGENIO_FPGA_SPI_CTRL_RW(1);
            /* Enter critical region */
            if (down_interruptible(&sem_spi_readwrite_mutex)) {
                return -ERESTARTSYS;
            }

            /* Do SPI DATA transaction */
	    do {
		ctrl = FPGA_SPI_CTRL;
		eieio();
	    } while ((ctrl & VITGENIO_FPGA_SPI_CTRL_RUNN) & LOOP_DO);
            if(!LOOP_NITEST) {
                /* Leave critical region */
                up(&sem_spi_readwrite_mutex);
                printk( KERN_WARNING "Write wait loop exceeded max: %d, debug_val=0x%x\n", 
		       (unsigned int) lloop, ctrl);
                return -EBUSY;
	    } else {
                lloop = 0; 
	    }

	    ctrl = (readwrite.ctrl & 0xffff) | dev->fpga_spi_bin_setup.fpga_bin_ctrl;
	    /* write */
	    if (ctrl & VITGENIO_FPGA_SPI_CTRL_RW(1)) {
		FPGA_SPI_DATA1 = readwrite.data[0];
		eieio();
		if (ctrl & VITGENIO_FPGA_SPI_CTRL_DOUBLE(1)) {
		    FPGA_SPI_DATA2 = readwrite.data[1];
		    eieio();
		}
	    }
            FPGA_SPI_CTRL = ctrl;
            eieio();
	    /* read */
	    if (!(ctrl & VITGENIO_FPGA_SPI_CTRL_RW(1))) {
		do {
		    ctrl = FPGA_SPI_CTRL;
		    eieio();
		} while ((!(ctrl & VITGENIO_FPGA_SPI_CTRL_DRDY)) & LOOP_DO);
		if(!LOOP_NITEST) {
		    /* Leave critical region */
		    up(&sem_spi_readwrite_mutex);
		    printk( KERN_WARNING "Read wait loop exceeded max: %d, debug_val=0x%x\n", 
			   (unsigned int) lloop, ctrl);
		    return -EBUSY;
		} else {
		    lloop = 0; 
		}
		readwrite.data[0] = FPGA_SPI_DATA1;
		eieio();
		if (ctrl & VITGENIO_FPGA_SPI_CTRL_DOUBLE(1)) {
		    readwrite.data[1] = FPGA_SPI_DATA2;
		    eieio();
		}
	    }

            /* Leave critical region */
            up(&sem_spi_readwrite_mutex);

            if (__copy_to_user((struct vitgenio_fpga_spi_readwrite*)arg,
                               &readwrite,
                               sizeof(struct vitgenio_fpga_spi_readwrite) )) {
                return -EFAULT;
            }
            return 0;
        }

        case VITGENIO_ENABLE_FPGA_SPI:
        {
	    unsigned long fpga_id, fpga_part;
	    
	    if (CPLD_HW_ID == 0x03) {
		return -ENODEV; /* Genie HW ID==3 has no FPGA. */
	    }

	    fpga_id = FPGA_SPI_ID;
            eieio();
	    fpga_part = fpga_id >> 16;
	    if (fpga_part != HIGH_SPEED_FPGA_ID) {
		return -ENODEV;
	    }
	    
            FPGA_SPI_CTRL |= VITGENIO_FPGA_SPI_CTRL_OE(1); /* activate OE */
            eieio();
            return 0;
        }

        case VITGENIO_DISABLE_FPGA_SPI:
        {
            FPGA_SPI_CTRL &= ~VITGENIO_FPGA_SPI_CTRL_OE(1); /* deactivate OE */
            eieio();
            return 0;
        }


        default:
            return -ENOIOCTLCMD;
    }
    return 0;
}

/* The pgprot_noncached function is taken from /drivers/char/mem.c */
#ifndef pgprot_noncached
/*
 * This should probably be per-architecture in <asm/pgtable.h>
 */
static inline pgprot_t pgprot_noncached(pgprot_t _prot)
{
	unsigned long prot = pgprot_val(_prot);

#if defined(__i386__)
	/* On PPro and successors, PCD alone doesn't always mean 
	    uncached because of interactions with the MTRRs. PCD | PWT
	    means definitely uncached. */ 
	if (boot_cpu_data.x86 > 3)
		prot |= _PAGE_PCD | _PAGE_PWT;
#elif defined(__powerpc__)
	prot |= _PAGE_NO_CACHE | _PAGE_GUARDED;
#elif defined(__mc68000__)
#ifdef SUN3_PAGE_NOCACHE
	if (MMU_IS_SUN3)
		prot |= SUN3_PAGE_NOCACHE;
	else
#endif
	if (MMU_IS_851 || MMU_IS_030)
		prot |= _PAGE_NOCACHE030;
	/* Use no-cache mode, serialized */
	else if (MMU_IS_040 || MMU_IS_060)
		prot = (prot & _CACHEMASK040) | _PAGE_NOCACHE_S;
#elif defined(__mips__)
	prot = (prot & ~_CACHE_MASK) | _CACHE_UNCACHED;
#endif

	return __pgprot(prot);
}
#endif /* !pgprot_noncached */

static int vitgenio_mmap(struct file *file, struct vm_area_struct *vma)
{
    /* The offset parameter of the mmap() system call */
    const unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

    struct vitgenio_dev *const dev = file->private_data;
    const unsigned int cs_select = dev->cs_target_setup.cs_select;
    int result;

    /* Specify non-cached access */
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    /* Don't try to swap out physical pages */
    vma->vm_flags |= VM_RESERVED;
    /* Don't dump addresses that are not real memory to a core file */
    vma->vm_flags |= VM_IO;

    result = remap_page_range(vma->vm_start, (unsigned long)(vitgen_cs_target_physical_addr(cs_select))+offset, vma->vm_end-vma->vm_start, vma->vm_page_prot);
    if (result) {
        printk( KERN_INFO "vitgenio: remap_page_range(virt_addr=0x%08X,phys_addr=0x%08X,size=0x%08X,prot=0x%08lX) failed:%d.\n",
                (uint)vma->vm_start, (uint)((unsigned long)(vitgen_cs_target_physical_addr(cs_select))+offset), (uint)(vma->vm_end-vma->vm_start), pgprot_val(vma->vm_page_prot), result );
        return -EAGAIN;
    }
	return 0;
}

static struct file_operations vitgenio_fops =
{
	owner:		THIS_MODULE,
	ioctl:		vitgenio_ioctl,
	mmap:		vitgenio_mmap,
	open:		vitgenio_open,
	release:	vitgenio_release,
};

static struct miscdevice vitgenio_miscdev =
{
	VITGENIO_MINOR,
	"vitgenio",
	&vitgenio_fops
};

int __init vitgenio_init(void)
{
    int result;

        vitgen_cpld_base = ioremap(EXBITGEN_CPLD_ADDR, EXBITGEN_CPLD_SIZE);
        if(!vitgen_cpld_base){
            printk(KERN_CRIT "vitgenio_init() vitgen_cpld_base ioremap failed\n");
            return -1;
        }
        vitgen_fpga_base = ioremap(EXBITGEN_FPGA_ADDR, EXBITGEN_FPGA_SIZE);
        if(!vitgen_fpga_base){
            printk(KERN_CRIT "vitgenio_init() vitgen_fpga_base ioremap failed\n");
            return -1;
        }

        vitgen_cs2_target_base = ioremap(EXBITGEN_CS2_TARGET_ADDR, EXBITGEN_CS2367_TARGET_SIZE);
        if(!vitgen_cs2_target_base){
            printk(KERN_CRIT "vitgenio_init() vitgen_cs2_target_base ioremap failed\n");
            return -1;
        }
        vitgen_cs3_target_base = ioremap(EXBITGEN_CS3_TARGET_ADDR, EXBITGEN_CS2367_TARGET_SIZE);
        if(!vitgen_cs3_target_base){
            printk(KERN_CRIT "vitgenio_init() vitgen_cs3_target_base ioremap failed\n");
            return -1;
        }
        vitgen_cs6_target_base = ioremap(EXBITGEN_CS6_TARGET_ADDR, EXBITGEN_CS2367_TARGET_SIZE);
        if(!vitgen_cs6_target_base){
            printk(KERN_CRIT "vitgenio_init() vitgen_cs6_target_base ioremap failed\n");
            return -1;
        }
        vitgen_cs7_target_base = ioremap(EXBITGEN_CS7_TARGET_ADDR, EXBITGEN_CS2367_TARGET_SIZE);
        if(!vitgen_cs7_target_base){
            printk(KERN_CRIT "vitgenio_init() vitgen_cs7_target_base ioremap failed\n");
            return -1;
        }

        /* Set PerReady timeout to 2048 PerClk cycles (instead of 16) */
        mtdcr(DCRN_EBCCFGADR,0x23); /* EBC0_CFG is at offset 0x23. */
        mtdcr(DCRN_EBCCFGDATA,0x3FC00000); /* Chip's default after reset is 0x07C00000. */

    printk( KERN_INFO "VITESSE-Generic multi-purpose I/O driver. HWID=0x%02X, DIPSW=0x%02X.\n",
            CPLD_HW_ID, CPLD_DIP_SWITCH);

    result = misc_register(&vitgenio_miscdev);
	if (result) {
        printk( KERN_WARNING "vitgenio: Error %d registering device.\n", result );
	} else {
        printk( KERN_INFO "vitgenio: Device registered.\n" );
	}

    if ((result=check_mem_region(VITGEN_CPLD_ADDR,VITGEN_CPLD_SIZE))<0) {
        printk( KERN_WARNING "vitgenio: Error %d allocating Genie CPLD memory space.\n", result );
    } else {
        request_mem_region(VITGEN_CPLD_ADDR,VITGEN_CPLD_SIZE,"Genie CPLD");
    }
    if ((result=check_mem_region(VITGEN_FPGA_ADDR,VITGEN_FPGA_SIZE))<0) {
        printk( KERN_WARNING "vitgenio: Error %d allocating Genie FPGA memory space.\n", result );
    } else {
        request_mem_region(VITGEN_FPGA_ADDR,VITGEN_FPGA_SIZE,"Genie FPGA");
    }
    if ((result=check_mem_region(VITGEN_CS2_TARGET_ADDR,VITGEN_CS2367_TARGET_SIZE))<0) {
        printk( KERN_WARNING "vitgenio: Error %d allocating Genie CS2 I/O device memory space.\n", result );
    } else {
        request_mem_region(VITGEN_CS2_TARGET_ADDR,VITGEN_CS2367_TARGET_SIZE,"I/O device on Genie CS2");
    }
    if ((result=check_mem_region(VITGEN_CS3_TARGET_ADDR,VITGEN_CS2367_TARGET_SIZE))<0) {
        printk( KERN_WARNING "vitgenio: Error %d allocating Genie CS3 I/O device memory space.\n", result );
    } else {
        request_mem_region(VITGEN_CS3_TARGET_ADDR,VITGEN_CS2367_TARGET_SIZE,"I/O device on Genie CS3");
    }
    if ((result=check_mem_region(VITGEN_CS6_TARGET_ADDR,VITGEN_CS2367_TARGET_SIZE))<0) {
        printk( KERN_WARNING "vitgenio: Error %d allocating Genie CS6 I/O device memory space.\n", result );
    } else {
        request_mem_region(VITGEN_CS6_TARGET_ADDR,VITGEN_CS2367_TARGET_SIZE,"I/O device on Genie CS6");
    }
    if ((result=check_mem_region(VITGEN_CS7_TARGET_ADDR,VITGEN_CS2367_TARGET_SIZE))<0) {
        printk( KERN_WARNING "vitgenio: Error %d allocating Genie CS7 I/O device memory space.\n", result );
    } else {
        request_mem_region(VITGEN_CS7_TARGET_ADDR,VITGEN_CS2367_TARGET_SIZE,"I/O device on Genie CS7");
    }

	/* disable Open Drain function on CPU_GPIO[9:1] */
	GPIO0_ODR = GPIO0_ODR & ~(0x1FF<<(31-9));
	eieio();

#if 0
    /* test only */
	CPLD_LEDS = CPLD_DIP_SWITCH;
#endif

    sema_init(&sem_spi_readwrite_mutex,1);

	return 0;
}

void __exit vitgenio_exit(void)
{
	misc_deregister(&vitgenio_miscdev);
}


EXPORT_NO_SYMBOLS;
module_init(vitgenio_init);
module_exit(vitgenio_exit);


