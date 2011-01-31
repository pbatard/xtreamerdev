/* NAND Flash functions for RTD Serial Recovery (rtdsr)
 *
 * copyright (c) 2011 Pete B. <xtreamerdev@gmail.com>
 * based on flashdev_n.c for RTD1283 by Realtek
 *
 * based on ymodem.c for bootldr, copyright (c) 2001 John G Dorsey
 * baded on ymodem.c for reimage, copyright (c) 2009 Rich M Legrand
 * crc16 function from PIC CRC16, by Ashley Roll & Scott Dattalo
 * crc32 function from crc32.c by Craig Bruce
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
#include "flashdev_n.h"
#include "flashdev_n_reg.h"
#include "util.h"
#include "printf.h"

UINT32 pages_per_block;
UINT32 blocks_per_flash;

#define PAGE_TMP_ADDR		FLASH_TMP_ADDR
#define BLK_STATE_BASE		FLASH_BST_ADDR

static UINT8 *blk_state;				// bootcode block state array
static UINT8 *blk_state_copy;			// second unmodified copy, for write operations
static UINT32 blk_state_len;			// length of bootcode block state array
static INT8 nf_find_blk(n_device_type *device, UINT32 start_block, UINT32 search_depth, UINT32 offset);
static int set_block_state(UINT32 block_no, UINT8 state);
static int get_block_state(UINT32 block_no, UINT8 *state);
static INT8 nf_read_to_table(n_device_type *device, UINT32 page_no, UINT32 length);


//-----------------------------------------//

static void nf_ctrler_init()
{
	/*******************************
	* init NAND flash start
	*******************************/
	// change NF freq divider (need to disabled NF clock when change divider)
	REG32(0xb800000c) = REG32(0xb800000c) & (~0x00800000);	// disable clock
	REG32(0xb8000034) = 0x4;	// set clk_nf to 43.2MHz
	REG32(0xb800000c) = REG32(0xb800000c) | 0x00800000;	// enable clock

	// disable EDO timing at low speed
	REG32(REG_MultiChMod) = REG32(REG_MultiChMod) & ~(0x20);

	// setup nf controller frequency
	REG32(REG_T3) = 0x2;
	REG32(REG_T2) = 0x2;
	REG32(REG_T1) = 0x2;

	// setup T_WHR_ADL delay clock
	REG32(REG_DELAY_CNT) = 0x9;

#if 0
	// setup descriptor error INT enable
	REG32(REG_IER) = 0x41;
#else
	// disable all interrupt from nf controller
	REG32(REG_IER) = 0x1;
#endif


	/*******************************
	* reset NAND flash start
	*******************************/
	// switch pad to NAND flash
	REG32(0xb800036c) = 0x103;

	// select which nand flash is used
	// there are five flash can be chosen.
	// the selected flash should assign 0 to its bit map in 0xb8010130
	// Ex. flash 0 ->0b11110, flash 1 ->0b11101
#ifdef FPGA
	REG32(REG_Chip_En) = 0x1d;
#else
	REG32(REG_Chip_En) = 0x1e;
#endif

	// set 'reset' command
	REG32(REG_CMD1) = 0xff;

	// issue command
	REG32(REG_CTL) = 0x80;

	// polling go bit to until go bit de-assert (command completed)
	while (REG32(REG_CTL) & 0x80)
		;

	// check ready/busy pin of NAND flash
	while ((REG32(REG_CTL) & 0x40) == 0)
		;
}

static UINT8 nf_get_spare(UINT32 *spare)
{
	// configure spare area data in PP (16 byte: 6 byte for user-defined, 10 byte for ECC)
	REG32(REG_PP_RDY) = 0; // disable read_by_pp
	REG32(REG_SRAM_CTL) = 0x30 | 0xe; // enable direct access to PP #3

	*spare = REG32(REG_BASE_ADDR); // set spare area of first PP

	REG32(REG_SRAM_CTL) = 0x0; // disable direct access
	return 0;
}

