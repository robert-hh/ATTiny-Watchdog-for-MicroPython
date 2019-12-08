
from machine import RMT, Pin
from utime import sleep_us
BAUDRATE = const(9600)

class Watchdog:
    def __init__(self, gpio, status=None, restart=None, baudrate=BAUDRATE):
        self.pin = Pin(gpio, Pin.OUT, value=1)
        self.data = bytearray(9)
        self.duration = 1000000 // baudrate
        if status is not None:
            self.status_pin = Pin(status, Pin.IN, Pin.PULL_DOWN)
            if restart is True:
                self.status_pin.callback(Pin.IRQ_FALLING, self._restart)
        else:
            self.status_pin = None

    def _restart(self, pin):
        self.feed()

    def feed(self):
        self.pin(0)
        sleep_us(self.duration - 20)  # Just the start bit
        self.pin(1)

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
        rmt = RMT(channel=4, gpio=self.gpio, tx_idle_level=1)

        if type(msg) is str:
            msg = msg.encode()
        for byte in msg:
            self.data[0] = 0  # Start bit
            for bit in range(1, 9):
                self.data[bit] = byte & 1
                byte >>= 1
            rmt.pulses_send(self.duration, tuple(self.data))
            sleep_us(self.duration * 2)  # 2 stop bits
        rmt.deinit()
        self.pin = Pin(self.gpio, Pin.OUT, value=1)

