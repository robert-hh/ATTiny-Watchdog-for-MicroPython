/* 
   Soft Watchdod Timer
   Receives both Feed pulse and Commands from Pin 4
*/
   
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/wdt.h>


#define FEED_PIN 4
#define ACK_PIN 1
#define RESET_PIN 3

// Globals
int timeout = 3600 * 8; // default timeout 8 hours. Sleepy dog
int timer = timeout;
int tick_seen = 0;
enum {
  SLEEPING, WATCHING
} state = SLEEPING; 

// Function Prototypes
int system_sleep(int duration, int mode) ;
void send_ack(int, int);
void send_burst(int, int);
int get_lowtime(int);

#define DEBUG 0
#define MIN_PULSE 2
#define MAX_PULSE 3600
#define SUSPEND_PULSE 5 

void setup() {
    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(FEED_PIN, INPUT_PULLUP);
    pinMode(ACK_PIN, OUTPUT);
    state = SLEEPING;
}

void loop() {
    byte received = 0;
    int new_time = timeout;
    int pulse_time, food;
    int reset_state = 1;

    if (! DEBUG) {
      if (state == SLEEPING) {
        digitalWrite(ACK_PIN, LOW);
      } else {
        digitalWrite(ACK_PIN, HIGH);
      }
    }


    food = system_sleep(WDTO_1S, SLEEP_MODE_PWR_DOWN);

    reset_state = digitalRead(RESET_PIN); // could be reset of the master which happened
    pulse_time = get_lowtime(FEED_PIN);
    if (DEBUG) {
      send_ack(ACK_PIN, food);
      send_ack(ACK_PIN, 1 - reset_state);
      send_burst(ACK_PIN, pulse_time);
    }
    if (reset_state == 0) { // reset forces Sleep or Feed??
        state = SLEEPING;
    } else if (state == SLEEPING) { // Suspended or startup
      if (pulse_time > (SUSPEND_PULSE * 2) && pulse_time < MAX_PULSE) {
        timeout = pulse_time;
        timer = timeout;
        state = WATCHING;
      } else {
        if (DEBUG) {
          send_ack(ACK_PIN, 3);
        }
      }
    } else if (state == WATCHING) {  // Armed
        if (food && pulse_time < SUSPEND_PULSE) { // Short pulse
          timer = timeout;
          if (DEBUG) {
            send_ack(ACK_PIN, 1);
          }
        } else if (pulse_time >= SUSPEND_PULSE && pulse_time < (SUSPEND_PULSE * 2)) {
          state = SLEEPING;
        } else if (pulse_time >= (SUSPEND_PULSE * 2)) {
          timeout = pulse_time;
          timer = timeout;
        } else {
          timer -= 1;
          if (timer == 0) {
  
            pinMode(RESET_PIN, OUTPUT);
            digitalWrite(RESET_PIN, LOW);
            delay(100);
            digitalWrite(RESET_PIN, HIGH);
            pinMode(RESET_PIN, INPUT_PULLUP);
  
            state = SLEEPING;
          }
          if (DEBUG) {
              send_burst(ACK_PIN, timer);
          }
      }
    }
}

int get_lowtime(int pin)
{
  int i;
  for (i = 0; i < MAX_PULSE; i++) {
    if (digitalRead(pin) == 1) {
      break;
    }
    delay(1);
  }
  return i;
}

void send_ack(int pin, int duration)
{
    digitalWrite(pin, HIGH);
    delay(duration);
    digitalWrite(pin, LOW);
  
}

void send_burst(int pin, int burst)
{
    for (int i = 0; i < burst; i++) {
      send_ack(ACK_PIN, 0);
    }
  
}

//****************************************************************  
// set system into the sleep state 
// system wakes up when wtchdog is timed out
int system_sleep(int duration, int mode) 
{
    tick_seen = 0;
    GIMSK |= _BV(PCIE);                     // Enable Pin Change Interrupts
    PCMSK |= (_BV(PCINT4) | _BV(PCINT3));   // Use PB3 and PB4 as interrupt pin
    wdt_enable(duration);
    set_sleep_mode(mode); // sleep mode is set here
    wdt_reset();
#ifdef _AVR_IOTNX5_H_  
    WDTCR |= (1<<WDIE); // reenable interrupt to prevent system reset
    ADCSRA &= ~_BV(ADEN);                     //disable ADC
#else    
    WDTCSR |= (1<<WDIE); // reenable interrupt to prevent system reset
#endif
    sei();                                  // Enable interrupts
    sleep_enable();
    sleep_cpu();
    cli();                                  // Disable interrupts
    PCMSK &= ~(_BV(PCINT4) | (_BV(PCINT3)));               // Turn off PB4 as interrupt pin
    sleep_disable();                     // System continues execution here when watchdog timed out 
#ifdef _AVR_IOTNX5_H_  
    ADCSRA |= _BV(ADEN);                 //enable ADC
#endif    
    sei();                                  // Enable interrupts
    return tick_seen;
}

//****************************************************************  

ISR(WDT_vect) 
{
   wdt_disable();
}

ISR(PCINT0_vect) {
    // This is called when the interrupt occurs, but I don't need to do anything in it
    tick_seen = 1;
}
