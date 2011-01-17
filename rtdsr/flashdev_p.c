#include "flashdev_p.h"
#include "util.h"


/************************************************************************
 *
 *                          get_sectorindex
 *  Description :
 *  -------------
 *  get the sector index of flash 
 *  
 *
 *  Parameters :
 *  ------------
 *
 *
 *
 *  Return values :
 *  ---------------
 *
 *  
 *
 *
 ************************************************************************/
UINT32 get_sector_index(p_device_type *device, unsigned int p_addr)
{
	int i;
	int addr = p_addr;
	

	addr = FLASH_ADDR_BASE-addr;
        
	/* if flash size exceed 16M bytes */
	if ( addr > 0xffffff )
		return (unsigned int)-1;
		
	for( i = device->total_sector - 1; i >= 0; i--)
	{      
		if(addr >= device->flash_sectors[i])
			break;
	}
	
	return (unsigned int) i;
}

/************************************************************************
 *
 *                          do_write_p
 *  Description :
 *  -------------
 *  implement the flash write 
 *  
 *
 *  Parameters :
 *  ------------
 *
 *
 *
 *  Return values :
 *  ---------------
 *
 *  
 *
 *
 ************************************************************************/
int do_write_p(void *dev, 
               unsigned char* array_ptr,
               unsigned char* dest,      
               unsigned int   rem_size,
               unsigned int   signature)
{
	unsigned char dw ;
	unsigned int i;
//	p_device_type *device = (p_device_type *)dev;
	
	while(rem_size --)
	{
		dw = REG8(array_ptr);
		
		/* issue 1st write cycle */
		REG8(((unsigned int)dest&0xffff0000)+0xf556)  = 0xaa;
		
        /* issue 2nd write cycle */
        REG8(((unsigned int)dest&0xffff0000)+0xfaa9)  = 0x55;
        
        /* issue 3rd write cycle */
        REG8(((unsigned int)dest&0xffff0000)+0xf556)  = 0xa0;
        
        /* issue 4th write cycle */
        REG8(dest) = dw ;
        
        i = 100000;
        while( (REG8(dest) != dw) && (i-- !=0)) ;
        
        /* Verify programmed byte */
        if ( REG8(dest) != dw )
        {
        	/* show '-' in console */
        	*( (volatile unsigned int *) 0xb801b200) = 0x2d;//cy test	   
            return -222;
        }
        
        if ( (rem_size%1024) == 1023)
        {
        	/* show '.' in console */
        	*( (volatile unsigned int *) 0xb801b200) = 0x2e;//cy test
        }
        
        dest++;
        array_ptr++;
    }
    
    return 0;
}

/************************************************************************
 *
 *                          do_erase_p
 *  Description :
 *  -------------
 *  implement the flash erase 
 *  
 *
 *  Parameters :
 *  ------------
 *
 *
 *
 *  Return values :
 *  ---------------
 *
 *  
 *
 *
 ************************************************************************/
int do_erase_p(void *dev,
               unsigned char* dest,
               unsigned int rem_size) 
{
	int index, sector_address_begin, sector_address_end;
    int i;
    p_device_type *device = (p_device_type *)dev;
	
	/* erase FLASH area */
    index = get_sector_index(device, (unsigned int)dest);
    
    sector_address_begin = device->flash_sectors[index];
    
    sector_address_end = device->flash_sectors[get_sector_index(device, (unsigned int)dest + rem_size - 1)] ;
    
    for (i = sector_address_begin; i >= sector_address_end && index >=0; i = device->flash_sectors[--index])
    { 
        /* issue 1st erase cycle */
        REG8(((unsigned int)dest&0xffff0000)+0xf556)  = 0xaa;
        
        /* issue 2nd erase cycle */
        REG8(((unsigned int)dest&0xffff0000)+0xfaa9)  = 0x55;
        
        /* issue 3rd erase cycle */
        REG8(((unsigned int)dest&0xffff0000)+0xf556)  = 0x80;
        
        /* issue 4th erase cycle */
        REG8(((unsigned int)dest&0xffff0000)+0xf556)  = 0xaa;
        
        /* issue 5th erase cycle */
        REG8(((unsigned int)dest&0xffff0000)+0xfaa9)  = 0x55;
        
        /* issue 6th erase cycle */
        REG8(FLASH_ADDR_BASE-i)  = 0x30;
        
        /* read any bytes in erased sector to ensure erasing is done */
        while(REG8(FLASH_ADDR_BASE-i)!=0xFF);

        /* show '/' in console */
        REG32(0xb801b200)= 0x2f ; //cy test
    }
    
    return 0;
}

