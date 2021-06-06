#
# The MIT License (MIT)
#
# Copyright (c) 2021 RobertHammelrath
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# 

from machine import Pin
from esp32 import RMT
from time import sleep_us

BAUDRATE = const(9600)

class Watchdog:
    def __init__(self, gpio, *, status=None, restart=None, baudrate=BAUDRATE):
        self.rmt = RMT(0, pin=Pin(gpio), clock_div=80, idle=1)
        self.duration = 1000000 // baudrate  # tick duration 1us
        self.feedpulse = (self.duration, )

        if status is not None:
            self.status_pin = Pin(status, Pin.IN, Pin.PULL_DOWN)
            if restart is True:
                self.status_pin.irq(handler=self._restart, trigger=Pin.IRQ_FALLING)
        else:
            self.status_pin = None

    def _restart(self, pin):
        self.feed()

    def feed(self):
        self.rmt.write_pulses(self.feedpulse, start=0)

    def start(self, timeout):
        self.send(b"S{}".format(timeout))

    def stop(self, timeout=0):
        self.send(b"P{}".format(timeout))

    def status(self):
        if self.status_pin is not None:
            return self.status_pin()
        else:
            return None

    def send(self, msg):
        if type(msg) is str:
            msg = msg.encode()

        for byte in msg:
            data = [self.duration]  # start bit
            state = 0
            # create data bits
            for bit in range(8):
                if (byte & 1) == state:
                    data[-1] += self.duration
                else:
                    data.append(self.duration)
                    state = byte & 1
                byte >>= 1
            self.rmt.write_pulses(data, start=0)
            self.rmt.wait_done(timeout=self.duration * 9)  # Wait for the end
            sleep_us(self.duration * 2)  # Two stop bits

