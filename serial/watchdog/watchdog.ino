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

SoftwareSerial mySerial(RX_PIN, TX_PIN);
// Globals
int timeout = 3600 * 24; // default timeout 1 Day. Sleepy dog
int timer = timeout;
char message[10]; // Command Buffer
enum {
  SLEEPING, WATCHING
} state = SLEEPING; 

// Function Prototypes
char get_command(int *);
void system_sleep(int duration, int mode) ;

int DEBUG = false;

void setup() {
    mySerial.begin(9600);
    mySerial.setTimeout(100);
    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(STATUS_PIN, OUTPUT);
    state = SLEEPING;
}

void loop() {
    byte received = 0;
    char cmd = '\0';
    int value = timeout;
    int reset_state;

    digitalWrite(STATUS_PIN, state);  // Signal WD state

    system_sleep(WDTO_1S, SLEEP_MODE_PWR_DOWN);

    reset_state = digitalRead(RESET_PIN); // could be reset of the master which happened
    if (reset_state == 0) {  // reset is low, stop watchdog
        state = SLEEPING;
        if (DEBUG) {
            mySerial.println("Manual reset");
        }
    } else if (mySerial.available() > 0) {
      cmd = get_command(&value);
      if (DEBUG) {
        mySerial.print("Command ");
        mySerial.println(message);
      }
      if (cmd) {
        switch (cmd) {
          case 'S':
            state = WATCHING;
            timeout = value;
            timer = timeout;
            break;
          case 'P':
            state = SLEEPING;
            break;
          case 'D':
            if (value <= 0) {
              DEBUG = ! DEBUG;
            } else {
              DEBUG = value;
            }
            break;
          default:
            timer = timeout;
        }
      }
    }
    if (state == SLEEPING) { // Suspended or startup
        if (DEBUG) {
          mySerial.println("Sleeping");
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
        } else {
          if (DEBUG) {
            mySerial.println(timer, DEC);
          }
        }
    }
}

char get_command(int *value) {
  int i;
  char resp = '\0';
  byte received = 0;
   
  received = mySerial.readBytes(message, sizeof(message));
  if (received > 0) {
    message[received] = '\0';
    for (i = 0; i < received; i++) {
      if (message[i] == 'S' ||
          message[i] == 'D' ||
          message[i] == 'P') {
        *value = atoi(message + i + 1);
        resp = message[i];
        break;
      } else {
        resp = 'F';
      }
    }
  }
  return resp;
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
