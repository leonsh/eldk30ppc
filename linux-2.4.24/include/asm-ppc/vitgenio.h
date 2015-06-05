/*
 * FILE NAME vitgenio.h
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

#ifndef __VITGENIO_H
#define __VITGENIO_H

#ifdef __KERNEL__

/* ----------------------------------------------------------------- */
/*  Physical addresses                                               */
/* ----------------------------------------------------------------- */

#include <linux/types.h>
#include <asm/ibm4xx.h>

#define PPC4xx_ONB_IO_ADDR	PPC4xx_ONB_IO_PADDR

#define GPIO0_OR	(*(volatile __u32 *)(PPC4xx_ONB_IO_ADDR|0x700))
#define GPIO0_TCR	(*(volatile __u32 *)(PPC4xx_ONB_IO_ADDR|0x704))
#define GPIO0_ODR	(*(volatile __u32 *)(PPC4xx_ONB_IO_ADDR|0x718))
#define GPIO0_IR	(*(volatile __u32 *)(PPC4xx_ONB_IO_ADDR|0x71C))

extern void *vitgen_cpld_base; /* this is also the base address of the FPGA */
extern void *vitgen_fpga_base; /* this is the 32bit base address of the FPGA */
extern void *vitgen_cs2_target_base;
extern void *vitgen_cs3_target_base;
extern void *vitgen_cs6_target_base;
extern void *vitgen_cs7_target_base;

#define CPLD_CONFIG     (*(volatile __u8 *)(vitgen_cpld_base + 0x00))
#define CPLD_LEDS       (*(volatile __u8 *)(vitgen_cpld_base + 0x01))
#define CPLD_HW_ID      (*(volatile __u8 *)(vitgen_cpld_base + 0x02))
#define CPLD_DIP_SWITCH (*(volatile __u8 *)(vitgen_cpld_base + 0x04))
#define CPLD_PHY_CTRL   (*(volatile __u8 *)(vitgen_cpld_base + 0x05))
#define CPLD_SPI_OUT    (*(volatile __u8 *)(vitgen_cpld_base + 0x07))
#define CPLD_SPI_IN     (*(volatile __u8 *)(vitgen_cpld_base + 0x08))
#define CPLD_MIIM_OUT   (*(volatile __u8 *)(vitgen_cpld_base + 0x09))
#define CPLD_MIIM_IN    (*(volatile __u8 *)(vitgen_cpld_base + 0x0A))
#define CPLD_GPIO_OUT   (*(volatile __u16 *)(vitgen_cpld_base + 0x0C))
#define CPLD_GPIO_IN    (*(volatile __u16 *)(vitgen_cpld_base + 0x0E))
#define CPLD_GPIO_DIR   (*(volatile __u16 *)(vitgen_cpld_base + 0x10))

/* "Narrow" (8bit) FPGA defines */
/* Not used
#define FPGA_SPI_CTRL   (*(volatile __u8 *)(vitgen_cpld_base + 0x26))
#define FPGA_SPI_OUT    (*(volatile __u8 *)(vitgen_cpld_base + 0x27))
#define FPGA_SPI_DATA11 (*(volatile __u8 *)(vitgen_cpld_base + 0x30))
#define FPGA_SPI_DATA12 (*(volatile __u8 *)(vitgen_cpld_base + 0x31))
#define FPGA_SPI_DATA13 (*(volatile __u8 *)(vitgen_cpld_base + 0x32))
#define FPGA_SPI_DATA14 (*(volatile __u8 *)(vitgen_cpld_base + 0x33))
#define FPGA_SPI_DATA15 (*(volatile __u8 *)(vitgen_cpld_base + 0x34))
#define FPGA_SPI_DATA16 (*(volatile __u8 *)(vitgen_cpld_base + 0x35))
#define FPGA_SPI_DATA21 (*(volatile __u8 *)(vitgen_cpld_base + 0x36))
#define FPGA_SPI_DATA22 (*(volatile __u8 *)(vitgen_cpld_base + 0x37))
#define FPGA_SPI_DATA23 (*(volatile __u8 *)(vitgen_cpld_base + 0x38))
#define FPGA_SPI_DATA24 (*(volatile __u8 *)(vitgen_cpld_base + 0x39))
#define FPGA_SPI_DATA25 (*(volatile __u8 *)(vitgen_cpld_base + 0x3A))
#define FPGA_SPI_DATA26 (*(volatile __u8 *)(vitgen_cpld_base + 0x3B))
*/

