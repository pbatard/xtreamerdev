/* command line processing for RTD Serial Recovery (rtdsr)
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

//#define CMD_DEBUG
//#define CMDHIST_DEBUG
#define MAX_HIST		32

/* command list start and end. filled in by the linker */
extern unsigned long __commandlist_start;
extern unsigned long __commandlist_end;

/* the first command */
commandlist_t *commands;

/* history */
static int	cmdhist_entries		= 0;
static int	cmdhist_read		= 0;
static int	cmdhist_write		= 0;
static char cmdhistory[MAX_HIST][MAX_COMMANDLINE_LENGTH];

int cmdhist_reset(void);
int cmdhist_push(char *cmd);
int cmdhist_next(char **cmd);
int cmdhist_prev(char **cmd);


void init_commands(void)
{
	commandlist_t *lastcommand;
	commandlist_t *cmd, *next_cmd;
	int i;

	cmdhist_read = 0;
	cmdhist_write = 0;
	cmdhist_entries = 0;
	commands = (commandlist_t*) &__commandlist_start;
	lastcommand = (commandlist_t*) &__commandlist_end;

	/* make a list */
	cmd = next_cmd = commands;
	next_cmd++;

	while (next_cmd < lastcommand) {
		cmd->next = next_cmd;
		cmd++;
		next_cmd++;
	}

	i = MAX_HIST - 1;
	while (i--) {
		cmdhistory[i][0]=0;
	}
}

#define STATE_WHITESPACE (0)
#define STATE_WORD (1)
static void parse_args(char *cmdline, int *argc, char **argv)
{
	char *c;
	int state = STATE_WHITESPACE;
	int i;

	*argc = 0;

	if (_strlen(cmdline) == 0)
		return;

	/* convert all tabs into single spaces */
	c = cmdline;
	while (*c != '\0') {
		if (*c == '\t')
			*c = ' ';

		c++;
	}

	c = cmdline;
	i = 0;

	/* now find all words on the command line */
	while (*c != '\0') {
		if (state == STATE_WHITESPACE) {
			if (*c != ' ') {
				argv[i] = c;
				i++;
				state = STATE_WORD;
			}
		} else { /* state == STATE_WORD */
			if (*c == ' ') {
				*c = '\0';
				state = STATE_WHITESPACE;
			}
		}

		c++;
	}

	*argc = i;

#ifdef CMD_DEBUG
	for (i = 0; i < *argc; i++) {
		printf("DBG: argv[%d] = %s\n", i, argv[i]);
	}
#endif
}




/* return the number of commands that match */
static int get_num_command_matches(char *cmdline)
{
	commandlist_t *cmd;
	int len;
	int num_matches = 0;

	len = _strlen(cmdline);

	for (cmd = commands; cmd != NULL; cmd = cmd->next) {
		if (cmd->magic != COMMAND_MAGIC) {
#ifdef CMD_DEBUG
		printf("DBG: %s(): Address = 0x%08x\n", __FUNCTION__, (unsigned long)cmd);
#endif
			return -EMAGIC;
		}

		if (_strncmp(cmd->name, cmdline, len) == 0)
			num_matches++;
	}

	return num_matches;
}


int parse_command(char *cmdline)
{
	commandlist_t *cmd;
	int argc, num_commands, len;
	char *argv[MAX_ARGS];

	parse_args(cmdline, &argc, argv);

	/* only whitespace */
	if (argc == 0)
		return 0;

	num_commands = get_num_command_matches(argv[0]);

	/* error */
	if (num_commands < 0)
		return num_commands;

	/* no command matches */
	if (num_commands == 0)
		return -ECOMMAND;

	/* ambiguous command */
	if (num_commands > 1)
		return -EAMBIGCMD;

	len = _strlen(argv[0]);

	/* single command, go for it */
	for (cmd = commands; cmd != NULL; cmd = cmd->next) {
		if (cmd->magic != COMMAND_MAGIC) {
#ifdef CMD_DEBUG
			printf("DBG: %s(): Address = 0x%08x\n", __FUNCTION__, (unsigned long)cmd);
#endif

			return -EMAGIC;
		}

		if (_strncmp(cmd->name, argv[0], len) == 0) {
			/* call function */
			return cmd->callback(argc, argv);
		}
	}

	return -ECOMMAND;
}



