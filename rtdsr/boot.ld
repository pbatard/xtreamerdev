MEMORY
{
	ram : org = 0x80100000, len = 0x200000   /* 2MB */
	rom : org = 0xbfc00000, len = 0x200000
}
SECTIONS
{
		
	.text 0x80100000 :  
	{ 
		_ftext = .;
		ram_boot.o(.text) 
		dvrmain.o(.text)
		util.o(.text)
		printf.o(.text)
		ymodem.o(.text)		
/*		flashdev_n.o(.text)
		flashdev_p.o(.text)
		flashdev_s.o(.text)
*/		etext = .;
	}
	
	.data	      ALIGN(0x4) : { *(.data) *(.sdata) *(.lit4) *(.lit8) }			   
	.bss	      ALIGN(0x4) : { 
				    . = ALIGN(4);
				    __bss_start = .; 
				    *(.sbss) *(.bss) *(.scommon)
				    . = ALIGN(4);
				    __bss_end = .; 
				   } /* 64KB */
	.rodata	      ALIGN(0x4) : { *(.rodata) }
}


