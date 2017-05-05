#include <arduino--.h>
#include <spi.h>
#define CLOCK_NO_ISR
#include <clock16.h>
#include <quadrature.h>
#include <pushbutton.h>
#include <limits.h>
#include "fonts/fonts.c"
#include "Noritake.h"

#define CALIBRATION_CYCLES 20
#if (CALIBRATION_CYCLES > UINT_MAX/500)
#error Frequency computation will overflow.
#endif

typedef _SPI<Pin::SPI_SCK, Pin::SPI_MISO, Pin::SPI_MOSI, Pin::SPI_SS> SPI_D;

typedef _Noritake<SPI_D, 112> Display;
// values from the quad encoder are scaled with a factor of 2
typedef _Quadrature<Pin::D7, Pin::C3, 20 * 2, 80 * 2> Quadrature;

typedef Pin::C0 DimmerOut;
typedef Pin::D4 DimmerSense;

enum Sweep {
    idle,
    up,
    down
};

volatile Sweep sweep = idle;
volatile unsigned int ac_crossings = 0;
volatile unsigned int t2 = 0;
volatile unsigned int t2_max = 0;
volatile bool ac_zero = false;
volatile unsigned int trigger = 0;

ISR(TIMER0_OVF_vect)
{
    Quadrature::update();
    clock16_isr();
}

// Called 62500 times a second: 16000000 Hz / 256 = 62500 Hz
ISR(TIMER2_OVF_vect)
{
	if (ac_zero) {
		return;
    }

	++t2;
	if (t2 > t2_max) {
		t2_max = t2;
    }

	// t2 == 0 is handled in ac_change
	if (t2 == trigger) {
        DimmerOut::set();
    }
}

ISR(PCINT2_vect)
{
	ac_zero = DimmerSense::read() != 0;
	if (ac_zero) // zero crossing rising edge
	{
        DimmerOut::clear();
		t2 = 0;
		++ac_crossings;

		// Sweep
		if ((ac_crossings % 2) == 0)
		{
			switch (sweep)
			{
			case up:
				++trigger;
				if (trigger >= t2_max)
				{
					sweep = down;
					trigger = t2_max - 1;
				}
				break;
			case down:
				--trigger;
				if (trigger <= 0)
				{
					sweep = up;
					trigger = 0;
				}
				break;
			default:
				break;
			}
		}
	}
	else
	{
		// reset timer2 to avoid jitter
        Timer2::reset();
		if ((trigger == 0) && (sweep != idle)) {
            DimmerOut::set();
        }
	}
}

void display_temp_scaled(byte value, byte x, byte y, const Glyph *font)
{
    char buf[5] = { 0 };

    if (value >= 200) {
        Display::string(x, y, (const byte*)"--.-", font);
        return;
    }

    itoa(value / 2, buf, 10);

    if (value % 2) {
        strcat(buf, ".5");
    }
    else {
        strcat(buf, ".0");
    }

    strcat(buf, "\xb0");
    Display::string(x, y, (byte*)buf, font);
}

int main(void)
{
    bool is_calibrated = false;

    Arduino::init();
    Quadrature::init();

    // uint16_t starttime = Clock16::millis();

    DimmerOut::modeOutput();
    DimmerOut::clear();
    DimmerSense::modeInput();
    DimmerSense::enableChangeInterrupt();

    Timer2::prescaler1();
    Timer2::modeNormal();
    Timer2::enableOverflowInterrupt();
    // Use internal clock for timer2
	ASSR |= (1<<AS2);

    SPI_D::init(1<<SPR0, true);

    Clock16::sleep(250);
    Display::clear();
    Display::on(false);

    Quadrature::position(29 * 2);

    display_temp_scaled(Quadrature::position(), 0, 0, glyphs_huge);

    display_temp_scaled(23 * 2, 66, 8, glyphs_medium);

    int temperature = Quadrature::position();

    for (;;) {

		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_enable();
		sleep_mode();

        if (temperature != Quadrature::position())
        {
            temperature = Quadrature::position();

            display_temp_scaled(temperature, 0, 0, glyphs_huge);
        }

		if (!is_calibrated && ac_crossings >= CALIBRATION_CYCLES)
		{
            trigger = t2_max;
            sweep = down;

#ifdef DEBUG
           	uint16_t = Clock16::millis() - starttime;

            Serial.print("t2: ");
            Serial.println(t2);

            Serial.print("t2 max: ");
            Serial.println(t2_max);

            Serial.print(ac_crossings*500/t);
            Serial.println(" Hz");

            Serial.print("A/C crossings: ");
            Serial.print(ac_crossings);
            Serial.print(" ms: ");
            Serial.println(t);
#endif
		}

    }

    return 0;
}