/* "Wide" (32bit) FPGA defines */

#define FPGA_SPI_CTRL  (*(volatile uint32_t *)(vitgen_fpga_base + 0x28))
#define FPGA_SPI_ID    (*(volatile uint32_t *)(vitgen_fpga_base + 0x3c))
#define FPGA_SPI_DATA1 (*(volatile uint32_t *)(vitgen_fpga_base + 0x24))
#define FPGA_SPI_DATA2 (*(volatile uint32_t *)(vitgen_fpga_base + 0x3c))
/* Long story */
/* We're forced to use 0x3c twice since FPGA address space is in limited 
   supply.
   Functionality of 0x3c is determined by 'last transaction'. If LT was
   a double read then 0x3c contains last four bytes, otherwise chip ID 
*/

#endif /* __KERNEL__ */

#include <linux/ioctl.h>

#define VITGENIO_IOC_MAGIC 'G'

/* ----------------------------------------------------------------- */
/*  CPLD Configuration                                               */
/* ----------------------------------------------------------------- */

/* enumeration */
#define VITGENIO_CPLD_CONFIG_GENERIC      0
#define VITGENIO_CPLD_CONFIG_GPIO2_READY  1 /* Connect CPLD GPIO2 pin to CPU PerReady pin (the CPLD inverts the signal) */

#define VITGENIO_SET_CPLD_CONFIGURATION   _IO(VITGENIO_IOC_MAGIC,0)  /* arg=value */

/* ----------------------------------------------------------------- */
/*  SPI using CPLD                                                   */
/* ----------------------------------------------------------------- */

/* enumeration */
#define VITGENIO_SPI_SS_CPLD_GPIO0  0
#define VITGENIO_SPI_SS_CPLD_GPIO1  1
#define VITGENIO_SPI_SS_CPLD_GPIO2  2
#define VITGENIO_SPI_SS_CPLD_GPIO3  3
#define VITGENIO_SPI_SS_CPLD_GPIO4  4
#define VITGENIO_SPI_SS_CPLD_GPIO5  5
#define VITGENIO_SPI_SS_CPLD_GPIO6  6
#define VITGENIO_SPI_SS_CPLD_GPIO7  7
#define VITGENIO_SPI_SS_CPLD_GPIO8  8
#define VITGENIO_SPI_SS_CPLD_GPIO9  9
#define VITGENIO_SPI_SS_CPLD_GPIO10 10
#define VITGENIO_SPI_SS_CS0         0x40
#define VITGENIO_SPI_SS_NONE        (-1)

#define VITGENIO_SPI_SS_ACTIVE_HIGH 0
#define VITGENIO_SPI_SS_ACTIVE_LOW  1
#define VITGENIO_SPI_CPOL_0         0
#define VITGENIO_SPI_CPOL_1         1
#define VITGENIO_SPI_CPHA_0         0
#define VITGENIO_SPI_CPHA_1         1
#define VITGENIO_SPI_LSBIT_FIRST    0
#define VITGENIO_SPI_MSBIT_FIRST    1
#define VITGENIO_SPI_LSBYTE_FIRST   0
#define VITGENIO_SPI_MSBYTE_FIRST   1

struct vitgenio_cpld_spi_setup {
    char ss_select;         /* Which of the CPLD_GPIOs is used for Slave Select */
    char ss_activelow;      /* Slave Select (Chip Select) active low: true, active high: false */
    char sck_activelow;     /* CPOL=0: false, CPOL=1: true */
    char sck_phase_early;   /* CPHA=0: false, CPHA=1: true */
    char bitorder_msbfirst;
    char reserved1;         /* currently unused, only here for alignment purposes */
    char reserved2;         /* currently unused, only here for alignment purposes */
    char reserved3;         /* currently unused, only here for alignment purposes */
    unsigned int ndelay;    /* minimum delay in nanoseconds, one of these delays are used per clock phase */
};

