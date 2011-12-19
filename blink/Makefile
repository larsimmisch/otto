ARDUINO-- = ../arduino--
MCU_TARGET = atmega328p
OPTIMIZE = -O3 -Os
DEFS = -I /usr/local/avr/avr/include -I $(ARDUINO--) -DF_CPU=16000000
LIBS = -B /usr/local/avr/avr/lib -L$(ARDUINO--) -larduino++
CC = avr-gcc
CXX = avr-g++
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump

CFLAGS = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
CXXFLAGS = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
LDFLAGS = -Wl,-Map,$@.map $(LIBS)

all: blink.bin blink.lst $(OBJ)

.SUFFIXES: .lst .elf .bin

.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $(<:.cpp=.o) $<

.c.o:
	$(CC) $(CFLAGS) -c -o $(<:.c=.o) $<

.elf.bin:
	$(OBJCOPY) -j .text -j .data -O binary $< $@

.elf.lst:
	$(OBJDUMP) -h -S $< > $@

.o.elf:
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Automatic dependencies
%.d: %.c
	$(CC) -M $(ALL_CFLAGS) $< | sed "s;$(notdir $*).o:;$*.o $*.d:;" > $@

%.d: %.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< | sed "s;$(notdir $*).o:;$*.o $*.d:;" > $@

%.d: %.cc
	$(CXX) -M $(ALL_CXXFLAGS) $< | sed "s;$(notdir $*).o:;$*.o $*.d:;" > $@

clean:
	rm -f *.o *.map *.lst *.elf *.bin $(CXXSRC:.cc=.d) $(SRC:.c=.d)

-include $(SRC:.c=.d)
-include $(CXXSRC:.cc=.d)