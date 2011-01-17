#include "flashdev_n.h"
#include "flashdev_n_reg.h"
#include "util.h"
#include "printf.h"

UINT32 pages_per_block;
UINT32 blocks_per_flash;

#define PAGE_TMP_ADDR		0xa0400000
#define BLK_STATE_BASE		0xa1700000

static UINT32  fileflash_phys_start ;
static UINT32  fileflash_phys_end ;
static UINT8 *blk_state;			// bootcode block state array
static UINT32 blk_state_len;			// length of bootcode block state array
static UINT32 current_env_start;		// (latest) env param page address
static UINT8 env_version_no;			// (latest) env param version on

static BB_t bbt[BBT_SIZE];

//-----------------------------------------//
//static INT8 nf_find_blk(n_device_type *device, UINT32 start_block, UINT32 search_depth, UINT8 offset, UINT8 magic, UINT32 *found_block);
static INT8 nf_find_blk(n_device_type *device, UINT32 start_block, UINT32 search_depth, UINT32 offset);
static int set_block_state(UINT32 block_no, UINT8 state);
static int get_block_state(UINT32 block_no, UINT8 *state);
static INT8 nf_read_to_table(n_device_type *device, UINT32 page_no, UINT32 length);
static int nf_write(n_device_type *device, UINT32 start_page, UINT8 *buf, UINT32 size);

static int bbt_exist(void);
static int init_bbt(void);
static int get_remap_block(UINT32 *start_blk);
static int build_bbt(void *dev);
static UINT32 initial_bad_block_offset(n_device_type *device);

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
	while ( REG32(REG_CTL) & 0x80)
		;

	// set 'read status' command to third command register???
	REG32(REG_CMD3) = 0x70;

	// detect command completion or not
	// please reference to 'read status' definition in nand flash
	// bit_sel = 0x6 (i.e. I/O ping 6), trig_poll = 1b
	REG32(REG_POLL_STATUS) = 0xd;

	while ( REG32(REG_POLL_STATUS) & 0x1)
		;

}

static UINT8 nf_get_spare(INT8 mem_region, UINT32 *spare, UINT32 offset)
{
	// configure spare area data in PP (16 byte: 6 byte for user-defined, 10 byte for ECC)
	REG32(REG_PP_RDY) = 0; // disable read_by_pp
	REG32(REG_SRAM_CTL) = 0x30 | mem_region; // enable direct access to PP #3

	*spare = REG32(0xb8010000 + offset); // set spare area of first PP
	//REG32(0xb8010004) = spare[0]; // (only first 6 byte is user-defined)

	REG32(REG_SRAM_CTL) = 0x0; // disable direct access
	return 0;
}

static UINT8 nf_set_spare(UINT32 spare, UINT32 offset)
{
	// configure spare area data in PP (16 byte: 6 byte for user-defined, 10 byte for ECC)
	REG32(REG_PP_RDY) = 0; // disable read_by_pp
	REG32(REG_SRAM_CTL) = 0x30 | 0x2; // enable direct access to PP #3

	REG32(0xb8010000 + offset) = spare; // set spare area of first PP
	//REG32(0xb8010004) = spare[0]; // (only first 6 byte is user-defined)

	REG32(REG_SRAM_CTL) = 0x0; // disable direct access
	return 0;
}



static int nf_read_page( n_device_type *device, UINT32 page_no, UINT8 *buf)
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


static int nf_write_page( n_device_type *device, UINT32 page_no, UINT8 *buf)
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
		/* show '.' in console */
		REG32(0xb801b200)= 0x2e ; //cy test
		return 0;
	}
	/* show '!' in console */
	REG32(0xb801b200)= 0x21 ; //cy test
	return -1;
}



static int nf_erase_block(n_device_type *device, UINT32 block)
{
	UINT32 page_addr, temp;

	page_addr = block * pages_per_block;

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
		/* show '/' in console */
		REG32(0xb801b200)= 0x2f ; //cy test
		return 0;
	}
	/* show 'X' in console */
	REG32(0xb801b200)= 0x58 ; //cy test
	return -1;
}


