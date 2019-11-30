#
# UART version of the serial watchdog stub
#

class Watchdog:
    def __init__(self, uart, status=None):
        self.uart = uart
        self.status_pin = status

    def feed(self):
        self.uart.write(b'\xff')

    def start(self, timeout):
        self.uart.write(b"S{}".format(timeout))

    def stop(self):
        self.uart.write(b"P")

    def status(self):
        if self.status_pin is not None:
            return self.status_pin()
        else:
            return None

    def send(self, msg):
        if type(msg) is str:
            msg = msg.encode()
        self.uart.write(msg)

