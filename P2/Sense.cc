#include "arduino--.h"
#include "max7219.h"
#include "clock16.h"
#include "ringbuffer.h"
#include "eventwait.h"

const static byte CYCLES = 64;
// The ADC has 10 bits resolution
const static uint16_t MAX_ENERGY = 1 << 10;

typedef struct {
    byte channel;
    int energy;
} ADCEvent;

typedef class _Max7219<Arduino::D10, Arduino::D8, Arduino::D9> Max7219;

typedef _Ringbuffer<ADCEvent, 4> ADCbuffer;

_EventWait<Clock16, ADCbuffer> ADCResults;

ISR(ADC_vect)
{
    static byte index = 0;
    static byte count = 0;
    static long accu[4];

    int v = 0;

    switch (index) {
    case 0:
        v = Pin::ADC0::analogValue();
        Pin::ADC1::analogActivate();
        break;
    case 1:
        v = Pin::ADC1::analogValue();
        Pin::ADC2::analogActivate();
        break;
    case 2:
        v = Pin::ADC2::analogValue();
        Pin::ADC3::analogActivate();
        break;
    case 3:
        v = Pin::ADC3::analogValue();
        Pin::ADC0::analogActivate();
        break;
    }
    
    accu[index] += v < 0 ? -v : v;
    ++count;

    if (count == CYCLES) {

        ADCEvent ev;

        ev.channel = index;
        ev.energy = accu[index] / CYCLES;
        
        ADCResults.push(ev);
        
        accu[index] = 0;
    }
    index = (index + 1) % 4;
}

byte bar(int energy)
{
    byte max_bit = energy / (MAX_ENERGY / 8);

    return (1 << max_bit) - 1;
}

int main(void)
{
    Arduino::init();
    Max7219::init();

    for(;;) {
        ADCResults.wait_event();

        if (!ADCResults.empty()) {

            ADCEvent ev(ADCResults.front());
            ADCResults.pop();

            Max7219::set(Max7219::ROW0 + ev.channel, bar(ev.energy));
            Max7219::set(Max7219::ROW1 + ev.channel, bar(ev.energy));
        }
    }

    return 0;
}