static UINT8 nf_set_spare(UINT32 spare)
{
	// configure spare area data in PP (16 byte: 6 byte for user-defined, 10 byte for ECC)
	REG32(REG_PP_RDY) = 0; // disable read_by_pp
	REG32(REG_SRAM_CTL) = 0x30 | 0x2; // enable direct access to PP #3

	REG32(REG_BASE_ADDR) = spare; // set spare area of first PP
	REG8(REG_BASE_ADDR + 4) = 0xff;
	REG8(REG_BASE_ADDR + 5) = 0xff;

	REG32(REG_SRAM_CTL) = 0x0; // disable direct access
	return 0;
}

static int nf_read_page(n_device_type *device, UINT32 page_no, UINT8 *buf)
{
	//Set data transfer count, data transfer count must be 0x200 at auto mode
	//Set SRAM path and access mode
	REG32(REG_DATA_CNT1) = 0;
	REG32(REG_DATA_CNT2) = 0x80|0x2;	// transfer mode, 0x200 bytes

	//Set page length at auto mode
	REG32(REG_PAGE_LEN) = device->PageSize / 0x200;	//Set page length, unit = ( 512B + 16B)

	REG32(REG_BLANK_CHK) = 0x1;		// enable blank check

	//Set PP
	REG32(REG_PP_RDY) = 0x80;	//NAND --> PP --> DRAM
	REG32(REG_PP_CTL0) = 0x01;
	REG32(REG_PP_CTL1) = 0;

	//Set table sram
	REG32(REG_TABLE_CTL) = 0;

	//Set command
	REG32(REG_CMD1) = 0x00;
	REG32(REG_CMD2) = 0x30;
	REG32(REG_CMD3) = 0x70;

	//Set address
	REG32(REG_PAGE_ADR0) =  (page_no & 0xff);
	REG32(REG_PAGE_ADR1) =  (page_no >> 8 ) & 0xff;
	REG32(REG_PAGE_ADR2) = ((page_no >>16 ) & 0x1f) | (device->addr_mode_rw << 5);
	REG32(REG_PAGE_ADR3) = ((page_no >> 21) & 0x7) << 5;
	REG32(REG_COL_ADR0)  = 0;
	REG32(REG_COL_ADR1)  = 0;

	//Set ECC
	REG32(REG_MultiChMod) = 0x0;
	REG32(REG_ECC_STOP) = 0x80;

	//Set DMA
	REG32(REG_DMA_ADR) = ((UINT32)buf) >> 3;	// 8 bytes unit
	REG32(REG_DMA_LEN) = device->PageSize / 0x200;	// 8*512 bytes
	REG32(REG_DMA_CONF) = 0x02|0x01;

	//Enable Auto mode
	REG32(REG_AUTO_TRIG) = 0x80|(device->PageSize == 512 ? 0x3 : 0x2);	//0x2: read in 2K page size
	while( REG32(REG_AUTO_TRIG) & 0x80 )
		;

	//Wait DMA done
	while( REG32(REG_DMA_CONF) & 0x1 );

	// return OK if all data bit is 1 (page is not written yet)
	if (REG32(REG_BLANK_CHK) & 0x2)
		return (DATA_ALL_ONE);

	// poll ECC state (err_not_clr)
	if (REG32(REG_ECC_STATE) & 0x8)
		return -1;

	return 0;
}

