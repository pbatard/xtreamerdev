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

#include "util.h"
#include "printf.h"
#include "ymodem.h"
/*
#include "flashdev_n.h"
#include "flashdev_p.h"
#include "flashdev_s.h"

#define FLASH_TYPE_NOR_PARALLEL		0x0
#define FLASH_TYPE_NOR_SERIAL		0x1
#define FLASH_TYPE_NAND				0x2
#define FLASH_TYPE_PCI				0x3

#define FLASH_MAGICNO_NOR_PARALLEL	0xbe
#define FLASH_MAGICNO_NAND			0xce
#define FLASH_MAGICNO_NOR_SERIAL	0xde
*/
#define DATA_TMP_ADDR			0xa0500000
#define DATA_TMP_SIZE			0x00010000
#define NOR_BULK_SIZE			0x00010000	// access NOR flash by sector (64KB)

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

void display_buffer_hex(UINT32 addr, UINT32 size)
{
	UINT32 i, j, k;

	for (i=0; i<size; i+=16) {
		printf("\n  %08lx  ", addr+i);
		for(j=0,k=0; k<16; j++,k++) {
			if (i+j < size) {
				printf("%02x", REG8(addr+i+j));
			} else {
				printf("  ");
			}
			printf(" ");
		}
		printf(" ");
		for(j=0,k=0; k<16; j++,k++) {
			if (i+j < size) {
				if ((REG8(addr+i+j) < 32) || (REG8(addr+i+j) > 126)) {
					printf(".");
				} else {
					printf("%c", REG8(addr+i+j));
				}
			}
		}
	}
	printf("\n" );
}


#define VERSION "0.4"
//#define DUMP_BOOTROM
#define YMODEM_RCV
//#define SERIAL_ECHO
int dvrmain(void)
{
	int c, ret;

	init_printf(NULL, putc);

	printf("\nrtdsr version " VERSION ", Copyright (c) 2011 Pete B.\n");
	printf("rtdsr comes with ABSOLUTELY NO WARRANTY.\n");
	printf("This is free software, and you are welcome to redistribute it\n");
	printf("under certain conditions; see the GNU GPL v3 for details.\n");

#ifdef DUMP_BOOTROM
	ret = ymodem_send((unsigned char*)0xBFC00000, 0x2000, "BootROM.bin");
#endif

#ifdef YMODEM_RCV
	ret = ymodem_receive((unsigned char*)DATA_TMP_ADDR, DATA_TMP_SIZE);
	if (ret > 0) {
		display_buffer_hex(DATA_TMP_ADDR, ret);
	}
	return 0;
#endif

#ifdef SERIAL_ECHO
	do {
		c = _getchar(-1);
		if (c < 0) {
			break;
		}
		if (c == 0x0D) {
			printf("\n");
		} else {
			printf("%c", c);
		}
	} while (c != 0x1B);
#endif

/*
	printf("Detecting flash type: ");
	switch(REG32(0xb8000304) & 0x3)
	{
		case FLASH_TYPE_NOR_PARALLEL:
			printf("parallel\n");
			break;

		case FLASH_TYPE_NOR_SERIAL:
			printf("serial\n");
			break;

		case FLASH_TYPE_NAND:
			printf("nand\n");
			break;

		case FLASH_TYPE_PCI:
		default:
			printf("unknown!\n");
			return -1;
	}
*/
	return 0;
}
