/* individual commands RTD Serial Recovery (rtdsr)
 *
 * copyright (c) 2011 Pete B. <xtreamerdev@gmail.com>
 *
 * based on command processing for openezx, copyright
 * (c) 1999 Erik Mouw, (c) 2001 Stefan Eletzhofer
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "../config.h"
#include "realtek.h"
#include "util.h"
#include "printf.h"
#include "errno.h"
#include "command.h"
#include "ymodem.h"
#include "flashdev_n.h"

/* the first command */
commandlist_t *commands;

/* Flash info */
n_device_type* device = NULL;

/* unsigned long to fixed length hexascii */
static void ul2ha(unsigned long num, char* bf)
{
	unsigned long dgt, d=0x10000000;

	while (d!=0) {
		dgt = num/d;
		*bf++ = dgt+(dgt<10?'0':'A'-10);
		num %= d;
		d = d>>4;
	}
	*bf=0;
}

static unsigned long display_buffer_hex(unsigned long addr, unsigned long size)
{
	register unsigned long i, j, k;

	for (i=0; i<size; i+=16) {
		printf("  %08lx  ", addr+i);
		for (j=0,k=0; k<16; j++,k++) {
			if (i+j < size) {
				printf("%02x", REG8(addr+i+j));
			} else {
				printf("  ");
			}
			printf(" ");
		}
		printf(" ");
		for (j=0,k=0; k<16; j++,k++) {
			if (i+j < size) {
				if ((REG8(addr+i+j) < 32) || (REG8(addr+i+j) > 126)) {
					printf(".");
				} else {
					printf("%c", REG8(addr+i+j));
				}
			}
		}
		printf("\n");

		/* Someone might want to abort - check for Ctrl-C */
		if (i%256 == 0) {
			register int c = serial_read();
			if ( (c == KEY_CTRL_C) || (c == KEY_ESCAPE) ) {
				i+=16;
				break;
			}
		}
	}
	/* Number of bytes read so far */
	return addr+i;
}


/*
 * Supported commands
 */


/* serial echo */
static int echo(int argc, char* argv[])
{
	int c;

	do {
		c = _getchar(-1);
		if (c < 0) {
			break;
		}
		printf("0x%X\n", c);
	} while (c != 0x20);

	return 0;
}

static char echo_help[] = "echo\n"
	"\nEcho the characters typed in hexa (space to quit)\n";

__commandlist(echo, "echo", echo_help);


/* flash info */
static int flash_info(int argc, char* argv[])
{
	/* Doubt anybody will need anything but NAND */
	if ((REG32(RTGALAXY_INFO_FLASH) & RTGALAXY_FLASH_TYPE_MASK) != RTGALAXY_FLASH_TYPE_NAND) {
		printf("Flash type not supported (NAND only)\n");
		return -1;
	}

	/* Identify NAND chip properties */
	if (nf_identify(&device) < 0) {
		printf("Unable to detect NAND Flash type\n");
		return -1;
	}

	printf("NAND Flash: Type:%s, Size:%dMB, PageSize:0x%x, BlockSize:0x%x\n",
		device->string, (device->size)/0x100000, device->PageSize, device->BlockSize);

	/* This call initializes the Block State Table @ FLASH_BST_ADDR    */
	/* Do it here, so we can overwrite the BST with yreceive if needed */
	nf_init(device);

	return 0;
}

static char finfo_help[] = "finfo\n"
	"\nProvide info (page size, block size, total size) about the Flash\n";

__commandlist(flash_info, "finfo", finfo_help);