#define VITGENIO_CPLD_SPI_SETUP           _IOW(VITGENIO_IOC_MAGIC,1,struct vitgenio_cpld_spi_setup)

struct vitgenio_cpld_spi_readwrite {
    unsigned int length;  /* number of bytes in transaction */
    char buffer[16];    /* ioctl() calls with smaller or larger buffer are valid */
};

#define VITGENIO_CPLD_SPI_READWRITE       _IOWR(VITGENIO_IOC_MAGIC,2,struct vitgenio_cpld_spi_readwrite)

#define VITGENIO_ENABLE_CPLD_SPI          _IO(VITGENIO_IOC_MAGIC,3)
#define VITGENIO_DISABLE_CPLD_SPI         _IO(VITGENIO_IOC_MAGIC,4)

/* ----------------------------------------------------------------- */
/*  Misc.                                                            */
/* ----------------------------------------------------------------- */

#define VITGENIO_GET_HWID                 _IO(VITGENIO_IOC_MAGIC,10) /* returns CPLD_HWID[7:0], CPLD_HWID0 is LSB */
#define VITGENIO_GET_DIPSW                _IO(VITGENIO_IOC_MAGIC,11) /* arg=(mask<<16), returns CPLD_DIPSW[7:0], CPLD_DIPSW0 is LSB */
#define VITGENIO_SET_LED                  _IO(VITGENIO_IOC_MAGIC,12) /* arg=(mask<<16)|value */
#define VITGENIO_SET_PHY                  _IO(VITGENIO_IOC_MAGIC,13) /* arg=(mask<<16)|value, arg includes OE bit */
#define VITGENIO_SET_EEPROM_WE            _IO(VITGENIO_IOC_MAGIC,14) /* value=(arg?1:0), (value=1 means output active, i.e. LOW signal) */

#define VITGENIO_SET_CPU_LED              _IO(VITGENIO_IOC_MAGIC,20) /* value=(arg?1:0), (a.k.a. CPU_GPIO[3]) */
#define VITGENIO_GET_CPU_IRQ              _IO(VITGENIO_IOC_MAGIC,21) /* arg=mask<<16, returns IRQ[0:6], (a.k.a. CPU_GPIO[17:23]), IRQ6 is LSB */

struct vitgenio_version {
    char buffer[256];
};
/* Returns zero-terminated string. Do not interpret. Format of string may change. */
#define VITGENIO_VERSION                  _IOR(VITGENIO_IOC_MAGIC,25,struct vitgenio_version)

/* ----------------------------------------------------------------- */
/*  CPLD_GPIO                                                        */
/* ----------------------------------------------------------------- */

#define VITGENIO_SET_CPLD_GPIO_DIRECTION  _IO(VITGENIO_IOC_MAGIC,30) /* arg=(mask<<16)|value, (value=1 means output) */
#define VITGENIO_SET_CPLD_GPIO            _IO(VITGENIO_IOC_MAGIC,31) /* arg=(mask<<16)|value */
#define VITGENIO_GET_CPLD_GPIO            _IO(VITGENIO_IOC_MAGIC,32) /* arg=(mask<<16), returns CPLD_GPIO[10:0], CPLD_GPIO0 is LSB */

/* ----------------------------------------------------------------- */
/*  CPU_GPIO                                                         */
/* ----------------------------------------------------------------- */

#define VITGENIO_SET_CPU_GPIO_1_9_DIRECTION _IO(VITGENIO_IOC_MAGIC,40) /* arg=(mask<<16)|value, (value=1 means output) */
#define VITGENIO_SET_CPU_GPIO_1_9         _IO(VITGENIO_IOC_MAGIC,41) /* arg=(mask<<16)|value */
#define VITGENIO_GET_CPU_GPIO_1_9         _IO(VITGENIO_IOC_MAGIC,42) /* arg=(mask<<16), returns CPU_GPIO[1:9], CPU_GPIO9 is LSB */

/* ----------------------------------------------------------------- */
/*  CPLD_MIIM a.k.a. CPLD_MDIO                                       */
/* ----------------------------------------------------------------- */