static int nf_write_page(n_device_type *device, UINT32 page_no, UINT8 *buf)
{
	UINT32 temp;

	//Set data transfer count, data transfer count must be 0x200 at auto mode
	//Set SRAM path and access mode
	REG32(REG_DATA_CNT1) = 0;
	REG32(REG_DATA_CNT2) = 0x00|0x2;	// transfer mode, 0x200 bytes

	//Set page length at auto mode
	REG32(REG_PAGE_LEN) = device->PageSize / 0x200;	//Set page length, unit = ( 512B + 16B)

	//Set PP
	REG32(REG_PP_RDY) = 0x0;	//NAND --> PP --> DRAM
	REG32(REG_PP_CTL0) = 0x0;
	REG32(REG_PP_CTL1) = 0;

	//Set table sram
	REG32(REG_TABLE_CTL) = 0;

	//Set command
	REG32(REG_CMD1) = 0x80;
	REG32(REG_CMD2) = 0x10;
	REG32(REG_CMD3) = 0x70;

	//Set address
	REG32(REG_PAGE_ADR0) =  page_no & 0xff;
	REG32(REG_PAGE_ADR1) =  page_no >> 8 ;
	REG32(REG_PAGE_ADR2) = ((page_no >>16 ) & 0x1f) | (device->addr_mode_rw << 5);
	REG32(REG_PAGE_ADR3) = ((page_no >> 21) & 0x7) << 5;
	REG32(REG_COL_ADR0)  = 0;
	REG32(REG_COL_ADR1)  = 0;

	//Set ECC
	REG32(REG_MultiChMod) = 0x0;
	REG32(REG_ECC_STOP) = 0x80;

	//Set DMA
	REG32(REG_DMA_ADR) = ((UINT32)buf) >> 3;	// 8 bytes unit
	REG32(REG_DMA_LEN) = device->PageSize / 0x200;	// 8*512 bytes
	REG32(REG_DMA_CONF) = 0x01;

	//Enable Auto mode
	REG32(REG_AUTO_TRIG) = 0x80|(device->PageSize == 512 ? 0x0 : 0x1);	//0x1: write in 2K page size
	while( REG32(REG_AUTO_TRIG) & 0x80 )
		;

	//Wait DMA done
	while( REG32(REG_DMA_CONF) & 0x1 );


	// execute command3 register and wait for executed completion
	REG32(REG_POLL_STATUS) = (0x6<<1) | 0x1;
	while ( REG32(REG_POLL_STATUS) & 0x1)
		;

	temp = REG32(REG_DATA) & 0x1;

	if (temp == 0) {
		printf(".");
		return 0;
	}
	printf("!");
	return -1;
}

static int nf_erase_block(n_device_type *device, UINT32 block)
{
	UINT32 page_addr, temp;

	page_addr = block * pages_per_block;

	printf("erasing block 0x%x... ", block);

	//Set command
	REG32(REG_CMD1) = 0x60;		//Set CMD1
	REG32(REG_CMD2) = 0xd0;		//Set CMD2
	REG32(REG_CMD3) = 0x70;		//Set CMD3

	//Set address
	//note. page_addr[5:0] is ignored to be truncated as block
	REG32(REG_PAGE_ADR0) =  page_addr & 0xff;
	REG32(REG_PAGE_ADR1) =  page_addr >> 8;
	REG32(REG_PAGE_ADR2) = ((page_addr >>16 ) & 0x1f) | (device->addr_mode_erase << 5);
	REG32(REG_PAGE_ADR3) = ((page_addr >> 21) & 0x7) << 5;
	REG32(REG_COL_ADR0)  = 0;
	REG32(REG_COL_ADR1)  = 0;

	//Set ECC: Set HW no check ECC, no_wait_busy
	REG32(REG_MultiChMod) = 0x1 << 4;

	//Enable Auto mode: Set and enable auto mode
	// and wait until auto mode done
	REG32(REG_AUTO_TRIG) = 0x8a;
	while ( REG32(REG_AUTO_TRIG) & 0x80)
		;

	// execute command3 register and wait for executed completion
	REG32(REG_POLL_STATUS) = (0x6<<1) | 0x1;
	while ( REG32(REG_POLL_STATUS) & 0x1)
		;

	temp = REG32(REG_DATA) & 0x1;

	if (temp == 0) {
		printf("success\n");
		return 0;
	}
	printf("failed!!\n");
	return -1;
}

