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

extern int info(int argc, char* argv[]);

int dvrmain(void)
{
	int r;
	unsigned short cpu_id;
	char commandline[MAX_COMMANDLINE_LENGTH];

	init_printf(NULL, _putc);
	init_commands();

	printf("\n");
	info(0, NULL);

	/* check CPU ID */
	cpu_id = REG32(RTGALAXY_SB2_CHIP_ID) & 0xffff;
	if (cpu_id != RTGALAXY_MARS) {
		printf("Wrong CPU ID detected (%04X) - aborting\n", cpu_id);
		return -1;
	}

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
