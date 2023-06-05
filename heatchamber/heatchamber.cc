#include <arduino--.h>
#include <spi.h>
#include <onewire.h>
#define CLOCK_NO_ISR
#include <clock16.h>
#include <quadrature.h>
#include <pushbutton.h>
#include <pid.h>
#include <limits.h>
#include "fonts/fonts.c"
#include "Noritake.h"

const int QUAD_SCALE = 10;
const float HYSTERESIS = 0.1;

typedef _SPI<Pin::SPI_SCK, Pin::SPI_MISO, Pin::SPI_MOSI, Pin::SPI_SS> SPI_D;

typedef _Noritake<SPI_D, 140> Display;
// values from the quad encoder are scaled with a factor of QUAD_SCALE
typedef _Quadrature<Pin::D7, Pin::C3, 20 * QUAD_SCALE, 80 * QUAD_SCALE> Quadrature;

static Buttons<Pin::D4> OneWireButtons;

typedef Pin::D5 Relay;
// typedef Pin::C1 DimmerSense;

float convert_ds18b20(uint16_t t)
{
	float v = (t & 0xf) / 16.0 + ((t & 0x7f0) >> 4);

	if (t & 0xf000) {
		return -v;
	}

	return v;
}

void display_temp(float value, byte x, byte y, const Glyph *font)
{
	char buf[6] = { 0 };

	if (value >= 100.0 || value <= -10.0) {
		Display::string(x, y, "--.-", font);
		return;
	}

	if (value >= 0.0 && value < 10.0) {
		snprintf(buf, sizeof(buf), " %.1f\xb0", value);
	}

	snprintf(buf, sizeof(buf), "%.1f\xb0", value);

	Display::string(x, y, buf, font);
}

int main(void)
{
	Arduino::init();
	Quadrature::init();

    // disable a2d
    _SFR_BYTE(ADCSRA) &= ~_BV(ADEN);

	OneWireButtons.Init();
	OneWireButtons.Scan();

	// uint16_t starttime = Clock16::millis();

	Relay::modeOutput();
	Relay::set();

	Timer2::prescaler1();
	Timer2::modeNormal();
	Timer2::enableOverflowInterrupt();
	// Use internal clock for timer2
	ASSR |= (1<<AS2);

	SPI_D::init(1<<SPR0, true);

	Clock16::sleep(250);
	Display::clear();
	Display::on(false);

	Quadrature::position(29 * QUAD_SCALE);

	float targetTemp = Quadrature::position() / (float)QUAD_SCALE;

	display_temp(targetTemp, 0, 0, glyphs_huge);

	OneWireButtons.GetTemperatures();

	int selected = Quadrature::position();
	uint16_t temp = OneWireButtons[0].Temperature();

	float currentTemp = convert_ds18b20(temp);

	bool relayOn = targetTemp > currentTemp;

	// the Relay will be on if the Relay Pin is driven low
	relayOn ? Relay::clear() : Relay::set();

	display_temp(currentTemp, 66, 8, glyphs_medium);

	for (;;) {

		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_enable();
		sleep_mode();

		if (selected != Quadrature::position())
		{
			selected = Quadrature::position();

			targetTemp = selected / (float)QUAD_SCALE;

			display_temp(targetTemp, 0, 0, glyphs_huge);
		}

		OneWireButtons.GetTemperatures();

		if (temp != OneWireButtons[0].Temperature())
		{
			temp = OneWireButtons[0].Temperature();
			currentTemp = convert_ds18b20(temp);
			display_temp(currentTemp, 66, 8, glyphs_medium);
		}

		if (relayOn) {
			if (targetTemp - HYSTERESIS <= currentTemp) {
				relayOn = false;
			}
		}
		else {
			relayOn = targetTemp > currentTemp;
		}

		// the Relay will be on if the Relay Pin is driven low
		relayOn ? Relay::clear() : Relay::set();
	}

	return 0;
}
