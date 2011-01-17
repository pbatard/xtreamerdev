/*
#define	zero	$0
#define	AT	$1
#define	v0	$2
#define	t0	$8
#define	k0	$26

#define C0_EPC	14
*/

/*  Workaround for bug in early revisions of MIPS 4K family of 
 *  processors.
 *
 *  This concerns the nop instruction before mtc0 in the 
 *  MTC0 macro below.
 *
 *  The bug is described in :
 *
 *  MIPS32 4K(tm) Processor Core Family RTL Errata Sheet
 *  MIPS Document No: MD00003
 *
 *  The bug is identified as : C27
 */

#define MTC0(src, dst)       \
		nop;	     \
	        mtc0 src,dst;\
		NOPS

#define DMTC0(src, dst)       \
		nop;	      \
	        dmtc0 src,dst;\
		NOPS

#define MFC0(dst, src)       \
	  	mfc0 dst,src

#define DMFC0(dst, src)       \
	  	dmfc0 dst,src

#define MFC0_SEL_OPCODE(dst, src, sel)\
	  	.##word (0x40000000 | ((dst)<<16) | ((src)<<11) | (sel))

#define MTC0_SEL_OPCODE(src, dst, sel)\
	  	.##word (0x40800000 | ((src)<<16) | ((dst)<<11) | (sel));\
		NOPS

#define LDC1(dst, src, offs)\
		.##word (0xd4000000 | ((src)<<21) | ((dst)<<16) | (offs))

#define SDC1(src, dst, offs)\
		.##word (0xf4000000 | ((dst)<<21) | ((src)<<16) | (offs))

/* Release 2 */
#define RDPGPR( rd, rt )\
		.##word (0x41400000 | ((rd) <<11) | (rt<<16))

#define WRPGPR( rd, rt )\
		.##word (0x41c00000 | ((rd) <<11) | (rt<<16))
	