/* flash read */
static int flash_read(int argc, char* argv[])
{
	unsigned long offset=0, size=0, dest=RAM_BASE;
	unsigned long start_page;

	if ((device == NULL) && (flash_info(0, NULL) != 0)) {
		return -1;
	}

	if (argc > 1) {
		offset = _strtoul(argv[1], NULL, 16);
		if (offset % device->PageSize) {
			printf("offset 0x%08x is not a multiple of PageSize (0x%x) - aborting\n", offset, device->PageSize);
			return -1;
		}
	}
	if (argc > 2) {
		size = _strtoul(argv[2], NULL, 16);
		if (size % device->PageSize) {
			printf("size 0x%x is not a multiple of PageSize (0x%x) - aborting\n", size, device->PageSize);
			return -1;
		}
	}
	if (size == 0) {
		size = device->PageSize;
	}
	if (argc > 3) {
		dest = _strtoul(argv[3], NULL, 16);
	}

	start_page = offset/device->PageSize;

	/* Read a set of pages */
	if (nf_read(device, start_page, (UINT8*)dest, size)) {
		printf("Reading of Flash failed\n");
		return -1;
	}
	printf("Read 0x%x bytes (0x%x pages) starting at offset 0x%08x (page 0x%x)\n",
		size, size/device->PageSize, offset, start_page);

	return 0;
}

static char fread_help[] = "fread [offset] [size] [dest]\n"
	"\nRead <size> bytes, starting at <offset> from beginning of Flash, to <dest>\n"
	"If omitted, offset=0, size=<NAND PageSize> and dest=<RAM_BASE>\n"
	"<offset> and <size> must be multiples of the NAND BlockSize\n";

__commandlist(flash_read, "fread", fread_help);


/* flash write */
static int flash_write(int argc, char* argv[])
{
	unsigned long offset=0, size=0, src=RAM_BASE;
	unsigned long start_block;
	int c;

	if ((device == NULL) && (flash_info(0, NULL) != 0)) {
		return -1;
	}

	if (argc < 3) {
		printf("You must supply a Flash offset and size argument\n");
		return -1;
	}

	offset = _strtoul(argv[1], NULL, 16);
	if (offset % device->BlockSize) {
		printf("offset 0x%08x is not a multiple of BlockSize (0x%x) - aborting\n", offset, device->BlockSize);
		return -1;
	}
	size = _strtoul(argv[2], NULL, 16);
	if (size % device->BlockSize) {
		printf("size 0x%x is not a multiple of BlockSize (0x%x) - aborting\n", size, device->BlockSize);
		return -1;
	}
	if (size == 0) {
		printf("size cannot be zero\n");
		return -1;
	}
	if (argc > 3) {
		src = _strtoul(argv[3], NULL, 16);
	}

	start_block = offset/device->BlockSize;

	printf("About to write 0x%x bytes from RAM:0x%08x to Flash offset:0x%08x\n",
		size, src, offset);
	printf("If this is really what you want, press Y\n");
	c = _getchar(-1);
	if (c != 'Y' && c != 'y') {
		printf("Operation cancelled\n");
		return -1;
	}

	/* Write a set of blocks */
	if (nf_write(device, start_block, (UINT8*)src, size)) {
		printf("\nWriting of Flash failed!\n");
		return -1;
	}
	printf("\nWrote 0x%x bytes (0x%x blocks) starting at offset 0x%08x (block 0x%x)\n",
		size, size/device->BlockSize, offset, start_block);

	return 0;
}

static char fwrite_help[] = "fwrite <offset> <size> [src]\n"
	"\nWrite <size> bytes, from address <src> into Flash, at offset <offset>\n"
	"<offset> and <size> are mandatory and must be multiples of the NAND PageSize\n"
	"If omitted, src=<RAM_BASE>\n";

__commandlist(flash_write, "fwrite", fwrite_help);


