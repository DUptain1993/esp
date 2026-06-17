"""Rolling XOR stream cipher — must match firmware/src/core/encryption.*

  session_key = SHARED_SECRET ^ nonce
  per byte:    data[i] ^= (key >> ((i % 4) * 8)) & 0xFF
  per packet:  key = ((key << 1) | (key >> 31)) & 0xFFFFFFFF   (32-bit rotl)

Symmetric: process() both encrypts and decrypts. TX and RX must use
independent Crypto instances kept in lock-step with the peer.
"""

# Must equal CRYPTO_SHARED_SECRET in the firmware.
SHARED_SECRET = 0xA5C3F19E


class Crypto:
    def __init__(self, shared_secret: int = SHARED_SECRET, nonce: int = 0):
        self._secret = shared_secret & 0xFFFFFFFF
        self._key = (self._secret ^ (nonce & 0xFFFFFFFF)) & 0xFFFFFFFF

    def set_nonce(self, nonce: int):
        self._key = (self._secret ^ (nonce & 0xFFFFFFFF)) & 0xFFFFFFFF

    @property
    def key(self) -> int:
        return self._key

    def _evolve(self):
        self._key = ((self._key << 1) | (self._key >> 31)) & 0xFFFFFFFF

    def process(self, data: bytes) -> bytes:
        out = bytearray(data)
        key = self._key
        for i in range(len(out)):
            out[i] ^= (key >> ((i % 4) * 8)) & 0xFF
        self._evolve()
        return bytes(out)
