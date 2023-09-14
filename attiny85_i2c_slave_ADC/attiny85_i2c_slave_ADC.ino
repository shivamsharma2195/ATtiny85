#define I2C_SLAVE_ADDRESS 0x21 // the 7-bit address (remember to change this when adapting this example)
#include <TinyWireS.h>
// The default buffer size, Can't recall the scope of defines right now
#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE ( 16 )
#endif


int ADC3=3; 
int ADC4=2; 
int control=1; 
volatile uint8_t val1=0; 
int val2=0; 
int max_value=5; 
volatile uint8_t read_value=0; 



volatile uint8_t i2c_regs[] =
{
    0xDE, 
    0xAD, 
    0xBE, 
    0xEF, 
};
// Tracks the current register pointer position
volatile byte reg_position;
const byte reg_size = sizeof(i2c_regs);
const byte reg_size_lessone = reg_size-1;

/**
 * This is called for each read request we receive, never put more than one byte of data (with TinyWireS.send) to the 
 * send-buffer when using this callback
 * 
 */
void requestEvent()
{  
    TinyWireS.write(val1);
    // Increment the reg position on each read, and loop back to zero
    reg_position++;
    if (reg_position >= reg_size_lessone)
    {
        reg_position = 0;
    }
}

// TODO: Either update this to use something smarter for timing or remove it alltogether
//void blinkn(uint8_t blinks)
//{
//    digitalWrite(3, HIGH);
//    while(blinks--)
//    {
//        digitalWrite(3, LOW);
//        tws_delay(50);
//        digitalWrite(3, HIGH);
//        tws_delay(100);
//    }
//}

/**
 * The I2C data received -handler
 *
 * This needs to complete before the next incoming transaction (start, data, restart/stop) on the bus does
 * so be quick, set flags for long running tasks to be called from the mainloop instead of running them directly,
 */
void receiveEvent(uint8_t howMany)
{
    if (howMany < 1)
    {
        // Sanity-check
        return;
    }
    if (howMany > TWI_RX_BUFFER_SIZE)
    {
        // Also insane number
        return;
    }

    reg_position = TinyWireS.read();
    howMany--;
    if (!howMany)
    {
        // This write was only to set the buffer for next read
        return;
    }
    while(howMany--)
    {
        i2c_regs[reg_position] = TinyWireS.read();
        reg_position++;
        if (reg_position >= reg_size_lessone)
        {
            reg_position = 0;
        }
    }
}


void setup()
{
    // TODO: Tri-state this and wait for input voltage to stabilize 
//    pinMode(3, OUTPUT); // OC1B-, Arduino pin 3, ADC
//    digitalWrite(3, LOW); // Note that this makes the led turn on, it's wire this way to allow for the voltage sensing above.
//
//    pinMode(1, OUTPUT); // OC1A, also The only HW-PWM -pin supported by the tiny core analogWrite
    pinMode(ADC3,INPUT);
//    pinMode(ADC4,INPUT);
    pinMode(control,OUTPUT);
    digitalWrite(control,LOW);
    /**
     * Reminder: taking care of pull-ups is the masters job
     */

    TinyWireS.begin(I2C_SLAVE_ADDRESS);
    TinyWireS.onReceive(receiveEvent);
    TinyWireS.onRequest(requestEvent);

    
    // Whatever other setup routines ?
    
//    digitalWrite(3, HIGH);
}

void loop()
{
    /**
     * This is the only way we can detect stop condition 
     * it needs to be called in a very tight loop in order not to miss any (REMINDER: Do *not* use delay() anywhere, use tws_delay() instead).
     * It will call the function registered via TinyWireS.onReceive(); if there is data in the buffer on stop.
     */
    digitalWrite(control,LOW);
    delay(1000);
    val1=analogRead(ADC3);
    delay(500);
//    val2=analogRead(ADC4);
//    delay(500);
    read_value=abs(val1);
    if (read_value<max_value){
    digitalWrite(control,HIGH);
    delay(1000);
    }
    TinyWireS_stop_check();
}
