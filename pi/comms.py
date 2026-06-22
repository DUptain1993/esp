"""Communications layer — mirrors firmware/src/core/comms.*

Wire payload layout (inside the protocol PAYLOAD field):
    [FLAGS][MSG_LEN][PAD_LEN][BODY...]
BODY = encrypt(compress(msg + random_padding)).

TX and RX use independent cipher streams that stay in lock-step with the
firmware as long as the link is reliable (USB serial). A handshake re-seeds
both streams and doubles as a resync point.
"""

import os
import time
import struct
import random
import logging

import serial

from . import protocol
from . import compression
from . import commands as C
from .encryption import Crypto, SHARED_SECRET

log = logging.getLogger("comms")


class Comms:
    def __init__(self, port="/dev/ttyUSB0", baud=115200, obfuscate=True):
        self.ser = serial.Serial(port, baud, timeout=0)
        self.parser = protocol.Parser()
        self.tx = Crypto(SHARED_SECRET, 0)
        self.rx = Crypto(SHARED_SECRET, 0)
        self.obfuscate = obfuscate
        self.handler = None          # callable(channel, device_id, msg: bytes)
        self._last_noise = time.time()
        self._noise_interval = 4.0

    # -- setup -------------------------------------------------------------
    def set_handler(self, fn):
        self.handler = fn

    def close(self):
        try:
            self.ser.close()
        except Exception:
            pass

    # -- send --------------------------------------------------------------
    def send(self, channel, device_id, msg, encrypt=True, compress=True, jitter=False):
        msg = bytes(msg)
        pad_len = random.randint(1, 4) if self.obfuscate else 0
        body = bytearray(msg) + bytes(random.getrandbits(8) for _ in range(pad_len))
        total = len(body)

        flags = 0
        if compress and compression.should_compress(total):
            d = compression.delta_encode(bytes(body))
            r = compression.rle_encode(d)
            if len(r) < total:
                body = bytearray(r)
                flags |= (C.FLAG_DELTA | C.FLAG_RLE)

        if encrypt:
            body = bytearray(self.tx.process(bytes(body)))
            flags |= C.FLAG_ENC

        payload = bytes([flags, len(msg) & 0xFF, pad_len & 0xFF]) + bytes(body)
        if len(payload) > protocol.MAX_PAYLOAD:
            raise ValueError("framed payload exceeds 255 bytes; chunk the message")

        frame = protocol.build_frame(channel, device_id, payload)
        if jitter and self.obfuscate:
            time.sleep(random.uniform(0.015, 0.090))
        self.ser.write(frame)

    def send_cmd(self, channel, device_id, cmd, encrypt=True):
        self.send(channel, device_id, bytes([cmd]), encrypt=encrypt, compress=False)

    # -- handshake / resync ------------------------------------------------
    def handshake(self, nonce=None):
        """Re-seed both cipher streams and notify the peer.

        Sent with the current (pre-reseed) TX key so the firmware decodes it
        with its matching RX key, then both ends reset to the new nonce.
        """
        if nonce is None:
            nonce = random.getrandbits(32)
        # Flush stale in-flight bytes so we don't misdecode pre-handshake data.
        try:
            self.ser.reset_input_buffer()
        except Exception:
            pass
        self.parser.reset()
        msg = bytes([C.CMD_HANDSHAKE]) + struct.pack("<I", nonce)
        self.send(C.CH_CONTROL, C.DEVICE_SELF, msg, encrypt=False, compress=False)
        self.tx.set_nonce(nonce)
        self.rx.set_nonce(nonce)
        log.info("handshake sent, nonce=0x%08X key=0x%08X", nonce, self.tx.key)
        return nonce

    # -- receive -----------------------------------------------------------
    def update(self):
        try:
            waiting = self.ser.in_waiting
        except Exception:
            waiting = 0
        if waiting:
            data = self.ser.read(waiting)
            for pkt in self.parser.feed(data):
                self._dispatch(pkt)

        if self.obfuscate:
            now = time.time()
            if now - self._last_noise >= self._noise_interval:
                self._last_noise = now
                noise = bytes([C.CMD_NOISE]) + bytes(
                    random.getrandbits(8) for _ in range(random.randint(1, 4)))
                self.send(C.CH_CONTROL, C.DEVICE_BROADCAST, noise,
                          encrypt=True, compress=False, jitter=True)
                self._noise_interval = random.uniform(3.0, 7.0)

    def _dispatch(self, pkt):
        payload = pkt.payload
        if len(payload) < 3:
            return
        flags, msg_len, pad_len = payload[0], payload[1], payload[2]
        body = bytearray(payload[3:])

        if flags & C.FLAG_ENC:
            body = bytearray(self.rx.process(bytes(body)))

        if flags & C.FLAG_RLE:
            try:
                tmp = compression.rle_decode(bytes(body))
            except ValueError:
                return
            plain = compression.delta_decode(tmp) if (flags & C.FLAG_DELTA) else tmp
        elif flags & C.FLAG_DELTA:
            plain = compression.delta_decode(bytes(body))
        else:
            plain = bytes(body)

        if len(plain) < msg_len:
            return
        msg = plain[:msg_len]

        # Drop obfuscation noise.
        if pkt.channel == C.CH_CONTROL and msg and msg[0] == C.CMD_NOISE:
            return

        if self.handler:
            self.handler(pkt.channel, pkt.device_id, msg)

    # -- diagnostics -------------------------------------------------------
    @property
    def rx_good(self):
        return self.parser.good

    @property
    def rx_bad(self):
        return self.parser.bad
