/************************************************
 Comments in Big5:				
	此檔描述 款Flash Memory 
	動作時所需的函式				
				
************************************************/
#ifndef __FLASHDEV_S_H__
#define __FLASHDEV_S_H__


#define	MX_4Mbit				0x1320c2 
#define	MX_25L1605_16Mbit		0x1520c2
#define	MX_25L6405D_64Mbit		0x1720c2
#define	MX_25L12805D_128Mbit	0x1820c2

#define	PMC_4Mbit				0x7e9d7f

#define	SPANSION_16Mbit			0x140201
#define	SPANSION_32Mbit			0x150201
#define	SPANSION_64Mbit			0x160201
#define	SPANSION_128Mbit		0x182001
#define	SPANSION_128Mbit_64s	0x0103
#define	SPANSION_128Mbit_256s	0x0003

#define	SST_8Mbit				0x80bf
#define	SST_4Mbit				0x8d25bf
#define	SST_16Mbit				0x4125bf

#define	STM_64Mbit				0x172020
#define	STM_128Mbit				0x182020

#define	EON_EN25B64_64Mbit			0x17201c

typedef struct 
{
    unsigned int    id ;
    unsigned char   sec_256k_en ;
    unsigned char   sec_64k_en ;
    unsigned char   sec_32k_en ;
    unsigned char   sec_4k_en ;
    unsigned char   page_program ;
    
} s_device_type;


/**********************************************
************************************************/

static const s_device_type s_device[] = 
{
 {SST_4Mbit,             0, 1, 1, 1, 0} ,
 {SST_8Mbit,             0, 0, 1, 1, 0} ,
 {SST_16Mbit,            0, 1, 1, 1, 0} ,
 {PMC_4Mbit,             0, 1, 0, 1, 0} ,
 {MX_4Mbit,              0, 1, 0, 0, 0} ,
 {MX_25L1605_16Mbit,     0, 1, 0, 0, 0} ,
 {MX_25L6405D_64Mbit,    0, 1, 0, 0, 1} ,
 {MX_25L12805D_128Mbit,  0, 1, 0, 0, 1} ,
 {SPANSION_16Mbit,       0, 1, 0, 0, 0} ,
 {SPANSION_32Mbit,       0, 1, 0, 0, 0} ,
 {SPANSION_64Mbit,       0, 1, 0, 0, 1} ,
 {SPANSION_128Mbit,      0, 0, 0, 0, 0} ,  //super device-id, try to check ext-id later
 {SPANSION_128Mbit_64s,  0, 1, 0, 0, 1} ,
 {SPANSION_128Mbit_256s, 1, 0, 0, 0, 1} ,
 {STM_64Mbit,            0, 1, 0, 0, 1} ,
 {STM_128Mbit,           1, 0, 0, 0, 1} ,
 {EON_EN25B64_64Mbit,    0, 1, 0, 0, 1} ,
} ; 


#define DEV_SIZE_S	(sizeof(s_device)/sizeof(s_device_type))

/************************************************************************
 *  Public function
 ************************************************************************/
int do_erase_s(void  *dev,
               unsigned char* dest,
               unsigned int   rem_size);

int do_write_s(void *dev,
               unsigned char* array_ptr,
               unsigned char* dest,
               unsigned int   rem_size,
               unsigned int   signature);

int do_identify_s(void **dev);

void do_init_s(void *dev);

void do_exit_s(void *dev);

#endif
