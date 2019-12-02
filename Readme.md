# ATTiny based Watchdog (for MicroPython devices)

These are two variants of an external watchdog, which I made for use with ES32 based MicroPython
devices. Even if the ESP32 has a built-in watchdog, it is not really reliable. Actually, using
it does not improve the long term reliability of an application.

These two variants consists of two components:

- Software for an AttinyX5 device, which is the actual Watchdog. For (my) convenience
I made it with the Arduino development environment. I selected the ATTiny because it is easy 
available, cheap, well documented, and easy to use and available in various packages. 
Besides the controller and a pull-up resistor for the input, no other component is required.
- A small class for the MicroPython device controlling the watchdog.

These two variants differ on how the communication between the micropython device and
the watchdog is done.

- Pulse: Communication to the watchdog is via one GPIO line and pulses of various duration. 
That is the only drawback of using an external watchdog: it requires an GPIO port for feeding.
- Serial: Communication to the Watchdog is via one GPIO line and serial commands at 9600 baud.
Since only data is sent, I used the RMT module for sending, saving the UART port. For devices
without RMT as UART based variant is supplied.

The run-time overhead of both variants is comparable and differs only in the length of the 
feed pulse. For the **pulse** variant, it is 20µs, for the **serial** variant, it is 100µs (the length
of the start bit at 9600 baud).

Both watchdogs have two states, a sleep state and a watch state. They start in sleep state and have 
to be armed by a dedicated signal. The watch state can be suspended, e.g. before going to deep sleep.
The current consumption of the watchdog is about 1 µA, with short bursts of 1 mA for 500µs every second.

When the watchdog fires (Barks), it creates reset pulse of 100ms duration. For that time, the 
respective GPIO pin of the ATTiny will set to output mode. In all other times, it is in
input mode with pull-up active.

In both version, an Reset signal on the supervized micro is detected and will cause the watchdog 
to enter the sleep state. If the sleep state is timed (see command stop() below), then for that period only.

## Python class

### Serial version, RMT
watchdog = Watchdog(port, status=None, baudrate=9600)

Port is the name or number of the GPIO port. It is important not to supply a pin
object, but the name, e.g. "P9" or 14. The GPIO pin will be initialized by the class. status is the
name/number of a pin, which, if connected, can be used to read back the status of the watchdog.
baudrate is the speed to be used. The default is 9600 and hardly ever changed.

### Serial version, UART
watchdog = Watchdog(uart, status=None)

uart has be be an UART object created with the proper baud rate and pin assignments.
status is a Pin object, defined in input mode for reading back the status pin.

### Pulse version
watchdog = Watchdog(port, status=None)

Port is the name or number of the GPIO port. It is important not to supply a pin
object, but the name, e.g. "P9" or 14. The GPIO pin will be initialized by the class. status is the
name/number of a pin, which, if connected, can be used to read back the status of the watchdog.

## Methods

### watchdog.start(seconds)

Set the timeout period of the watchdog to seconds and start the watch mode. The granularity is seconds.

### watchdog.feed()

Feed the dog. The timeout time will be set to the value given by the start() command.

### watchdog.stop(seconds=0)

Suspend the watch state for the given time and enter the sleep state. 
If the value of seconds is not zero, sleep only for that amount of time 
and return to the watch state after that.

### watchdog.status()

Read back the status of the watchdog using the pin set in the constructor. Return values:  

0: watchdog is sleeping  
1: watchdog is active  
None: Port no set

### watchdog.send(string)

Send string to the watchdog device using the baud rate set in the constructor. 
That is the basic command used by the methods start() and stop(). 
You can use that to toggle DEBUG output from the watchdog, by calling send("D").
In the pulse version, send() exists but does not do anything.

## Command interface of the ATTiny

### Pin assignments of the Attiny:

|Pin|Port|Function|
|:-:|:-:|:-|
|2|PB3|Reset signal to host (required)|
|3|PB4|Feed and command input pin (recommended)|
|4|GND|GND (required)|
|6|PB1|Status pin (optional)|
|7|PB2|Debug output pin. (optional)|
|8|Vcc|3.3 - 5V Vcc (required)|


### Serial interface

The following commands are supported: 

| Command | Description |
|:-:|:--|
|Snnn|Set the timeout of the watchdog and change to the watch state. nnn is the value in seconds. The command can be repeated while in watch state and will set a new timeout.|
|Pnnn|Suspend the watch state and return to the sleep state. If the number nnn is given and > 0, then sleep for that time only and return to the watch state after that.|
|Dn|n is a number, which may be omitted. If the value is 0, toggle the DEBUG output. If the value is not 0, switch DEBUG ouput on. The output is send to Port PB2 of the ATTiny with 9600 baud.|
|anything else|Feed the dog. For speed, the feed command actually only creates a start bit, and the ATTiny then reads 0xff as character.|

The DEBUG output of the ATTiny will echo the command and countdown the time in watch mode.  
The status output will indicate the watchdog state; low for sleeping and high for watching.

For the serial version a pull-up resistor at the communication link is highly recommended. The serial 
interface in the ATTiny uses the SoftSerial module of Arduino. It depends on the internal clock of 
the ATTiny (1 MHz here), which is not 100% accurate. In my test, the clock speed was 
about 1.005 MHz +/- 2 KHz. For asynchronous serial, that is still within the tolerated error range.

### Pulse interface

The following pulse time ranges are defined: 

|State| Duration| Description |
|:-:|:-:|:--|
|sleep|pulse < 5 ms|Feed pulse. Will be ignored|
|sleep|5 ms <= pulse < 10 ms|Stay in sleep mode. If after less than 10 ms it is followed by another pulse, that time is taken as sleep timeout.|
|sleep|10 ms <= pulse < 3600 ms|Set the timeout of the watchdog and change to the watch state. The pulse duration in ms defines the timeout value in seconds.|
|watch|20 µs < pulse < 5 ms|Feed the dog.| 
|watch|5 ms <= pulse < 10 ms|Suspend the watch state and enter the sleep state. If after less than 10 ms it is followed by another pulse, that time is taken as sleep timeout.|
|watch|10 ms <= pulse < 3600 ms|Restart the watch state and set the new timeout.|

Pulses longer that 3600ms will be ignored in both states. That covers a line which is permanently low. The determination of the pulse duration is not very accurate, but good enough for the purpose.

The debug output will respond to the input with a series of pulses.   
- The first pulse length shows, if a internal timeout (~200µs) or a signal from the host (~1 ms) happened.   
- The second pulse shows the state of the watchdog, short for sleep and long for watch.   
- The third pulse shows the state of the reset line, short for high and long for low.  
- That is followed  by a burst of short pulses. The burst length indicates the remaining time until
timout in the respective state. 

The status output will indicate the watchdog state; low for sleeping and high for watching.

## Example

```
import watchdog

wd = watchdog.Watchdog("P9")
wd.start(100) # set a timeout of 100 seconds and start
#
# To feed, call regularly
#
wd.feed()
#
# to stop, call
#
wd.stop()
```