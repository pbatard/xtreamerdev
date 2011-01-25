/* utility calls for RTD Serial Recovery (rtdsr)
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

#include "realtek.h"
#include "util.h"


/************************************************************************
 *  Definitions for UART0
 ************************************************************************/

//#define USE_HANDSHAKE                  /* uncomment to use RTS/CTS    */
#define POLLSIZE                0x405    /* 1029 */
#define HW_LIMIT_STOP   (POLLSIZE-64)    /* RTS OFF when 64 chars in buf */
#define HW_LIMIT_START  (POLLSIZE-32)    /* RTS  ON when 32 chars in buf */
#define SERIAL_MCR_DTR           0x01    /* Data Terminal Ready         */
#define SERIAL_MCR_RTS           0x02    /* Request To Send             */
#define SERIAL_LSR_DR            0x01    /* Data ready                  */
#define SERIAL_LSR_THRE          0x20    /* Transmit Holding empty      */
#define SERIAL_MSR_CTS           0x10    /* Clear to send               */


/************************************************************************
 *  Static variables
 ************************************************************************/
static UINT16  recv_buffer[POLLSIZE];
static UINT16 *putptr = &recv_buffer[0];
static UINT16 *getptr = &recv_buffer[0];



void sync()
{
	asm(".set mips3");
	asm("sync");
	asm(".set mips1");
}

static int serial_poll()
{
	UINT32 lstat;
	UINT32 rdata;

	for(lstat = REG32(RTGALAXY_UART0_LSR); lstat & SERIAL_LSR_DR; lstat = REG32(RTGALAXY_UART0_LSR))
	{
		rdata = REG32(RTGALAXY_UART0_RBR_THR_DLL) & 0xff;

#ifdef USE_HANDSHAKE
		if (room <= HW_LIMIT_STOP * sizeof(*putptr) )
		{
			REG32(RTGALAXY_UART0_MCR) &= ~SERIAL_MCR_RTS;
		}
#endif
		*putptr = (lstat << 8) | rdata;


		/* increase putptr to its future position */
		if( ++putptr >= &recv_buffer[POLLSIZE] )
			putptr= &recv_buffer[0];

		if (putptr == getptr)
		{
			if (putptr == &recv_buffer[0])
				putptr = &recv_buffer[POLLSIZE-1];
			else
				putptr--;
		}

	}

	return lstat;
}

void serial_write(UINT8  *p_param)
{
	UINT32 x;
#ifdef USE_HANDSHAKE
	UINT32 y;
#endif

	for (;;) {
		/* OBS: LSR_OE, LSR_PE, LSR_FE and LSR_BI are cleared on read */
		x = serial_poll();
		x = x & SERIAL_LSR_THRE;
#ifdef USE_HANDSHAKE
		y = REG32(RTGALAXY_UART0_MSR);
		y = y & SERIAL_MSR_CTS;
		if ( x && y )
#else
		if ( x )
#endif
			break;
	}

	REG32(RTGALAXY_UART0_RBR_THR_DLL) = *p_param;
}

int serial_read()
{
	int c = -1;

	serial_poll();
	if (getptr != putptr) {
		c = *getptr & 0xFF;
		if (++getptr >= &recv_buffer[POLLSIZE]) {
			getptr= &recv_buffer[0];
		}
#ifdef USE_HANDSHAKE
		if (((REG32(RTGALAXY_UART0_MCR) & SERIAL_MCR_RTS) == 0 )&&
			(((UINT32)getptr - (UINT32)putptr) &
			((POLLSIZE - 1) * sizeof(*getptr))
				>= HW_LIMIT_START * sizeof(*getptr)) ) {
			REG32(RTGALAXY_UART0_MCR) |= SERIAL_MCR_RTS;
		}
#endif
	}
	return c;
}

/* Get a character with a timeout in seconds. Negative timeout means infinite */
int _getchar(int timeout)
{
	int c;
	unsigned long start_timer, current_timer, end_timer;

	if (timeout >=0) {
		REG32(RTGALAXY_TIMER_TC2CR) = 0x80000000;
		start_timer = REG32(RTGALAXY_TIMER_TC2CVR);
		end_timer = start_timer + timeout*CPU_FREQUENCY;
	}

	do {
		c = serial_read();
		if( c >= 0) {
			return c;
		}
		if (timeout >= 0) {
			current_timer = REG32(RTGALAXY_TIMER_TC2CVR);
			if ( (current_timer > end_timer) &&
				 ((end_timer >= start_timer) || (current_timer < start_timer)) ) {
				break;
			}
		}
	} while(1);

	return -1;
}

void _putchar(int c)
{
	unsigned char b = c & 0xFF;
	serial_write(&b);
}

void _memset(void *dst, UINT8 value, UINT32 size)
{
	UINT32 i;
	for (i=0; i<size; i++)
		REG8(((UINT32)dst) + i) = value;
}

void _memcpy(void *dst, void *src, UINT32 size)
{
	UINT32 i;
	for (i=0; i<size; i++)
		REG8(((UINT32)dst) + i) = REG8(((UINT32)src) + i);
}

void _msleep(unsigned long ms)
{
	unsigned long start_timer, current_timer, end_timer;

	/* Make sure the timer is started */
	REG32(RTGALAXY_TIMER_TC2CR) = 0x80000000;

	start_timer = REG32(RTGALAXY_TIMER_TC2CVR);
	end_timer = start_timer + ms*(CPU_FREQUENCY/1000);

	do {
		current_timer = REG32(RTGALAXY_TIMER_TC2CVR);
	} while ( (current_timer < end_timer) ||
			  ((end_timer < start_timer) && (current_timer >= start_timer)) );
}

void _sleep(unsigned long seconds)
{
	_msleep(seconds*1000);
}
