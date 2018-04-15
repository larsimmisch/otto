/*
    Basic Pin setup:
          ------------
          |ARDUINO  B5|-> SCLK (N-2)
          |         B4|-> MISO (N-6)
          |         B3|-> MOSI (N-4)
          |         B2|-> G
          |         B1|-> RCK
          |         B0|-> /SS  (N-3)
          |         D7|->
          |         D6|-> QE2
I2C/SCL <-|a5       D5|-> QE1
 IR/SDA <-|a4       D4|-> QEB
 CH4   -> |a3       D3|->
 CH3   -> |a2       D2|-> MB (N-9)
 CH2   -> |a1       D1|->
 CH1   -> |a0       D0|->
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
#include <pushbutton.h>
#include "fonts/fonts.c"
#include "Noritake.h"

typedef _SPI<Pin::SPI_SCK, Pin::SPI_MISO, Pin::SPI_MOSI, Pin::B0> SPI_D;
typedef PushButton<Clock16, Pin::D5, 50> Button;

typedef _Noritake<SPI_D, 112> Display;
typedef _Quadrature<Pin::D6, Pin::D4, 0, 127> Quadrature;

const char d_cd[] PROGMEM = "CD\0";
const char d_phono[] PROGMEM = "Phono\0";
const char d_mp3[] PROGMEM = "AirPlay\0";
const char d_aux[] PROGMEM = "Aux\0";

ISR(TIMER0_OVF_vect)
{
    Quadrature::update();
    clock16_isr();
}

// All relays are controlled by two cascaded TPIC6B595 shift registers.
// The first shift register controls the volume control relays
// The second shift register controls input selection and auxilliary power
template<class RCK_, class G_>
class _CombiControl
{
public:

    enum Output {
        cd = 1,
        phono = 2,
        mp3 = 4,
        aux = 8,
        pwr = 64
    };

    static void init(byte vol = 0) {
        // initialize /G
        G_::modeOutput();
        G_::clear();

        // Initialize RCK
        RCK_::modeOutput();
        RCK_::set();

        state = ((pwr | cd) << 8);
        volume(vol, 0);
        transfer(state);
    }

    static void set(byte b, bool on) {
        byte v = 1 << b;

        if (on)
            state |= v;
        else
            state &= ~v;

        transfer(state);
    }

    static void volume(byte v, byte last) {

        byte diff = v & last;
        byte b;
        char i;

        if (v && !last) {
            // unmute
            set(7, true);
        }
        else if (!v && last) {
            set(7, false);
        }

        // set the attenuated stages first (0 bits)
        // most significant to least
        for (i = 6; i >= 0; i--) {
            b = 1 << i;
            // set attenuated - 0 - bits
            if ((last & b) != (diff & b )) {
                set(i, false);
            }

        }
        // now set the pass through stages (1 bits)
        // least significant to most
        for (i = 0; i <= 6; i++) {
            b = 1 << i;
            // set non-attenuated - 1 - bits
            if (((v & b) == b) && ((diff & b) != b)) {
                set(i, true);
            }
        }
    }

    static const char * PROGMEM outputName() {
        switch (output()) {
        case cd:
            return d_cd;
        case phono:
            return d_phono;
        case mp3:
            return d_mp3;
        default:
            break;
        }

        return d_aux;
    }

    static byte output() {
        return (state >> 8) & 0x0f;
    }

    static const char * PROGMEM nextOutput() {

        byte o = output() << 1;
        if (o > aux)
            o = cd;

        state &= 0xf0ff;
        state |= o << 8;

        transfer(state);

        return outputName();
    }

    static void on() {
        state |= (pwr << 8);

        transfer(state);
    }

    static void off() {
        state &= ~(pwr << 8);

        // all relays off
        transfer((uint16_t)0);
    }

    static void transfer(uint16_t value) {
        SPI::transfer(value);
        RCK_::clear();
        RCK_::set();
    }

protected:
    static uint16_t state;
};

template<class RCK_, class G_>
uint16_t _CombiControl<RCK_, G_>::state = 0;

typedef _CombiControl<Pin::D2, Pin::D3> CombiControl;

int main(void)
{
    byte volume = 60;
    bool on = true;

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

    CombiControl::init(volume);

    Quadrature::position(volume);
    Display::volume(volume);
    Button::init();

    // char buf[4] = { 0 };
    // itoa(CombiControl::output(), buf, 10);
    // Display::string(0, 0, buf, glyphs_medium);

    Display::string_P(0, 0, CombiControl::outputName(), glyphs_medium);

    for (;;) {

        if (Button::read() == Button::keyup) {

            if (!on) {
                on = true;
                Display::on(false);
                CombiControl::on();
            }
            else if (Button::duration() > 400) {
                on = false;
                Display::off();
                CombiControl::off();
            }
            else {
                CombiControl::nextOutput();

                // clear the first line
                Display::area(0, 0, Display::width - 1, 9, 'C');
                Display::string_P(0, 0, CombiControl::outputName(),
                                  glyphs_medium);
            }
        }

        Clock16::sleep(50);
        if (on) {
            byte nv = Quadrature::position();
            if (volume != nv) {
                Display::volume(nv);
                CombiControl::volume(nv, volume);
                volume = nv;
            }
        }
    }

    return 0;
}
