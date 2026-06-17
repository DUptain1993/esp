"""Graphical UI mirror to the ESP32 canvas (CH_STREAM / STR_MIRROR).

Draw-op encoding matches firmware/src/app/mirror.cpp (little-endian):
  FILL  [01][color:2]
  PIXEL [02][x:2][y:2][color:2]
  RECT  [03][x:2][y:2][w:2][h:2][color:2]
  HLINE [04][x:2][y:2][len:2][color:2]
Colours are RGB565.
"""

import struct

from . import commands as C


class MirrorHandler:
    def __init__(self, comms):
        self.comms = comms

    def set_mode(self, mode, device_id=C.DEVICE_SELF):
        self.comms.send(C.CH_CONTROL, device_id,
                        bytes([C.CMD_SET_MODE, mode & 0xFF]), compress=False)

    def _send_ops(self, ops, device_id):
        self.comms.send(C.CH_STREAM, device_id, bytes([C.STR_MIRROR]) + ops)

    def fill(self, color, device_id=C.DEVICE_SELF):
        self._send_ops(struct.pack("<BH", C.MOP_FILL, color & 0xFFFF), device_id)

    def pixel(self, x, y, color, device_id=C.DEVICE_SELF):
        self._send_ops(struct.pack("<BHHH", C.MOP_PIXEL, x, y, color & 0xFFFF), device_id)

    def rect(self, x, y, w, h, color, device_id=C.DEVICE_SELF):
        self._send_ops(struct.pack("<BHHHHH", C.MOP_RECT, x, y, w, h, color & 0xFFFF), device_id)

    def hline(self, x, y, length, color, device_id=C.DEVICE_SELF):
        self._send_ops(struct.pack("<BHHHH", C.MOP_HLINE, x, y, length, color & 0xFFFF), device_id)

    @staticmethod
    def rgb565(r, g, b):
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
