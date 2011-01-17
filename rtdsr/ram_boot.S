#include "mips.inc"
#include "eregdef.h"

	.bss
	.globl	_my_base
_my_base:	
	.rept 10000
	.byte 0
	.endr

	.globl	_stack_base
_stack_base:
	.rept 4096
	.byte 0
	.endr

	.balign 16
	.globl	_stack 
_stack:
	.rept 8
	.long 0
	.endr
			
    .set noreorder
    
    .text
	.ent	_ram_boot
	.globl	_ram_boot
	.extern	dvrmain
	
_ram_boot:
#define RA		t3
        
        move    RA, ra   	    
	mfc0	s0, C0_SR
	nop
	or	s0, s0, SR_IMASK
	and	s0, s0, ~SR_BEV
	#or	s0, s0, 0x30000000
	mtc0	s0, C0_SR
	nop

	la	sp, _stack
	la	s0, dvrmain
	jalr	s0
	nop
	
	move	ra, RA
	jr 	ra
	nop
		
.end	_ram_boot

