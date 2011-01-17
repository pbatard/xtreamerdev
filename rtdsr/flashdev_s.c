#include "flashdev_s.h"
#include "util.h"

#define INV_DATA_SEG 0xa1000000


/************************************************************************
 *
 *                          do_erase_s 
 *  Description :
 *  -------------
 *  implement the flash erase
 *
 *  Parameters :
 *  ------------
 *
 *  Return values :
 *  ---------------
 *
 ************************************************************************/

int do_erase_s(void *dev,
               unsigned char* dest,
               unsigned int   rem_size)
{
    volatile unsigned char tmp;
    unsigned int size;
    s_device_type *device = (s_device_type *)dev;                                         
    
    while(rem_size > 0)
    {
        /* show '/' in console */
        REG32(0xb801b200)= 0x2f ; //cy test
        
        /* write enable */
        REG32(0xb801a800)=0x00520006;
        REG32(0xb801a804)=0x00180000;
        tmp = *dest;
        

        //image size >= 256K
        if (rem_size >= 0x40000)
        {
            if (device->sec_256k_en == 1)
            {
                REG32(0xb801a800)=0x000000d8;
                REG32(0xb801a804)=0x00000008;
                tmp = *dest;
                size = 0x40000;
            }
            else if (device->sec_64k_en == 1)
            {
                REG32(0xb801a800)=0x000000d8;
                REG32(0xb801a804)=0x00000008;
                tmp = *dest;
                size = 0x10000;
            }
            else if (device->sec_32k_en == 1)
            {
                REG32(0xb801a800)=0x00000052;
                REG32(0xb801a804)=0x00000008;
                tmp = *dest;
                size = 0x8000;
            }
            else if (device->sec_4k_en == 1)
            {
                if (device->id==PMC_4Mbit)
                    REG32(0xb801a800)=0x000000d7;
                else
                    REG32(0xb801a800)=0x00000020;
                
                REG32(0xb801a804)=0x00000008;
                tmp = *dest;
                size = 0x1000;
            }
        }
        //256K > image size >= 64K
        else if ((0x40000 > rem_size) && (rem_size >= 0x10000))
        {
            if (device->sec_64k_en == 1)
            {
                REG32(0xb801a800)=0x000000d8;
                REG32(0xb801a804)=0x00000008;
                tmp = *dest;
                size = 0x10000;
            }
            else if (device->sec_32k_en == 1)
            {
                REG32(0xb801a800)=0x00000052;
                REG32(0xb801a804)=0x00000008;
                tmp = *dest;
                size = 0x8000;
            }
            else if (device->sec_4k_en == 1)
            {
                if (device->id==PMC_4Mbit)
                    REG32(0xb801a800)=0x000000d7;
                else
                    REG32(0xb801a800)=0x00000020;
                
                REG32(0xb801a804)=0x00000008;
                tmp = *dest;
                size = 0x1000;
            }
            else if (device->sec_256k_en == 1)
            {
                REG32(0xb801a800)=0x000000d8;
                REG32(0xb801a804)=0x00000008;
                tmp = *dest;
                size = 0x40000;
            }
        }
        //64K > image size >= 32K
        else if ((0x10000 > rem_size) && (rem_size >= 0x8000))
        {
            if (device->sec_32k_en == 1)
            {
                REG32(0xb801a800)=0x00000052;
                REG32(0xb801a804)=0x00000008;
                tmp = *dest;
                size = 0x8000;
            }
            else if (device->sec_4k_en == 1)
            {
                if (device->id==PMC_4Mbit)
                    REG32(0xb801a800)=0x000000d7;
                else
                    REG32(0xb801a800)=0x00000020;
                
                REG32(0xb801a804)=0x00000008;
                tmp = *dest;
                size = 0x1000;
            }
            else if (device->sec_64k_en == 1)
            {
                REG32(0xb801a800)=0x000000d8;
                REG32(0xb801a804)=0x00000008;
                tmp = *dest;
                size = 0x10000;
            }
            else if (device->sec_256k_en == 1)
            {
                REG32(0xb801a800)=0x000000d8;
                REG32(0xb801a804)=0x00000008;
                tmp = *dest;
                size = 0x40000;
            }
        }
        //32K > image size 
        else if (0x8000 > rem_size)
        {
            
            if (device->sec_4k_en == 1)
            {
                if (device->id==PMC_4Mbit)
                    REG32(0xb801a800)=0x000000d7;
                else
                    REG32(0xb801a800)=0x00000020;
                
                REG32(0xb801a804)=0x00000008;
                tmp = *dest;
                size = 0x1000;
            }
            else if (device->sec_32k_en == 1)
            {
                REG32(0xb801a800)=0x00000052;
                REG32(0xb801a804)=0x00000008;
                tmp = *dest;
                size = 0x8000;
            }
            else if (device->sec_64k_en == 1)
            {
                REG32(0xb801a800)=0x000000d8;
                REG32(0xb801a804)=0x00000008;
                tmp = *dest;
                size = 0x10000;
            }
            else if (device->sec_256k_en == 1)
            {
                REG32(0xb801a800)=0x000000d8;
                REG32(0xb801a804)=0x00000008;
                tmp = *dest;
                size = 0x40000;
            }
        }
        
        /* read status registers until write operation completed*/
        do
        {
            REG32(0xb801a800)=0x00000005;
            REG32(0xb801a804)=0x00000010;
        } while((tmp = *dest)&0x1);
        
        size = (rem_size > size)? size : rem_size ;
        rem_size-=size;
        
        dest+=size;
    }
    
    return 0;
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
int do_write_s(void *dev,
               unsigned char* array_ptr,
               unsigned char* dest,
               unsigned int   rem_size,
               unsigned int   signature)
{
    volatile unsigned char tmp;
    int i = 0;
    int inv_cnt;
    unsigned int val;
    s_device_type *device = (s_device_type *)dev;
    
    // if dest address is not on 256 byte boundary, use byte program until reach boundary
    while ((((UINT32)dest) & 0xff) && (rem_size > 0))
    {
        /* write enable */
        REG32(0xb801a800)=0x00520006;
        REG32(0xb801a804)=0x00180000;
        tmp = *dest;
        
        /* Byte pgm */
        REG32(0xb801a800)=0x00000002;
        REG32(0xb801a804)=0x00000018;
        *dest++ = *array_ptr++;
        rem_size--;
        
        /* read status registers */
        do {
            REG32(0xb801a800)=0x00000005;
            REG32(0xb801a804)=0x00000010;
        } while((tmp = *dest)&0x1);
        
    }
    
    //if flash support page program, we can use page program to reduce program time
    if (device->page_program)
    {
        //if writing data greater than one page(256 bytes)
        while(rem_size > 256)
        { 
            /* show '.' in console */
            if ( !(i++ % 4))
                REG32(0xb801b200)= 0x2e ; //cy test
            
            /* data moved by MD module is endian inverted. revert it before moved to flash */
            for (inv_cnt = 0; inv_cnt < 255; inv_cnt+=4)
            {
            	val = *((volatile unsigned int *)(array_ptr + inv_cnt));
            	
            	*((volatile unsigned int *)(INV_DATA_SEG + inv_cnt)) = 
            	                               ( ( val >> 24)             |
            	                                 ((val >> 8 ) & 0xff00  ) |
            	                                 ((val << 8 ) & 0xff0000) |
                       	                         ( val << 24) );
            }
            
            //(write enable)
    	    REG32(0xb801a800)=  0x00520006;
    	    REG32(0xb801a804)=  0x00180000;
            tmp = *dest;
            
            //issue write command
            REG32(0xb801a800) =  0x00000002;
    	    REG32(0xb801a804) =  0x00000018;
    	    
    	    //setup MD DDR addr and flash addr
        	REG32(0xb800b088) = INV_DATA_SEG;
    	    REG32(0xb800b08c) = (UINT32)dest;
        	//setup MD direction and move data length
        	REG32(0xb800b090) = 0x06000100;
    	    
    	    //go 
    	    REG32(0xb800b094) = 0x3;
    	
    	    /* wait for MD done its operation */
    	    while(REG32(0xb800b094) & 0x1) ;
    	    
    	    /* wait for flash controller done its operation */
            do {
                REG32(0xb801a800)=0x00000005;
                REG32(0xb801a804)=0x00000010;
            } while((tmp = *dest)&0x1);
    	    
    	    /* shift to next page to writing */
    	    array_ptr+=256;
    	    dest+=256;
    	    rem_size-=256;
        
        }//end of page program
        
    }//end of page program
    
    
    /* left data in-sufficient to one page, we write it using byte program 
       or flash doesn't support page program mode */
    while(rem_size--)
    { 
        /* show '.' in console */
        if ((rem_size % 1024 )== 1023)
            REG32(0xb801b200)= 0x2e ; //cy test
            
        /* write enable */
        REG32(0xb801a800)=0x00520006;
        REG32(0xb801a804)=0x00180000;
        tmp = *dest;
        
        /* Byte pgm */
        REG32(0xb801a800)=0x00000002;
        REG32(0xb801a804)=0x00000018;
        *dest++ = *array_ptr++;
        
        /* read status registers */
        do {
            REG32(0xb801a800)=0x00000005;
            REG32(0xb801a804)=0x00000010;
        } while((tmp = *dest)&0x1);
        
    }
    
    return 0;
}
 
/************************************************************************
 *
 *                          do_identify_s
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
int do_identify_s(void **dev)
{
	UINT32 id;
	UINT32 idx;

	UINT8 * temp_ptr = (UINT8 *)0xbec00000;

    /* switch to in-order mode */
    asm(".set mips3");
    asm("sync");
    asm(".set mips1");
	//disable w-merge and r-bypass
    REG32(0xb801a018)=((REG32(0xb801a018) | 0x8 ) & ~0x3);
    asm(".set mips3");
    asm("sync");
    asm(".set mips1");
    //enable command inorder
    REG32(0xb801a018)= (REG32(0xb801a018) | 0xc);
    asm(".set mips3");
    asm("sync");
    asm(".set mips1");


    REG32(0xb801a814)=0x00000001;   //set serial flash controller latch data at rising edge
    REG32(0xb801a808)=0x01010007;   //lowering frequency

    REG32(0xb801a80c)=0x00090101;   //setup control edge	
	
	/* read Manufacture & device ID */
    REG32(0xb801a800)=0x0000009f;
    REG32(0xb801a804)=0x00000010;
    id = *(unsigned int*)temp_ptr & 0xffffff ;
    
    /* find the match flash brand */
    for (idx = 0; idx < DEV_SIZE_S; idx++)
    {
        if ( s_device[idx].id == id )
        {
        	//special case: the same id have two mode, check ext-id
        	if (id == SPANSION_128Mbit)
        	{
        		/* read extended device ID */
                REG32(0xb801a800)=0x0000009f;
                REG32(0xb801a804)=0x00000013;
                id = *(unsigned int*)temp_ptr & 0xffff;
                
        		continue;
        	}
        	
            *dev = (void *)&s_device[idx];
            return 0;
        }
    }
    
    //device not found, down freq and try again
    if (idx == DEV_SIZE_S)
    {
        /* maybe not JEDEC , lower the freq. try again*/
        REG32(0xb801a808)=0x0101000b;  //lower frequency 
        REG32(0xb801a800)=0x00000090;  //read id
        REG32(0xb801a804)=0x00000010;  //issue command
        id = *(unsigned int*)temp_ptr & 0xffff ;
           
        for (idx = 0; idx < DEV_SIZE_S; idx++)
        {
            if ( s_device[idx].id == id )
            {
            	//special case: the same id have two mode, check ext-id
            	if (id == SPANSION_128Mbit)
            	{
            		/* read extended device ID */
                    REG32(0xb801a800)=0x00000090;
                    REG32(0xb801a804)=0x0000001b;
                    id = *(unsigned int*)temp_ptr & 0xffff;
                
            		continue;
            	}

                *dev = (void *)&s_device[idx];
                return 0;
            }
        }
                
        if (idx == DEV_SIZE_S)
        {
        	*dev = (void *)0x0;
            return -1;
        }
    }

	*dev = (void *)0x0;
	return -1;
}
/************************************************************************
 *
 *                          do_init_s
 *  Description :
 *  -------------
 *  implement the init flash controller
 *
 *  Parameters :
 *  ------------
 *
 *  Return values :
 *  ---------------
 *
 ************************************************************************/
