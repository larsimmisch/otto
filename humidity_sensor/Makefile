include Makefile.local

OPTIMIZE = -O2 # -Os
DEFS = -I /usr/local/opt/avr-libc/avr/include -I ../arduino-- -DF_CPU=$(CPU_FREQUENCY)
LIBS = -B /usr/local/opt/avr-libc/avr/lib -lm
CC = avr-gcc
CXX = avr-g++
AR = avr-ar
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump

VPATH=.

CFLAGS = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
CXXFLAGS = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
LDFLAGS = -Wl,-Map,$@.map $(LIBS) -Wl,-u,vfprintf -lprintf_flt

BIN = humidity_sensor.bin

all: $(BIN) $(BIN:.bin=.lst)

.depend: *.cc *.h 
	$(CC) $(DEFS) -mmcu=$(MCU_TARGET) -MM *.cc > .depend

.SUFFIXES: .elf .lst .bin _upload

.cc.o: 
	$(CXX) $(CXXFLAGS) -c -o $(<:.cc=.o) $<

.c.o:
	$(CC) $(CFLAGS) -c -o $(<:.c=.o) $<

.elf.bin:
	$(OBJCOPY) -j .text -j .data -O binary $< $@

.elf.lst:
	$(OBJDUMP) -h -S $< > $@

.o.elf:
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean: 
	rm -f *.o *.map *.lst *.elf *.bin .depend 

.bin_upload:
	avrdude -F -V -p $(MCU_TARGET) -c $(AVR_PROGRAMMER) -B $(USBTINY_SPEED) -U flash:w:$<

-include .depend
