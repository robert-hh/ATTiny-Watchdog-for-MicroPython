/* 
   Soft Watchdog Timer
   Receives both Feed pulse and Commands from Pin 4
*/
   
#include <SoftwareSerial.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/wdt.h>


#define RX_PIN 4
#define TX_PIN 2
#define STATUS_PIN 1
#define RESET_PIN 3
#define MAX_MSG 10

SoftwareSerial mySerial(RX_PIN, TX_PIN);
// Globals
int watch_period = 3600; // default timeout 1 Hour. Lazy dog
int sleep_period = 0;    // sleeps forever, once it does
int timer = watch_period;
enum {
  SLEEPING, WATCHING
} state = SLEEPING; 
int DEBUG = false;

// Data and Function Prototypes
struct result {
    char cmd;
    int value;
};
void system_sleep(int duration, int mode) ;
result get_command(char *);

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
    char message[MAX_MSG + 1]; // Command Buffer

    digitalWrite(STATUS_PIN, state);  // Signal WD state

    system_sleep(WDTO_1S, SLEEP_MODE_PWR_DOWN);

    reset_state = digitalRead(RESET_PIN); // could be reset of the master which happened
    if (reset_state == 0) {  // reset is low, stop watchdog
        state = SLEEPING;
        timer = sleep_period;
        if (DEBUG) {
          mySerial.println("Manual reset");
        }
    } else if (mySerial.available() > 0) {
      input = get_command(message);
      if (DEBUG) {
        mySerial.print("Command ");
        mySerial.println(message);
      }
      if (input.cmd) {
        switch (input.cmd) {
          case 'S':
            state = WATCHING;
            timer = watch_period = input.value;
            break;
          case 'P':
            state = SLEEPING;
            timer = sleep_period = input.value;
            break;
          case 'D':
            if (input.value <= 0) {
              DEBUG = ! DEBUG;
            } else {
              DEBUG = input.value;
            }
            break;
          default:
            if (state == WATCHING) { // Feed the dog
              timer = watch_period;
            }
        }
      }
    }
    if (state == SLEEPING) { // Stopped
        if (sleep_period > 0) {
          timer -= 1;
          if (timer <= 0) {
            timer = watch_period;
            state = WATCHING;
            if (DEBUG) {
              mySerial.println("Yawn");
            }
          } else {
            if (DEBUG) {
              mySerial.print(timer, DEC);
              mySerial.println(" Sleeping");
            }
          }
        } else {
          if (DEBUG) {
            mySerial.println("Sleeping");
          }
        }
    } else {  // Armed
        timer -= 1;
        if (timer <= 0) {
          if (DEBUG) {
            mySerial.println("Bark");
          }

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
        } else {
          if (DEBUG) {
            mySerial.print(timer, DEC);
            mySerial.println(" Watching");
          }
        }
    }
}

result get_command(char *message) {
  result input = {'\0', 0};
  byte received = 0;
   
  received = mySerial.readBytes(message, MAX_MSG);
  if (received > 0) {
    message[received] = '\0';
    for (int i = 0; i < received; i++) {
      if (message[i] == 'S' ||
          message[i] == 'D' ||
          message[i] == 'P') {
        input.cmd = message[i];
        input.value = atoi(message + i + 1);
        if (input.value < 0) { // no negative values
          input.value = 0;
        }
        break;
      } else {
        input.cmd = 'F';
      }
    }
  }
  return input;
}

//****************************************************************  
// set system into the sleep state 
// system wakes up when wtchdog is timed out
void system_sleep(int duration, int mode) 
{
    PCMSK |= _BV(PCINT3);   // Use PB3 too as interrupt pin
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
    PCMSK &= ~_BV(PCINT3);               // Turn off PB3 as interrupt pin
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