#define VITGENIO_ENABLE_CPLD_MIIM         _IO(VITGENIO_IOC_MAGIC,50) /* arg=minimum delay in microseconds, timing t.b.d. */
#define VITGENIO_DISABLE_CPLD_MIIM        _IO(VITGENIO_IOC_MAGIC,51)
#define VITGENIO_SET_CPLD_MIIM            _IO(VITGENIO_IOC_MAGIC,52) /* arg=(phy_address<<(16+5))|(phy_register<<16)|value */
#define VITGENIO_GET_CPLD_MIIM            _IO(VITGENIO_IOC_MAGIC,53) /* arg=(phy_address<<5)|phy_register, returns result */

/* ----------------------------------------------------------------- */
/*  Parallel addressing of Target                                    */
/* ----------------------------------------------------------------- */

struct vitgenio_8bit_readwrite {
    unsigned long   offset;     /* offset (in chars) from vitgen_8bit_target_base */
    unsigned char   value;
    unsigned char   reserved1;  /* currently unused, only here for alignment purposes */
    unsigned char   reserved2;  /* currently unused, only here for alignment purposes */
    unsigned char   reserved3;  /* currently unused, only here for alignment purposes */
};

struct vitgenio_16bit_readwrite {
    unsigned long   offset;     /* offset (in shorts) from vitgen_16bit_target_base */
    unsigned short  value;
    unsigned short  reserved1;  /* currently unused, only here for alignment purposes */
};

struct vitgenio_32bit_readwrite {
    unsigned long   offset;     /* offset (in longs) from vitgen_32bit_target_base */
    unsigned long   value;
};

#define VITGENIO_8BIT_READ                _IOWR(VITGENIO_IOC_MAGIC,66,struct vitgenio_16bit_readwrite)
#define VITGENIO_8BIT_WRITE               _IOW(VITGENIO_IOC_MAGIC,67,struct vitgenio_16bit_readwrite)
#define VITGENIO_16BIT_READ               _IOWR(VITGENIO_IOC_MAGIC,60,struct vitgenio_16bit_readwrite)
#define VITGENIO_16BIT_WRITE              _IOW(VITGENIO_IOC_MAGIC,61,struct vitgenio_16bit_readwrite)
#define VITGENIO_32BIT_READ               _IOWR(VITGENIO_IOC_MAGIC,62,struct vitgenio_32bit_readwrite)
#define VITGENIO_32BIT_WRITE              _IOW(VITGENIO_IOC_MAGIC,63,struct vitgenio_32bit_readwrite)

/* enumeration */
#define VITGENIO_CS2 2
#define VITGENIO_CS3 3
#define VITGENIO_CS_FPGA32 4 /* FPGA 32 bit wide */
#define VITGENIO_CS_CPLD8  5 /* CPLD/FPGA 8 bit wide */
#define VITGENIO_CS6 6
#define VITGENIO_CS7 7

struct vitgenio_cs_setup_timing {
    unsigned int cs;        /* Which of the PowerPC Chip Selects (2,3,6 or 7) is affected */
    unsigned int bw;        /* Bus Width: bme=0,1,2 (aliases 8,16,32 also accepted) */
    unsigned int bme;       /* Burst Mode Enable. */
    unsigned int twt;       /* Transfer Wait. Only used when bme=0. */
    unsigned int fwt;       /* First Wait. Only used when bme=1. */
    unsigned int bwt;       /* Burst Wait. Only used when bme=1. */
    unsigned int csn;
    unsigned int oen;
    unsigned int wbn;
    unsigned int wbf;
    unsigned int th;
    unsigned int re;
    unsigned int sor;
    unsigned int bem;
    unsigned int pen;
};

#define VITGENIO_CS_SETUP_TIMING          _IOW(VITGENIO_IOC_MAGIC,64,struct vitgenio_cs_setup_timing)
#define VITGENIO_CS_SELECT                _IO(VITGENIO_IOC_MAGIC,65) /* arg=VITGENIO_CSx */
/* Defined above
#define VITGENIO_8BIT_READ                _IOWR(VITGENIO_IOC_MAGIC,66,struct vitgenio_8bit_readwrite)
#define VITGENIO_8BIT_WRITE               _IOW(VITGENIO_IOC_MAGIC,67,struct vitgenio_8bit_readwrite)
*/