void do_init_s(void *dev)
{
	UINT8 tmp;
	UINT8 * temp_ptr = (UINT8 *)0xbec00000;
	
	/* configure serial flash controller */
    
    asm(".set mips3");
    asm("sync");
    asm(".set mips1");
	//disable w-merge and r-bypass
    REG32(0xb801a018)=((REG32(0xb801a018) | 0x8 ) & ~0x3);
    asm(".set mips3");
    asm("sync");
    asm(".set mips1");
    //enable command inorder
    REG32(0xb801a018)= (REG32(0xb801a018) | 0xc);
    asm(".set mips3");
    asm("sync");
    asm(".set mips1");
    
//    REG32(0xb801a808)=0x01010004;   //setup freq divided no
    REG32(0xb801a808)=0x0101000f;   //lowering frequency

    REG32(0xb801a80c)=0x00090101;   //setup control edge

    REG32(0xb801a810)=0x00000000;	//disable hardware potection
	
    /* enable write status register */        
    REG32(0xb801a800)=0x00000050;
    REG32(0xb801a804)=0x00000000;
    tmp = *temp_ptr;
    *temp_ptr = 0x00;
    
    /* write status register , no memory protection*/
    REG32(0xb801a800)=0x00000001; 
    REG32(0xb801a804)=0x00000010;
    *temp_ptr = 0x00;
}
/************************************************************************
 *
 *                          do_exit_s
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
void do_exit_s(void *dev)
{
	UINT8 tmp;
	UINT8 * temp_ptr = (UINT8 *)0xbec00000;
		
	REG32(0xb801a800)=0x00000003; //switch flash to read mode
    REG32(0xb801a804)=0x00000018; //command cycle
    
    tmp = *temp_ptr;
}
