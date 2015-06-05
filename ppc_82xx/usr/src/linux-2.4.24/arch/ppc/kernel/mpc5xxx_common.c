/*
 *
 * Author: Dale Farnsworth <dale.farnsworth@mvista.com>
 *
 * Copyright 2003 MontaVista Software Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/config.h>
#include <asm/mpc5xxx.h>
#ifdef CONFIG_UBOOT
#include <asm/ppcboot.h>
#endif

void
mpc5xxx_restart(char *cmd)
{
	__cli();

	/* Turn on watchdog to reset the machine */
	*(u32 *) MPC5xxx_GPT0_COUNTER = 0xf;
	*(u32 *) MPC5xxx_GPT0_ENABLE = 0x9004;

	while (1);
}

void
mpc5xxx_halt(void)
{
	__cli();
	while (1);
}

void
mpc5xxx_power_off(void)
{ 
	mpc5xxx_halt();
}

/*
 * Set BAT 3 to map 0xf0000000 to end of physical memory space.
 */
void __init
mpc5xxx_set_bat(void)
{
	mb();
	mtspr(DBAT2U, 0xf0001ffe);
	mtspr(DBAT2L, 0xf000002a);
	mb();
}

#ifdef  CONFIG_SERIAL_TEXT_DEBUG
#if   CONFIG_PPC_5xxx_PSC_CONSOLE_PORT == 0
#define MPC5xxx_CONSOLE	MPC5xxx_PSC1
#elif CONFIG_PPC_5xxx_PSC_CONSOLE_PORT == 1
#define MPC5xxx_CONSOLE	MPC5xxx_PSC2
#elif CONFIG_PPC_5xxx_PSC_CONSOLE_PORT == 2
#define MPC5xxx_CONSOLE	MPC5xxx_PSC3
#else
#error "mpc5xxx PSC for console not selected"
#endif
void
mpc5xxx_progress(char *s, unsigned short hex)
{
    	struct mpc5xxx_psc *psc = (struct mpc5xxx_psc *)MPC5xxx_CONSOLE;
	char c;

	while ((c = *s++) != 0) {
		if (c == '\n') {
			while (!(in_be16(&psc->mpc5xxx_psc_status) &
					MPC5xxx_PSC_SR_TXRDY)) ;
			out_8(&psc->mpc5xxx_psc_buffer_8, '\r');
		}
		while (!(in_be16(&psc->mpc5xxx_psc_status) &
			    	MPC5xxx_PSC_SR_TXRDY)) ;
		out_8(&psc->mpc5xxx_psc_buffer_8, c);
	}
}
#endif	/* CONFIG_SERIAL_TEXT_DEBUG */

unsigned long __init
mpc5xxx_find_end_of_memory(void)
{
#if defined(CONFIG_MPC5100)
    	unsigned long last_sdram_block = in_be32((u32 *)MPC5xxx_SDRAM_STOP);

	return MPC5xxx_SDRAM_UNIT * (last_sdram_block + 1);
#elif defined(CONFIG_MPC5200)
#ifdef CONFIG_UBOOT
	extern unsigned char __res[];
	bd_t *bd = (bd_t *)__res;
	return bd->bi_memsize;
#else
    	u32 sdram_config_0 = in_be32((u32 *)MPC5xxx_SDRAM_CONFIG_0);
    	u32 sdram_config_1 = in_be32((u32 *)MPC5xxx_SDRAM_CONFIG_1);
	u32 ramsize = 0;

	if ((sdram_config_0 & 0x1f) >= 0x13)
		ramsize = 1 << ((sdram_config_0 & 0xf) + 17);

	if (((sdram_config_1 & 0x1f) >= 0x13) &&
			((sdram_config_1 & 0xfff00000) == ramsize))
		ramsize += 1 << ((sdram_config_1 & 0xf) + 17);

	return ramsize;
#endif
#else
#error "Only CONFIG_MPC5100 and CONFIG_MPC5200 supported"
#endif
}

void __init
mpc5xxx_map_io(void)
{
	io_block_mapping(0x40000000, 0x40000000, 0x10000000, _PAGE_IO);
	io_block_mapping(0x50000000, 0x50000000, 0x01000000, _PAGE_IO);
	io_block_mapping(0x80000000, 0x80000000, 0x10000000, _PAGE_IO);
	io_block_mapping(0xf0000000, 0xf0000000, 0x10000000, _PAGE_IO);
}

#ifdef CONFIG_VT

#define SC_LIM 89

/* Simple translation table for the SysRq keys */

#ifdef CONFIG_MAGIC_SYSRQ

unsigned char mpc5xxx_sysrq_xlate[128] =
"\000\0331234567890-=\177\t"			/* 0x00 - 0x0f */
"qwertyuiop[]\r\000as"				/* 0x10 - 0x1f */
"dfghjkl;'`\000\\zxcv"				/* 0x20 - 0x2f */
"bnm,./\000*\000 \000\201\202\203\204\205"	/* 0x30 - 0x3f */
"\206\207\210\211\212\000\000789-456+1"		/* 0x40 - 0x4f */
"230\177\000\000\213\214\000\000\000\000\000\000\000\000\000\000" /* 0x50 - 0x5f */
"\r\000/";					/* 0x60 - 0x6f */

#endif


int mpc5xxx_setkeycode(unsigned int scancode, unsigned int keycode)
{
  return -EINVAL;
}

int mpc5xxx_getkeycode(unsigned int scancode)
{
  return -EINVAL;
}

int mpc5xxx_translate(unsigned char scancode, unsigned char *keycode,
			   char raw_mode)
{
  //0xFF is sent by a few keyboards, ignore it. 0x00 is error 
  if (scancode == 0x00 || scancode == 0xff) {
    return 0;
  }
  *keycode = scancode;
  return 1;
}

char mpc5xxx_unexpected_up(unsigned char keycode)
{
  /* unexpected, but this can happen: maybe this was a key release for a
     FOCUS 9000 PF key; if we want to see it, we have to clear up_flag */
  if (keycode >= SC_LIM || keycode == 85)
    return 0;
  else
    return 0200;
}

void mpc5xxx_kbd_init_hw(void) 
{
  return ;
}

#endif
