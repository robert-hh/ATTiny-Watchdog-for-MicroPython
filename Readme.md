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
without RMT a UART based variant is supplied.

The run-time overhead of both variants is comparable and differs only in the length of the 
feed pulse. For the **pulse** variant, it is 20µs, for the **serial** and **uart**  variant,
it is 100µs (the length of the start bit at 9600 baud).

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
watchdog = Watchdog(port, status=None, restart=False, baudrate=9600)

**Port** is the name or number of the GPIO port. It is important not to supply a pin
object, but the name, e.g. "P9" or 14. The GPIO pin will be initialized by the class.  
**status** is the name/number of a pin, which, if connected, can be used to read back the 
status of the watchdog. If the status pin is set,   
**restart**=True will cause a feed signal to be sent when status starts toggling,
which is when the watch period is halfway expired. This is useful in situations, when there is no
suitable place for a feed() call. The drawback: feed() is then called in a callback function, which
may even be executed when the to-be-supervized functions stoppped working.
**baudrate** is the speed to be used. The default is 9600 and hardly ever changed.

### Serial version, UART
watchdog = Watchdog(uart, status=None)

**uart** has be be an UART object created with the proper baud rate and pin assignments.  
**status** is a Pin object, defined in input mode for reading back the status pin.

### Pulse version
watchdog = Watchdog(port, status=None, restart=False)

**Port** is the name or number of the GPIO port. It is important not to supply a pin
object, but the name, e.g. "P9" or 14. The GPIO pin will be initialized by the class.  
**status** is the name/number of a pin, which, if connected, can be used to read back 
the status of the watchdog.  
**restart**=True will cause a feed signal to be sent when status starts toggling,
which is when the watch period is halfway expired. This is useful in situations, when there is no
suitable place for a feed() call. The drawback: feed() is then called in a callback function, which
may even be executed when the to-be-supervized functions stoppped working.  

## Methods

### watchdog.start(seconds)

Set the timeout period of the watchdog to seconds and start the watch mode. The granularity is seconds.
The shortest timeout period that can be set in the pulse version is about 15 seconds.

### watchdog.feed()

Feed the dog. The timeout time will be set to the value given by the start() command.
Do not call feed(), e.g. in a different thread, before the start() command terminated, 
which may take a while. After calling feed(), it takes about 2 ms until the status as reported by a
call to status() changes. 

### watchdog.stop(seconds=0)

Suspend the watch state for the given time and enter the sleep state. 
If the value of seconds is not zero, sleep only for that amount of time 
and return to the watch state after that.

### watchdog.status()

Read back the status of the watchdog using the pin set in the constructor. Return values:  

|Value|Meaning|
|:-:|:-|
|0|The watchdog is sleeping|
|1|The watchdog is active. If the watch period is halfway elapsed, the value gets togggling once per second. That can be used for instance to trigger a callback, which feeds the watchdog.|
|None|Port no set|

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
|watch|5 ms <= pulse < 10 ms|Suspend the watch state and enter the sleep state. If after less than 10 ms it is followed by another pulse, the length of that pulse is taken as sleep timeout.|
|watch|10 ms <= pulse < 3600 ms|Restart the watch state and set the new timeout.|

Pulses longer that 3600ms will be ignored in both states. That covers a line which is permanently low. The determination of the pulse duration is not very accurate, but good enough for the purpose.

The debug output will respond to the input with a series of pulses.   
- The first pulse length shows, if a internal timeout (~200µs) or a signal from the host (~1 ms) happened.   
- The second pulse shows the state of the watchdog, short for sleep and long for watch.   
- The third pulse shows the state of the reset line, short for high and long for low.  
- That is followed  by a burst of short pulses. The burst length indicates the remaining time until
timeout in the respective state. 

The status output will indicate the watchdog state; low for sleeping and high for watching.

## Example

Basic example:

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

Basic example using the internal callback for feeding:

```
import watchdog

wd = watchdog.Watchdog("P9", "P10", True)
wd.start(100) # set a timeout of 100 seconds and start
#
# No explicit call to feed is required
#
# to stop, call
#
wd.stop()
```


UART example using an dedicated callback for feeding:

```
import watchdog
from machine import UART, Pin

uart=UART(1, 9600)
pin = Pin("P10", Pin.IN)
wd = watchdog_uart.Watchdog(uart, pin)
pin.callback(Pin.IRQ_FALLING, handler=lambda p: wd.feed())

wd.start(100) # set a timeout of 100 seconds and start
#
# No explicit call to feed is required
#
# to stop, call
#
wd.stop()
```

## License

The MIT License (MIT)

Copyright (c) 2021 RobertHammelrath

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
 