/* display help */
static int help(int argc, char* argv[])
{
	commandlist_t *cmd;

	/* help on a command? */
	if (argc >= 2) {
		for (cmd = commands; cmd != NULL; cmd = cmd->next) {
			if (_strncmp(cmd->name, argv[1],
				   MAX_COMMANDLINE_LENGTH) == 0) {
				printf("Usage: %s", cmd->help);
				return 0;
			}
		}
		printf("unknown command %s\n", argv[1]);

		return -EINVAL;
	}

	printf("The following commands are supported:");

	for (cmd = commands; cmd != NULL; cmd = cmd->next) {
		printf("\n* %s", cmd->name);
	}

	printf("\nAll numeric parameters must be provided in hexa (with or without '0x' prefix)\n");
	printf("Use \"help command\" to get help on a specific command\n");

	return 0;
}

static char help_help[] = "help [command]\n"
	"\nGet help on <command>, or a list of supported commands if a command is omitted\n";

__commandlist(help, "help", help_help);


/* hexdump of memory */
static int hexdump(int argc, char* argv[])
{
	static unsigned long address = RAM_BASE, size = 0x100;

	if (argc > 1) {
		address = _strtoul(argv[1], NULL, 16);
	}
	if (argc > 2) {
		size = _strtoul(argv[2], NULL, 16);
	}

	/* remember last address reached */
	address = display_buffer_hex(address, size);

	return 0;
}

static char hexdump_help[] = "memdump [address] [size]\n"
	"\nhexdump <size> bytes of memory, starting at <address>\n"
	"If no parameter is given, hexdump continues from last address (or <RAM_BASE>)\n"
	"Can be interrupted with Ctrl-C\n";

__commandlist(hexdump, "memdump", hexdump_help);


/* quit application */
static int quit(int argc, char* argv[])
{
	return PROGRAM_EXIT;
}

static char quit_help[] = "quit\n"
	"\nQuit application (return to Boot ROM console)\n";

__commandlist(quit, "quit", quit_help);


/* reset console */
static int reset(int argc, char* argv[])
{
	int i;

	printf("          c");
	for(i = 0; i < 100; i++)
		printf("\n");

	printf("c");

	return 0;
}

static char reset_help[] = "reset\n"
	"\nReset the console\n";

__commandlist(reset, "reset", reset_help);


/* receive a file through the Ymodem protocol */
static int yreceive(int argc, char* argv[])
{
	static unsigned long address = RAM_BASE, max_size = 1024*1024;
	int r;

	if (argc > 1) {
		address = _strtoul(argv[1], NULL, 16);
	}
	if (argc > 2) {
		max_size = _strtoul(argv[2], NULL, 16);
	}

	r = ymodem_receive((unsigned char*)address, max_size);
	return (r > 0)?0:r;
}

static char yreceive_help[] = "yreceive [address] [max_size]\n"
	"\nReceive a file, through Ymodem (Ymodem-G not supported)\n"
	"If <address> is not provided, <RAM_BASE> will be used\n"
	"<max_size> can also be used to specify the maximum area to use from <address>\n";

__commandlist(yreceive, "yreceive", yreceive_help);


/* Send a data block through the Ymodem protocol */
static int ysend(int argc, char* argv[])
{
	static unsigned long address = RAM_BASE, size = 0x2000;
	char filename[] = "XXXXXXXX_XXXXXXXX.bin";
	int r;

	if (argc > 1) {
		address = _strtoul(argv[1], NULL, 16);
	}
	if (argc > 2) {
		size = _strtoul(argv[2], NULL, 16);
	}
	if (argc <= 3) {
		ul2ha(address, filename);
		filename[8] = '_';
		ul2ha(size, filename+9);
		filename[17] = '.';
	}

	r = ymodem_send((unsigned char*)address, size, (argc>3)?argv[3]:filename);
	return (r > 0)?0:r;
}

static char ysend_help[] = "ysend [address] [size] [filename]\n"
	"\nSend a file, through Ymodem (Ymodem-G not supported)\n"
	"If <address> is not provided, <RAM_BASE> will be used for the start address\n"
	"If <size> is not specified, the default 8KB size (0x2000) is used\n"
	"If <filename> is not specified, it is set to '<address>_<size>.bin'\n";

__commandlist(ysend, "ysend", ysend_help);