/************************************************************************
*
*                          nf_erase
*  Description :
*  -------------
*  implement the flash erase
*  (make sure there's enough blocks for do_write_n function)
*
*  Parameters :
*  ------------
*
*  Return values :
*  ---------------
*
************************************************************************/
int nf_erase(n_device_type* device,      //flash device
			 unsigned long  start_block, //start of block NO.
			 unsigned long  size)        //request data length
{
	UINT32 req_block_no;
	UINT32 current_block_no = start_block;
	UINT8 state;

	if (size % device->BlockSize) {
		printf("erase size must be a multiple of block size\n");
		return -1;
	}

	// calculate required number of blocks
	req_block_no = size / device->BlockSize;

	while (req_block_no != 0)
	{
		if (get_block_state(current_block_no, &state))
			// return failed if reach bootcode blocks boundary (32MB)
			return -1;

		switch(state)
		{
		case BLOCK_CLEAN:
			current_block_no++;
			req_block_no--;
			break;;
		case BLOCK_BAD:
			printf("bad block detected (0x%x) - will try to erase it anyway\n", current_block_no);
		default:
			// if erase current_block_no fail, mark this block as fail
			if (nf_erase_block(device, current_block_no) < 0)
			{
				// write 'BAD_BLOCK' signature to spare cell
				nf_set_spare(BLOCK_BAD);
				nf_write_page(device, current_block_no * pages_per_block, (UINT8*)PAGE_TMP_ADDR);
				set_block_state(current_block_no, BLOCK_BAD);
				// abort
				return -1;
			}
			// block is good after erase
			set_block_state(current_block_no, BLOCK_CLEAN);
			current_block_no++;
			req_block_no--;
			break;
		}

	}// end of while (req_block_no != 0)

	return 0;
}


/************************************************************************
*
*                          nf_write
*  Description :
*  -------------
*  implement the flash write
*
*  Parameters :
*  ------------
*
*  Return values :
*  ---------------
*
************************************************************************/
int nf_write(n_device_type* device,      //flash device
			 unsigned long  start_block, //start of block NO.
			 unsigned char* buf,         //source buffer
			 unsigned long  size)        //requested size
{
	unsigned long data_len = size;
	UINT32 current_page;
	UINT32 signature;
	UINT8* data_ptr = buf;

	if ((device == NULL) || (size % device->BlockSize)) {
		return -1;
	}

	//erase blocks we need to save data
	if (nf_erase(device, start_block, size) < 0) {
		printf("erase failed\n");
		return -1;
	}

	current_page = start_block * pages_per_block;

	// write data to nand flash pages
	while (data_len != 0)
	{
		// read signature from the table at FLASH_BST_ADDR
		signature = (UINT32)blk_state_copy[current_page/pages_per_block];
		// set signature to pp buffer and wait for writing to spare cell
		nf_set_spare(signature);

		if (current_page % pages_per_block == 0) {
			printf("writing block 0x%x", current_page / pages_per_block);
		}

		if (nf_write_page(device, current_page, data_ptr) < 0)
		{
			UINT32 current_block_no = current_page / pages_per_block;
			printf(" error: failed at page 0x%x!\n", current_page);

			// erase whole block to write signature to spare cell
			nf_erase_block(device, current_block_no);

			//write 'BAD_BLOCK' signature to spare cell
			nf_set_spare(BLOCK_BAD);
			set_block_state(current_block_no, BLOCK_BAD);

			// perform writing
			nf_write_page(device, current_block_no * pages_per_block, (UINT8*)PAGE_TMP_ADDR);

			return -1;
		}
		set_block_state(current_page / pages_per_block, signature);
		current_page++;
		data_ptr += device->PageSize;
		data_len -= device->PageSize;

		if (current_page % pages_per_block == 0) {
			printf(" success\n");
		}
	}

	return 0;
}

