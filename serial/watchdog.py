
from machine import RMT, Pin
from utime import sleep_us
BAUDRATE = const(9600)

class Watchdog:
    def __init__(self, gpio, baudrate=BAUDRATE):
        self.gpio = gpio
        self.pin = Pin(self.gpio, Pin.OUT, value=1)
        self.data = bytearray(9)
        self.baudrate = baudrate
        self.duration = 1000000 // self.baudrate

    def feed(self):
        self.pin(0)
        sleep_us(self.duration - 40)  # Just the start bit
        self.pin(1)

    def set(self, timeout):
        self.send(b"S{}".format(timeout))

    def suspend(self):
        self.send(b"P")

    def status(self):
        return None  # Just to show the intention of returning None

    def send(self, msg):
        rmt = RMT(channel=4, gpio=self.gpio, tx_idle_level=1)

        if type(msg) is str:
            msg = msg.encode()
        for byte in msg:
            self.data[0] = 0  # Start bit
            for bit in range(1, 9):
                self.data[bit] = byte & 1
                byte >>= 1
            rmt.pulses_send(duration, tuple(self.data))
            sleep_us(duration * 2)  # 2 stop bits
        rmt.deinit()
        self.pin = Pin(self.gpio, Pin.OUT, value=1)

