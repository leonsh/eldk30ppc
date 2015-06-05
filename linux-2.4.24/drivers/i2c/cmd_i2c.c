/*
 * (C) Copyright 2001
 * Gerald Van Baren, Custom IDEAS, vanbaren@cideas.com.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * I2C Functions similar to the standard memory functions.
 *
 * There are several parameters in many of the commands that bear further
 * explanations:
 *
 * Two of the commands (imm and imw) take a byte/word/long modifier
 * (e.g. imm.w specifies the word-length modifier).  This was done to
 * allow manipulating word-length registers.  It was not done on any other
 * commands because it was not deemed useful.
 *
 * {i2c_chip} is the I2C chip address (the first byte sent on the bus).
 *   Each I2C chip on the bus has a unique address.  On the I2C data bus,
 *   the address is the upper seven bits and the LSB is the "read/write"
 *   bit.  Note that the {i2c_chip} address specified on the command
 *   line is not shifted up: e.g. a typical EEPROM memory chip may have
 *   an I2C address of 0x50, but the data put on the bus will be 0xA0
 *   for write and 0xA1 for read.  This "non shifted" address notation
 *   matches at least half of the data sheets :-/.
 *
 * {addr} is the address (or offset) within the chip.  Small memory
 *   chips have 8 bit addresses.  Large memory chips have 16 bit
 *   addresses.  Other memory chips have 9, 10, or 11 bit addresses.
 *   Many non-memory chips have multiple registers and {addr} is used
 *   as the register index.  Some non-memory chips have only one register
 *   and therefore don't need any {addr} parameter.
 *
 *   The default {addr} parameter is one byte (.1) which works well for
 *   memories and registers with 8 bits of address space.
 *
 *   You can specify the length of the {addr} field with the optional .0,
 *   .1, or .2 modifier (similar to the .b, .w, .l modifier).  If you are
 *   manipulating a single register device which doesn't use an address
 *   field, use "0.0" for the address and the ".0" length field will
 *   suppress the address in the I2C data stream.  This also works for
 *   successive reads using the I2C auto-incrementing memory pointer.
 *
 *   If you are manipulating a large memory with 2-byte addresses, use
 *   the .2 address modifier, e.g. 210.2 addresses location 528 (decimal).
 *
 *   Then there are the unfortunate memory chips that spill the most
 *   significant 1, 2, or 3 bits of address into the chip address byte.
 *   This effectively makes one chip (logically) look like 2, 4, or
 *   8 chips.  This is handled (awkwardly) by #defining
 *   CFG_I2C_EEPROM_ADDR_OVERFLOW and using the .1 modifier on the
 *   {addr} field (since .1 is the default, it doesn't actually have to
 *   be specified).  Examples: given a memory chip at I2C chip address
 *   0x50, the following would happen...
 *     imd 50 0 10      display 16 bytes starting at 0x000
 *                      On the bus: <S> A0 00 <E> <S> A1 <rd> ... <rd>
 *     imd 50 100 10    display 16 bytes starting at 0x100
 *                      On the bus: <S> A2 00 <E> <S> A3 <rd> ... <rd>
 *     imd 50 210 10    display 16 bytes starting at 0x210
 *                      On the bus: <S> A4 10 <E> <S> A5 <rd> ... <rd>
 *   This is awfully ugly.  It would be nice if someone would think up
 *   a better way of handling this.
 *
 * Adapted from cmd_mem.c which is copyright Wolfgang Denk (wd@denx.de).
 */

#include <common.h>
#include <command.h>
#include <i2c.h>
#include <asm/byteorder.h>

#if (CONFIG_COMMANDS & CFG_CMD_I2C)


/* Display values from last command.
 * Memory modify remembered values are different from display memory.
 */
static uchar	i2c_dp_last_chip;
static uint	i2c_dp_last_addr;
static uint	i2c_dp_last_alen;
static uint	i2c_dp_last_length = 0x10;

static uchar	i2c_mm_last_chip;
static uint	i2c_mm_last_addr;
static uint	i2c_mm_last_alen;

#if defined(CFG_I2C_NOPROBES)
static uchar i2c_no_probes[] = CFG_I2C_NOPROBES;
#endif

static int
mod_i2c_mem(cmd_tbl_t *cmdtp, int incrflag, int flag, int argc, char *argv[]);
extern int cmd_get_data_size(char* arg, int default_size);

/*
 * Syntax:
 *	imd {i2c_chip} {addr}{.0, .1, .2} {len}
 */
#define DISP_LINE_LEN	16

int do_i2c_md ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u_char	chip;
	uint	addr, alen, length;
	int	j, nbytes, linebytes;

	/* We use the last specified parameters, unless new ones are
	 * entered.
	 */
	chip   = i2c_dp_last_chip;
	addr   = i2c_dp_last_addr;
	alen   = i2c_dp_last_alen;
	length = i2c_dp_last_length;

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if ((flag & CMD_FLAG_REPEAT) == 0) {
		/*
		 * New command specified.
		 */
		alen = 1;

		/*
		 * I2C chip address
		 */
		chip = simple_strtoul(argv[1], NULL, 16);

		/*
		 * I2C data address within the chip.  This can be 1 or
		 * 2 bytes long.  Some day it might be 3 bytes long :-).
		 */
		addr = simple_strtoul(argv[2], NULL, 16);
		alen = 1;
		for(j = 0; j < 8; j++) {
			if (argv[2][j] == '.') {
				alen = argv[2][j+1] - '0';
				if (alen > 4) {
					printf ("Usage:\n%s\n", cmdtp->usage);
					return 1;
				}
				break;
			} else if (argv[2][j] == '\0') {
				break;
			}
		}

		/*
		 * If another parameter, it is the length to display.
		 * Length is the number of objects, not number of bytes.
		 */
		if (argc > 3)
			length = simple_strtoul(argv[3], NULL, 16);
	}

	/*
	 * Print the lines.
	 *
	 * We buffer all read data, so we can make sure data is read only
	 * once.
	 */
	nbytes = length;
	do {
		unsigned char	linebuf[DISP_LINE_LEN];
		unsigned char	*cp;

		linebytes = (nbytes > DISP_LINE_LEN) ? DISP_LINE_LEN : nbytes;

		if(i2c_read(chip, addr, alen, linebuf, linebytes) != 0) {
			puts ("Error reading the chip.\n");
		} else {
			printf("%04x:", addr);
			cp = linebuf;
			for (j=0; j<linebytes; j++) {
				printf(" %02x", *cp++);
				addr++;
			}
			puts ("    ");
			cp = linebuf;
			for (j=0; j<linebytes; j++) {
				if ((*cp < 0x20) || (*cp > 0x7e))
					puts (".");
				else
					printf("%c", *cp);
				cp++;
			}
			putc ('\n');
		}
		nbytes -= linebytes;
	} while (nbytes > 0);

	i2c_dp_last_chip   = chip;
	i2c_dp_last_addr   = addr;
	i2c_dp_last_alen   = alen;
	i2c_dp_last_length = length;

	return 0;
}

int do_i2c_mm ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return mod_i2c_mem (cmdtp, 1, flag, argc, argv);
}


int do_i2c_nm ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return mod_i2c_mem (cmdtp, 0, flag, argc, argv);
}

/* Write (fill) memory
 *
 * Syntax:
 *	imw {i2c_chip} {addr}{.0, .1, .2} {data} [{count}]
 */
int do_i2c_mw ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	uchar	chip;
	ulong	addr;
	uint	alen;
	uchar	byte;
	int	count;
	int	j;

	if ((argc < 4) || (argc > 5)) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	/*
 	 * Chip is always specified.
 	 */
	chip = simple_strtoul(argv[1], NULL, 16);

	/*
	 * Address is always specified.
	 */
	addr = simple_strtoul(argv[2], NULL, 16);
	alen = 1;
	for(j = 0; j < 8; j++) {
		if (argv[2][j] == '.') {
			alen = argv[2][j+1] - '0';
			if(alen > 4) {
				printf ("Usage:\n%s\n", cmdtp->usage);
				return 1;
			}
			break;
		} else if (argv[2][j] == '\0') {
			break;
		}
	}

	/*
	 * Value to write is always specified.
	 */
	byte = simple_strtoul(argv[3], NULL, 16);

	/*
	 * Optional count
	 */
	if(argc == 5) {
		count = simple_strtoul(argv[4], NULL, 16);
	} else {
		count = 1;
	}

	while (count-- > 0) {
		if(i2c_write(chip, addr++, alen, &byte, 1) != 0) {
			puts ("Error writing the chip.\n");
		}
		/*
		 * Wait for the write to complete.  The write can take
		 * up to 10mSec (we allow a little more time).
		 *
		 * On some chips, while the write is in progress, the
		 * chip doesn't respond.  This apparently isn't a
		 * universal feature so we don't take advantage of it.
		 */
		udelay(11000);
#if 0
		for(timeout = 0; timeout < 10; timeout++) {
			udelay(2000);
			if(i2c_probe(chip) == 0)
				break;
		}
#endif
	}

	return (0);
}


