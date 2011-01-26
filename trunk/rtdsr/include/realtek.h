/* CPU definitions for RTD Serial Recovery (rtdsr)
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

#ifndef _REALTEK_H
#define _REALTEK_H

/* Most of the following comes from linux-mips-rt-galaxy */
#define RTGALAXY_MARS				0x1283

#define RTGALAXY_REG_BASE			0xb8010000

#define RTGALAXY_SB2_BASE_OFFSET	0xa000
#define RTGALAXY_UART0_BASE_OFFSET	0xb200
#define RTGALAXY_TIMER_BASE_OFFSET	0xb500

/*
 * SB2
 */
#define RTGALAXY_SB2_BASE			(RTGALAXY_REG_BASE + RTGALAXY_SB2_BASE_OFFSET)
#define RTGALAXY_SB2_CHIP_ID		(RTGALAXY_SB2_BASE + 0x200)
#define RTGALAXY_SB2_CHIP_INFO		(RTGALAXY_SB2_BASE + 0x204)

/*
 * UART0 (see http://www.lammertbies.nl/comm/info/serial-uart.html)
 */
#define RTGALAXY_UART0_BASE			(RTGALAXY_REG_BASE + RTGALAXY_UART0_BASE_OFFSET)
#define RTGALAXY_UART0_RBR_THR_DLL	(RTGALAXY_UART0_BASE + 0x000)
#define RTGALAXY_UART0_IER_DLH		(RTGALAXY_UART0_BASE + 0x004)
#define RTGALAXY_UART0_IIR_FCR		(RTGALAXY_UART0_BASE + 0x008)
#define RTGALAXY_UART0_LCR			(RTGALAXY_UART0_BASE + 0x00c)
#define RTGALAXY_UART0_MCR			(RTGALAXY_UART0_BASE + 0x010)
#define RTGALAXY_UART0_LSR			(RTGALAXY_UART0_BASE + 0x014)
#define RTGALAXY_UART0_MSR			(RTGALAXY_UART0_BASE + 0x018)
#define RTGALAXY_UART0_SCR			(RTGALAXY_UART0_BASE + 0x01c)

/*
* Timer
*/
#define RTGALAXY_TIMER_BASE			(RTGALAXY_REG_BASE + RTGALAXY_TIMER_BASE_OFFSET)
#define RTGALAXY_TIMER_TC0TVR		(RTGALAXY_TIMER_BASE + 0x000)
#define RTGALAXY_TIMER_TC1TVR		(RTGALAXY_TIMER_BASE + 0x004)
#define RTGALAXY_TIMER_TC2TVR		(RTGALAXY_TIMER_BASE + 0x008)
#define RTGALAXY_TIMER_TC0CVR		(RTGALAXY_TIMER_BASE + 0x00c)
#define RTGALAXY_TIMER_TC1CVR		(RTGALAXY_TIMER_BASE + 0x010)
#define RTGALAXY_TIMER_TC2CVR		(RTGALAXY_TIMER_BASE + 0x014)
#define RTGALAXY_TIMER_TC0CR		(RTGALAXY_TIMER_BASE + 0x018)
#define RTGALAXY_TIMER_TC1CR		(RTGALAXY_TIMER_BASE + 0x01c)
#define RTGALAXY_TIMER_TC2CR		(RTGALAXY_TIMER_BASE + 0x020)
#define RTGALAXY_TIMER_TC0ICR		(RTGALAXY_TIMER_BASE + 0x024)
#define RTGALAXY_TIMER_TC1ICR		(RTGALAXY_TIMER_BASE + 0x028)
#define RTGALAXY_TIMER_TC2ICR		(RTGALAXY_TIMER_BASE + 0x02c)
#define RTGALAXY_TIMER_TCWCR		(RTGALAXY_TIMER_BASE + 0x030)
#define RTGALAXY_TIMER_TCWTR		(RTGALAXY_TIMER_BASE + 0x034)
#define RTGALAXY_TIMER_CLK27M		(RTGALAXY_TIMER_BASE + 0x038)
#define RTGALAXY_TIMER_CLK90K_TM_HI	(RTGALAXY_TIMER_BASE + 0x03c)
#define RTGALAXY_TIMER_CLK90K_TM_LO	(RTGALAXY_TIMER_BASE + 0x040)

/*
 * Misc.
 */

#define RTGALAXY_INFO_BONDING		0xb8000300
#define RTGALAXY_INFO_FLASH			0xb8000304

#define RTGALAXY_FLASH_TYPE_MASK			0x3
#define RTGALAXY_FLASH_TYPE_NOR_PARALLEL	0x0
#define RTGALAXY_FLASH_TYPE_NOR_SERIAL		0x1
#define RTGALAXY_FLASH_TYPE_NAND			0x2
#define RTGALAXY_FLASH_TYPE_PCI				0x3

#endif
