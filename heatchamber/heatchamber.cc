#include <avr/eeprom.h>
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

const float DEFAULT_TEMP = 27.0;
const int QUAD_SCALE = 10;
const float HYSTERESIS = 0.2;
const int EEPROM_MIN_WAIT = 1000;

float EEMEM DefaultTemp = DEFAULT_TEMP;

typedef _SPI<Pin::SPI_SCK, Pin::SPI_MISO, Pin::SPI_MOSI, Pin::SPI_SS> SPI_D;

typedef _Noritake<SPI_D, 140> Display;
// values from the quad encoder are scaled with a factor of QUAD_SCALE
typedef _Quadrature<Pin::D7, Pin::C3, 20 * QUAD_SCALE, 80 * QUAD_SCALE> Quadrature;

static Buttons<Pin::D4> OneWireButtons;

typedef Pin::C1 Relay;

ISR(TIMER0_OVF_vect)
{
    Quadrature::update();
    clock16_isr();
}

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

	Quadrature::initChangeInterrupt();

	Relay::modeOutput();
	Relay::set();

    // disable a2d
    _SFR_BYTE(ADCSRA) &= ~_BV(ADEN);

	SPI_D::init(1<<SPR0, true);

	// The OneWire initialisation takes some time that the Noritake display needs for setup (250 ms)
	// It works in practice, I didn't measure it.

	OneWireButtons.Init();
	OneWireButtons.Scan();

	OneWireButtons.GetTemperatures();

	Display::clear();
	Display::on(false);

	float targetTemp = eeprom_read_float(&DefaultTemp);

	if (isnan(targetTemp)) {
		// uninitialized EEPROM will read as 0xffffffff, which is NAN
		targetTemp = DEFAULT_TEMP;
		eeprom_write_float(&DefaultTemp, targetTemp);
	}

	Quadrature::position((int)(targetTemp * QUAD_SCALE));

	display_temp(targetTemp, 0, 0, glyphs_huge);

	int selected = Quadrature::position();
	uint16_t temp = OneWireButtons[0].Temperature();

	float currentTemp = convert_ds18b20(temp);

	bool relayOn = targetTemp > currentTemp;
	bool cooling = !relayOn;

	// the Relay will be on if the Relay Pin is driven low
	relayOn ? Relay::clear() : Relay::set();

	display_temp(currentTemp, 66, 8, glyphs_medium);

	bool updatePending = false;
	typename Clock16::time_res_t updateWaitStart = 0;
	int updateClearCounter = 0;
	const char* updateIndicator = "\x02";
	byte updateIndicatorWidth = Display::glyph_width(updateIndicator, glyphs_medium);

	for (;;) {

		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_enable();
		sleep_mode();

		if (selected != Quadrature::position())
		{
			selected = Quadrature::position();

			targetTemp = selected / (float)QUAD_SCALE;

			display_temp(targetTemp, 0, 0, glyphs_huge);

			updatePending = true;
			// update the wait start timer even if it is active already,
			// so we wait the proper period after the last value change
			updateWaitStart = Clock16::millis();
		}

		// EEPROM update & write indicator

		if (updateClearCounter) {
			// clear the update indicator
			--updateClearCounter;
			if (!updateClearCounter) {
				Display::area(130, 5, 130 + updateIndicatorWidth, 16, 'C'); // 'C' is clear
			}
		}

		if (updatePending && (Clock16::millis() - updateWaitStart) >= EEPROM_MIN_WAIT) {
			updatePending = false;
			// show arrow for update
			Display::string(130, 5, updateIndicator, glyphs_medium);
			eeprom_update_float(&DefaultTemp, targetTemp);
			updateClearCounter = 2;
		}

		OneWireButtons.GetTemperatures();

		if (temp != OneWireButtons[0].Temperature())
		{
			temp = OneWireButtons[0].Temperature();
			currentTemp = convert_ds18b20(temp);
			display_temp(currentTemp, 66, 8, glyphs_medium);
		}

		if (relayOn) {
			if (targetTemp <= currentTemp) {
				relayOn = false;
				cooling = true;
			}
		}
		else {
			if (cooling) {
				relayOn = targetTemp - HYSTERESIS > currentTemp;
				if (relayOn) {
					cooling = false;
				}
			}
			else {
				relayOn = targetTemp > currentTemp;
			}
		}

		// the Relay will be on if the Relay Pin is driven low
		relayOn ? Relay::clear() : Relay::set();
	}

	return 0;
}
