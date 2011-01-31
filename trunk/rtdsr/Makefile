XCC = sde-gcc
OBJDUMP= sde-objdump
OBJCOPY= sde-objcopy

CFLAGS        = -mno-gpopt -gdwarf-2 -O0 -G0 -Wa,-W -nostdlib -I include
CXXFLAGS      = $(CFLAGS) -Wa,--gdwarf2

EXTRACFLAGS   = -Wall -march=mips32r2 -EL
EXTRACXXFLAGS = $(EXTRACFLAGS) -fno-exceptions 

LDFLAGS       = -nostartfiles -L/lib -I/include
LIBS          = -Tboot.ld 
LD            = $(XCC)
XCXX          = $(XCC)

OBJECTS       = ram_boot.o dvrmain.o util.o printf.o ymodem.o command.o commands.o flashdev_n.o
TARGET	      = rtdsr
MEMORY_LAYOUT = xMemLayout.map

###### RULES
.PHONY: all clean

all: clean $(TARGET).elf $(TARGET).bin

clean:
	-rm -f *.o *.dis $(TARGET).elf $(TARGET).bin $(MEMORY_LAYOUT)

%.o: %.c
	$(XCC) -c -o $*.o $(CXXFLAGS) $(EXTRACFLAGS) $<

%.o: %.cxx
	$(XCXX) -c -o $*.o $(CXXFLAGS) $(EXTRACXXFLAGS) $<

%.o: %.C
	$(XCXX) -c -o $*.o $(CXXFLAGS) $(EXTRACXXFLAGS) $<

%.o: %.cc
	$(XCXX) -c -o $*.o $(CXXFLAGS) $(EXTRACXXFLAGS) $<

%.o: %.cpp
	$(XCXX) -c -o $*.o $(CXXFLAGS) $(EXTRACXXFLAGS) $<
	
%.o: %.s %.h
	$(XCXX) -c -o $*.o $(CXXFLAGS) $(EXTRACXXFLAGS) $<

%.o: %.S
	$(XCXX) -c -o $*.o $(CXXFLAGS) $(EXTRACXXFLAGS) $<

$(TARGET).elf: $(OBJECTS) Makefile
	touch $(MEMORY_LAYOUT)
	$(LD) $(LDFLAGS) -e _ram_boot -o $@ $(OBJECTS) \
	$(LIBS) -EL -Wl,-Map $(MEMORY_LAYOUT)	
		
$(TARGET).bin: $(TARGET).elf
	# convert to binary executable
	$(OBJCOPY) -O binary $< $@
