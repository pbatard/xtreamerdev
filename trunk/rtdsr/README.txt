rtdsr: Serial Recovery utility for Realtek RTD1283/Mars SoCs

This program is meant to be used in the Serial Recovery console from the Realtek
RTD1283 SoC BootROM ("s/d/g/c>"). It aims at providing the following features:
 - memory hexdump
 - memory block transfer, using Ymodem
 - flash read/write to/from memory

To compile the utility see the INSTALL.txt file

To run the utility:

1. Run a serial console emulator, with Ymodem support (minicom with ltrzsz on
   UNIX, hyperterminal on Windows). Set it to 115200 8N1.
2. Power on your unit while maintaining the Space key pressed, until you get an
   "s/d/g/c>" prompt
3. Type 's', and use Ymodem to send in the relevant hw_setting bin file for your
   device. You can find sample hw_setting files, along with the .config they
   were generated from and the source to convert from .config to .bin in the
   hw_setting directory. Note that the mars.BGA.64x2.bin also appears to work on
   Mars BGA units with 2x128 MB of flash.
   When the setting file transfer is complete, it should display "hello world!"
   and you should get the "s/d/g/c>" prompt back by pressing enter. If not, then
   you will need to use a different hw_setting file.
4. Type 'd' and transfer the 'rtdsr.bin' that you compiled
5. Type 'g' to run the executable.