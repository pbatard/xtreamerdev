/* NAND Flash functions for RTD Serial Recovery (rtdsr)
 *
 * copyright (c) 2011 Pete B. <xtreamerdev@gmail.com>
 * based on flashdev_n.h for RTD1283 by Realtek
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

#ifndef __FLASHDEV_N_H__
#define __FLASHDEV_N_H__


/* Read ID, The first 2 cycles -- MakeID,DeviceID */
/* MakerCode -- 0x98:Toshiba, 0xAD: Hynix, 0xEC: Samsung, 0x20: Numonyx */

#define K9F8G08U0M		0xA610D3EC	// Samsung 8Gb (SLC single die) 1st generation
//#define K9F5608B0D		0x75EC		// Samsung 256Mb (SLC single die, small block)
//#define K9F1208U0C		0x76EC		// Samsung 512Mb (SLC single die, small block)
#define K9F1G08U0B		0x9500F1EC	// Samsung 1Gb (SLC single die) 3rd generation
#define K9F2G08U0B		0x9510DAEC	// Samsung 2Gb (SLC single die) 3rd generation
#define K9G4G08U0A		0x2514DCEC	// Samsung 4Gb (MLC single die) 2nd generation
#define K9G4G08U0B		0xA514DCEC	// Samsung 4Gb (MLC single die) 3rd generation
#define K9G8G08U0M		0x2514D3EC	// Samsung 8Gb (MLC single die) 1st generation
#define K9G8G08U0A		0xA514D3EC	// Samsung 8Gb (MLC single die) 2nd generation
#define K9K8G08U0A		0x9551D3EC	// Samsung 8Gb (SLC die stack)	2nd generation

//#define HY27US0812		0x76AD		// Hynix 512Mb (SLC single die, small block)
#define HY27UF081G2A	0x1D80F1AD	// Hynix 1Gb   (SLC single die) 2nd generation
#define HY27UF082G2A	0x1D80DAAD	// Hynix 2Gb   (SLC single die) 2nd generation
#define HY27UF082G2B	0x9510DAAD	// Hynix 2Gb   (SLC single die) 3rd generation
#define HY27UF084G2M	0x9580DCAD	// Hynix 4Gb   (SLC single die) 1st generation
#define HY27UT084G2M	0x2584DCAD	// Hynix 4Gb   (MLC single die) 1st generation
#define HY27UT084G2A	0xA514DCAD	// Hynix 4Gb   (MLC single die) 2nd generation
#define HY27UF084G2B	0x9510DCAD	// Hynix 4Gb   (SLC single die) 3rd generation
//#define HY27UG088G5B	0x9510DCAD	// Hynix 8Gb   (SLC Double die) 3rd generation
#define HY27UT088G2A	0xA514D3AD	// Hynix 8Gb   (MLC single die) 2nd generation
#define H27U8G8T2B		0xB614D3AD	// Hynix 8Gb   (MLC single die) 3rd generation

#define NAND01GW3B2BN6	0x1D80F120	// ST 1Gb      (SLC single die) 2nd version
#define NAND01GW3B2CN6	0x1D00F120	// ST 1Gb      (SLC single die) 3rd version
#define NAND02GW3B2DN6	0x9510DA20	// ST 2Gb      (SLC single die) 4th version
#define NAND04GW3B2DN6	0x9510DC20	// ST 4Gb      (SLC single die) 4th version
#define NAND04GW3B2B	0x9580DC20	// Numonyx 4Gb (SLC single die) 2nd version
#define NAND04GW3C2B	0xA514DC20	// Numonyx 4Gb (MLC single die)
#define NAND08GW3C2BN6	0xA514D320	// Numonyx 8Gb (MLC single die) 2nd version

#define TC58NVG0S3C		0x9590F198	// Toshiba 1Gb (SLC single die)
#define TC58NVG0S3E		0x1590D198	// Toshiba 1Gb (SLC single die)
#define TC58NVG1S3C		0x9590DA98	// Toshiba 2Gb (SLC single die)
#define TC58NVG1S3E		0x1590DA98	// Toshiba 2Gb (SLC single die)
#define TC58NVG2S3E		0x1590DC98	// Toshiba 4Gb (SLC single die)

