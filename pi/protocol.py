"""Binary protocol framing — must match firmware/src/core/protocol.*

Frame: [0xAA][CHANNEL][DEVICE_ID][LEN][PAYLOAD...][CHK][0x55]
Checksum: XOR of CHANNEL + DEVICE_ID + LEN + PAYLOAD bytes.
"""

SOF = 0xAA
EOF = 0x55

MAX_PAYLOAD = 255

# Channels
CH_CONTROL = 0x01
CH_STATUS = 0x02
CH_STREAM = 0x03
CH_OTA = 0x04
CH_ROUTING = 0x05


class Packet:
    __slots__ = ("channel", "device_id", "payload")

    def __init__(self, channel: int, device_id: int, payload: bytes):
        self.channel = channel & 0xFF
        self.device_id = device_id & 0xFF
        self.payload = bytes(payload)

    def __repr__(self):
        return f"Packet(ch=0x{self.channel:02X}, dev=0x{self.device_id:02X}, len={len(self.payload)})"


def checksum(channel: int, device_id: int, length: int, payload: bytes) -> int:
    chk = (channel ^ device_id ^ length) & 0xFF
    for b in payload:
        chk ^= b
    return chk & 0xFF


def build_frame(channel: int, device_id: int, payload: bytes) -> bytes:
    payload = bytes(payload)
    length = len(payload)
    if length > MAX_PAYLOAD:
        raise ValueError("payload exceeds 255 bytes")
    chk = checksum(channel, device_id, length, payload)
    return bytes([SOF, channel & 0xFF, device_id & 0xFF, length]) + payload + bytes([chk, EOF])


class Parser:
    """Streaming, byte-at-a-time state machine matching the firmware parser."""

    S_SOF, S_CH, S_DEV, S_LEN, S_PAYLOAD, S_CHK, S_EOF = range(7)

    def __init__(self):
        self.reset()
        self.good = 0
        self.bad = 0

    def reset(self):
        self._state = self.S_SOF
        self._channel = 0
        self._device = 0
        self._len = 0
        self._payload = bytearray()
        self._chk_calc = 0
        self._chk_recv = 0

    def feed(self, data: bytes):
        """Feed bytes; yields complete validated Packet objects."""
        for b in data:
            pkt = self._feed_byte(b)
            if pkt is not None:
                yield pkt

    def _feed_byte(self, b: int):
        s = self._state
        if s == self.S_SOF:
            if b == SOF:
                self._state = self.S_CH
                self._chk_calc = 0
        elif s == self.S_CH:
            self._channel = b
            self._chk_calc ^= b
            self._state = self.S_DEV
        elif s == self.S_DEV:
            self._device = b
            self._chk_calc ^= b
            self._state = self.S_LEN
        elif s == self.S_LEN:
            self._len = b
            self._chk_calc ^= b
            self._payload = bytearray()
            self._state = self.S_CHK if b == 0 else self.S_PAYLOAD
        elif s == self.S_PAYLOAD:
            self._payload.append(b)
            self._chk_calc ^= b
            if len(self._payload) >= self._len:
                self._state = self.S_CHK
        elif s == self.S_CHK:
            self._chk_recv = b
            self._state = self.S_EOF
        elif s == self.S_EOF:
            if b == EOF and self._chk_recv == self._chk_calc:
                self.good += 1
                pkt = Packet(self._channel, self._device, bytes(self._payload))
                self.reset()
                return pkt
            else:
                self.bad += 1
                self.reset()
                if b == SOF:
                    self._state = self.S_CH
                    self._chk_calc = 0
        return None
