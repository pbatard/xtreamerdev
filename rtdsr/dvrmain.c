/* RTD Serial Recovery (rtdsr)
 *
 * copyright (c) 2011 Pete B. <xtreamerdev@gmail.com>
 *
 * based on util.c from Realtek bootloader set_pll, not copyrighted
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
#include "util.h"
#include "printf.h"
#include "ymodem.h"
#include "command.h"
/*
#include "flashdev_n.h"
#include "flashdev_p.h"
#include "flashdev_s.h"

#define FLASH_TYPE_NOR_PARALLEL		0x0
#define FLASH_TYPE_NOR_SERIAL		0x1
#define FLASH_TYPE_NAND				0x2
#define FLASH_TYPE_PCI				0x3

#define FLASH_MAGICNO_NOR_PARALLEL	0xbe
#define FLASH_MAGICNO_NAND			0xce
#define FLASH_MAGICNO_NOR_SERIAL	0xde
*/

/*
extern UINT32 pages_per_block;
extern UINT32 blocks_per_flash;
*/

/* for printf -*/
void putc(void* p, char c)
{
	if (c == '\n') {
		c = 0xd;
		serial_write(&c);
		c = 0xa;
	}
	serial_write(&c);
}

void display_buffer_hex(UINT32 addr, UINT32 size)
{
	UINT32 i, j, k;

	for (i=0; i<size; i+=16) {
		printf("\n  %08lx  ", addr+i);
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
	}
	printf("\n" );
}

#define VERSION "0.4"

int dvrmain(void)
{
	int r;
	char commandline[MAX_COMMANDLINE_LENGTH];

	init_printf(NULL, putc);
	init_commands();

	printf("\n\nrtdsr v" VERSION ", Copyright (c) 2011 Pete B. <xtreamerdev@gmail.com>\n\n");
	printf("rtdsr comes with ABSOLUTELY NO WARRANTY.\n");
	printf("This program is free software, you are welcome to redistribute it under\n");
	printf("certain conditions. See http://www.gnu.org/licenses/gpl.html for details.\n\n");

	for (;;) {
		printf("rtdsr> ");

		/* wait 10 minutes for a command */
		r = get_command(commandline, MAX_COMMANDLINE_LENGTH, 600);

		if (r > 0) {
			if ((r = parse_command(commandline)) < 0 ) {
				if (r == PROGRAM_EXIT) {
					return 0;
				}
				printf("error %d executing command\n", r);
			}
		}
	}

#ifdef DUMP_BOOTROM
	ret = ymodem_send((unsigned char*)0xBFC00000, 0x2000, "BootROM.bin");
#endif

#ifdef YMODEM_RCV
	ret = ymodem_receive((unsigned char*)DATA_TMP_ADDR, DATA_TMP_SIZE);
	if (ret > 0) {
		display_buffer_hex(DATA_TMP_ADDR, ret);
	}
	return 0;
#endif

/*
	printf("Detecting flash type: ");
	switch(REG32(0xb8000304) & 0x3)
	{
		case FLASH_TYPE_NOR_PARALLEL:
			printf("parallel\n");
			break;

		case FLASH_TYPE_NOR_SERIAL:
			printf("serial\n");
			break;

		case FLASH_TYPE_NAND:
			printf("nand\n");
			break;

		case FLASH_TYPE_PCI:
		default:
			printf("unknown!\n");
			return -1;
	}
*/
	return 0;
}
