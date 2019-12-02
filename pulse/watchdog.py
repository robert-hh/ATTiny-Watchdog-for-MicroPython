
from machine import Pin
from utime import sleep_us, sleep_ms
MAXTIMEOUT = const(3600)
SUSPEND = const(7)

class Watchdog:
    def __init__(self, gpio, status=None):
        self.gpio = gpio
        self.pin = Pin(self.gpio, Pin.OUT, value=1)
        if status is not None:
            self.status_pin = Pin(status, Pin.IN)
        else:
            self.status_pin = None

    def feed(self):
        self.pin(0)
        sleep_us(20)  # Just a short blip
        self.pin(1)

    def start(self, timeout):
        timeout = min(MAXTIMEOUT, timeout)
        self.pin(0)
        sleep_ms(timeout)  # Pulse as long as timeout set
        self.pin(1)

    def stop(self, timeout=0):
        self.start(SUSPEND)  # Longer pulse for suspend
        if timeout > 0: # second pulse signals sleep period
            utime.sleep_ms(SUSPEND)
            self.start(timeout)

    def status(self):
        if self.status_pin is not None:
            return self.status_pin()
        else:
            return None

    def send(self):
        self.feed()