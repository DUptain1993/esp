from .protocol import CH_STREAM

class StreamHandler:
    def __init__(self, comms):
        self.comms = comms

    def send_text(self, text, device_id=1):
        # Break into chunks if too long
        data = text.encode("utf-8")
        for i in range(0, len(data), 200):
            chunk = data[i:i+200]
            self.comms.send_packet(CH_STREAM, device_id, chunk)

    def send_ansi_color(self, color_code, text, device_id=1):
        ansi_text = f"\033[{color_code}m{text}\033[0m"
        self.send_text(ansi_text, device_id)
