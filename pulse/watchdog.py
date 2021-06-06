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
from utime import sleep_us, sleep_ms
MAXTIMEOUT = const(3600)
SUSPEND = const(7)

class Watchdog:
    def __init__(self, gpio, status=None, restart=False):
        self.pin = Pin(gpio, Pin.OUT, value=1)
        if status is not None:
            self.status_pin = Pin(status, Pin.IN, Pin.PULL_DOWN)
            if restart is True:
                self.status_pin.callback(Pin.IRQ_FALLING, self._restart)
        else:
            self.status_pin = None
        sleep_ms(100)  # let the Pin settle

    def delay(self, ms): # more precise delay.
        for _ in range(ms):  # Pulse as long as timeout set
            sleep_us(950)    # using a loop, due to the sleep_ms jitter

    def _restart(self, pin):
        self.feed()

    def feed(self):
        self.pin(0)
        sleep_us(20)  # Just a short blip
        self.pin(1)

    def start(self, timeout):
        timeout = min(MAXTIMEOUT, timeout) # less than 1 hour
        timeout = max(SUSPEND * 2, timeout)  # more that a stop pulse
        self._start(timeout)

    def _start(self, timeout):
        self.pin(0)
        self.delay(timeout)
        self.pin(1)

    def stop(self, timeout=0):
        self._start(SUSPEND)  # Longer pulse for suspend
        if timeout > 0: # second pulse signals sleep period
            self.delay(SUSPEND)
            self._start(timeout)

    def status(self):
        if self.status_pin is not None:
            return self.status_pin()
        else:
            return None

    def send(self):
        self.feed()
