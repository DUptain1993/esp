"""Multi-device tracking and routing on the controller side."""

import time

from . import commands as C

STALE_SECONDS = 30.0


class DeviceManager:
    def __init__(self, comms):
        self.comms = comms
        self.devices = {}            # id -> last_seen (monotonic)
        self.active = C.DEVICE_SELF

    def seen(self, device_id):
        if device_id == C.DEVICE_BROADCAST:
            return
        new = device_id not in self.devices
        self.devices[device_id] = time.monotonic()
        return new

    def select(self, device_id):
        self.active = device_id
        return device_id

    def expire(self):
        now = time.monotonic()
        for dev in [d for d, t in self.devices.items()
                    if d != C.DEVICE_SELF and now - t > STALE_SECONDS]:
            del self.devices[dev]

    def list_devices(self):
        now = time.monotonic()
        return [(d, now - t, d == self.active) for d, t in sorted(self.devices.items())]

    def broadcast_cmd(self, channel, cmd):
        self.comms.send_cmd(channel, C.DEVICE_BROADCAST, cmd)

    def send_list_to(self, stream, device_id=None):
        device_id = device_id if device_id is not None else self.active
        lines = ["devices:"]
        for d, age, is_active in self.list_devices():
            mark = ">" if is_active else " "
            lines.append("%s 0x%02X  %.0fs ago" % (mark, d, age))
        stream.send_text("\n".join(lines) + "\n", device_id)
