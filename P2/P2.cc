/*
    Basic Pin setup:
          ------------                      
          | ARDUINO 13|-> SCLK (N-2)
          |         12|-> MISO (N-6)
          |         11|-> MOSI (N-4)
          |         10|-> 
          |          9|-> 
          |          8|-> /SS  (N-3)
          |          7|-> 
          |          6|-> QE2
I2C/SCL <-|a5        5|-> QE1
 IR/SDA <-|a4        4|-> QEB
 CH4   -> |a3        3|-> 
 CH3   -> |a2        2|-> MB (N-9)
 CH2   -> |a1        1|->
 CH1   -> |a0        0|->
          -------------


    Pinout of the volume control is:

	6 5 + 4 3 (1 - red)
    7 8 + 1 2
*/

#include <arduino--.h>
#include <spi.h>
#define CLOCK_NO_ISR
#include <clock16.h>
#include <quadrature.h>
#include "fonts/fonts.c"
#include "Noritake.h"

typedef _SPI<Pin::SPI_SCK, Pin::SPI_MISO, Pin::SPI_MOSI, Pin::B0> SPI_D;

typedef _Noritake<SPI_D, 112> Display;
typedef _Quadrature<Pin::D5, Pin::D6, 0, 127> Quadrature;

const char phono[] PROGMEM = "Phono\0";

ISR(TIMER0_OVF_vect)
{
    Quadrature::update();
    clock16_isr();
}

int main(void)
{
    byte volume = 60;

    Arduino::init();
    Quadrature::init();

    // The SS Pin needs to be an output (or held high), otherwise
    // the SPI will switch to slave mode when SS goes low.
    // We need to do this here because we use a nonstandard SS pin
    Pin::SPI_SS::modeOutput();

    SPI_D::init(1<<SPR0, true);

    Clock16::sleep(250);
    Display::clear();
    Display::on(false);

    Quadrature::position(volume);
    Display::volume(volume);

    Display::string_P(0, 0, phono, glyphs_medium);

    for (;;) {
        Clock16::sleep(50);
        byte nv = Quadrature::position();
        if (volume != nv) {
            volume = nv;
            Display::volume(volume);
        }
    }

    return 0;
}
