/*
 * arch/ppc/boot/simple/misc-ppmc260.c
 *
 * Board-specific bootloader code for the Force PowerPMC260 board.
 *
 * Author: Randy Vinson <rvinson@mvista.com>
 *
 * 2003 (c) MontaVista, Software, Inc.  This file is licensed under the terms
 * of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */

#include <linux/types.h>
#include <asm/bootinfo.h>
#include <asm/string.h>
#include <platforms/ppmc260.h>

extern int gt64260_iic_read(uint devaddr, u8 *buf, uint offset,
			    uint offset_size, uint count);
extern struct bi_record *decompress_kernel(unsigned long load_addr,
					   int num_words,
					   unsigned long cksum);

static bd_t board_info;

void
ppmc260_get_board_info(void)
{
	int dev = PPMC260_I2C_MAC_EEPROM_ADDR;
	bd_t *bi = &board_info;
	int bytes;

	bytes = gt64260_iic_read(dev, (u8*)bi, 0, 1, sizeof(board_info));

	if (bytes != sizeof(board_info))
		puts("Error reading GT64260 MAC addresses\n");
}

struct bi_record *
load_kernel(unsigned long load_addr, int num_words, unsigned long cksum)
{
	struct bi_record *bi_recs;

	bi_recs = decompress_kernel(load_addr, num_words, cksum);

	ppmc260_get_board_info();

	bootinfo_append(BI_BOARD_INFO, sizeof(board_info), (void*)&board_info);

	return bi_recs;
}
