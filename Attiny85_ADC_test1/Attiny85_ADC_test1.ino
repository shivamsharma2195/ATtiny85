#include <EEPROM.h>

#define CAPPOS PB1                            // Digital pin 9 for CapPos.
#define CAPNEG PB4                            // Digital pin 12 for CapNeg.
#define ECPIN PB3                             // Digital pin 11 for EC probe

//#define NTC_ADC 2                             // The ADC that's linked to the CAPNEG pin.
#define CHARGEDELAY 30                        // Time in microseconds it takes for the cap to charge; at least 5x RC.
#define EC_TIMEOUT 2000                       // Timeout for the EC measurement in microseconds.

// Time scale to go from clock pulses to microseconds.
// 1 MHz clock: 1 = 2^0 pulses per microsecond, TIMESCALE 0.
// 8 MHz clock: 8 = 2^3 pulses per microsecond, TIMESCALE 3.
// 16 MHz clock: 16 = 2^4 pulses per microsecond, TIMESCALE 4.
#if (F_CPU == 1000000)
#define TIMESCALE 0
#elif (F_CPU == 8000000)
#define TIMESCALE 3
#elif (F_CPU == 16000000)
#define TIMESCALE 4
#else
#error Unknown CPU clock speed, TIMESCALE undefined.
#endif

volatile uint32_t dischargeCycles;

void setup() {
  TCCR1A = 0;                                 //  clear control register A

// ATtiny:
//  TCCR1 = 0;                                  //  clear timer control register
//  TCCR1 |= (1 << CS10);                       //  Set to no prescaler

// ATmega:
  TCCR1B = 0;                                 //  clear timer control register
  TCCR1B |= (1 << CS10);                      //  Set to no prescaler

  // Enable pin change interrupts.
  // ATtiny:
//  GIMSK |= (1 << PCIE);

  // ATmega328 (Arduino):
  PCICR |= (1 << PCIE0);                      // set PCIE0 to enable PCMSK0 scan

  Serial.begin(115200);
}

void loop() {

  readEC();
  Serial.println();
  delay(2000);
 
}

/*
   Take a single reading from the EC probe.
*/
void readEC() {

  uint32_t totalCycles = 0;
  uint8_t timeouts = 0;
  for (uint8_t i = 0; i < 64; i++) {

    // Stage 1: charge the cap, positive cycle.
    DDRB |= (1 << CAPPOS);                        // CAPPOS output.
    DDRB |= (1 << CAPNEG);                        // CAPNEG output.
    DDRB &= ~(1 << ECPIN);                        // ECPIN input.
    PORTB |= (1 << CAPPOS);                       // CAPPOS HIGH: Charge the cap.
    PORTB &= ~(1 << CAPNEG);                      // CAPNEG LOW.
    PORTB &= ~(1 << ECPIN);                       // ECPIN pull up resistor off.
    delayMicroseconds(CHARGEDELAY);               // wait for cap to charge.

    // Stage 2: measure positive discharge cycle.
    dischargeCycles = 0;
    DDRB &= ~(1 << CAPPOS);                       // CAPPOS input.
    DDRB |= (1 << ECPIN);                         // ECPIN output.
    PORTB &= ~(1 << CAPPOS);                      // CAPPOS pull up resistor off.
    PORTB &= ~(1 << ECPIN);                       // ECPIN LOW.
    TCNT1 = 0;                                    // Start the timer.

    uint32_t startMicros = micros();
    PCMSK0 |= (1 << CAPPOS);          // Set PCINT1 to trigger an interrupt on state change 
    while (micros() - startMicros < EC_TIMEOUT) {
      if (dischargeCycles) {
        break;
      }
    }
    PCMSK0 &= ~(1 << CAPPOS);          // Clear the pin change interrupt on CAPPOS.
    totalCycles += dischargeCycles;
    if (dischargeCycles == 0)
      timeouts++;
        
    // Stage 3: charge the cap, negative cycle.
    DDRB &= ~(1 << ECPIN);                        // ECPIN input
    DDRB |= (1 << CAPPOS);                        // CAPPOS output
    PORTB &= ~(1 << CAPPOS);                      // CAPPOS LOW
    PORTB |= (1 << CAPNEG);                       // CAPNEG HIGH: Charge the cap
    PORTB &= ~(1 << ECPIN);                       // ECPIN pull up resistor off.
    delayMicroseconds(CHARGEDELAY);               //  wait for cap to charge

    // Stage 4: discharge the cap, compenstation cycle.
    DDRB &= ~(1 << CAPPOS);                       // CAPPOS input
    DDRB |= (1 << ECPIN);                         // ECPIN output
    PORTB &= ~(1 << CAPPOS);                      // CAPPOS pull up resistor off.
    PORTB |= (1 << ECPIN);                        // ECPIN HIGH

    // delay based on dischargeCycles
    if (dischargeCycles)
      delayMicroseconds(dischargeCycles >> TIMESCALE);
    else
      delayMicroseconds(EC_TIMEOUT);
  }

  // Disconnect EC probe: all pins to INPUT.
  DDRB &= ~(1 << CAPPOS);                         // CAPPOS input
  DDRB &= ~(1 << CAPNEG);                         // CAPNEG input
  DDRB &= ~(1 << ECPIN);                          // ECPIN input
  PORTB &= ~(1 << CAPPOS);                        // CAPPOS pull up resistor off.
  PORTB &= ~(1 << CAPNEG);                        // CAPNEG pull up resistor off.
  PORTB &= ~(1 << ECPIN);                         // ECPIN pull up resistor off.

  uint16_t averageCycles = (totalCycles >> 6);

  Serial.print("Timeouts: ");
  Serial.print(timeouts);
  Serial.print(", total cycles: ");
  Serial.print(totalCycles);
  Serial.print(", average cycles: ");
  Serial.println(averageCycles);

  return;
}

ISR(PCINT0_vect) {
  dischargeCycles = TCNT1;
}
