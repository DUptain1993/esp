SHARED_SECRET = 0xDEADC0DE

class Encryption:
    def __init__(self, session_key=0):
        self.current_key = session_key

    def derive_key(self, nonce):
        return SHARED_SECRET ^ nonce

    def process(self, data):
        if self.current_key == 0:
            return data
            
        data = bytearray(data)
        for i in range(len(data)):
            key_byte = (self.current_key >> ((i % 4) * 8)) & 0xFF
            data[i] ^= key_byte
            
        # Evolve key
        self.current_key = (self.current_key * 1103515245 + 12345) & 0xFFFFFFFF
        return bytes(data)
