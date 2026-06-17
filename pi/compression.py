"""Lightweight compression — must match firmware/src/core/compression.*

  RLE:   [count][value] pairs (count 1..255)
  Delta: first byte verbatim, then intra-buffer byte differences

Both are symmetric and bounds-safe. Compression auto-enables for payloads
of at least COMPRESS_THRESHOLD bytes.
"""

COMPRESS_THRESHOLD = 32


def should_compress(n: int) -> bool:
    return n >= COMPRESS_THRESHOLD


def rle_encode(data: bytes) -> bytes:
    out = bytearray()
    i = 0
    n = len(data)
    while i < n:
        val = data[i]
        run = 1
        while i + run < n and data[i + run] == val and run < 255:
            run += 1
        out.append(run)
        out.append(val)
        i += run
    return bytes(out)


def rle_decode(data: bytes) -> bytes:
    if len(data) % 2 != 0:
        raise ValueError("RLE stream must be count/value pairs")
    out = bytearray()
    for i in range(0, len(data), 2):
        out.extend(bytes([data[i + 1]]) * data[i])
    return bytes(out)


def delta_encode(data: bytes) -> bytes:
    if not data:
        return b""
    out = bytearray(len(data))
    out[0] = data[0]
    for i in range(1, len(data)):
        out[i] = (data[i] - data[i - 1]) & 0xFF
    return bytes(out)


def delta_decode(data: bytes) -> bytes:
    if not data:
        return b""
    out = bytearray(len(data))
    out[0] = data[0]
    for i in range(1, len(data)):
        out[i] = (out[i - 1] + data[i]) & 0xFF
    return bytes(out)
