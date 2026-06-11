import struct
from .protocol import CH_STREAM

class MirrorHandler:
    def __init__(self, comms):
        self.comms = comms

    def draw_rect(self, x, y, w, h, color, device_id=1):
        # Subtype 0x01
        payload = struct.pack("<BHHHHH", 0x01, x, y, w, h, color)
        self.comms.send_packet(CH_STREAM, device_id, payload)

    def clear(self, device_id=1):
        # Subtype 0x03
        payload = struct.pack("<B", 0x03)
        self.comms.send_packet(CH_STREAM, device_id, payload)
