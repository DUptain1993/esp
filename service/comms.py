import serial
import time
import random
import struct
from .protocol import Packet, parse_packet, PKT_START_BYTE
from .encryption import Encryption

class Comms:
    def __init__(self, port="/dev/ttyUSB0", baudrate=115200):
        self.ser = serial.Serial(port, baudrate, timeout=0.1)
        self.enc = Encryption()
        self.buffer = bytearray()
        self.callbacks = {}

    def register_callback(self, channel, cb):
        self.callbacks[channel] = cb

    def send_packet(self, channel, device_id, payload):
        # Padding
        payload = bytearray(payload)
        padding_len = random.randint(1, 4)
        for _ in range(padding_len):
            payload.append(random.randint(0, 255))
        
        # Encrypt
        encrypted_payload = self.enc.process(payload)
        
        pkt = Packet(channel, device_id, encrypted_payload)
        
        # Jitter
        time.sleep(random.uniform(0.015, 0.090))
        
        self.ser.write(pkt.serialize())

    def update(self):
        if self.ser.in_waiting > 0:
            self.buffer.extend(self.ser.read(self.ser.in_waiting))
            
        while len(self.buffer) > 0:
            pkt, consumed = parse_packet(self.buffer)
            if pkt:
                self.buffer = self.buffer[consumed:]
                
                # Decrypt
                decrypted_payload = self.enc.process(pkt.payload)
                pkt.payload = decrypted_payload
                
                if pkt.channel in self.callbacks:
                    self.callbacks[pkt.channel](pkt)
            elif consumed > 0:
                self.buffer = self.buffer[consumed:]
            else:
                break
                
    def handshake(self):
        nonce = random.getrandbits(32)
        payload = struct.pack("<BI", 0xF0, nonce) # CMD_HANDSHAKE
        self.send_packet(0x01, 0, payload)
        
        key = self.enc.derive_key(nonce)
        self.enc = Encryption(key)
        print(f"Handshake sent. Key: {key:08X}")
