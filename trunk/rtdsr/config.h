/* General configuration for RTD Serial Recovery (rtdsr)
 *
 * copyright (c) 2011 Pete B. <xtreamerdev@gmail.com>
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

#ifndef CONFIG_H
#define CONFIG_H

#define VERSION "0.6"

/* Adjust the following according to your CPU frequency (Hz)       */
#define CPU_FREQUENCY			27000000

/* Start of a RAM buffer, reserved for Flash R/W operations.       */
/* This block MUST contain enough space to read or write blocks of */
/* flash without overlapping with RAM_BASE                         */
#define FLASH_TMP_ADDR			0xa0300000

/* Another RAM buffer to store the Block State Table for the flash */
#define FLASH_BST				0xa0400000

/* Default address for the RAM area                                */
#define RAM_BASE				0xa0500000

#endif
