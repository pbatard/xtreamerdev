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

#ifndef _COMMAND_H
#define _COMMAND_H

#define COMMAND_MAGIC		(0x436d6420)	/* "Cmd " */
#define ESCAPE_CHARACTER	0x1B	// Esc
#define ESCAPE_SEQUENCE		0x5B	// [
#define KEY_UP				0x41	// Up
#define KEY_DOWN			0x42	// Down
#define KEY_LEFT			0x43	// Left
#define KEY_RIGHT			0x44	// Right
#define KEY_CTRL_C			0x03
#define KEY_ESCAPE			0x1B

typedef int(*commandfunc_t)(int, char *[]);


typedef struct commandlist {
	unsigned long magic;
	char *name;
	char *help;
	commandfunc_t callback;
	struct commandlist *next;
} commandlist_t;


#define __command __attribute__((unused, __section__(".commandlist")))

/*
 * Without a proper linker script the linker would happily discard all
 * sections it doesn't know about, so the carefully crafted command
 * list entries would be lost. To avoid this, we use a linker script
 * which (among other things) describes how to deal with the command list
 * entries:
 *	. = ALIGN(4);
 *	.commandlist : {
 *		__commandlist_start = .;
 *		*(.commandlist)
 *		__commandlist_end = .;
 *	}
 *
 * This tells the linker to start an output section called .commandlist
 * at a 4 byte aligned address, defines the __commandlist_start symbol
 * with that address, groups all .commandlist input sections (containing
 * all commandlist_t entries) into the output section, and defines the
 * __commandlist_end symbol with the current address.
 *
 * In this way we have created a list of commands nicely bounded by
 * __commandlist_start and __commandlist_end. During initialisation
 * these two addresses are used to find the correct number of commands.
 */

#define __commandlist(fn, nm, hlp) \
static commandlist_t __command_##fn __command = { \
	magic:    COMMAND_MAGIC, \
	name:     nm, \
	help:     hlp, \
	callback: fn }

extern commandlist_t *commands;

#define MAX_COMMANDLINE_LENGTH	(128)
#define MAX_ARGS				(MAX_COMMANDLINE_LENGTH/4)
#define PROGRAM_EXIT			(-99)

void init_commands(void);
int parse_command(char *cmdline);
int get_command(char *command, int len, int timeout);

#endif
