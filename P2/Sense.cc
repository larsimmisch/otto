#include "arduino--.h"
#include "max7219.h"
#include "clock16.h"
#include "ringbuffer.h"
#include "eventwait.h"

const static uint16_t CYCLES = 64;
// We use 8 bits ADC resolution, but zero is at v/2
const static uint16_t ENERGY_SCALE = ((1 << 7) * CYCLES / 256);

struct ADCEvent {
    byte channel;
    byte energy;
};

typedef class _Max7219<Pin::B2, Pin::B0, Pin::B1> Max7219;

typedef _Ringbuffer<ADCEvent, 4> ADCbuffer;

_EventWait<Clock16, ADCbuffer> ADCResults;

ISR(ADC_vect)
{
    static byte count = 0;
    static int accu = 0;

    int8_t v = ADCMux::analogLeftAdjusted() - 128;
    accu += v < 0 ? -v : v;
    ++count;

    if (count == CYCLES) {

        ADCEvent ev;
        byte tmp = ADMUX;

        ev.channel = tmp & 0x0f;
        ev.energy = accu / ENERGY_SCALE;
        accu = 0;

        // ADMUX = (tmp & ~0x0f) | ((ev.channel + 1) % 4);

        ADCResults.push(ev);
    }
}

int main(void)
{
    Arduino::init();
    Max7219::init();

    // At our clock frequency of 16 MHz, this runs the ADC clock at 500kHz and 
    // we get a sampling frequency of 38kHz
    // At this ADC clock rate, we don't get the full resolution, but we don't 
    // care
    ADCMux::prescaler32();
    ADCMux::enableInterrupt();
    Pin::ADC0::analogStart(Pin::ADC0::LEFT_ADJUST);

    for(;;) {
        ADCResults.wait_event();

        if (!ADCResults.empty()) {

            ADCEvent ev(ADCResults.front());
            ADCResults.pop();

            Max7219::set(Max7219::ROW0 + ev.channel, ev.energy);
            // Max7219::set(Max7219::ROW0 + i * 2, bar(ev.energy[i]));
            // Max7219::set(Max7219::ROW1 + i * 2, bar(ev.energy[i]));
        }
    }

    return 0;
}