/************************************************************************
 *
 *                          do_erase_n
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
int do_erase_n(void *dev,               //flash device
               unsigned char* dest,     //start of block NO.
               unsigned int   rem_size) //request data length
{
	UINT32 signature;
	n_device_type *device = (n_device_type *)dev;

	UINT32 req_block_no, total_blocks, first_block;
	UINT32 current_block_no = (UINT32)dest;
	int res;
	UINT8 state;

	// calculate required number of blocks
	req_block_no = rem_size / device->BlockSize;
	if (rem_size % device->BlockSize)
		req_block_no++;

	total_blocks = req_block_no;
	first_block = (UINT32)dest;

	while (req_block_no != 0)
	{
		// return failed if reach bootcode blocks boundary (16MB)
		if (current_block_no > (blk_state_len - 1))
			return -1;

		res = get_block_state(current_block_no, &state);
		if (res)
		{
			// block state table has no record (out of range?)
			// try to read first page in block then check data
			// in spare area those load to PP
			res = nf_read_page( device, current_block_no * pages_per_block, (UINT8 *)PAGE_TMP_ADDR);

			// read page failed (restart to find contiguous clean blocks)
			if (res == -1)
			{
				current_block_no++;
				req_block_no = total_blocks;
				first_block = current_block_no;	// re-init start block no
				continue;
			}

			nf_get_spare(0x2, &signature, 0);
			state = signature & 0xff;
		}

		// block is good
		if (state == BLOCK_CLEAN)
		{
			current_block_no++;
			req_block_no--;
			continue;
		}

		// bad block (restart to find contiguous clean blocks)
		if(state == BLOCK_BAD)
		{
			current_block_no++;
			req_block_no = total_blocks;
			first_block = current_block_no;	// re-init start block no
			continue;
		}
		else
		{
			// if erase current_block_no fail, mark this block as fail
			if (nf_erase_block(device, current_block_no) < 0)
			{
				// write 'BAD_BLOCK' signature to spare cell
				// (restart to find contiguous clean blocks)
				nf_set_spare(BLOCK_BAD, 0);
				nf_write_page( device, current_block_no * pages_per_block, (UINT8 *)PAGE_TMP_ADDR);

				set_block_state(current_block_no, BLOCK_BAD);
				current_block_no++;
				req_block_no = total_blocks;
				first_block = current_block_no;	// re-init start block no
				continue;
			}
			else
			{
				// block is good after erase
				set_block_state(current_block_no, BLOCK_CLEAN);
				current_block_no++;
				req_block_no--;
			}// end of erase block success
		}

	}// end of while (req_block_no != 0)

	// return start block available to write
	return first_block;

}

/************************************************************************
 *
 *                          do_write
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
int do_write_n(void *dev,
               unsigned char* array_ptr,
               unsigned char* dest,
               unsigned int   rem_size,
               unsigned int signature)
{
	n_device_type *device = (n_device_type *)dev;
	int data_len = rem_size;
	UINT32 current_page;
	UINT8 * data_ptr = array_ptr;
	int ith;
	UINT32 end_blk;

	//erase blocks we need to save data
	ith = do_erase_n(dev, dest, rem_size);

	if (ith == -1)
		return -1;	// no more blocks available

	current_page = ((UINT32)ith) * pages_per_block;

	// write data to nand flash pages
	while (data_len > 0)
	{
		// set signature to pp buffer and wait for writing to spare cell
		nf_set_spare(signature, 0);

		if ( nf_write_page( device, current_page, data_ptr) < 0)
		{
			UINT32 current_block_no = current_page / pages_per_block;

			// erase whole block to write signature to spare cell
			nf_erase_block(device, current_block_no);

			//write 'BAD_BLOCK' signature to spare cell
			nf_set_spare(BLOCK_BAD, 0);
			set_block_state(current_block_no, BLOCK_BAD);

			// perform writing
			nf_write_page( device, current_block_no * pages_per_block, (UINT8 *)PAGE_TMP_ADDR);

			// redo all operation from start
			ith = do_erase_n(dev, dest, rem_size);
			if (ith == -1)
				return -1;	// no more blocks available

			data_len = rem_size;
			current_page = ((UINT32)ith) * pages_per_block;
			data_ptr = array_ptr;

		}
		else
		{
			set_block_state(current_page / pages_per_block, signature);
			current_page ++;
			data_ptr += device->PageSize;
			data_len -= device->PageSize;
		}
	}

	// adjust first block available for env
	end_blk = current_page / pages_per_block;
	if (end_blk >= fileflash_phys_start )
		fileflash_phys_start = end_blk + 1;

	// check if latest env has been overwritten
	if (current_page > current_env_start)
		current_env_start = -1;

	return current_page;
}

/************************************************************************
 *
 *                          do_identify_n
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
int do_identify_n(void **dev)
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
			*dev = (void *)&n_device[idx];

			// compute block number per flash and page size per block
			pages_per_block  = n_device[idx].BlockSize / n_device[idx].PageSize;
			blocks_per_flash = n_device[idx].size      / n_device[idx].BlockSize;

			return 0;
		}
	}

	*dev = 0;

	return -1;	// cannot find any matched ID
}

/************************************************************************
 *
 *                          do_init_s
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
void do_init_n(void *dev)
{
	n_device_type *device = (n_device_type *)dev;
	UINT32 i;
	INT32 res;
	//UINT8 state;

	// init block state table
	blk_state_len = NAND_BOOTCODE_SIZE / device->BlockSize;	// only bootcode blocks (16MB of the flash)
	for (i = 0; i < blk_state_len; i++)
		REG8(BLK_STATE_BASE + i) = BLOCK_UNDETERMINE;
	blk_state = (UINT8 *)BLK_STATE_BASE;


	/* fill block state table */
	// search with non-exist magic no (this guarantees we can visit to the end of the bootcode blocks)
	res = nf_find_blk(device, 0, blk_state_len, 0);
}
/************************************************************************
 *
 *                          do_exit_n
 *  Description :
 *  -------------
 *  implement the exit flash writing operation
 *
 *  Parameters :
 *  ------------
 *
 *  Return values :
 *  ---------------
 *
 ************************************************************************/