/* Address mode of different flash device */
#define ADDR_MODE_C8_R24	0x0	// 000:  8 bit Column address + 24 bit Row address
#define ADDR_MODE_C16_R24	0x1	// 001: 16 bit Column address + 24 bit Row address
#define ADDR_MODE_C8_R16	0x2	// 010:  8 bit Column address + 16 bit Row address
#define ADDR_MODE_C16_R16	0x3	// 011: 16 bit Column address + 16 bit Row address
#define ADDR_MODE_R24		0x4	// 100: 24 bit Row address
#define ADDR_MODE_R16		0x6	// 110: 16 bit Row address
#define ADDR_MODE_R8		0x7	// 111:  8 bit Row address

/* Position of initial bad block mark */
#define INITIAL_BB_POS_FIRST 0x00	// in the first page of initial bad block
#define INITIAL_BB_POS_LAST  0xff	// in the last  page of initial bad block

/* NF freq divider value */
#define CLK_NF_27MHz		0x7
#define CLK_NF_31MHz		0x6
#define CLK_NF_36MHz		0x5
#define CLK_NF_43MHz		0x4
#define CLK_NF_54MHz		0x3
#define CLK_NF_72MHz		0x2

typedef struct
{
	unsigned int    id;
	unsigned short  PageSize;
	unsigned int    BlockSize;
	unsigned int    size;
	unsigned char   addr_mode_rw;		// read/write address mode
	unsigned char   addr_mode_erase;	// erase address mode
	unsigned char   num_chips;			// number of chips/die in the flash
	unsigned char   initial_bb_pos;		// position of initial bad block mark
	unsigned char   t1;					// timing t1
	unsigned char   t2;					// timing t2
	unsigned char   t3;					// timing t3
	unsigned char   nf_div;				// NF freq divide value
	unsigned char   read_id_len;		// length of READ ID result
	unsigned char   fifth_id;			// 5th byte of READ ID result (0xff if read_id_len = 4)
	unsigned char   *string;
} n_device_type;

static const n_device_type n_device[] =
{
	{K9F8G08U0M,        4096, 256*1024, 0x08000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x64, "K9F8G08U0M" } ,
	{K9F1G08U0B,        2048, 128*1024, 0x08000000, ADDR_MODE_C16_R16, ADDR_MODE_R16, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x40, "K9F1G08U0B" } ,
	{K9F2G08U0B,        2048, 128*1024, 0x10000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x44, "K9F2G08U0B" } ,
	{K9G4G08U0A,        2048, 256*1024, 0x20000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_LAST,  0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x54, "K9G4G08U0A" } ,
	{K9G4G08U0B,        2048, 256*1024, 0x20000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_LAST,  0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x54, "K9G4G08U0B" } ,
	{K9G8G08U0M,        2048, 256*1024, 0x40000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_LAST,  0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x64, "K9G8G08U0M" } ,
	{K9G8G08U0A,        2048, 256*1024, 0x40000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_LAST,  0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x64, "K9G8G08U0A" } ,
	{K9K8G08U0A,        2048, 128*1024, 0x40000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x58, "K9K8G08U0A" } ,

	{HY27UF081G2A,      2048, 128*1024, 0x08000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 4, 0xff, "HY27UF081G2A" } ,
	{HY27UF082G2A,      2048, 128*1024, 0x10000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x0,  "HY27UF082G2A" } ,
	{HY27UF082G2B,      2048, 128*1024, 0x10000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x44, "HY27UF082G2B" } ,
	{HY27UF084G2M,      2048, 128*1024, 0x20000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 4, 0xff, "HY27UF084G2M" } ,
	{HY27UT084G2M,      2048, 256*1024, 0x20000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_LAST,  0x1, 0x1, 0x1, CLK_NF_43MHz, 4, 0xff, "HY27UT084G2M" } ,
	{HY27UT084G2A,      2048, 256*1024, 0x20000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_LAST,  0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x24, "HY27UT084G2A" } ,
	{HY27UF084G2B,      2048, 256*1024, 0x20000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x54, "HY27UF084G2B" } ,
	//    {HY27UG088G5B,      2048, 128*1024, 0x40000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 2, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x54, "HY27UG088G5B" } ,
	{HY27UT088G2A,      2048, 256*1024, 0x40000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_LAST,  0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x34, "HY27UT088G2A"} ,
	{H27U8G8T2B,        4096, 512*1024, 0x40000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_LAST,  0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x34, "H27U8G8T2B"} ,

	{NAND01GW3B2BN6,    2048, 128*1024, 0x08000000, ADDR_MODE_C16_R16, ADDR_MODE_R16, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 4, 0xff, "NAND01GW3B2BN6"} ,
	{NAND01GW3B2CN6,    2048, 128*1024, 0x08000000, ADDR_MODE_C16_R16, ADDR_MODE_R16, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 4, 0xff, "NAND01GW3B2CN6"} ,
	{NAND02GW3B2DN6,    2048, 128*1024, 0x10000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x44, "NAND02GW3B2DN6"} ,
	{NAND04GW3B2DN6,    2048, 128*1024, 0x20000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x54, "NAND04GW3B2DN6"} ,
	{NAND04GW3B2B,      2048, 128*1024, 0x20000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 4, 0xff, "NAND04GW3B2B"} ,
	{NAND04GW3C2B,      2048, 256*1024, 0x20000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_LAST,  0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x24, "NAND04GW3C2B"} ,
	{NAND08GW3C2BN6,    2048, 256*1024, 0x40000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_LAST,  0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x34, "NAND08GW3C2BN6"} ,

	{TC58NVG0S3C,       2048, 128*1024, 0x08000000, ADDR_MODE_C16_R16, ADDR_MODE_R16, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 4, 0xff, "TC58NVG0S3C"} ,
	{TC58NVG0S3E,       2048, 128*1024, 0x08000000, ADDR_MODE_C16_R16, ADDR_MODE_R16, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x76, "TC58NVG0S3E"} ,
	{TC58NVG1S3C,       2048, 128*1024, 0x10000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 4, 0xff, "TC58NVG1S3C"} ,
	{TC58NVG1S3E,       2048, 128*1024, 0x10000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x76, "TC58NVG1S3E"} ,
	{TC58NVG2S3E,       2048, 128*1024, 0x20000000, ADDR_MODE_C16_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 5, 0x76, "TC58NVG2S3E"} ,
	//    {K9F5608B0D,         512,  16*1024, 0x02000000,  ADDR_MODE_C8_R16, ADDR_MODE_R16, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 2, 0x0, "K9F5608B0D" } ,
	//    {K9F1208U0C,         512,  16*1024, 0x04000000,  ADDR_MODE_C8_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 2, 0x0, "K9F1208U0C" } ,
	//    {HY27US0812,         512,  16*1024, 0x04000000,  ADDR_MODE_C8_R24, ADDR_MODE_R24, 1, INITIAL_BB_POS_FIRST, 0x1, 0x1, 0x1, CLK_NF_43MHz, 2, 0x0, "HY27US0812" }
} ;



