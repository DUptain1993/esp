import struct

PKT_START_BYTE = 0xAA
PKT_END_BYTE = 0x55

CH_CONTROL = 0x01
CH_STATUS  = 0x02
CH_STREAM  = 0x03
CH_OTA     = 0x04
CH_ROUTING = 0x05

class Packet:
    def __init__(self, channel, device_id, payload):
        self.channel = channel
        self.device_id = device_id
        self.payload = bytes(payload)
        self.length = len(self.payload)
        self.checksum = self.calculate_checksum()

    def calculate_checksum(self):
        chk = self.channel ^ self.device_id ^ self.length
        for b in self.payload:
            chk ^= b
        return chk & 0xFF

    def serialize(self):
        return struct.pack("BBBB", PKT_START_BYTE, self.channel, self.device_id, self.length) + \
               self.payload + \
               struct.pack("BB", self.checksum, PKT_END_BYTE)

def parse_packet(buffer):
    if len(buffer) < 6: return None, 0
    if buffer[0] != PKT_START_BYTE: return None, 1
    
    channel = buffer[1]
    device_id = buffer[2]
    length = buffer[3]
    
    if len(buffer) < 6 + length: return None, 0
    
    payload = buffer[4:4+length]
    checksum = buffer[4+length]
    end_byte = buffer[5+length]
    
    if end_byte != PKT_END_BYTE: return None, 6 + length
    
    pkt = Packet(channel, device_id, payload)
    if pkt.checksum != checksum: return None, 6 + length
    
    return pkt, 6 + length
