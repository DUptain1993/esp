def rle_encode(data):
    if not data: return b""
    res = bytearray()
    i = 0
    while i < len(data):
        count = 1
        while i + count < len(data) and data[i+count] == data[i] and count < 255:
            count += 1
        res.append(count)
        res.append(data[i])
        i += count
    return bytes(res)

def rle_decode(data):
    res = bytearray()
    for i in range(0, len(data), 2):
        count = data[i]
        val = data[i+1]
        res.extend([val] * count)
    return bytes(res)

class DeltaCompression:
    def __init__(self, size):
        self.prev = bytearray(size)

    def encode(self, data):
        res = bytearray()
        for i in range(len(data)):
            val = data[i]
            res.append((val - self.prev[i]) & 0xFF)
            self.prev[i] = val
        return bytes(res)

    def decode(self, data):
        res = bytearray()
        for i in range(len(data)):
            val = (data[i] + self.prev[i]) & 0xFF
            res.append(val)
            self.prev[i] = val
        return bytes(res)
