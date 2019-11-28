
from machine import Pin
from utime import sleep_us, sleep_ms
MAXTIMEOUT = const(3600)
SUSPEND = const(15)

class Watchdog:
    def __init__(self, gpio):
        self.gpio = gpio
        self.pin = Pin(self.gpio, Pin.OUT, value=1)

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
        self.pin(0)
        sleep_ms(SUSPEND)  # Long pulse for suspend
        self.pin(1)