#define DEV_SIZE_N	(sizeof(n_device)/sizeof(n_device_type))


enum trans_mode{
	TRANS_CMD  = 0x80,
	TRANS_ADDR = 0x81,
	TRANS_SBR  = 0x82,
	TRANS_MBW  = 0x83,		//Multi Byte Write
	TRANS_MBR  = 0x84		//Multi Byte Read
};

/* bad block table structure */
typedef struct  __attribute__ ((__packed__)){
	unsigned int bad_block;			// block no. marked as bad
	unsigned int remap_block;		// remapping block no.
	unsigned char bad_block_type;	// bad block type
} BB_t;

/* block state */
#define BLOCK_BAD				0x00
#define BLOCK_HWSETTING			0x23
#define BLOCK_BOOTCODE			0x79
#define BLOCK_DATA				0x80
#define BLOCK_ENVPARAM_MAGICNO	0x81
#define BLOCK_OTHER_DATA		0xd0	// unknown data (user program into flash)
#define BLOCK_BBT				0xbb	// bad block table
#define BLOCK_CLEAN				0xff	// block is empty
#define BLOCK_UNDETERMINE		0x55	// block state is undetermined
// (NOTE: BLOCK_UNDETERMINE is a fake state, all blocks in the flash SHALL NOT have this state or it will cause some problem)

#define DATA_ALL_ONE		1			// read one page and all bit is '1'
#define NULL				((void *)0)
#define NAND_BOOTCODE_SIZE	0x2000000	// bootcode area size in NAND flash (first 32MB)
#define NAND_ENV_SIZE		0x10000		// size of env param + magic no

#define BBT_SIZE 			200			// size of bad block table
#define BB_INIT 			0xF0000000	// initial bad_block value in bad block table
#define RB_INIT				0xF1000000	// initial remap_block value in bad block table

/************************************************************************
*  Public function
************************************************************************/
int  nf_erase(n_device_type* device, unsigned long start_block, unsigned long size);
int  nf_read(n_device_type* device, unsigned long start_page, unsigned char* buf, unsigned long size);
int  nf_write(n_device_type* device, unsigned long start_block, unsigned char* buf, unsigned long size);
int  nf_identify(n_device_type** device);
void nf_init(n_device_type* device);

#endif
