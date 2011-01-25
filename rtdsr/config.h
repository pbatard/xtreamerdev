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

/* Adjust the following according to your CPU frequency (Hz) */
#define CPU_FREQUENCY			27000000

/* Base address for the RAM to use */
#define DATA_TMP_ADDR			0xa0500000

/* RAM size */
#define DATA_TMP_SIZE			0x00010000

/* Size of a flash data sector (64KB) */
#define NOR_BULK_SIZE			0x00010000

#endif
