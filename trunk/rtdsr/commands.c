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
#include "flashdev_p.h"
#include "flashdev_s.h"


#define FLASH_MAGICNO_NOR_PARALLEL	0xbe
#define FLASH_MAGICNO_NAND			0xce
#define FLASH_MAGICNO_NOR_SERIAL	0xde


/* the first command */
commandlist_t *commands;


static unsigned long display_buffer_hex(unsigned long addr, unsigned long size)
{
	unsigned long i, j, k;

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
		if ((i%256 == 0) && (serial_read() == KEY_CTRL_C)) {
			i+=16;
			break;
		}
	}
	/* Number of bytes read so far */
	return addr+i;
}


/*
 * Supported commands
 */

/* detect flash type */
static int detect_flash(int argc, char* argv[])
{
	printf("Detecting flash type: ");
	switch(REG32(RTGALAXY_INFO_FLASH) & RTGALAXY_FLASH_TYPE_MASK)
	{
		case RTGALAXY_FLASH_TYPE_NOR_PARALLEL:
			printf("NOR Parallel\n");
			break;

		case RTGALAXY_FLASH_TYPE_NOR_SERIAL:
			printf("NOR Serial\n");
			break;

		case RTGALAXY_FLASH_TYPE_NAND:
			printf("NAND (Parallel)\n");
			break;

		case RTGALAXY_FLASH_TYPE_PCI:
		default:
			printf("unknown!\n");
			break;
	}

	return 0;
}

static char detect_flash_help[] = "detect_flash\n"
	"\nDetect the Flash type\n";

__commandlist(detect_flash, "detect_flash", detect_flash_help);


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
	static unsigned long address = RAM_BASE, max_size = RAM_SIZE;
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
	if (argc > 3) {
		// TODO
	}

	r = ymodem_send((unsigned char*)address, size, "test.bin");
	return (r > 0)?0:r;
}

static char ysend_help[] = "ysend [address] [size] [filename]\n"
	"\nSend a file, through Ymodem (Ymodem-G not supported)\n"
	"If <address> is not provided, <RAM_BASE> will be used for the start address\n"
	"If <size> is not specified, the default 8KB size (0x2000) is used\n"
	"If <filename> is not specified, it is set to '<address>_<size>.bin'\n";

__commandlist(ysend, "ysend", ysend_help);
