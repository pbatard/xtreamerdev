/*
 * errno.h: error numbers for blob
 *
 * copyright (c) 2011 Pete B. <xtreamerdev@gmail.com>
 *
 * based on errno.h for for openezx, copyright (c) 2001 Erik Mouw
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

#ifndef _ERRNO_H
#define _ERRNO_H

#define ENOERROR	0	/* no error at all */
#define EINVAL		1	/* invalid argument */
#define ENOPARAMS	2	/* not enough parameters */
#define EMAGIC		3	/* magic value failed */
#define ECOMMAND	4	/* invalid command */
#define ENAN		5	/* not a number */
#define EALIGN		6	/* addres not aligned */
#define ERANGE		7	/* out of range */
#define ETIMEOUT	8	/* timeout exceeded */
#define ETOOSHORT	9	/* short file */
#define ETOOLONG	10	/* long file */
#define EAMBIGCMD	11	/* ambiguous command */
#define EFLASHERASE	12	/* can't erase flash block */
#define EFLASHPGM	13	/* flash program error */
#define ESERIAL		14	/* serial port error */

#endif