/* more or less like SerialInputString(), but with echo and backspace */
/* unlike the version in libblob, this version has a command history */
int GetCommand(char *command, int len, int timeout)
{
	int c;
	int i;
	int numRead;
	int maxRead = len - 1;

	cmdhist_reset();

	for (numRead = 0, i = 0; numRead < maxRead;) {
		/* try to get a byte from the serial port */
		c = _getchar(timeout);

		/* check for errors */
		if (c < 0) {
			command[i++] = '\0';
			printf("\n");
			return c;
		}

		if ((c == '\r') || (c == '\n')) {
			command[i++] = '\0';

			/* print newline */
			printf("\n");
			cmdhist_push(command);
			return(numRead);
		} else if (c == '\b') {
			if (i > 0) {
				i--;
				numRead--;
				/* cursor one position back. */
				printf("\b \b");
			}
		} else if (c == CMDHIST_KEY_UP) {
			char *cmd = NULL;
			/* get cmd from history */
			if (cmdhist_next(&cmd) != 0)
				continue;

			/* clear line */
			while (numRead--) {
				printf("\b \b");
			}

			/* display it */
			printf("%s", cmd);
			i = numRead = _strlen(cmd);
			_strncpy(command, cmd, MAX_COMMANDLINE_LENGTH);
		} else if (c == CMDHIST_KEY_DN) {
			char *cmd = NULL;
			/* get cmd from history */
			if (cmdhist_prev(&cmd) != 0)
				continue;

			/* clear line */
			while (numRead--) {
				printf("\b \b");
			}

			/* display it */
			printf("%s", cmd);
			i = numRead = _strlen(cmd);
			_strncpy(command, cmd, MAX_COMMANDLINE_LENGTH);
		} else {
			command[i++] = c;
			numRead++;

			/* print character */
			_putchar(c);
		}
	}

	cmdhist_push(command);
	return(numRead);
}

/* Push a command to the history buffer */
int cmdhist_push(char *cmd)
{
	if (!cmd)
		return -EINVAL;

	if (_strlen(cmd) > MAX_COMMANDLINE_LENGTH)
		return -EINVAL;

	if (_strlen( cmd ) == 0)
		return 0;

	_strncpy(cmdhistory[cmdhist_write], cmd, MAX_COMMANDLINE_LENGTH);

	cmdhist_write ++;
	cmdhist_write = cmdhist_write % MAX_HIST;

	if (cmdhist_entries < MAX_HIST)
		cmdhist_entries++;

#if CMDHIST_DEBUG
	printf("e=%d, r=%d, w=%d\n", cmdhist_entries, cmdhist_read, cmdhist_write);
#endif

	return 0;
}


/* Resets read ptr */
int cmdhist_reset(void)
{
	cmdhist_read = cmdhist_write;

	return 0;
}

/* Gets next command in history */
int cmdhist_next(char **cmd)
{
	int ptr;

	if (!cmdhist_entries)
		return -EINVAL;

	if (!cmd)
		return -EINVAL;

	ptr = cmdhist_read;

	if (ptr == 0) {
		if ( cmdhist_entries != MAX_HIST )
			return -EINVAL;
		ptr = MAX_HIST - 1;
	} else {
		ptr--;
	}

	if (!cmdhistory[ptr][0])
		return -EINVAL;

	*cmd = cmdhistory[ptr];

	cmdhist_read = ptr;

#if CMDHIST_DEBUG
	printf("e=%d, r=%d, w=%d\n", cmdhist_entries, cmdhist_read, cmdhist_write);
#endif

	return 0;
}

/* Gets previous command from history */
int cmdhist_prev(char **cmd)
{
	int ptr;

	if (!cmd)
		return -EINVAL;

	if (!cmdhist_entries)
		return -EINVAL;

	ptr = cmdhist_read + 1;
	ptr = ptr % MAX_HIST;

	if (ptr == cmdhist_write)
		return -EINVAL;

	if (!cmdhistory[ptr][0])
		return -EINVAL;

	*cmd = cmdhistory[ptr];

	cmdhist_read = ptr;

	return 0;
}


/*
 * Supported commands
 */


/* help command */
static int help(int argc, char *argv[])
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
int quit(int argc, char *argv[])
{
	return PROGRAM_EXIT;
}

static char quit_help[] = "quit\n"
	"Quit application (return to boot ROM console)\n";

__commandlist(quit, "quit", quit_help);


/* reset console */
int reset(int argc, char *argv[])
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