/************************************************************************
*
*                          nf_identify
*  Description :
*  -------------
*  implement the identyfy flash type
*
*  Parameters :
*  ------------
*
*  Return values :
*  ---------------
*
************************************************************************/
int nf_identify(n_device_type** device)
{
	UINT32 chipid = 0;
	UINT32 idx;

	nf_ctrler_init();

	/***************************************
	* get maker ID and device ID from flash
	* to DDR
	***************************************/

	//set read length, SRAM target and direction
	// {0xb8010104[5:0], 0xb8010100[7:0]} = 0x200 -->the value must be multiple of 512
	// 0xb801b104[7] = 1: direction = nf->outside
	// 0xb801b104[6] = 0: SRAM target = PP buffer
	REG32(REG_DATA_CNT1) = 0x0;
	REG32(REG_DATA_CNT2) = 0x82;

	// Set page length, value 0x1 = ( 512B + 16B), value 0x2 = (1024B + 32B), ...
	REG32(REG_PAGE_LEN) = 0x1;

	// Data read to DRAM from NAND through PP buffer
	REG32(REG_PP_RDY) = 0x80;

	// enable PP to start request DMA cycle over PP buffer
	// {0xb8010110[5:4], 0xb801012c[7:0]} = 0 --> start of pp buffer to put data
	REG32(REG_PP_CTL0) = 0x1;
	REG32(REG_PP_CTL1) = 0x0;

	// disable table ram
	REG32(REG_TABLE_CTL) = 0x0;

	// setup command
	REG32(REG_CMD1) = 0x90;
	REG32(REG_CMD2) = 0x0;
	REG32(REG_CMD3) = 0x0;

	// set page address
	// {0xb801002c[7:5], 0xb8010008[4:0], 0xb8010004[7:0], 0xb8010000[7:0]} = 0 (24bit)
	REG32(REG_PAGE_ADR0) = 0x0;    //PA[7:0]
	REG32(REG_PAGE_ADR1) = 0x0;    //PA[15:8]
	REG32(REG_PAGE_ADR2) = 0xe0;   //PA[20:16], address mode = 0x7
	REG32(REG_PAGE_ADR3) = 0x0;    //PA[23:21]

	// set column address
	REG32(REG_COL_ADR0) = 0x0;    //CA[7:0]
	REG32(REG_COL_ADR1) = 0x0;    //CA[15:8]

	// disable ECC check and calculate
	REG32(REG_MultiChMod) = 0xc0;
	REG32(REG_ECC_STOP) = 0x80;   // no stop when ecc error

	// setup DDR address and length, and direction
	// this is design restriction. DMA real address will be multiple of 8
	REG32(REG_DMA_ADR) = PAGE_TMP_ADDR >> 3;
	REG32(REG_DMA_LEN) = 0x1;    // 1 unit = 512B
	REG32(REG_DMA_CONF) = 0x3;    // from dma buffer to DDR and start transfer

	// trigger auto mode with issue command and wait its completion
	REG32(REG_AUTO_TRIG) = 0x88;
	while ( REG32(REG_AUTO_TRIG) & 0x80)
		;

	// trigger auto mode with read data and wait its completion
	REG32(REG_AUTO_TRIG) = 0x84;
	while ( REG32(REG_AUTO_TRIG) & 0x80)
		;

	//Wait DMA done
	while (REG32(REG_DMA_CONF) & 0x1)
		;

	/***************************************
	* compare maker id and device ID with
	* IDs in nand flash table
	***************************************/

	//get maker id and device id from DDR
	chipid = REG32(PAGE_TMP_ADDR);

	/* find the match flash brand */
	for (idx = 0; idx < DEV_SIZE_N; idx++)
	{
		if ( n_device[idx].id == chipid )
		{
			*device = (n_device_type*)&n_device[idx];

			// compute block number per flash and page size per block
			pages_per_block  = n_device[idx].BlockSize / n_device[idx].PageSize;
			blocks_per_flash = n_device[idx].size      / n_device[idx].BlockSize;

			return 0;
		}
	}

	*device = 0;

	return -1;	// cannot find any matched ID
}

/************************************************************************
*
*                          nf_init
*  Description :
*  -------------
*  implement the following NAND flash init job:
*	1. setup block state table for bootcode area (first 16MB of flash)
*	2. erase old bootcode blocks
*	3. determine block range for env param
*
*  Parameters :
*  ------------
*
*  Return values :
*  ---------------
*
************************************************************************/
void nf_init(n_device_type* device)
{
	UINT32 i;
	INT32 res;

	// init block state table
	blk_state_len = NAND_BOOTCODE_SIZE / device->BlockSize;	// only bootcode blocks (32MB of the flash)
	for (i = 0; i < blk_state_len; i++)
		REG8(BLK_STATE_BASE + i) = BLOCK_UNDETERMINE;
	blk_state = (UINT8*)BLK_STATE_BASE;
	blk_state_copy = (UINT8*)(BLK_STATE_BASE + blk_state_len);

	/* fill block state table */
	// search with non-exist magic no (this guarantees we can visit to the end of the bootcode blocks)
	res = nf_find_blk(device, 0, blk_state_len, 0);
}

