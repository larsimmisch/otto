include Makefile.local

OPTIMIZE = -O2 -Os
DEFS = -I /usr/local/opt/avr-libc/avr/include -I ../arduino-- -DF_CPU=$(CPU_FREQUENCY)
LIBS = -B /usr/local/opt/avr-libc/avr/lib
CC = avr-gcc
CXX = avr-g++
AR = avr-ar
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump

VPATH=.

CFLAGS = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
CXXFLAGS = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
LDFLAGS = -Wl,-Map,$@.map $(LIBS)

BIN = heatchamber.bin

all: $(BIN) $(BIN:.bin=.lst)

fonts/sample.i: fonts/Sample.bmp
	python fonts/convert_sample.py -o $@ $<

fonts/fonts.c fonts/fonts.h: fonts/fonts.py
	cd fonts; python fonts.py -f fonts.c -i fonts.h

Noritake.h: fonts/fonts.h

heatchamber.cc: fonts/fonts.c

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
	rm -f *.o *.map *.lst *.elf *.bin fonts/fonts.c fonts/fonts.h .depend 

.bin_upload:
	avrdude -F -V -p $(MCU_TARGET) -c $(AVR_PROGRAMMER) -b $(AVR_RATE) -P $(AVR_TTY) -U flash:w:$<

-include .depend