void do_exit_n(void *dev)
{

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
 *  'device',       	IN,    variable of type, n_device_type.
 *  'page_no',  	IN,    page address
 *  'length',  		IN,    read data area size (multiple of 512B and cannot exceed one page)
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
 *                          FLASH_NAND_search_latest_env
 *  Description :
 *  -------------
 *  Search latest env param data
 *
 *
 *  Parameters :
 *  ------------
 *
 *  'device',   IN,    variable of type, n_device_type.
 *  'start',    IN,    start block no
 *  'end',   	IN,    end block no
 *  'latest',  	INOUT, latest env param start page address
 *
 *
 *  Return values :
 *  ---------------
 *
 * 'OK' = 0x00:                         System FLASH OK
 *
 ************************************************************************/
static int nf_search_latest_env(n_device_type *device, UINT32 start, UINT32 end, UINT32 *latest)
{
	UINT32 page_start, page_end, increment;
	UINT32 i, spare;
	UINT8 res, version, lowest, highest;
	UINT8 wrap_version;	// version no (wrap case)
	UINT32 wrap_latest;	// latest env address (wrap case)

	if (device == NULL)
		return (-1);

	// env param size is 64KB
	increment = NAND_ENV_SIZE / device->PageSize;

	// start == -1 means search from the first block (skip block 0 because it's used by YAFFS)
	page_start = (start == -1 ? 1 : start) * pages_per_block;

	// end == -1 means search to the end of the bootcode blocks
	page_end = (end == -1 ? blk_state_len : end) * pages_per_block;

	// search to the last page of the block
	page_end += pages_per_block;

	version = highest = wrap_latest = wrap_version = 0;
	lowest = 255;
	for (i = page_start; i < page_end; i += increment)
	{
		res = nf_read_to_table(device, i, 1024);	// read 1KB is enough
		// skip empty page
		if (res == DATA_ALL_ONE)
			continue;

		nf_get_spare(0xe, &spare, 0);

		// check 1st byte of spare area identifies env param
		if ((spare & 0xff) != BLOCK_ENVPARAM_MAGICNO)
			continue;

		// get 2nd byte of the spare area
		res = (spare >> 8) & 0xff;
		if (res >= version)
		{
			version = res;
			*latest = i;
		}

		// update lowest & highest version no
		if (res < lowest)
			lowest = res;
		if (res > highest)
			highest = res;

		// update possible wraped latest no
		// (number of env in flash is expected to be smaller than 128)
		/*
			why 128?
			if BlockSize=256K, env=64K
			=> one block contains at most 4 versions
			=> thus we support at most 128/4=32 blocks for storing env

			=> if BlockSize=128K, we can support up to 64 blocks !!
		*/
		if ((res >= wrap_version) && (res < 128))
		{
			wrap_version = res;
			wrap_latest = i;
		}
	}

	// check wrap condition (when version 0 and 255 both exists)
	if ((lowest == 0) && (highest == 255))
	{
		// version no 255 is not latest, use warped ones instead
		version = wrap_version;
		*latest = wrap_latest;
	}

	// update env param version no
	env_version_no = version;

	return 0;
}


/************************************************************************
 *
 *                          nf_search_next_env
 *  Description :
 *  -------------
 *  Search next writable address for env param data
 *
 *
 *  Parameters :
 *  ------------
 *
 *  'device',	IN,	variable of type, n_device_type.
 *  'page_no',  INOUT,	page address of next writable address
 *
 *
 *  Return values :
 *  ---------------
 *
 *  '-1': device is NULL or cannot found writable address
 *
 ************************************************************************/
static int nf_search_next_env(n_device_type *device, UINT32 *page_no)
{
	INT8 res;
	UINT32 current_env_end, page_cnt;
	UINT32 current_blk, next_blk;
	UINT8 state;

	if ((device == NULL) || (page_no == NULL))
		return (-1);

	// check if latest env exist
	if (current_env_start == -1)
	{
		// no env found in flash
		//return (-1);

		// no current env, start from fileflash_phys_start
		for (next_blk = fileflash_phys_start; next_blk <= fileflash_phys_end; next_blk++)
		{
			if (get_block_state(next_blk, &state))
				continue;

			switch (state)
			{
				case BLOCK_CLEAN:
					*page_no =  next_blk * pages_per_block;
					return (0);

				case BLOCK_ENVPARAM_MAGICNO:
					res = nf_erase_block(device, next_blk);
					if (res == 0)
					{
						*page_no = next_blk * pages_per_block;
						return (0);
					}
					else	// erase failed, mark as bad block
						set_block_state(next_blk, BLOCK_BAD);
					break;

				default:
					break;
			}
		}

		// cannot find any available block
		return (-1);
	}

	/* find next writable 64KB area */
	page_cnt = NAND_ENV_SIZE / device->PageSize;
	current_env_end = current_env_start + page_cnt - 1;
	current_blk = current_env_end / pages_per_block;
	//printf("current_env_end:%x, current_blk:%x\n", current_env_end, current_blk);

	// check if current env block has space
	if ( current_blk == ((current_env_end + page_cnt) / pages_per_block) )
	{
		// current env & next env in the same block
		*page_no = current_env_start + page_cnt;
		return (0);
	}
	else
	{
		// current env & next env not in the same block
		next_blk = (current_blk >= fileflash_phys_end ? fileflash_phys_start : current_blk + 1);
		while (next_blk != current_blk)
		{
			// try to find next available block
			res = get_block_state(next_blk, &state);
			if (res == 0)
			{
				switch (blk_state[next_blk])
				{
					// skip bad block
					case BLOCK_BAD:
					// skip bootcode related block
					case BLOCK_HWSETTING:
					case BLOCK_BOOTCODE:
					case BLOCK_DATA:
						break;

					case BLOCK_CLEAN:
						*page_no = next_blk * pages_per_block;
						return (0);

					case BLOCK_ENVPARAM_MAGICNO:
					//case BLOCK_OTHER_DATA:
						res = nf_erase_block(device, next_blk);
						if (res == 0)
						{
							*page_no = next_blk * pages_per_block;
							return (0);
						}
						else	// erase failed, mark as bad block
							set_block_state(next_blk, BLOCK_BAD);
						break;

					default:
						break;
				}
			}
			// end of bootcode blocks, wrap to fileflsh start
			if (next_blk >= fileflash_phys_end)
				next_blk = fileflash_phys_start;
			else
				next_blk++;
		}
		return (-1);
	}

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
 *  'device',       	IN,    variable of type, n_device_type.
 *  'start_page',  	IN,    start page address to read
 *  'buf', 	        IN,    pointer for buffer of data to read
 *  'size',       	IN,    number of bytes to read
 *
 *  Return values :
 *  ---------------
 *  '-1': device is NULL or read beyond flash or read failed
 *
 ************************************************************************/
static int nf_read(n_device_type *device, UINT32 start_page, UINT8 *buf, UINT32 size)
{
	UINT32 stop_page;
	INT32 res;

	// validate arguments (size should be aligned to page size boundary)
	if ( (device == NULL) || (buf == NULL)
		|| (start_page > pages_per_block * blocks_per_flash)
		|| (size & (device->PageSize - 1))
		|| (size == 0) )
		return (-1);

	// do not allow read past end of flash
	stop_page = start_page + size / device->PageSize;
	if (stop_page > pages_per_block * blocks_per_flash)
		return (-1);

	while (start_page < stop_page)
	{
		res = nf_read_page(device, start_page, buf);
		switch (res)
		{
			case DATA_ALL_ONE:	// page is clean
			case 0:
				break;

			default:
				return (-1);
		}

		buf += device->PageSize;
		start_page++;
	}
	return 0;
}


/************************************************************************
 *
 *                          nf_write
 *  Description :
 *  -------------
 *  write data into NAND flash
 *
 *  Parameters :
 *  ------------
 *  'device',       	IN,    variable of type, n_device_type.
 *  'start_page',  	IN,    start page address to write
 *  'buf', 	        IN,    pointer for buffer of data to be written
 *  'size',       	IN,    number of bytes to write
 *
 *  Return values :
 *  ---------------
 *  '-1': device is NULL or write beyond flash or write failed
 *
 ************************************************************************/
static int nf_write(n_device_type *device, UINT32 start_page, UINT8 *buf, UINT32 size)
{
	UINT32 stop_page;
	INT32 res;

	// validate arguments (size should be aligned to page size boundary)
	if ( (device == NULL) || (buf == NULL)
		|| (start_page > pages_per_block * blocks_per_flash)
		|| (size & (device->PageSize - 1))
		|| (size == 0) )
		return (-1);

	// do not allow write past end of flash
	stop_page = start_page + size / device->PageSize;
	if (stop_page > pages_per_block * blocks_per_flash)
		return (-1);

	while (start_page < stop_page)
	{
		res = nf_write_page(device, start_page, buf);
		if (res)
			return (-1);
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
 *  'offset',       	IN,    offset bytes in the spare area
 *  'magic',   	    	IN,    magic no. to be searched in the spare area
 *  'found_block',   INOUT,    first found block no. with specified magic no.
 *
 *  Return values :
 *  ---------------
 *  '-1': device is NULL or start_block beyond flash or not found
 *
 ************************************************************************/
//static INT8 nf_find_blk(n_device_type *device, UINT32 start_block, UINT32 search_depth, UINT8 offset, UINT8 magic, UINT32 *found_block)
//{
//	UINT32 blk, limit, page_no;
//	UINT32 spare;
//	INT8 res;
//	UINT8 state;
//
//	// validate arguments
//	if ((device == NULL) || (start_block > blocks_per_flash) ||
//		(offset > 16) || (found_block == NULL))
//		return (-1);
//
//	// determine search limit
//	if (search_depth == 0)
//		//limit = blocks_per_flash;	// no limit (until last block)
//		limit = NAND_BOOTCODE_SIZE / device->BlockSize;	// only bootcode blocks (first 16MB of the flash)
//	else if (search_depth == 0xffffffff)
//		limit = blocks_per_flash;	// search until end of flash
//	else
//	{
//		limit = start_block + search_depth;
//
//		// cannot search beyond flash (max is last block of flash)
//		if (limit > blocks_per_flash)
//			limit = blocks_per_flash;
//	}
//
//	for (blk = start_block, page_no = blk * pages_per_block;
//		blk < limit ;
//		blk++, page_no += pages_per_block)
//	{
//		// read first page of the block to table sram
//		res = nf_read_to_table(device, page_no, 512);	// read 512B is enough
//		//res = nf_read_page(device, page_no, (UINT8 *) 0xa2001000);
//
//		switch (res)
//		{
//			case DATA_ALL_ONE:
//				set_block_state(blk, BLOCK_CLEAN);
//				continue;
//
//			case 0:		// read to table success
//				nf_get_spare(0xe, &spare, offset);
//				//nf_get_spare(0x2, &spare, offset);
//				spare &= 0xff;
//				break;
//
//			default:	// read to table has error
//				set_block_state(blk, BLOCK_BAD);
//				continue;	// next block
//		}
//
//		// NOTE: BLOCK_UNDETERMINE is a fake state,
//		// all blocks in the flash "SHALL NOT" have this state or it will cause some problem!!
//		if (spare == BLOCK_UNDETERMINE)
//			continue;	// should not happen
//
//		/*
//		// check if block is beyond the range of block state table
//		res = get_block_state(blk, &state);
//		if (res)
//			continue;
//		*/
//
//		// update with new magic no. in spare area
//		if (offset == 0)
//			set_block_state(blk, (spare & 0xff));
//
//		if (spare == magic)
//		{
//			*found_block = blk;
//			return (0);
//		}
//	}
//
//	// not found
//	return (-1);
//}
//

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
	//UINT8 state;

	// validate arguments
	if ((device == NULL) || (start_block >= blocks_per_flash) || (offset >= pages_per_block))
		return (-1);

	// determine search limit
	if (search_depth == 0)
		limit = NAND_BOOTCODE_SIZE / device->BlockSize;	// only bootcode blocks (first 16MB of the flash)
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
		res = nf_read_to_table(device, page_no, 2048);	// read page to table SRAM
		//res = nf_read_page(device, page_no, (UINT8 *) 0xa2001000);

		switch (res)
		{
			case DATA_ALL_ONE:
				set_block_state(blk, BLOCK_CLEAN);
				continue;

			case 0:		// read to table success
				nf_get_spare(0xe, &spare, 0);
				//nf_get_spare(0x2, &spare, 0);
				spare &= 0xff;
				break;

			default:	// read to table has error
				set_block_state(blk, BLOCK_BAD);
				continue;	// next block
		}

		// NOTE: BLOCK_UNDETERMINE is a fake state,
		// all blocks in the flash "SHALL NOT" have this state or it will cause some problem!!
		if (spare == BLOCK_UNDETERMINE)
			continue;	// should not happen

		/*
		// check if block is beyond the range of block state table
		res = get_block_state(blk, &state);
		if (res)
			continue;
		*/

		// update with new magic no. in spare area
		set_block_state(blk, (UINT8)(spare & 0xff));
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
/*
	if (block_no >= blk_state_len)
		return (-1);

	blk_state[block_no] = state;
	return (0);
*/
	blk_state[block_no] = state;
	return (block_no < blk_state_len ? 0 : -1);
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
/*
	if (block_no >= blk_state_len)
		return (-1);

	*state = blk_state[block_no];
	return 0;
*/
	*state = blk_state[block_no];
	return (block_no < blk_state_len ? 0 : -1);
}


/************************************************************************
 *
 *                          get_env_n
 *  Description :
 *  -------------
 *  get env param from flash to DRAM
 *
 *  Parameters :
 *  ------------
 *  'device',	IN,	variable of type, n_device_type.
 *  'buf',	IN,	env data pointer
 *
 *  Return values :
 *  ---------------
 *  '-1': means device or buf is NULL, or no env found, or copy env to DRAM failed
 *
 ************************************************************************/
int get_env_n(void *dev, UINT8 *buf)
{
	INT32 res;
	n_device_type *device = (n_device_type *)dev;

	if ((device == NULL) || (buf == NULL))
		return (-1);

        // search latest env data from fileflash_phys_start
       	nf_search_latest_env(device, fileflash_phys_start, fileflash_phys_end, &current_env_start);
	if (current_env_start == -1)
        	return (-1);	// no env found

        // copy latest env data to DDR
        res = nf_read(device, current_env_start, buf, NAND_ENV_SIZE);
       	return (res);
}


/************************************************************************
 *
 *                          save_env_n
 *  Description :
 *  -------------
 *  save env param from DRAM to flash
 *
 *  Parameters :
 *  ------------
 *  'device',	IN,	variable of type, n_device_type.
 *  'buf',	IN,	env data pointer
 *
 *  Return values :
 *  ---------------
 *  '-1': means device or buf is NULL, or no env found, or find next writable page address failed
 *
 ************************************************************************/
int save_env_n(void *dev, UINT8 *buf)
{
	n_device_type *device = (n_device_type *)dev;
	UINT32 start_page, spare;
	int rc;

	if ((device == NULL) || (buf == NULL))
		return (-1);

	// find next writable page address
	rc = nf_search_next_env(dev, &start_page);
	if (rc != 0)
		return rc;

	// if this is the first time to write env param, start with version 0
	spare = (current_env_start == -1 ? 0 : env_version_no + 1);
	spare = (spare << 8) | BLOCK_ENVPARAM_MAGICNO;
	nf_set_spare(spare, 0);
	rc = nf_write(device, start_page, buf, NAND_ENV_SIZE);
//	printf("start_page:%x, env_version_no: %x\n", start_page, (current_env_start == -1 ? 0 : env_version_no + 1));
	if (rc == 0)
	{
		// write env param success
		current_env_start = start_page;	// update env param page address
		env_version_no++;		// increment env param version no

		// update block state array
		if (set_block_state(start_page / pages_per_block, BLOCK_ENVPARAM_MAGICNO))
			return (-1);
	}

	return rc;

}

static int bbt_exist(void)
{
	//int res;
	UINT8 state = 0;

	// first check block 0
	get_block_state(0, &state);
	switch (state)
	{
		case BLOCK_CLEAN:	// block 0 is empty
			return (0);

		case BLOCK_BBT:		// block 0 has bad block table
			return (1);

		case BLOCK_BAD:
			printf("block 0 failed\n");
		default:
			break;
	}

	// block 0 failed, check block 1 instead
	get_block_state(1, &state);
	switch (state)
	{
		case BLOCK_CLEAN:	// block 1 is empty
			return (0);

		case BLOCK_BBT:		// block 1 has bad block table
			return (1);

		case BLOCK_BAD:
			printf("block 1 failed\n");
		default:
			return (-1);
	}
}

static int init_bbt(void)
{
	UINT32 i, remap_idx;
	UINT32 bbt_limit;

	// reset bad block table with initial value
	for (i = 0 ; i < BBT_SIZE; i++)
	{
		bbt[i].bad_block = BB_INIT;
		bbt[i].remap_block = RB_INIT;
		bbt[i].bad_block_type = 0;
	}

	// fill the remap block field
	remap_idx = blocks_per_flash - 1;
	bbt_limit = remap_idx - BBT_SIZE;
	for (i = 0; i < BBT_SIZE; i++)
	{
		if (get_remap_block(&remap_idx))
			return (-1);		// cannot find remap block anymore

		// remapping block no. is limited to the last "BBT_SIZE" blocks of flash
		if (remap_idx < bbt_limit)
			break;

		bbt[i].remap_block = remap_idx;
		remap_idx--;
	}

	return 0;
}

static int get_remap_block(UINT32 *start_blk)
{
	UINT8 state;
	UINT32 ith;

	if (start_blk == NULL)
		return (-1);

	// cannot remap to bootcode area (first 16MB)
	if (*start_blk < blk_state_len)
		return (-1);

	// cannot remap beyond flash
	if (*start_blk >= blocks_per_flash)
		return (-1);

	// search backward
	for (ith = *start_blk; ith >= blk_state_len; ith--)
	{
		get_block_state(ith, &state);

		// found replaceable block
		if (state == BLOCK_CLEAN)
		{
			*start_blk = ith;
			return (0);
		}
	}

	return (-1);
}

static int build_bbt(void *dev)
{
	//int res;
	UINT32 i, bbt_i;
	//UINT32 start_blk;
	UINT8 state;
	n_device_type *device = (n_device_type *)dev;

	// check for bad block
	for (i = NAND_BOOTCODE_SIZE / device->BlockSize, bbt_i = 0;
		(i < blocks_per_flash) && (bbt_i < BBT_SIZE); // stop when bad block table is full
		i++)
	{
		get_block_state(i, &state);
		if (state == BLOCK_BAD)
		{
			// check remapping block is valid
			if (bbt[bbt_i].remap_block == RB_INIT)
				break;			// reach the end of valid table entry

			// register into bad block table
			bbt[bbt_i].bad_block = i;
			bbt_i++;
		}
	}

	return (0);
}

static UINT32 initial_bad_block_offset(n_device_type *device)
{

	// initial bad block mark is in first page of the block
	if (device->initial_bb_pos == INITIAL_BB_POS_FIRST)
		return 0;
	// initial bad block mark is in last page of the block
	else if (device->initial_bb_pos == INITIAL_BB_POS_LAST)
		return (pages_per_block - 1);
	// should not happen
	else
		return -1;
}

/************************************************************************
 *
 *                          do_read
 *  Description :
 *  -------------
 *  implement the flash read
 *
 *  Parameters :
 *  ------------
 *
 *  Return values :
 *  ---------------
 *
 ************************************************************************/
int do_read_n(void *dev,
               unsigned int*  start_blk,	// start of block NO.
               unsigned char* dest,		// dest memory address
               unsigned int   rem_size,		// request data length
               unsigned int   signature)
{
	n_device_type *device = (n_device_type *)dev;
	UINT32 i, page_len;
	UINT8 state;

	// search block state table
	if ((signature) && (*start_blk < blk_state_len))
	{
		for (i = *start_blk; i < blk_state_len; i++)
		{
			get_block_state(i, &state);
			if (state == signature)
			{
				*start_blk = i;
				break;
			}
		}

		if (i == blk_state_len)
			return -1;		// not found
	}

	// align data length to page boundary
	page_len = rem_size / device->PageSize;
	if (rem_size % device->PageSize)
		page_len++;

	return nf_read(device, (*start_blk * pages_per_block), dest,
					(page_len * device->PageSize));
}