/************************************************************************
*
*                          nf_read_to_table
*  Description :
*  -------------
*  read one page from NAND flash into table SRAM
*
*
*  Parameters :
*  ------------
*
*  'device',        IN,    variable of type, n_device_type.
*  'page_no',       IN,    page address
*  'length',        IN,    read data area size (multiple of 512B and cannot exceed one page)
*
*
*  Return values :
*  ---------------
*
*  "0"                  : means read page success (ECC is ok)
*  "-1"                 : means device is NULL
*  "DATA_ALL_ONE" = 0x1 : means all bits in the page is 1 (including spare area)
*  "8"                  : means ECC is uncorrectable
*
*
************************************************************************/
static INT8 nf_read_to_table(n_device_type *device, UINT32 page_no, UINT32 length)
{
	if (device == NULL)
		return (-1);

	// data length must be multiple of 512B and cannot exceed one page
	if ((length & 0x1ff) || (length > device->PageSize))
		return (-1);

	//Set data transfer count, data transfer count must be 0x200 at auto mode
	//Set SRAM path and access mode
	REG32(REG_DATA_CNT1) = 0;
	REG32(REG_DATA_CNT2) = 0xc2;		// to table RAM

	//set page length at auto mode (512B unit)
	REG32(REG_PAGE_LEN) = length >> 9;

	REG32(REG_PP_RDY) = 0;			// no read_by_pp
	REG32(REG_PP_CTL0) = 0;			// disable PP
	REG32(REG_TABLE_CTL) = 0x1;		// enable table SRAM

	REG32(REG_BLANK_CHK) = 0x1;		// enable blank check

	//set command
	REG32(REG_CMD1) = 0;
	REG32(REG_CMD2) = 0x30;
	REG32(REG_CMD3) = 0x70;

	//Set address
	REG32(REG_PAGE_ADR0) =  page_no & 0xff;
	REG32(REG_PAGE_ADR1) =  page_no >> 8 ;
	REG32(REG_PAGE_ADR2) = ((page_no >>16 ) & 0x1f) | (device->addr_mode_rw << 5);
	REG32(REG_PAGE_ADR3) = ((page_no >> 21) & 0x7) << 5;
	REG32(REG_COL_ADR0)  = 0;
	REG32(REG_COL_ADR1)  = 0;

	//ste ECC
	REG32(REG_MultiChMod) = 0;
	REG32(REG_ECC_STOP) = 0x80;		// non-stop

	//Enable Auto mode
	REG32(REG_AUTO_TRIG) = 0x80 | (device->PageSize == 512 ? 0x3 : 0x2);	//0x2: read in 2K page size
	while( REG32(REG_AUTO_TRIG) & 0x80 )
		;

	// return OK if all data bit is 1 (page is not written yet)
	if (REG32(REG_BLANK_CHK) & 0x2)
		return (DATA_ALL_ONE);

	// return ECC result
	return (REG32(REG_ECC_STATE) & 0x8);	// ecc_not_clr
}

/************************************************************************
*
*                          nf_read
*  Description :
*  -------------
*  read NAND flash
*
*  Parameters :
*  ------------
*  'device',       IN,    variable of type, n_device_type.
*  'start_page',   IN,    start page address to read
*  'buf',          IN,    pointer for buffer of data to read
*  'size',         IN,    number of bytes to read
*
*  Return values :
*  ---------------
*  '-1': device is NULL or read beyond flash or read failed
*
************************************************************************/
int nf_read(n_device_type *device, unsigned long start_page, unsigned char* buf, unsigned long size)
{
	unsigned long stop_page;
	int res;

	// validate arguments (size should be aligned to page size boundary)
	if ( (device == NULL) || (buf == NULL)
		|| (start_page > pages_per_block * blocks_per_flash)
		|| (size & (device->PageSize - 1))
		|| (size == 0) )
		return (-1);

	// do not allow read past end of flash
	stop_page = start_page + size / device->PageSize;
	if (stop_page > pages_per_block * blocks_per_flash) {
		printf("attempted to read past end of flash\n");
		return (-1);
	}

	while (start_page < stop_page)
	{
		res = nf_read_page(device, start_page, buf);
		switch (res)
		{
		case DATA_ALL_ONE:	// page is clean
		case 0:
			break;

		default:
			printf("page %d is marked not clean!\n", start_page);
			return (-1);
		}

		buf += device->PageSize;
		start_page++;
	}
	return 0;
}


