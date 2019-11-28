# ATTiny based Watchdog (for MicroPython devices)

These are two variants of an external watchdog, which I made for use with ES32 based MicroPython
devices. Even if the ESP32 has a built-in watchdog, it is not really reliable. Actually, using
it does not improve the long term reliability of an application.

These two variants consists of two components:

- Software for an AttinyX5 device, which is the actual Watchdog. For (my) convenience
I made it with the Arduino development environment. I selected the ATTiny because it is easy 
available, cheap, well documented, and easy to use and available in various packages. 
Besides the controller, no other component is required.
- A small class for the MicroPython device controlling the watchdog.

These two variants differ on how the communication between the micropython device and
the watchdog is done.

- Pulse: Communication to the watchdog is via one GPIO line and pulses of various duration. 
That is the only drawback of using an external watchdog: it requires an GPIO port for feeding.
- Serial: Communication to the Watchdog is via one GPIO line and serial commands at 9600 baud.
Since only data is sent, I used the RMT module for sending, saving the UART port.

The run-time overhead of both variants is comparable and differs only in the length of the 
feed pulse. For the **pulse** variant, it is 20µs, for the **serial** variant, it is 100µs (the length
of the start bit at 9600 baud).

Both watchdogs have two states, a sleep state and a watch state. They start in sleep mode and have 
to be armed by a dedicated signal. The watch state can be suspended, e.g. before going to deep sleep.
The current consumption of the watchdog is about 1 µA, whith short bursts of 1 mA for 500µs every second.

When the watchdog fires (Barks), it creates reset pulse of 100ms duration. For that, the GPIO pin of the 
ATTiny will set to output mode. In all other times, it is in input mode with pull-up activated.

## Python class

watchdog = Watchdog(port)

Port is the name or number of the GPIO port. It is important not to supply a pin
object, but the name, e.g. "P9" or 14. The GPIO pin will be initialized by the class.

## Methods

### watchdog.set(seconds)

Set the timeout period of the watchdog to seconds and start the watch mode. The granularity is seconds.
The serial variant allows to reset the timeout while in watch mode. 
The pulse variant has to be stopped first.

### watchdog.feed()

Feed the dog. The timeout time will be reset to the value given in the set method

### watchdog.suspend()

Suspend the watch state and return to the sleep state.

### watchdog.send(string, baudrate) ## **Serial version only**

Send string to the watchdog device using the given baud rate. That is the basic command used by the
methods set() and suspend(). You can use that to toggle DEBUG output from the watchdog.

## Command interface 

### Serial interface

The following commands are supported: 
| Command | Description |
|:-:|:--|
|Snnn|Set the timeout of the watchdog and start the watch mode. nnn is the value in seconds. The command can be repeated while in watch mode and will set a new timeout.|
|P|Suspend the watch state and return to sleep mode.|
|D|Toggle the DEBUG output. The output is send to Port 1 of the ATTiny with 9600 baud.|
|anything else|Feed the dog. For speed, the feed command actually only creates a start bit, and the ATTiny then reads 0xff as character.|

The DEBUG output will echo the command and countdown the time in watch mode.

For the serial version a pull-up resistor at the communication link is highly recommended. The serial 
interface in the ATTiny uses the SoftSerial module of Arduino. It depends on the internal clock of the ATTiny (1 MHz here), which is not 100% accurate. In my test, the clock was about 1.004 MHz +/- 2 KHz. For asynchronous serial, that is still within the tolerated error.

### Pulse interface

The following pulse time ranges are defined: 
|State| Duration| Description |
|:-:|:-:|:--|
|sleep|2 ms < pulse < 3600 ms|Set the timeout of the watchdog and start the watch mode. The pulse duration defines the value in seconds. |
|watch|10 ms <= pulse < 3600 ms|Suspend the watch state and return to sleep mode.|
|watch|20 µs < pulse < 10 ms|Feed the dog.| 

Pulses longer that 3600ms will be ignored in both states. That covers a line which is permanently low. The determination of the pulse duration is not very accurate, but good enough for the purpose.

The debug output will respond to the input with a series of pulses. The first pulse length shows, if a internal timeout (~200µs) or a signal from the host (~1 ms) happened. That is followed: 
- in sleep mode by a single pulse of another ~3 ms duration
- in watch mode by a burst of short pulses. The burst length indicates the remaining time until reset. 
- the feed signal is confirmed by two pulses of 1 ms duration.
