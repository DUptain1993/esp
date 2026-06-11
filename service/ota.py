import struct
import time
from .protocol import CH_OTA

class OTAHandler:
    def __init__(self, comms):
        self.comms = comms

    def update_firmware(self, file_path, device_id=1):
        with open(file_path, "rb") as f:
            data = f.read()
            
        size = len(data)
        print(f"Starting OTA for {file_path} ({size} bytes)")
        
        # CMD_OTA_START (0x08)
        self.comms.send_packet(CH_OTA, device_id, struct.pack("<BI", 0x08, size))
        time.sleep(1) # Wait for ESP32 to prepare
        
        # CMD_OTA_CHUNK (0x09)
        chunk_size = 128
        for i in range(0, size, chunk_size):
            chunk = data[i:i+chunk_size]
            payload = struct.pack("<B", 0x09) + chunk
            self.comms.send_packet(CH_OTA, device_id, payload)
            print(f"Sent chunk {i}/{size}", end="\r")
            
        # CMD_OTA_END (0x0A)
        self.comms.send_packet(CH_OTA, device_id, struct.pack("<B", 0x0A))
        print("\nOTA Finished")
