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

#include "util.h"
#include "printf.h"
#include "errno.h"
#include "command.h"

/* the first command */
commandlist_t *commands;

/*
 * Supported commands
 */

/* echo */
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
	"echo the characters typed in hex (space to quit)\n";

__commandlist(echo, "echo", echo_help);

/* help command */
static int help(int argc, char* argv[])
{
	commandlist_t *cmd;

	/* help on a command? */
	if (argc >= 2) {
		for (cmd = commands; cmd != NULL; cmd = cmd->next) {
			if (_strncmp(cmd->name, argv[1],
				   MAX_COMMANDLINE_LENGTH) == 0) {
				printf("Help for '%s' ':\n\nUsage: %s", argv[1], cmd->help);
				return 0;
			}
		}

		return -EINVAL;
	}

	printf("The following commands are supported:");

	for (cmd = commands; cmd != NULL; cmd = cmd->next) {
		printf("\n* %s", cmd->name);
	}

	printf("\nUse \"help command\" to get help on a specific command\n");

	return 0;
}

static char help_help[] = "help [command]\n"
	"Get help on [command], "
"	or a list of supported commands if a command is omitted.\n";

__commandlist(help, "help", help_help);


/* reset console */
static int quit(int argc, char* argv[])
{
	return PROGRAM_EXIT;
}

static char quit_help[] = "quit\n"
	"Quit application (return to boot ROM console)\n";

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
	"Reset terminal\n";

__commandlist(reset, "reset", reset_help);
