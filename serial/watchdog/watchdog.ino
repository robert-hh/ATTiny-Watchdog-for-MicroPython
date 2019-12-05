/* 
   Soft Watchdog Timer
   Receives both Feed pulse and Commands from Pin 4
*/
   
#include <SoftwareSerial.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/wdt.h>


#define RX_PIN 3 // was 4
#define TX_PIN 2
#define STATUS_PIN 1
#define RESET_PIN 4 // was 3
#define MAX_MSG 10

SoftwareSerial mySerial(RX_PIN, TX_PIN);
// Globals
// if you want to start the watchdog busy, set sleep period and watch period accordingly.
// then it needs no further interaction to run.
//
int watch_period = 3600; // default timeout 1 Hour. Lazy dog
int sleep_period = 0;    // sleeps forever, once it does
int timer = sleep_period;
int DEBUG = false;
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
// Data and Function Prototypes
struct result {
    tags cmd;
    int value;
};
void system_sleep(int duration, int mode) ;
result get_input();

void setup() {
    mySerial.begin(9600);
    mySerial.setTimeout(100);
    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(STATUS_PIN, OUTPUT);
    state = SLEEPING;
}

void loop() {
    result input;
    int reset_state;

    digitalWrite(STATUS_PIN, state);  // Signal WD state

    system_sleep(WDTO_1S, SLEEP_MODE_PWR_DOWN);

    reset_state = digitalRead(RESET_PIN); // could be reset of the master which happened
    if (reset_state == 0) {  // reset is low, stop watchdog
        state = SLEEPING;
        timer = sleep_period;
        if (DEBUG) {
          mySerial.println("Reset");
        }
    } else { // check messages
      input = get_input();
      switch (input.cmd) {
        case START: // Start
          state = WATCHING;
          timer = watch_period = input.value;
          break;
        case STOP: // Stop
          state = SLEEPING;
          timer = sleep_period = input.value;
          break;
        case LOGGING: //Debug
          if (input.value <= 0) {
            DEBUG = ! DEBUG;
          } else {
            DEBUG = input.value;
          }
          break;
        case FEED: // Feed
          if (state == WATCHING) { // Feed the dog
            timer = watch_period;
          }
        case TIMEOUT:
        default: // Timeout
          if (state == SLEEPING) { // Stopped
            if (sleep_period > 0) {
              timer -= 1;
              if (timer <= 0) {
                timer = watch_period;
                state = WATCHING;
              }
            }
          } else {  // Armed
            timer -= 1;
            if (timer <= 0) {
              // Fire the reset Pulse
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
    if (DEBUG) {
      mySerial.print(state ? "Watch " : "Sleep ");
      mySerial.println(timer, DEC);
    }
}

result get_input() {
  result input;
  byte received = 0;
  char message[MAX_MSG + 1]; // Command Buffer

  if (mySerial.available() > 0) { // message seen 
    received = mySerial.readBytes(message, MAX_MSG);
    if (received > 0) {
      message[received] = '\0';
      if (DEBUG) {
        mySerial.print("Cmd ");
        mySerial.println(message);
      }
      for (int i = 0; i < received; i++) {
        if (message[i] == 'S' ||  // dedicated command?
            message[i] == 'D' ||
            message[i] == 'P') {
          input = {(tags)message[i], max(atoi(message + i + 1), 0)};
          break;
        } else { // Everything else is considered Feed
          input = {FEED, 0};
        }
      }
    }
  } else {
    input = {TIMEOUT, 0};
  }
  return input;
}

//****************************************************************  
// set system into the sleep state 
// system wakes up when wtchdog is timed out
void system_sleep(int duration, int mode) 
{
    PCMSK |= _BV(PCINT4);   // Use PB4 too as interrupt pin
    wdt_enable(duration);
    set_sleep_mode(mode); // sleep mode is set here
    wdt_reset();
#ifdef _AVR_IOTNX5_H_  
    WDTCR |= (1<<WDIE); // reenable interrupt to prevent system reset
    ADCSRA &= ~_BV(ADEN);                     //disable ADC
#else    
    WDTCSR |= (1<<WDIE); // reenable interrupt to prevent system reset
#endif
    sleep_enable();
    sleep_cpu();
    sleep_disable();                     // System continues execution here when watchdog timed out 
    PCMSK &= ~_BV(PCINT4);               // Turn off PB4 as interrupt pin
#ifdef _AVR_IOTNX5_H_  
    ADCSRA |= _BV(ADEN);                 //enable ADC
#endif    
}

//****************************************************************  

ISR(WDT_vect) 
{
   wdt_disable();
}

ISR(PCINT1_vect) {
    // This is called when the interrupt occurs
}