/* Calculate a CRC on memory
 *
 * Syntax:
 *	icrc32 {i2c_chip} {addr}{.0, .1, .2} {count}
 */
int do_i2c_crc (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	uchar	chip;
	ulong	addr;
	uint	alen;
	int	count;
	uchar	byte;
	ulong	crc;
	ulong	err;
	int	j;

	if (argc < 4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	/*
 	 * Chip is always specified.
 	 */
	chip = simple_strtoul(argv[1], NULL, 16);

	/*
	 * Address is always specified.
	 */
	addr = simple_strtoul(argv[2], NULL, 16);
	alen = 1;
	for(j = 0; j < 8; j++) {
		if (argv[2][j] == '.') {
			alen = argv[2][j+1] - '0';
			if(alen > 4) {
				printf ("Usage:\n%s\n", cmdtp->usage);
				return 1;
			}
			break;
		} else if (argv[2][j] == '\0') {
			break;
		}
	}

	/*
	 * Count is always specified
	 */
	count = simple_strtoul(argv[3], NULL, 16);

	printf ("CRC32 for %08lx ... %08lx ==> ", addr, addr + count - 1);
	/*
	 * CRC a byte at a time.  This is going to be slooow, but hey, the
	 * memories are small and slow too so hopefully nobody notices.
	 */
	crc = 0;
	err = 0;
	while(count-- > 0) {
		if(i2c_read(chip, addr, alen, &byte, 1) != 0) {
			err++;
		}
		crc = crc32 (crc, &byte, 1);
		addr++;
	}
	if(err > 0)
	{
		puts ("Error reading the chip,\n");
	} else {
		printf ("%08lx\n", crc);
	}

	return 0;
}


/* Modify memory.
 *
 * Syntax:
 *	imm{.b, .w, .l} {i2c_chip} {addr}{.0, .1, .2}
 *	inm{.b, .w, .l} {i2c_chip} {addr}{.0, .1, .2}
 */

static int
mod_i2c_mem(cmd_tbl_t *cmdtp, int incrflag, int flag, int argc, char *argv[])
{
	uchar	chip;
	ulong	addr;
	uint	alen;
	ulong	data;
	int	size = 1;
	int	nbytes;
	int	j;
	extern char console_buffer[];

	if (argc != 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

#ifdef CONFIG_BOOT_RETRY_TIME
	reset_cmd_timeout();	/* got a good command to get here */
#endif
	/*
	 * We use the last specified parameters, unless new ones are
	 * entered.
	 */
	chip = i2c_mm_last_chip;
	addr = i2c_mm_last_addr;
	alen = i2c_mm_last_alen;

	if ((flag & CMD_FLAG_REPEAT) == 0) {
		/*
		 * New command specified.  Check for a size specification.
		 * Defaults to byte if no or incorrect specification.
		 */
		size = cmd_get_data_size(argv[0], 1);

		/*
	 	 * Chip is always specified.
	 	 */
		chip = simple_strtoul(argv[1], NULL, 16);

		/*
		 * Address is always specified.
		 */
		addr = simple_strtoul(argv[2], NULL, 16);
		alen = 1;
		for(j = 0; j < 8; j++) {
			if (argv[2][j] == '.') {
				alen = argv[2][j+1] - '0';
				if(alen > 4) {
					printf ("Usage:\n%s\n", cmdtp->usage);
					return 1;
				}
				break;
			} else if (argv[2][j] == '\0') {
				break;
			}
		}
	}

	/*
	 * Print the address, followed by value.  Then accept input for
	 * the next value.  A non-converted value exits.
	 */
	do {
		printf("%08lx:", addr);
		if(i2c_read(chip, addr, alen, (char *)&data, size) != 0) {
			puts ("\nError reading the chip,\n");
		} else {
			data = cpu_to_be32(data);
			if(size == 1) {
				printf(" %02lx", (data >> 24) & 0x000000FF);
			} else if(size == 2) {
				printf(" %04lx", (data >> 16) & 0x0000FFFF);
			} else {
				printf(" %08lx", data);
			}
		}

		nbytes = readline (" ? ");
		if (nbytes == 0) {
			/*
			 * <CR> pressed as only input, don't modify current
			 * location and move to next.
			 */
			if (incrflag)
				addr += size;
			nbytes = size;
#ifdef CONFIG_BOOT_RETRY_TIME
			reset_cmd_timeout(); /* good enough to not time out */
#endif
		}
#ifdef CONFIG_BOOT_RETRY_TIME
		else if (nbytes == -2) {
			break;	/* timed out, exit the command	*/
		}
#endif
		else {
			char *endp;

			data = simple_strtoul(console_buffer, &endp, 16);
			if(size == 1) {
				data = data << 24;
			} else if(size == 2) {
				data = data << 16;
			}
			data = be32_to_cpu(data);
			nbytes = endp - console_buffer;
			if (nbytes) {
#ifdef CONFIG_BOOT_RETRY_TIME
				/*
				 * good enough to not time out
				 */
				reset_cmd_timeout();
#endif
				if(i2c_write(chip, addr, alen, (char *)&data, size) != 0) {
					puts ("Error writing the chip.\n");
				}
#ifdef CFG_EEPROM_PAGE_WRITE_DELAY_MS
				udelay(CFG_EEPROM_PAGE_WRITE_DELAY_MS * 1000);
#endif
				if (incrflag)
					addr += size;
			}
		}
	} while (nbytes);

	chip = i2c_mm_last_chip;
	addr = i2c_mm_last_addr;
	alen = i2c_mm_last_alen;

	return 0;
}

/*
 * Syntax:
 *	iprobe {addr}{.0, .1, .2}
 */
int do_i2c_probe (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int j;
#if defined(CFG_I2C_NOPROBES)
	int k, skip;
#endif

	puts ("Valid chip addresses:");
	for(j = 0; j < 128; j++) {
#if defined(CFG_I2C_NOPROBES)
		skip = 0;
		for (k = 0; k < sizeof(i2c_no_probes); k++){
			if (j == i2c_no_probes[k]){
				skip = 1;
				break;
			}
		}
		if (skip)
			continue;
#endif
		if(i2c_probe(j) == 0) {
			printf(" %02X", j);
		}
	}
	putc ('\n');

#if defined(CFG_I2C_NOPROBES)
	puts ("Excluded chip addresses:");
	for( k = 0; k < sizeof(i2c_no_probes); k++ )
		printf(" %02X", i2c_no_probes[k] );
	putc ('\n');
#endif

	return 0;
}


/*
 * Syntax:
 *	iloop {i2c_chip} {addr}{.0, .1, .2} [{length}] [{delay}]
 *	{length} - Number of bytes to read
 *	{delay}  - A DECIMAL number and defaults to 1000 uSec
 */
int do_i2c_loop(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u_char	chip;
	ulong	alen;
	uint	addr;
	uint	length;
	u_char	bytes[16];
	int	delay;
	int	j;

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	/*
	 * Chip is always specified.
	 */
	chip = simple_strtoul(argv[1], NULL, 16);

	/*
	 * Address is always specified.
	 */
	addr = simple_strtoul(argv[2], NULL, 16);
	alen = 1;
	for(j = 0; j < 8; j++) {
		if (argv[2][j] == '.') {
			alen = argv[2][j+1] - '0';
			if (alen > 4) {
				printf ("Usage:\n%s\n", cmdtp->usage);
				return 1;
			}
			break;
		} else if (argv[2][j] == '\0') {
			break;
		}
	}

	/*
	 * Length is the number of objects, not number of bytes.
	 */
	length = 1;
	length = simple_strtoul(argv[3], NULL, 16);
	if(length > sizeof(bytes)) {
		length = sizeof(bytes);
	}

	/*
	 * The delay time (uSec) is optional.
	 */
	delay = 1000;
	if (argc > 3) {
		delay = simple_strtoul(argv[4], NULL, 10);
	}
	/*
	 * Run the loop...
	 */
	while(1) {
		if(i2c_read(chip, addr, alen, bytes, length) != 0) {
			puts ("Error reading the chip.\n");
		}
		udelay(delay);
	}

	/* NOTREACHED */
	return 0;
}

unsigned char	macbuf[] = "001309123456";
unsigned char   backflag;
void macincrement(char *cp)
{
  unsigned long int macaddress;
  int i,j;
  char macchar;

  j=1048576;
  macaddress = 0;
  for(i=0;i<6;i++){
    macchar = *cp++;
    if((macchar > 0x2f )&&(macchar < 0x3a)) {
      macchar = macchar - 0x30;
    }
    else {
      if((macchar > 64 )&&(macchar < 71)){
        macchar = macchar - 55;
      }
      else {
        if((macchar > 96 )&&(macchar < 103)){
          macchar = macchar - 87;
        }
        else {
          printf("Input Mac address wrong\n");
        }
      }
    }
    macaddress= macaddress + macchar * j;
    j = j/16;
  }

  macaddress++;

  j=1048576;
  for(i=0;i<6;i++){
    macchar = macaddress / j;
    macaddress = macaddress - macchar * j;
    if((macchar>=0)&&(macchar<=9))
      macchar = macchar + 0x30;
    else
      macchar = macchar + 87;
    macbuf[6+i]=macchar;
    j=j/16;
  }
}

/* JieHan 08/24/2005 */
extern char console_buffer[];
volatile int flag_password = 0;
volatile char administrator[] = "concord";
int do_i2c_hw ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    u_char	chip;
    uchar	byte;
	uint	addr;
	int	i,offset,offset1,offset2,linebytes,nbytes;
    int  flag_nmschange;
    unsigned char	linebuf[150];
	unsigned char	*cp;

    if (argc != 1) {
	  printf ("Usage:%s\n", cmdtp->usage);
	  return 1;
	}

    chip = CFG_I2C_EEPROM_ADDR;
    flag_nmschange = 0;

    /* JieHan 06/15/2005 Password protect */
    flag_password = 1;
    printf("Please input administrator's password");
    nbytes = readline (" ? ");
    flag_password = 0;       

    if(nbytes == -1) {
      printf("\nDiscard input password\n");
      return 1;
    }
    if (nbytes == -2) {
      printf("\nTime out,exit the command\n");  /* timed out, exit the command	*/
      return 1;
    }
    if((nbytes == 1) && (console_buffer[0] == '.')) {
      printf("\nExit input password\n");
      return 1;
    }

    if(strcmp(administrator,console_buffer) != 0){
      addr = 620;
      linebytes = 1;
      if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
        puts ("Error reading the eeprom.\n");
        return 1;
      }
      else {
        /* password length*/
        linebytes = linebuf[0];
      }

      addr = 621;
      if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
        puts ("Error reading the eeprom.\n");
        return 1;
      }
      linebuf[linebytes] = 0;

      if(strcmp(linebuf,console_buffer) != 0) {
        printf("You havenot the right to set parameter......\n");
        return 1;
      }
    }


    offset = 0;   /* board index offset */
    offset1 = 0;  /* GIG_E Mac Address & Optical parameter offset */
    offset2 = 0;
    backflag = 0xff;
    do {
      /* Display board information */
      switch(offset) {
        case 0:
          addr = 0;
          linebytes = 8;
          if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
            puts ("Error reading the eeprom.\n");
            return 1;
          }
          else {
            printf("Production Date:%c%c/%c%c/%c%c%c%c\n",linebuf[0],linebuf[1],
                   linebuf[2],linebuf[3],linebuf[4],linebuf[5],linebuf[6],linebuf[7]);
          }
          break;
        case 1:
          addr = 8;
          linebytes = 32;
          if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
            puts ("Error reading the eeprom.\n");
            return 1;
          } 
          else {
            cp = linebuf;
            printf("Production Type:");
            for (i=0; i<linebytes; i++)
              printf("%c",*cp++);
            printf("\n");
          }
          break;
        case 2:
          addr = 40;
          linebytes = 4;
          if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
            puts ("Error reading the eeprom.\n");
            return 1;
          }
          else {
            cp = linebuf;
            printf("Hardware Version:");
            for (i=0; i<linebytes; i++)
              printf("%c",*cp++);
            printf("\n");
          }
          break;
        case 3:
          addr = 44;
          linebytes = 32;
          if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
            puts ("Error reading the eeprom.\n");
            return 1;
          }
          else {
            cp = linebuf;
            printf("Serial Number:");
            for (i=0; i<linebytes; i++)
              printf("%c",*cp++);
            printf("\n");
          }
          break;
        case 4:
          addr = 76;
          linebytes = 12;
          if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
            puts ("Error reading the eeprom.\n");
            return 1;
          }
          else {
            cp = linebuf;
            printf("NMS MAC Address:");
            for (i=0; i<6; i++) {
              printf("%c",*cp++);
              printf("%c ",*cp++);
            }
            printf("\n");
          }
          break;
        case 5:
          addr = 88;
          linebytes = 12;
          if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
            puts ("Error reading the eeprom.\n");
            return 1;
          }
          else {
            cp = linebuf;
            printf("EPON MAC Address:");
            for (i=0; i<6; i++) {
              printf("%c",*cp++);
              printf("%c ",*cp++);
            }
            printf("\n");
          }
          break;
        case 6:
          linebytes = 12;
          addr = 100+12*offset1;
          if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
            puts ("Error reading the eeprom.\n");
            return 1;
          }
          else {
            cp = linebuf;
            printf("GIG-E MAC Address(%d):",(offset1+1));
            for (i=0; i<6; i++) {
              printf("%c",*cp++);
              printf("%c ",*cp++);
            }

            if((offset1 != 0) && (backflag == 0xff)){
              /* display new mach address JieHan 05/07/2005 */
              printf(" NewMacAddress:");
              cp = macbuf;
              for (i=0; i<6; i++) {
                printf("%c",*cp++);
                printf("%c ",*cp++);
              }
            }
            printf("\n");
          }
          break;
        case 7:
          addr = 196;
          linebytes = 12;
          if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
            puts ("Error reading the eeprom.\n");
            return 1;
          }
          else {
            cp = linebuf;
            printf("OAM MAC Address:");
            for (i=0; i<6; i++) {
              printf("%c",*cp++);
              printf("%c ",*cp++);
            }
            printf("\n");
          }
          break;
        case 8:
          addr = 208;
          linebytes = 12;
          if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
            puts ("Error reading the eeprom.\n");
            return 1;
          }
          else {
            cp = linebuf;
            printf("Mgmt MAC Address:");
            for (i=0; i<6; i++) {
              printf("%c",*cp++);
              printf("%c ",*cp++);
            }
            printf("\n");
          }
          break;
        case 9:
          linebytes = 6;
          addr = 416+12*offset1+6*offset2;
          if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
            puts ("Error reading the eeprom.\n");
            return 1;
          }
          else {
            cp = linebuf;
            if(offset2 == 0)
              printf("Optical Transmit Power parameter(%d):",(offset1+1));
            else
              printf("Optical Receive Sensitivity Parameter(%d):",(offset1+1));
            for (i=0; i<6; i++) 
              printf("%c",*cp++);
            printf("\n");
          }
          break;
        case 10:
          addr = 464;
          linebytes = 128;
          if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
            puts ("Error reading the eeprom.\n");
            return 1;
          }
          else {
            cp = linebuf;
            printf("Deployment address:");
            for (i=0; i<128; i++) 
              printf("%c",*cp++);
            printf("\n");
          }
          break;
        default:
          if(flag_nmschange == 1){
            saveenv();
          }
          return 1;
      }

      nbytes = readline (" ? ");

      if(nbytes == -1) {
        printf("\n");
        if(flag_nmschange == 1) {
          saveenv();
        }
        return 1;
      }
      if (nbytes == -2) {
        printf("\n");          /* timed out, exit the command	*/
        if(flag_nmschange == 1) {
          saveenv();
        }
        return 1;
      }

      if((nbytes == 1) && (console_buffer[0] == '.')) {
        printf("\n");
        if(flag_nmschange == 1) {
          saveenv();
        }
        return 1;
      }

      /* JieHan write changeless address 05/07/2005 */
      if((nbytes == 0) && (offset == 10)) {
        /* Make sure nbytes != 0 or 1, so we can write changeless address */
        nbytes=2; 
      }

      if (nbytes == 0 || (nbytes == 1 && console_buffer[0] == '-')) {
        /* <CR> pressed as only input, don't modify current
         * location and move to next. "-" pressed will go back.
         */
        if(nbytes == 1) {
          switch(offset) {
            case 6:
              if(offset1 == 0)
                offset--;
              else {
                offset1--;
                backflag = 1;
              }
              break;
            case 7:
              offset--;
              offset1 = 7;
              break;
            case 9:
              if(offset1 == 0) {
                if(offset2 == 0)
                  offset--;
                else
                  offset2 = 0;
              }
              else {
                if(offset2 == 0) {
                  offset1--;
                  offset2=1;
                }
                else
                  offset2 = 0;
              }
              break;
            case 10:
              offset--;
              offset1=3;
              offset2=1;
              break;
            default:
              if(offset == 0) {
                printf("It's the first place of board specific information area,cannot backward\n");
                return 1;
              }
              else
                offset--;
              break;
          }
        }
        else {
          switch(offset) {
            case 6:
              /* write new mac address,new mac address increment */
              /* JieHan 05/07/2005 */
              if((offset1 != 0) && (backflag == 0xff)) {
                addr = 100+12*offset1;
                byte = 0x20;
                for (i=0;i<12;i++) {
                  /* write space char 0x20 to fill the parameter space */
                  if(i2c_write(chip, addr++, 1, &byte, 1) != 0) {
                    puts ("Error writing the chip.\n");
                    return 1;
                  }
                  udelay(11000);
                }
                addr = 100+12*offset1;
                for (i=0;i<12;i++) {
                  /* write space char 0x20 to fill the parameter space */
                  if(i2c_write(chip, addr++, 1, &macbuf[i], 1) != 0) {
                    puts ("Error writing the chip.\n");
                    return 1;
                  }
                  udelay(11000);
                }
                macincrement(&macbuf[6]);
              }
              else {
                linebytes = 12;
                addr = 100+12*offset1;
                if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
                  puts ("Error reading the eeprom.\n");
                  return 1;
                }
                macincrement(&linebuf[6]);
                backflag=0xff;
              }

              if(offset1 < 7)
                offset1++;
              else {
                offset++;
                offset1=0;
              }
              break;
            case 9:
              if(offset1 < 3) {
                if(offset2 == 0)
                  offset2 = 1;
                else {
                  offset2 = 0;
                  offset1++;
                }
              }
              else {
                if(offset2 == 0) {
                  offset2=1;
                }
                else {
                  offset++;
                  offset1=0;
                  offset2=0;
                }
              }
              break;
            default:
              offset++;
              break;
          }
        }

        nbytes = 1;
      }
      else {
        if(console_buffer[nbytes-1]=='.') {
          printf("\n");
          return 1;
        }
        /* Clean EEPROM & Write parameter to EEPROM */
        switch(offset){
          case 0:
            addr = 0;
            linebytes = 8;
            byte = 0x20;
            for (i=0;i<linebytes;i++) {
              /* write space char 0x20 to fill the parameter space */
              if(i2c_write(chip, addr++, 1, &byte, 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            if(linebytes < nbytes)
              nbytes=linebytes;
            addr = 0;
            for (i=0;i<nbytes;i++) {
              /* write specific information to parameter space */
              if(i2c_write(chip, addr++, 1, &console_buffer[i], 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            offset++;
            break;
          case 1:
            addr = 8;
            linebytes = 32;
            byte = 0x20;
            for (i=0;i<linebytes;i++) {
              /* write space char 0x20 to fill the parameter space */
              if(i2c_write(chip, addr++, 1, &byte, 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            if(linebytes < nbytes)
              nbytes=linebytes;
            addr = 8;
            for (i=0;i<nbytes;i++) {
              /* write specific information to parameter space */
              if(i2c_write(chip, addr++, 1, &console_buffer[i], 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            offset++;
            break;
          case 2:
            addr = 40;
            linebytes = 4;
            byte = 0x20;
            for (i=0;i<linebytes;i++) {
              /* write space char 0x20 to fill the parameter space */
              if(i2c_write(chip, addr++, 1, &byte, 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            if(linebytes < nbytes)
              nbytes=linebytes;
            addr = 40;
            for (i=0;i<nbytes;i++) {
              /* write specific information to parameter space */
              if(i2c_write(chip, addr++, 1, &console_buffer[i], 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            offset++;
            break;
          case 3:
            addr = 44;
            linebytes = 32;
            byte = 0x20;
            for (i=0;i<linebytes;i++) {
              /* write space char 0x20 to fill the parameter space */
              if(i2c_write(chip, addr++, 1, &byte, 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            if(linebytes < nbytes)
              nbytes=linebytes;
            addr = 44;
            for (i=0;i<nbytes;i++) {
              /* write specific information to parameter space */
              if(i2c_write(chip, addr++, 1, &console_buffer[i], 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            offset++;
            break;
          case 4:
            addr = 76;
            linebytes = 12;
            byte = 0x20;
            for (i=0;i<linebytes;i++) {
              /* write space char 0x20 to fill the parameter space */
              if(i2c_write(chip, addr++, 1, &byte, 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            if(linebytes < nbytes)
              nbytes=linebytes;
            addr = 76;
            for (i=0;i<nbytes;i++) {
              /* write specific information to parameter space */
              if(i2c_write(chip, addr++, 1, &console_buffer[i], 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            /* JieHan 06/15/2005 setenv ethaddr */
            char varname[] = "ethaddr";
            char varvalue[18];
            varvalue[0] = console_buffer[0];
            varvalue[1] = console_buffer[1];
            varvalue[2] = 0x3a;
            varvalue[3] = console_buffer[2];
            varvalue[4] = console_buffer[3];
            varvalue[5] = 0x3a;
            varvalue[6] = console_buffer[4];
            varvalue[7] = console_buffer[5];
            varvalue[8] = 0x3a;
            varvalue[9] = console_buffer[6];
            varvalue[10] = console_buffer[7];
            varvalue[11] = 0x3a;
            varvalue[12] = console_buffer[8];
            varvalue[13] = console_buffer[9];
            varvalue[14] = 0x3a;
            varvalue[15] = console_buffer[10];
            varvalue[16] = console_buffer[11];
            varvalue[17] = 0;
            char *argv[4] = { "setenv", &varname, &varvalue, NULL };
            _do_setenv (0, 3, argv);
            flag_nmschange = 1;

            offset++;
            break;
          case 5:
            addr = 88;
            linebytes = 12;
            byte = 0x20;
            for (i=0;i<linebytes;i++) {
              /* write space char 0x20 to fill the parameter space */
              if(i2c_write(chip, addr++, 1, &byte, 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            if(linebytes < nbytes)
              nbytes=linebytes;
            addr = 88;
            for (i=0;i<nbytes;i++) {
              /* write specific information to parameter space */
              if(i2c_write(chip, addr++, 1, &console_buffer[i], 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            offset++;
            break;
          case 6:
            /* Input char:'p' means jump change this mac address,keep it */
            /* JieHan 05/07/2005 */
            if((nbytes == 1) && (console_buffer[0] == 'p')){
              linebytes = 12;
              addr = 100+12*offset1;
              if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
                puts ("Error reading the eeprom.\n");
                return 1;
              }
              macincrement(&linebuf[6]);
            }
            else {
              if(nbytes != 12){
                printf("error input mac address!\n");
                break;
              }

              addr = 100+12*offset1;
              linebytes = 12;
              byte = 0x20;
              for (i=0;i<linebytes;i++) {
                /* write space char 0x20 to fill the parameter space */
                if(i2c_write(chip, addr++, 1, &byte, 1) != 0) {
                  puts ("Error writing the chip.\n");
                  return 1;
                }
                udelay(11000);
              }
              if(linebytes < nbytes)
                nbytes=linebytes;
              addr = 100+12*offset1;
              for (i=0;i<nbytes;i++) {
                /* write specific information to parameter space */
                if(i2c_write(chip, addr++, 1, &console_buffer[i], 1) != 0) {
                  puts ("Error writing the chip.\n");
                  return 1;
                }
                udelay(11000);
              }
              /* write new mac address,new mac address increment */
              /* JieHan 05/07/2005 */
              if(offset1 < 7){
                macincrement(&console_buffer[6]);
              }
            }
            backflag = 0xff;

            if(offset1 < 7)
              offset1++;
            else {
              offset++;
              offset1=0;
            }
            break;
          case 7:
            addr = 196;
            linebytes = 12;
            byte = 0x20;
            for (i=0;i<linebytes;i++) {
              /* write space char 0x20 to fill the parameter space */
              if(i2c_write(chip, addr++, 1, &byte, 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            if(linebytes < nbytes)
              nbytes=linebytes;
            addr = 196;
            for (i=0;i<nbytes;i++) {
              /* write specific information to parameter space */
              if(i2c_write(chip, addr++, 1, &console_buffer[i], 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            offset++;
            break;
          case 8:
            addr = 208;
            linebytes = 12;
            byte = 0x20;
            for (i=0;i<linebytes;i++) {
              /* write space char 0x20 to fill the parameter space */
              if(i2c_write(chip, addr++, 1, &byte, 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            if(linebytes < nbytes)
              nbytes=linebytes;
            addr = 208;
            for (i=0;i<nbytes;i++) {
              /* write specific information to parameter space */
              if(i2c_write(chip, addr++, 1, &console_buffer[i], 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            offset++;
            break;
          case 9:
            addr = 416+12*offset1+6*offset2;
            linebytes = 12;
            byte = 0x20;
            for (i=0;i<linebytes;i++) {
              /* write space char 0x20 to fill the parameter space */
              if(i2c_write(chip, addr++, 1, &byte, 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            if(linebytes < nbytes)
              nbytes=linebytes;
            addr = 416+12*offset1+6*offset2;
            for (i=0;i<nbytes;i++) {
              /* write specific information to parameter space */
              if(i2c_write(chip, addr++, 1, &console_buffer[i], 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }

            if(offset1 < 3) {
              if(offset2 == 0)
                offset2 = 1;
              else {
                offset2 = 0;
                offset1++;
              }
            }
            else {
              if(offset2 == 0)
                offset2=1;
              else {
                offset++;
                offset1=0;
                offset2=0;
              }
            }
            break;
          case 10:
            addr = 464;
            linebytes = 128;
            byte = 0x20;
            for (i=0;i<linebytes;i++) {
              /* write space char 0x20 to fill the parameter space */
              if(i2c_write(chip, addr++, 1, &byte, 1) != 0) {
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            if(linebytes < nbytes)
              nbytes=linebytes;
            addr = 464;
            /* write changeless address Jie Han 05/07/2005 */
            char address[] = "9F Multi-media Mansion,No.757 Guangzhong West Rd,ShangHai,China";

            nbytes = sizeof(address);
            for (i=0;i<nbytes;i++) {
              /* write specific information to parameter space */
/*              if(i2c_write(chip, addr++, 1, &console_buffer[i], 1) != 0) {*/
              if(i2c_write(chip, addr++, 1, &address[i], 1) != 0) {     
                puts ("Error writing the chip.\n");
                return 1;
              }
              udelay(11000);
            }
            offset++;
            break;
        }
      }

    } while (nbytes);

    return 0;
}

/* JieHan 08/24/2005 */
int do_i2c_hr ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u_char	chip;
	uint	addr;
	int	i,j,linebytes;
    unsigned char	linebuf[150];
	unsigned char	*cp;


	if (argc != 1) {
		printf ("Usage:%s\n", cmdtp->usage);
		return 1;
	}

    chip = CFG_I2C_EEPROM_ADDR;

    /* Production Date */
    addr = 0;
    linebytes = 8;
    if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
      puts ("Error reading the eeprom.\n");
      return 1;
    }
    else {
      printf("Production Date:%c%c/%c%c/%c%c%c%c\n",linebuf[0],linebuf[1],
             linebuf[2],linebuf[3],linebuf[4],linebuf[5],linebuf[6],linebuf[7]);
    }

    /* Product Type */
    addr = 8;
    linebytes = 32;
    if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
      puts ("Error reading the eeprom.\n");
      return 1;
    }
    else {
      cp = linebuf;
      printf("Production Type:");
      for (i=0; i<linebytes; i++)
        printf("%c",*cp++);
      printf("\n");
    }

    /* Hardware version */
    addr = 40;
    linebytes = 4;
    if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
      puts ("Error reading the eeprom.\n");
      return 1;
    }
    else {
      cp = linebuf;
      printf("Hardware Version:");
      for (i=0; i<linebytes; i++)
        printf("%c",*cp++);
      printf("\n");
    }

    /* Serial Number */
    addr = 44;
    linebytes = 32;
    if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
      puts ("Error reading the eeprom.\n");
      return 1;
    }
    else {
      cp = linebuf;
      printf("Serial Number:");
      for (i=0; i<linebytes; i++)
        printf("%c",*cp++);
      printf("\n");
    }

    /* NMS MAC address */
    addr = 76;
    linebytes = 12;
    if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
      puts ("Error reading the eeprom.\n");
      return 1;
    }
    else {
      cp = linebuf;
      printf("NMS MAC Address:");
      for (i=0; i<6; i++) {
        printf("%c",*cp++);
        printf("%c ",*cp++);
      }
      printf("\n");
    }

    /* EPON MAC address */
    addr = 88;
    linebytes = 12;
    if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
      puts ("Error reading the eeprom.\n");
      return 1;
    }
    else {
      cp = linebuf;
      printf("EPON MAC Address:");
      for (i=0; i<6; i++) {
        printf("%c",*cp++);
        printf("%c ",*cp++);
      }
      printf("\n");
    }

    /* GIG-E MAC address */
    linebytes = 12;
    for(j=1;j<=8;j++){
      addr = 100+12*(j-1);
      if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
        puts ("Error reading the eeprom.\n");
        return 1;
      }
      else {
        cp = linebuf;
        printf("GIG-E MAC Address(%d):",j);
        for (i=0; i<6; i++) {
          printf("%c",*cp++);
          printf("%c ",*cp++);
        }
        printf("\n");
      }
    }

    /* OAM MAC address */
    addr = 196;
    linebytes = 12;
    if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
      puts ("Error reading the eeprom.\n");
      return 1;
    }
    else {
      cp = linebuf;
      printf("OAM MAC Address:");
      for (i=0; i<6; i++) {
        printf("%c",*cp++);
        printf("%c ",*cp++);
      }
      printf("\n");
    }
 
    /* Mgmt MAC address */
    addr = 208;
    linebytes = 12;
    if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
      puts ("Error reading the eeprom.\n");
      return 1;
    }
    else {
      cp = linebuf;
      printf("Mgmt MAC Address:");
      for (i=0; i<6; i++) {
        printf("%c",*cp++);
        printf("%c ",*cp++);
      }
      printf("\n");
    }

    /* Optical Transmit Power parameter 
       Optical Receive Sensitivity Parameter 
    */   
    linebytes = 12;
    for(j=1;j<=4;j++){
      addr = 416+12*(j-1);
      if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
        puts ("Error reading the eeprom.\n");
        return 1;
      }
      else {
        cp = linebuf;
        printf("Optical Transmit Power parameter(%d):",j);
        for (i=0; i<6; i++) 
          printf("%c",*cp++);
        printf("\n");
        printf("Optical Receive Sensitivity Parameter(%d):",j);
        for (i=0; i<6; i++) 
          printf("%c",*cp++);
        printf("\n");
      }
    }

    /* Deployment Address */
    addr = 464;
    linebytes = 128;
    if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
      puts ("Error reading the eeprom.\n");
      return 1;
    }
    else {
      cp = linebuf;
      printf("Deployment address:");
      for (i=0; i<128; i++) 
        printf("%c",*cp++);
      printf("\n");
    }

    return 0;
}

int do_i2c_image ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u_char	chip;
	uint	addr;
	int linebytes;
    unsigned char linebuf[20];
    unsigned char image_source;

    chip = CFG_I2C_EEPROM_ADDR;
    if (argc == 1) {
      /* get application image boot type */
      addr = 619;
      linebytes = 1;
      if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
        puts ("Error reading the eeprom.\n");
        return 1;
      }
      else {
        switch(linebuf[0]) {
          case 0:
            printf("Primary application image is choiced.\n");
            break;
          case 1:
            printf("Backup application image is choiced.\n");
            break;
          case 2:
            printf("Minimal application image is choiced.\n");
            break;
        }
      }

      return 0;
    }

    /* set application image boot type */
    addr = 619;
    linebytes = 1;
	image_source = simple_strtoul(argv[1], NULL, 16);
    if(i2c_write(chip, addr, 1, &image_source, 1) != 0) {
      puts ("Error writing the chip.\n");
      return 1;
    }
    udelay(11000);
    return 0;
}

int do_i2c_sfp ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u_char	chip;
	uint	addr;
	int linebytes;
    unsigned char linebuf[20];
    unsigned char sfp_channel;

    chip = 0x77;
    addr = 0;
    linebytes = 1;
    if (argc == 1) {
      linebytes = 1;
      if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
        puts ("Error reading the I2C Multiplexing Chip control register.\n");
        return 1;
      }
      else {
        printf("I2C Multiplexing Control Register:0x%x\n",linebuf[0]);
      }
      return 0;
    }

	sfp_channel = simple_strtoul(argv[1], NULL, 16);
    printf("input I2C Multiplexing Chip control register == 0x%x\n",sfp_channel);
    if(i2c_write(chip, addr, 1, &sfp_channel, 1) != 0) {
      puts ("Error writing the I2C Multiplexing Chip control register.\n");
      return 1;
    }
    udelay(11000);

    chip = 0x50;
    if(argc != 3)
      addr = 0;
    else
      addr = simple_strtoul(argv[2], NULL, 16);

    linebytes = 1;
    if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
      puts ("Error reading the SFP parameter.\n");
      return 1;
    }
    else {
      printf("SFP parameter(data address:0x%x) == 0x%x\n",addr,linebuf[0]);
    }

    return 0;
}


/* JieHan 06/27/2005 */
int do_i2c_password ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u_char	chip;
	uint	addr;
	int i,nbytes,linebytes;
    unsigned char linebuf[20];
    unsigned char newpassword[20];

    if (argc != 1) {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    flag_password = 1;
    printf("Please input current password");
    nbytes = readline (" ? ");
    flag_password = 0;       

    if(nbytes == -1) {
      printf("\nDiscard input password\n");
      return 1;
    }
    if (nbytes == -2) {
      printf("\nTime out,exit the command\n");  /* timed out, exit the command	*/
      return 1;
    }
    if((nbytes == 1) && (console_buffer[0] == '.')) {
      printf("\nExit input password\n");
      return 1;
    }

    chip = CFG_I2C_EEPROM_ADDR;
    if(strcmp(administrator,console_buffer) != 0){
      addr = 620;
      linebytes = 1;
      if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
        puts ("Error reading the eeprom.\n");
        return 1;
      }
      else {
        /* password length*/
        linebytes = linebuf[0];
      }

      addr = 621;
      if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
        puts ("Error reading the eeprom.\n");
        return 1;
      }
      linebuf[linebytes] = 0;
      if(strcmp(linebuf,console_buffer) != 0) {
        printf("You havenot the right to change password......\n");
        return 1;
      }
    }

    flag_password = 1;
    printf("Please input new password");
    nbytes = readline (" ? ");
    flag_password = 0;       

    if(nbytes == -1) {
      printf("\nDiscard input new password\n");
      return 1;
    }
    if (nbytes == -2) {
      printf("\nTime out,exit the command\n");  /* timed out, exit the command	*/
      return 1;
    }
    if((nbytes == 1) && (console_buffer[0] == '.')) {
      printf("\nExit input new password\n");
      return 1;
    }

    for(i=0;i<=nbytes;i++){
      newpassword[i]= console_buffer[i];
    }

    flag_password = 1;
    printf("Confirm input new password");
    nbytes = readline (" ? ");
    flag_password = 0;       

    if(nbytes == -1) {
      printf("\nDiscard confirm new password\n");
      return 1;
    }
    if (nbytes == -2) {
      printf("\nTime out,exit the command\n");  /* timed out, exit the command	*/
      return 1;
    }
    if((nbytes == 1) && (console_buffer[0] == '.')) {
      printf("\nExit confirm new password\n");
      return 1;
    }

    if(strcmp(newpassword,console_buffer) != 0) {
      printf("input new password cannot confirm exit......\n");
      return 1;
    }

    addr = 620;
    newpassword[0]=nbytes;
    if(i2c_write(chip, addr++, 1, &newpassword[0], 1) != 0) {     
      puts ("Error writing the chip.\n");
      return 1;
    }
    udelay(11000);

    addr = 621;
    for (i=0;i<nbytes;i++) {
      if(i2c_write(chip, addr++, 1, &console_buffer[i], 1) != 0) {
        puts ("Error writing the chip.\n");
        return 1;
      }
      udelay(11000);
    }

    return 0;
}

/* JieHan 05/19/2005 */
int do_i2c_rtcr ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u_char	chip;
	uint	addr;
	int linebytes;
    unsigned char linebuf[20];


    if (argc != 1) {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    /* M41ST85W chip adress */
    /* Slave address(MSB---LSB):1101000 */
    chip = 0x68;

    linebuf[12] = 0;
    addr = 12;
    if(i2c_write(chip, addr, 1, &linebuf[12], 1) != 0) {
        puts ("Error writing the RTC.\n");
        return 1;
    }
    udelay(11000);

    addr = 0;
    linebytes = 20;
    if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
      puts ("Error reading the RTC.\n");
      return 1;
    }
    else {
      printf("Time:%02x:%02x:%02x.%02x Date:20%02x-%02x-%02x\n",
             linebuf[3],linebuf[2],linebuf[1],linebuf[0],
             linebuf[7],linebuf[6],linebuf[5]);
    }

    return 0;
}

int do_i2c_rtcw ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u_char	chip;
	uint	addr;
	int	i,nbytes,linebytes;
    unsigned char linebuf[20];
    extern char console_buffer[];

    if (argc != 1) {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    /* M41ST85W chip adress */
    chip = 0x68;

    addr = 0;
    linebytes = 20;
    if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
      puts ("Error reading the RTC.\n");
      return 1;
    }
    else {
      printf("Date:20%02x-%02x-%02x\n",
             linebuf[7],linebuf[6],linebuf[5]);
      printf("Input date format:yymmdd\n");
    }
    nbytes = readline (" ? ");

    if(nbytes == -1) {
      printf("\n");
      return 1;
    }
    if (nbytes == -2) {
      printf("\n");          /* timed out, exit the command	*/
      return 1;
    }

    if((nbytes == 1) && (console_buffer[0] == '.')) {
      printf("\n");
      return 1;
    }

    /* <CR> pressed as only input, don't modify current
     * location and move to next. 
     */
    if (nbytes != 0 ){
      /* 0/0/10 Date/Date:Day of Month */
      linebuf[5] = (console_buffer[4]-0x30) << 4;
      linebuf[5] = linebuf[5] + (console_buffer[5]-0x30);

      /* 0/0/0/10M/month*/
      linebuf[6] = (console_buffer[2]-0x30) << 4;
      linebuf[6] = linebuf[6] + (console_buffer[3]-0x30);

      /* 10 years/year*/
      linebuf[7] = (console_buffer[0]-0x30) << 4;
      linebuf[7] = linebuf[7] + (console_buffer[1]-0x30);
    }

    printf("Time:%02x:%02x:%02x.%02x\n",
            linebuf[3],linebuf[2],linebuf[1],linebuf[0]);
    printf("Input date format:hhmmss\n");
    nbytes = readline (" ? ");

    if(nbytes == -1) {
      printf("\n");
      return 1;
    }
    if (nbytes == -2) {
      printf("\n");          /* timed out, exit the command	*/
      return 1;
    }

    if((nbytes == 1) && (console_buffer[0] == '.')) {
      printf("\n");
      return 1;
    }

    /* <CR> pressed as only input, don't modify current
     * location and move to next. 
     */
    if (nbytes != 0 ){
      /* ST/10 Seconds/Seconds */
      linebuf[1] = (console_buffer[4]-0x30) << 4;
      linebuf[1] = linebuf[1] + (console_buffer[5]-0x30);

      /* 0/10 Minutes/Minutes */
      linebuf[2] = (console_buffer[2]-0x30) << 4;
      linebuf[2] = linebuf[2] + (console_buffer[3]-0x30);

      /* CEB/CB/10Hours/Hours */
      linebuf[3] = (console_buffer[0]-0x30) << 4;
      linebuf[3] = linebuf[3] + (console_buffer[1]-0x30);
    }

    /* Clear HT flag, Halt Update Bit */
    linebuf[12] = 0;
    addr = 0;
    for (i=0;i<13;i++) {
      if(i2c_write(chip, addr++, 1, &linebuf[i], 1) != 0) {
        puts ("Error writing the RTC.\n");
        return 1;
      }
      udelay(11000);
    }

    return 0;
}

int do_i2c_rtcc ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u_char	chip;
	uint	addr;
	int	i,linebytes;
    unsigned char linebuf[20];
	unsigned char *cp;

    if (argc != 1) {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }
    /* M41ST85W chip adress */
    /* Slave address(MSB---LSB):1101000 */
    chip = 0x68;

    addr = 0;
    linebytes = 20;
    if(i2c_read(chip, addr, 1, linebuf, linebytes) != 0) {
      puts ("Error reading the RTC.\n");
      return 1;
    }
    else {
      if((linebuf[1] & 0x80) == 0x80) {
        printf("Stop Bit is 1,cause oscillator stop,please input new time & date,clean st bit\n");
        return 1;
      }

      if((linebuf[1] & 0x80) == 0x80) {
        printf("BL bit is 1,Battery low voltage\n");
        return 1;
      }

      linebuf[12] = 0;
      addr = 12;
      if(i2c_write(chip, addr, 1, &linebuf[12], 1) != 0) {
          puts ("Error writing the RTC.\n");
          return 1;
      }
      udelay(11000);

    }
    return 0;
}

/*
 * The SDRAM command is separately configured because many
 * (most?) embedded boards don't use SDRAM DIMMs.
 */
#if (CONFIG_COMMANDS & CFG_CMD_SDRAM)

/*
 * Syntax:
 *	sdram {i2c_chip}
 */
int do_sdram  ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u_char	chip;
	u_char	data[128];
	u_char	cksum;
	int	j;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	/*
	 * Chip is always specified.
 	 */
	chip = simple_strtoul(argv[1], NULL, 16);

	if(i2c_read(chip, 0, 1, data, sizeof(data)) != 0) {
		puts ("No SDRAM Serial Presence Detect found.\n");
		return 1;
	}

	cksum = 0;
	for (j = 0; j < 63; j++) {
		cksum += data[j];
	}
	if(cksum != data[63]) {
		printf ("WARNING: Configuration data checksum failure:\n"
			"  is 0x%02x, calculated 0x%02x\n",
			data[63], cksum);
	}
	printf("SPD data revision            %d.%d\n",
		(data[62] >> 4) & 0x0F, data[62] & 0x0F);
	printf("Bytes used                   0x%02X\n", data[0]);
	printf("Serial memory size           0x%02X\n", 1 << data[1]);
	puts ("Memory type                  ");
	switch(data[2]) {
		case 2:  puts ("EDO\n");	break;
		case 4:  puts ("SDRAM\n");	break;
		default: puts ("unknown\n");	break;
	}
	puts ("Row address bits             ");
	if((data[3] & 0x00F0) == 0) {
		printf("%d\n", data[3] & 0x0F);
	} else {
		printf("%d/%d\n", data[3] & 0x0F, (data[3] >> 4) & 0x0F);
	}
	puts ("Column address bits          ");
	if((data[4] & 0x00F0) == 0) {
		printf("%d\n", data[4] & 0x0F);
	} else {
		printf("%d/%d\n", data[4] & 0x0F, (data[4] >> 4) & 0x0F);
	}
	printf("Module rows                  %d\n", data[5]);
	printf("Module data width            %d bits\n", (data[7] << 8) | data[6]);
	puts ("Interface signal levels      ");
	switch(data[8]) {
		case 0:  puts ("5.0v/TTL\n");	break;
		case 1:  puts ("LVTTL\n");	break;
		case 2:  puts ("HSTL 1.5\n");	break;
		case 3:  puts ("SSTL 3.3\n");	break;
		case 4:  puts ("SSTL 2.5\n");	break;
		default: puts ("unknown\n");	break;
	}
	printf("SDRAM cycle time             %d.%d nS\n",
		(data[9] >> 4) & 0x0F, data[9] & 0x0F);
	printf("SDRAM access time            %d.%d nS\n",
		(data[10] >> 4) & 0x0F, data[10] & 0x0F);
	puts ("EDC configuration            ");
	switch(data[11]) {
		case 0:  puts ("None\n");	break;
		case 1:  puts ("Parity\n");	break;
		case 2:  puts ("ECC\n");	break;
		default: puts ("unknown\n");	break;
	}
	if((data[12] & 0x80) == 0) {
		puts ("No self refresh, rate        ");
	} else {
		puts ("Self refresh, rate           ");
	}
	switch(data[12] & 0x7F) {
		case 0:  puts ("15.625uS\n");	break;
		case 1:  puts ("3.9uS\n");	break;
		case 2:  puts ("7.8uS\n");	break;
		case 3:  puts ("31.3uS\n");	break;
		case 4:  puts ("62.5uS\n");	break;
		case 5:  puts ("125uS\n");	break;
		default: puts ("unknown\n");	break;
	}
	printf("SDRAM width (primary)        %d\n", data[13] & 0x7F);
	if((data[13] & 0x80) != 0) {
		printf("  (second bank)              %d\n",
			2 * (data[13] & 0x7F));
	}
	if(data[14] != 0) {
		printf("EDC width                    %d\n",
			data[14] & 0x7F);
		if((data[14] & 0x80) != 0) {
			printf("  (second bank)              %d\n",
				2 * (data[14] & 0x7F));
		}
	}
	printf("Min clock delay, back-to-back random column addresses %d\n",
		data[15]);
	puts ("Burst length(s)             ");
	if (data[16] & 0x80) puts (" Page");
	if (data[16] & 0x08) puts (" 8");
	if (data[16] & 0x04) puts (" 4");
	if (data[16] & 0x02) puts (" 2");
	if (data[16] & 0x01) puts (" 1");
	putc ('\n');
	printf("Number of banks              %d\n", data[17]);
	puts ("CAS latency(s)              ");
	if (data[18] & 0x80) puts (" TBD");
	if (data[18] & 0x40) puts (" 7");
	if (data[18] & 0x20) puts (" 6");
	if (data[18] & 0x10) puts (" 5");
	if (data[18] & 0x08) puts (" 4");
	if (data[18] & 0x04) puts (" 3");
	if (data[18] & 0x02) puts (" 2");
	if (data[18] & 0x01) puts (" 1");
	putc ('\n');
	puts ("CS latency(s)               ");
	if (data[19] & 0x80) puts (" TBD");
	if (data[19] & 0x40) puts (" 6");
	if (data[19] & 0x20) puts (" 5");
	if (data[19] & 0x10) puts (" 4");
	if (data[19] & 0x08) puts (" 3");
	if (data[19] & 0x04) puts (" 2");
	if (data[19] & 0x02) puts (" 1");
	if (data[19] & 0x01) puts (" 0");
	putc ('\n');
	puts ("WE latency(s)               ");
	if (data[20] & 0x80) puts (" TBD");
	if (data[20] & 0x40) puts (" 6");
	if (data[20] & 0x20) puts (" 5");
	if (data[20] & 0x10) puts (" 4");
	if (data[20] & 0x08) puts (" 3");
	if (data[20] & 0x04) puts (" 2");
	if (data[20] & 0x02) puts (" 1");
	if (data[20] & 0x01) puts (" 0");
	putc ('\n');
	puts ("Module attributes:\n");
	if (!data[21])       puts ("  (none)\n");
	if (data[21] & 0x80) puts ("  TBD (bit 7)\n");
	if (data[21] & 0x40) puts ("  Redundant row address\n");
	if (data[21] & 0x20) puts ("  Differential clock input\n");
	if (data[21] & 0x10) puts ("  Registerd DQMB inputs\n");
	if (data[21] & 0x08) puts ("  Buffered DQMB inputs\n");
	if (data[21] & 0x04) puts ("  On-card PLL\n");
	if (data[21] & 0x02) puts ("  Registered address/control lines\n");
	if (data[21] & 0x01) puts ("  Buffered address/control lines\n");
	puts ("Device attributes:\n");
	if (data[22] & 0x80) puts ("  TBD (bit 7)\n");
	if (data[22] & 0x40) puts ("  TBD (bit 6)\n");
	if (data[22] & 0x20) puts ("  Upper Vcc tolerance 5%\n");
	else                 puts ("  Upper Vcc tolerance 10%\n");
	if (data[22] & 0x10) puts ("  Lower Vcc tolerance 5%\n");
	else                 puts ("  Lower Vcc tolerance 10%\n");
	if (data[22] & 0x08) puts ("  Supports write1/read burst\n");
	if (data[22] & 0x04) puts ("  Supports precharge all\n");
	if (data[22] & 0x02) puts ("  Supports auto precharge\n");
	if (data[22] & 0x01) puts ("  Supports early RAS# precharge\n");
	printf("SDRAM cycle time (2nd highest CAS latency)        %d.%d nS\n",
		(data[23] >> 4) & 0x0F, data[23] & 0x0F);
	printf("SDRAM access from clock (2nd highest CAS latency) %d.%d nS\n",
		(data[24] >> 4) & 0x0F, data[24] & 0x0F);
	printf("SDRAM cycle time (3rd highest CAS latency)        %d.%d nS\n",
		(data[25] >> 4) & 0x0F, data[25] & 0x0F);
	printf("SDRAM access from clock (3rd highest CAS latency) %d.%d nS\n",
		(data[26] >> 4) & 0x0F, data[26] & 0x0F);
	printf("Minimum row precharge        %d nS\n", data[27]);
	printf("Row active to row active min %d nS\n", data[28]);
	printf("RAS to CAS delay min         %d nS\n", data[29]);
	printf("Minimum RAS pulse width      %d nS\n", data[30]);
	puts ("Density of each row         ");
	if (data[31] & 0x80) puts (" 512");
	if (data[31] & 0x40) puts (" 256");
	if (data[31] & 0x20) puts (" 128");
	if (data[31] & 0x10) puts (" 64");
	if (data[31] & 0x08) puts (" 32");
	if (data[31] & 0x04) puts (" 16");
	if (data[31] & 0x02) puts (" 8");
	if (data[31] & 0x01) puts (" 4");
	puts ("MByte\n");
	printf("Command and Address setup    %c%d.%d nS\n",
		(data[32] & 0x80) ? '-' : '+',
		(data[32] >> 4) & 0x07, data[32] & 0x0F);
	printf("Command and Address hold     %c%d.%d nS\n",
		(data[33] & 0x80) ? '-' : '+',
		(data[33] >> 4) & 0x07, data[33] & 0x0F);
	printf("Data signal input setup      %c%d.%d nS\n",
		(data[34] & 0x80) ? '-' : '+',
		(data[34] >> 4) & 0x07, data[34] & 0x0F);
	printf("Data signal input hold       %c%d.%d nS\n",
		(data[35] & 0x80) ? '-' : '+',
		(data[35] >> 4) & 0x07, data[35] & 0x0F);
	puts ("Manufacturer's JEDEC ID      ");
	for(j = 64; j <= 71; j++)
		printf("%02X ", data[j]);
	putc ('\n');
	printf("Manufacturing Location       %02X\n", data[72]);
	puts ("Manufacturer's Part Number   ");
	for(j = 73; j <= 90; j++)
		printf("%02X ", data[j]);
	putc ('\n');
	printf("Revision Code                %02X %02X\n", data[91], data[92]);
	printf("Manufacturing Date           %02X %02X\n", data[93], data[94]);
	puts ("Assembly Serial Number       ");
	for(j = 95; j <= 98; j++)
		printf("%02X ", data[j]);
	putc ('\n');
	printf("Speed rating                 PC%d\n",
		data[126] == 0x66 ? 66 : data[126]);

	return 0;
}
#endif	/* CFG_CMD_SDRAM */


/***************************************************/

U_BOOT_CMD(
	imd,	4,	1,	do_i2c_md,		\
	"imd     - i2c memory display\n",				\
	"chip address[.0, .1, .2] [# of objects]\n    - i2c memory display\n" \
);

U_BOOT_CMD(
 	imm,	3,	1,	do_i2c_mm,
	"imm     - i2c memory modify (auto-incrementing)\n",
	"chip address[.0, .1, .2]\n"
	"    - memory modify, auto increment address\n"
);
U_BOOT_CMD(
	inm,	3,	1,	do_i2c_nm,
	"inm     - memory modify (constant address)\n",
	"chip address[.0, .1, .2]\n    - memory modify, read and keep address\n"
);

U_BOOT_CMD(
	imw,	5,	1,	do_i2c_mw,
	"imw     - memory write (fill)\n",
	"chip address[.0, .1, .2] value [count]\n    - memory write (fill)\n"
);

U_BOOT_CMD(
	icrc32,	5,	1,	do_i2c_crc,
	"icrc32  - checksum calculation\n",
	"chip address[.0, .1, .2] count\n    - compute CRC32 checksum\n"
);

U_BOOT_CMD(
	iprobe,	1,	1,	do_i2c_probe,
	"iprobe  - probe to discover valid I2C chip addresses\n",
	"\n    -discover valid I2C chip addresses\n"
);

/*
 * Require full name for "iloop" because it is an infinite loop!
 */
U_BOOT_CMD(
	iloop,	5,	1,	do_i2c_loop,
	"iloop   - infinite loop on address range\n",
	"chip address[.0, .1, .2] [# of objects]\n"
	"    - loop, reading a set of addresses\n"
);

/* JieHan 04/21/2005 */
U_BOOT_CMD(
	hweepromw,	1,	0,	do_i2c_hw,
	"hweepromw     - write board specific information to EEPROM\n",
	"\n    - Write board information to EEPROM\n"
);

U_BOOT_CMD(
	hweepromr, 1, 0, do_i2c_hr,
	"hweepromr     - display board specific information from EEPROM\n",
	"\n    - Display board information from EEPROM\n"
);

/* JieHan 05/19/2005 */
U_BOOT_CMD(
	showtime, 1, 0, do_i2c_rtcr,
	"showtime     - display time information from RTC\n",
	"\n    - Display time information from RTC\n"
);

U_BOOT_CMD(
	settime, 1, 0, do_i2c_rtcw,
	"settime     - config time parameter to RTC\n",
	"\n    - config time parameter to RTC\n"
);

U_BOOT_CMD(
	adjusttime, 1, 0, do_i2c_rtcc,
	"adjusttime     - Calibrate Clock\n",
	"\n    - Calibrate Clock\n"
);

U_BOOT_CMD(
	changepassword, 1, 0, do_i2c_password,
	"changepassword     - Change password\n",
	"\n    - Change password\n"
);

U_BOOT_CMD(
	imagetype, 2, 1, do_i2c_image,
	"imagetype     - Choice boot application image type(0:primary,1:backup,2:minimal)\n",
	"\n    - Choice boot image type\n"
);

U_BOOT_CMD(
	sfpconfig, 3, 1, do_i2c_sfp,
	"sfpconfig     - show SFP paramter \n",
	"\n    - show SFP parameter\n"
);

#if (CONFIG_COMMANDS & CFG_CMD_SDRAM)
U_BOOT_CMD(
	isdram,	2,	1,	do_sdram,
	"isdram  - print SDRAM configuration information\n",
	"chip\n    - print SDRAM configuration information\n"
	"      (valid chip values 50..57)\n"
);
#endif
#endif	/* CFG_CMD_I2C */
