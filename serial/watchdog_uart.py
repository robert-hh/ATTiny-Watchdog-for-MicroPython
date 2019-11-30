#
# UART version of the serial watchdog stub
#

class Watchdog:
    def __init__(self, uart):
        self.uart = uart

    def feed(self):
        self.uart.write(b'\xff')

    def start(self, timeout):
        self.uart.write(b"S{}".format(timeout))

    def stop(self):
        self.uart.write(b"P")

    def status(self):
        return None  # Just to show the intention of returning None

    def send(self, msg):
        if type(msg) is str:
            msg = msg.encode()
        self.uart.write(msg)

