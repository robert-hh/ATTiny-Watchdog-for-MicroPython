
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

    def set(self, timeout):
        timeout = min(MAXTIMEOUT, timeout)
        self.pin(0)
        sleep_ms(timeout)  # Pulse as long as timeout set
        self.pin(1)

    def suspend(self):
        self.set(SUSPEND)  # Longer pulse for suspend

    def status(self):
        if self.status_pin is not None:
            return self.status_pin()
        else:
            return None