/************************************************************************
*
*                          nf_find_blk
*  Description :
*  -------------
*  scan NAND flash to get block state
*
*  Parameters :
*  ------------
*  'device',       	IN,    variable of type, n_device_type.
*  'start_block',  	IN,    start block to search
*  'search_depth', 	IN,    maximum number of blocks to search
*  'offset',       	IN,    page offset in the block
*
*  Return values :
*  ---------------
*  '-1': device is NULL or start_block beyond flash or not found
*
************************************************************************/
static INT8 nf_find_blk(n_device_type *device, UINT32 start_block, UINT32 search_depth, UINT32 offset)
{
	UINT32 blk, limit, page_no;
	UINT32 spare;
	INT8 res;

	// validate arguments
	if ((device == NULL) || (start_block >= blocks_per_flash) || (offset >= pages_per_block))
		return (-1);

	// determine search limit
	if (search_depth == 0)
		limit = NAND_BOOTCODE_SIZE / device->BlockSize;	// only bootcode blocks (first 32MB of the flash)
	else if (search_depth >= blocks_per_flash)
		limit = blocks_per_flash;	// search to end of flash
	else
	{
		limit = start_block + search_depth;

		// cannot search beyond flash (max is last block of flash)
		if (limit >= blocks_per_flash)
			limit = blocks_per_flash;
	}

	for (blk = start_block, page_no = (blk * pages_per_block + offset);
		blk < limit ;
		blk++, page_no += pages_per_block)
	{
		// read first page of the block to table sram
		res = nf_read_to_table(device, page_no, device->PageSize);	// read page to table SRAM

		switch (res)
		{
		case DATA_ALL_ONE:
			blk_state[blk] = blk_state_copy[blk] = BLOCK_CLEAN;
			continue;

		case 0:		// read to table success
			nf_get_spare(&spare);
			spare &= 0xff;
			break;

		default:	// read to table has error
			blk_state[blk] = blk_state_copy[blk] = BLOCK_BAD;
			continue;	// next block
		}

		// NOTE: BLOCK_UNDETERMINE is a fake state,
		// all blocks in the flash "SHALL NOT" have this state or it will cause some problem!!
		if (spare == BLOCK_UNDETERMINE)
			continue;	// should not happen

		// update with new magic no. in spare area
		blk_state[blk] = blk_state_copy[blk] = (UINT8)(spare & 0xff);
	}

	return (0);
}

/************************************************************************
*
*                          set_block_state
*  Description :
*  -------------
*  get block state
*
*  Parameters :
*  ------------
*  'block_no',	IN,	block no.
*  'state',	IN,	specified block state
*
*  Return values :
*  ---------------
*  '-1': block_no is out of range
*
************************************************************************/
static int set_block_state(UINT32 block_no, UINT8 state)
{
	if (block_no >= blk_state_len)
		return -1;
	blk_state[block_no] = state;
	return 0;
}


/************************************************************************
*
*                          get_block_state
*  Description :
*  -------------
*  get block state
*
*  Parameters :
*  ------------
*  'block_no',	IN,	block no.
*  'state',	INOUT,	specified block state
*
*  Return values :
*  ---------------
*  '-1': block_no is out of range
*
************************************************************************/
static int get_block_state(UINT32 block_no, UINT8 *state)
{
	if (block_no >= blk_state_len)
		return -1;
	*state = blk_state[block_no];
	return 0;
}