/************************************************************************
 *
 *                          do_identify_p
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
int do_identify_p(void **dev)
{
	UINT32 idx;
    UINT8 mid, did0, did1, did2; //manufacture ID, Device ID
	UINT8 *temp_ptr = (UINT8 *)0xbec00000;
	
	/* setup ATA2/parallel flash mux to parallel flash */
	REG32(0xb80000d8) = 0x8;
    sync();
    
    /* read Manufacture & device ID, then reset to return read mode */
	REG8(((unsigned int)temp_ptr & 0xffff0000) + 0xf556) = 0xaa;
	REG8(((unsigned int)temp_ptr & 0xffff0000) + 0xfaa9) = 0x55;
	REG8(((unsigned int)temp_ptr & 0xffff0000) + 0xf556) = 0x90;
	
	mid  = REG8(((unsigned int)temp_ptr & 0xffff0000) + 0xfffc); //get value from addr 0
    /* Reset Command */
    REG8(temp_ptr) = 0xf0;

	REG8(((unsigned int)temp_ptr & 0xffff0000) + 0xf556) = 0xaa; //xaaa = aa
	REG8(((unsigned int)temp_ptr & 0xffff0000) + 0xfaa9) = 0x55; //x555 = 55
	REG8(((unsigned int)temp_ptr & 0xffff0000) + 0xf556) = 0x90; //xaaa = aa
	did0 = REG8(((unsigned int)temp_ptr & 0xffff0000) + 0xfffe); //get value from addr x02 
    did1 = REG8(((unsigned int)temp_ptr & 0xffff0000) + 0xffe0); //get value from addr x1c
    did2 = REG8(((unsigned int)temp_ptr & 0xffff0000) + 0xffe2); //get value from addr x1e
    
    /* Reset Command */
    REG8(temp_ptr) = 0xf0;

	for (idx = 0; idx < DEV_SIZE_P; idx++)
	{
		if ( p_device[idx].mid == mid )
		{
			/* show '9' in console */
			( *(volatile unsigned int *) (0xb801b200)) = 0x39;
			
			switch (p_device[idx].did_cycle)
            {
            	case 1 :
            		if ( (p_device[idx].did & 0xff) == did0 )
            		{
            			( *(volatile unsigned int *) (0xb801b200)) = 0x30;
            			goto device_found;
            		}
            		
            		break;
                case 2 :
                	if ( (( p_device[idx].did       & 0xff) == did0) && 
                	     (((p_device[idx].did >> 8) & 0xff) == did1) )
                	{
                		( *(volatile unsigned int *) (0xb801b200)) = 0x31;
                		goto device_found;
                	}
                	
                	break; 
                case 3 :
                	if ( (( p_device[idx].did       & 0xff) == did0) && 
                	     (((p_device[idx].did >> 8) & 0xff) == did1) &&
                	     (((p_device[idx].did >>16) & 0xff) == did2) )
                    {
                    	( *(volatile unsigned int *) (0xb801b200)) = 0x33;
                    	goto device_found;
                    }
                    
                    break;                          
                default :
                	break;
            }
        }  
	}
	
	if (idx == DEV_SIZE_P)
	{
		*dev = (void *)0x0;
		return -1;
	}
              
device_found :
	*dev = (void *)&p_device[idx];
	return 0;
	
}
/************************************************************************
 *
 *                          do_init_p
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
void do_init_p(void *dev)
{
	/* setup ATA2/parallel flash mux to parallel flash */
	REG32(0xb8000368) = 0x0;
}
/************************************************************************
 *
 *                          do_exit_p
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
void do_exit_p(void *dev)
{
	UINT8 * temp_ptr = (UINT8 *)0xbec00000;
	
	/* Reset Command */
    REG8(temp_ptr) = 0xf0;
}

