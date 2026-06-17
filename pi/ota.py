"""Push firmware updates to the ESP32 (CH_OTA).

Matches firmware/src/app/ota.cpp:
  OTA_BEGIN [size:4]
  OTA_CHUNK [seq:2][data...]
  OTA_END   [size:4]
"""

import os
import time
import struct
import logging

from . import commands as C

log = logging.getLogger("ota")

CHUNK = 192  # data bytes per chunk (keeps framed payload < 255)


class OTAHandler:
    def __init__(self, comms):
        self.comms = comms
        self.active = False
        self.progress = 0
        self.failed = False
        self.done = False

    def handle(self, device_id, msg):
        cmd = msg[0]
        if cmd == C.OTA_PROGRESS:
            self.progress = msg[1]
            log.info("OTA progress: %d%%", self.progress)
        elif cmd == C.OTA_DONE:
            log.info("OTA confirmed success")
            self.done = True
            self.active = False
        elif cmd == C.OTA_FAIL:
            log.error("OTA failed on device")
            self.failed = True
            self.active = False

    def push_firmware(self, path, device_id=C.DEVICE_SELF, chunk_delay=0.01):
        size = os.path.getsize(path)
        log.info("OTA push %s (%d bytes) -> dev 0x%02X", path, size, device_id)

        # OTA is latency-sensitive and high-volume: disable jitter/noise.
        prev_obf = self.comms.obfuscate
        self.comms.obfuscate = False
        try:
            self.comms.send(C.CH_OTA, device_id,
                            bytes([C.OTA_BEGIN]) + struct.pack("<I", size), compress=False)
            time.sleep(0.5)

            seq = 0
            with open(path, "rb") as f:
                while True:
                    data = f.read(CHUNK)
                    if not data:
                        break
                    msg = bytes([C.OTA_CHUNK]) + struct.pack("<H", seq) + data
                    self.comms.send(C.CH_OTA, device_id, msg, compress=False)
                    seq += 1
                    if chunk_delay:
                        time.sleep(chunk_delay)

            self.comms.send(C.CH_OTA, device_id,
                            bytes([C.OTA_END]) + struct.pack("<I", size), compress=False)
            log.info("OTA complete: %d chunks", seq)
        finally:
            self.comms.obfuscate = prev_obf

    def abort(self, device_id=C.DEVICE_SELF):
        self.comms.send(C.CH_OTA, device_id, bytes([C.OTA_ABORT]), compress=False)
