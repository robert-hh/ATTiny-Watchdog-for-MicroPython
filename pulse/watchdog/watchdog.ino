/* 
   Soft Watchdod Timer
   Receives both Feed pulse and Commands from Pin 4
*/
   
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/wdt.h>

#define FEED_PIN 4
#define DEBUG_PIN 2
#define STATUS_PIN 1
#define RESET_PIN 3

#define DEBUG 0

#define MIN_PULSE 2
#define MAX_PULSE 3600
#define SUSPEND_PULSE 5 

// Globals
// if you want to start the watchdog busy, set sleep period and watch period accordingly.
// then it needs no further interaction to run.
//
int watch_period = 3600; // Default timeout 1 hour. Sleepy dog
int sleep_period = 0;    // Default timeout infinite
int timer = sleep_period;
int tick_seen = 0;
enum {
  SLEEPING, WATCHING
} state = SLEEPING; 

enum tags {
  TIMEOUT = 'T',
  START = 'S',
  STOP = 'P',
  FEED = 'F',
  LOGGING = 'D'
};

// Function and Data Prototypes
struct result {
    tags cmd;
    int value;
};
void system_sleep(int duration, int mode) ;
int get_pulsetime(int, int, int);
result get_input();
#if DEBUG
void send_ack(int, int);
void send_burst(int, int);
#endif

void setup() {
    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(FEED_PIN, INPUT_PULLUP);
    pinMode(STATUS_PIN, OUTPUT);
#if DEBUG
    pinMode(DEBUG_PIN, OUTPUT);
#endif
    state = SLEEPING;
}

void loop() {
    result input;
    int reset_state;

    if (state == SLEEPING) {
      digitalWrite(STATUS_PIN, LOW);
    } else {
      digitalWrite(STATUS_PIN, HIGH);
    }

    system_sleep(WDTO_1S, SLEEP_MODE_PWR_DOWN);

    reset_state = digitalRead(RESET_PIN); // could be reset of the master which happened
    input = get_input();
    if (reset_state == 0) { // reset forces Sleep or Feed??
        state = SLEEPING;
        timer = sleep_period;
    } else {
      switch (input.cmd) {
        case START:
          state = WATCHING;
          timer = watch_period = input.value;
          break;
        case STOP:
          state = SLEEPING;
          timer = sleep_period = input.value;
          break;
        case FEED:
          if (state == WATCHING) { // Feed the dog
            timer = watch_period;
          }
          break;
        case TIMEOUT: // 1 s Timer 
        default:  // can only be timeout of timer.
          if (state == SLEEPING) { // Suspended or startup
            if (sleep_period > 0) {
              timer -= 1;
              if (timer <= 0) {
                timer = watch_period;
                state = WATCHING;
              }
            }
          } else if (state == WATCHING) {  // Armed
            timer -= 1;
            if (timer <= 0) {
        
              pinMode(RESET_PIN, OUTPUT);
              digitalWrite(RESET_PIN, LOW);
              delay(100);
              digitalWrite(RESET_PIN, HIGH);
              pinMode(RESET_PIN, INPUT_PULLUP);
        
              state = SLEEPING;
              timer = sleep_period;
              // if the watchdog should be restarted instead of sleeping,
              // replace the two lines above by the single line
              // timer = watch_period;
            }
            break;
          }
      }
    }    
#if DEBUG    
    if (DEBUG) {
      send_ack(DEBUG_PIN, tick_seen);
      send_ack(DEBUG_PIN, state);
      send_ack(DEBUG_PIN, 1 - reset_state);
      send_burst(DEBUG_PIN, timer);
    }
#endif
}

result get_input()
{
  int pulse_time;
  result input;

  if (tick_seen) { // pulse seen; 
    pulse_time = get_pulsetime(FEED_PIN, LOW, MAX_PULSE);
    if (pulse_time < SUSPEND_PULSE) { // Short pulse, feed signal
      input = {FEED, 0};
    } else if (pulse_time >= SUSPEND_PULSE && pulse_time < (SUSPEND_PULSE * 2)) {
      get_pulsetime(FEED_PIN, HIGH, SUSPEND_PULSE * 2);
      input = {STOP, get_pulsetime(FEED_PIN, LOW, MAX_PULSE)}; // Stop signal
    } else if (pulse_time >= (SUSPEND_PULSE * 2)) { // start watch
      input = {START, pulse_time};  // Start signal
    } 
  } else { // No pulse, timeout of 1s timer.
    input = {TIMEOUT, 0};
  }
  return input;
}

int get_pulsetime(int pin, int level, int maxtime)
{
  int i;
  for (i = 0; i < maxtime; i++) {
    if (digitalRead(pin) != level) {
      break;
    }
    delayMicroseconds(950);
  }
  return i;
}

#if DEBUG
void send_ack(int pin, int duration)
{
    digitalWrite(pin, HIGH);
    delay(duration);
    digitalWrite(pin, LOW);
}

void send_burst(int pin, int burst)
{
    for (int i = 0; i < burst; i++) {
    digitalWrite(pin, HIGH);
    digitalWrite(pin, LOW);
    }
}
#endif
//****************************************************************  
// set system into the sleep state 
// system wakes up when wtchdog is timed out
void system_sleep(int duration, int mode) 
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
