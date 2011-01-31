rtdsr: Serial Recovery utility for Realtek RTD1283/Mars SoCs

This program is meant to be used in the Serial Recovery console from the Realtek
RTD1283 SoC BootROM ("s/d/g/c>"). It provides the following features:
 - memory hexdump
 - memory block transfer, using Ymodem
 - flash read/write to/from memory

!!NOTE!!: Flash operations are only supported for single chip NAND flash


To compile the utility see the INSTALL.txt file

To run the utility:

1. Run a serial console emulator, with Ymodem support (minicom with ltrzsz on
   UNIX, hyperterminal on Windows). Set it to 115200 8N1.
2. Power on your unit while maintaining <space> pressed, until you get an
   "s/d/g/c>" prompt
3. Type 's', and use Ymodem to send in the relevant hw_setting bin file for your
   device. You can find sample hw_setting files, along with the .config they
   were generated from and the source to convert from .config to .bin in the
   hw_setting directory. Note that the mars.BGA.64x2.bin also appears to work on
   Mars BGA units with 2x128 MB of RAM.
   When the setting file transfer is complete, it should display "hello world!"
   and you should get the "s/d/g/c>" prompt back by pressing enter. If not, then
   you will need to use a different hw_setting file.
4. Type 'd' and transfer the 'rtdsr.bin' that you compiled
5. Type 'g' to run the executable.
6. For a list of commands from within the utility, type 'help'


The following example details how one would use rtdsr to reflash the bootloader
on the Xtreamer Pro. The 256 KB Yamon bootloader used by the Xtreamer is located
at flash location 0x140000 (flash blocks 0xA & 0xB). A backup copy of the boot-
loader is supposed to be available on the PC host as "bootloader.bin"

1. rtdsr> yr ; start the Y-Modem receive operation into RAM_BASE (0xA0500000)
2. In hyperterminal, chose Transfer->Send and select the 'bootloader.bin' file
3. rtdsr> fi ; display the flash info and populates the Block State Table:
   NAND Flash: Type:HY27UF082G2B, Size:256MB, PageSize:0x800, BlockSize:0x20000
4. rtdsr> m a0400000 ;(optional) check that blocks 0xA, 0xB are set to type 0x79
   (BLOCK_BOOTCODE). If not, you might want to use the 'wb' command to change
   the type before writing to flash. For the list of types, see flashdev_n.h
5. rtdsr> fw 140000 40000 ; write 0x4000 bytes from RAM_BASE to flash 0x140000
   About to write 0x40000 bytes from RAM:0xa0500000 to Flash offset:0x00140000
   If this is really what you want, press Y
   erasing block 0xa... success
   erasing block 0xb... success
   writing block 0xa............. // .......... success
   writing block 0xb............. // .......... success
   
   Wrote 0x40000 bytes (0x2 blocks) starting at offset 0x00140000 (block 0xa)
