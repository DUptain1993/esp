import time
from .comms import Comms
from .stream import StreamHandler
from .mirror import MirrorHandler
from .ota import OTAHandler
from .protocol import CH_STATUS

def on_status(pkt):
    print(f"Status from Device {pkt.device_id}: {pkt.payload.hex()}")

def main():
    comms = Comms(port="/dev/ttyUSB0") # Adjust port as needed
    comms.register_callback(CH_STATUS, on_status)
    
    stream = StreamHandler(comms)
    mirror = MirrorHandler(comms)
    ota = OTAHandler(comms)
    
    print("ESP32 Terminal Service Started")
    
    # Initial Handshake
    comms.handshake()
    
    count = 0
    try:
        while True:
            comms.update()
            
            # Example: Send some test data
            if count % 100 == 0:
                stream.send_ansi_color(32, f"System Pulse: {count}")
                mirror.draw_rect(10, 10, 50, 50, 0x00FF, device_id=1)
                
            count += 1
            time.sleep(0.01)
    except KeyboardInterrupt:
        print("Service Stopped")

if __name__ == "__main__":
    main()
