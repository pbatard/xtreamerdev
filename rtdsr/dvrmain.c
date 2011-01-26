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
#include "realtek.h"
#include "util.h"
#include "printf.h"
#include "ymodem.h"
#include "command.h"


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


#define VERSION "0.4"

int dvrmain(void)
{
	int r;
	unsigned short cpu_id;
	char commandline[MAX_COMMANDLINE_LENGTH];

	init_printf(NULL, putc);
	init_commands();

	printf("\n\nrtdsr v" VERSION ", Copyright (c) 2011 Pete B. <xtreamerdev@gmail.com>\n\n");
	printf("rtdsr comes with ABSOLUTELY NO WARRANTY.");
	printf("This program is free software, you are welcome to redistribute it under\n");
	printf("certain conditions. See http://www.gnu.org/licenses/gpl.html for details.\n\n");

	/* check CPU ID */
	cpu_id = REG32(RTGALAXY_SB2_CHIP_ID) & 0xffff;
	if (cpu_id != RTGALAXY_MARS) {
		printf("Wrong CPU ID detected (%04X) - aborting\n", cpu_id);
		return -1;
	}

	printf("CPU:%d.%dMHz, RAM_BASE:0x%08x, FLASH_BASE:0x%08x\n", CPU_FREQUENCY/1000000,
		(CPU_FREQUENCY%1000000)/100000, RAM_BASE, FLASH_BASE);

	for (;;) {
		printf("rtdsr> ");

		r = get_command(commandline, MAX_COMMANDLINE_LENGTH, -1);

		if (r > 0) {
			if ((r = parse_command(commandline)) < 0 ) {
				if (r == PROGRAM_EXIT) {
					return 0;
				}
				printf("error %d executing command\n", r);
			}
		}
	}

	return 0;
}