/* ----------------------------------------------------------------- */
/*  SPI using FPGA                                                   */
/* ----------------------------------------------------------------- */

/* error codes */
#define VITGENIO_FPGA_NOT_PRESENT   1

/* bit fields */

#define VITGENIO_FPGA_SPI_RW_READ    0
#define VITGENIO_FPGA_SPI_RW_WRITE   1

#define VITGENIO_FPGA_SPI_SS_GPIO0  0
#define VITGENIO_FPGA_SPI_SS_GPIO1  1
#define VITGENIO_FPGA_SPI_SS_GPIO2  2
#define VITGENIO_FPGA_SPI_SS_GPIO3  3
#define VITGENIO_FPGA_SPI_SS_GPIO4  4
#define VITGENIO_FPGA_SPI_SS_GPIO5  5
#define VITGENIO_FPGA_SPI_SS_GPIO6  6
#define VITGENIO_FPGA_SPI_SS_GPIO7  7

#define VITGENIO_FPGA_SPI_PADDING_0BYTE   0
#define VITGENIO_FPGA_SPI_PADDING_1BYTE   1
#define VITGENIO_FPGA_SPI_PADDING_2BYTE   2
#define VITGENIO_FPGA_SPI_PADDING_3BYTE   3

#define VITGENIO_FPGA_SPI_OE_DISABLE   0
#define VITGENIO_FPGA_SPI_OE_ENABLE    1

#define VITGENIO_FPGA_SPI_SCK_SPEED_SLOW   0
#define VITGENIO_FPGA_SPI_SCK_SPEED_FAST   1 /* Clock comes from external clock generator. Clock must be 100kHz or faster. */

#define VITGENIO_FPGA_SPI_CTRL_ADDR(faddr)    (  ((faddr) & 0xff) <<  0 )
#define VITGENIO_FPGA_SPI_CTRL_SUBB(fsubb)    (  ((fsubb) & 0x0f) <<  8 )
#define VITGENIO_FPGA_SPI_CTRL_RW(frw)        (  (  (frw) & 0x01) << 12 ) 
#define VITGENIO_FPGA_SPI_CTRL_BLOCK(fblk)    (  ( (fblk) & 0x07) << 13 )

#if 0
#define VITGENIO_FPGA_SPI_USER(faddr,fsubb,frw,fblk) (\
            VITGENIO_FPGA_SPI_CTRL_ADDR(faddr) | \
            VITGENIO_FPGA_SPI_CTRL_SUBB(fsubb) | \
            VITGENIO_FPGA_SPI_CTRL_RW(frw)     | \
            VITGENIO_FPGA_SPI_CTRL_BLOCK(fblk))
#endif



/* This is the user struct */
struct vitgenio_fpga_spi_setup {
    char ss_select;         /* Which of the GPIOs is used for Slave Select */
    char speed_mode;        /* Internal or externally generated clock */
    char padding;           /* Padding length */
    char length;            /* transaction data length - only 4 or 8 is supported */
};

#define VITGENIO_FPGA_SPI_SETUP           _IOW(VITGENIO_IOC_MAGIC,70,struct vitgenio_fpga_spi_setup)

/*  bit27:    clock, 
    bit26:    OE, 
    bit25-24: padding
    bit23-16: one bit for each GPIO
    bit15-13: block, 
    bit12:    _write_/read, 
    bit11-8:  subblock
    bit7-0:   address */

struct vitgenio_fpga_spi_readwrite {
    unsigned int ctrl;    /* bit 8-15 block + rw + subblock, bit 0-7 address */
    unsigned int data[2];    /* spi data - also carries retval */
};


#define VITGENIO_FPGA_SPI_READWRITE       _IOWR(VITGENIO_IOC_MAGIC,71,struct vitgenio_fpga_spi_readwrite)

#define VITGENIO_ENABLE_FPGA_SPI          _IO(VITGENIO_IOC_MAGIC,72)
#define VITGENIO_DISABLE_FPGA_SPI         _IO(VITGENIO_IOC_MAGIC,73)

#endif /* __VITGENIO_H */


