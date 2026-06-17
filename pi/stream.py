"""Terminal streaming to the ESP32 (CH_STREAM)."""

import subprocess
import logging
from . import commands as C
from .shell import PtyShell

log = logging.getLogger("stream")
CHUNK = 192  # keep framed payload under the 255-byte protocol limit


class StreamHandler:
    def __init__(self, comms):
        self.comms = comms
        self.shell = None

    def ensure_shell(self):
        if not self.shell or not self.shell.is_alive():
            log.info("starting new pty shell")
            self.shell = PtyShell()
        return self.shell

    def shell_write(self, text, device_id=C.DEVICE_SELF):
        s = self.ensure_shell()
        # Append newline if not present for interactive feel? 
        # No, ESP32 keyboard might send it or not.
        if not text.endswith("\n"):
            text += "\n"
        s.write(text.encode("utf-8", "replace"))

    def shell_pump(self, device_id=C.DEVICE_SELF):
        if not self.shell:
            return
        data = self.shell.read()
        if data:
            self.send_bytes(data, device_id)
        if not self.shell.is_alive():
            self.send_ansi(31, "\n[shell terminated]\n", device_id)
            self.shell = None

    def send_bytes(self, data, device_id=C.DEVICE_SELF):
        for i in range(0, len(data), CHUNK):
            chunk = data[i:i + CHUNK]
            self.comms.send(C.CH_STREAM, device_id, bytes([C.STR_DATA]) + chunk)

    def send_text(self, text, device_id=C.DEVICE_SELF):
        self.send_bytes(text.encode("utf-8", "replace"), device_id)

    def send_ansi(self, color_code, text, device_id=C.DEVICE_SELF):
        self.send_text("\033[%dm%s\033[0m" % (color_code, text), device_id)

    def clear(self, device_id=C.DEVICE_SELF):
        self.comms.send(C.CH_STREAM, device_id, bytes([C.STR_CLEAR]), compress=False)

    def run_and_stream(self, cmd, device_id=C.DEVICE_SELF, timeout=20):
        """Execute a shell command and stream its output to the terminal."""
        self.send_ansi(36, "$ %s\n" % cmd, device_id)
        try:
            proc = subprocess.run(
                cmd, shell=True, capture_output=True, text=True, timeout=timeout)
            if proc.stdout:
                self.send_text(proc.stdout, device_id)
            if proc.stderr:
                self.send_ansi(31, proc.stderr, device_id)
            self.send_ansi(32 if proc.returncode == 0 else 31,
                           "\n[exit %d]\n" % proc.returncode, device_id)
        except subprocess.TimeoutExpired:
            self.send_ansi(31, "\n[timeout]\n", device_id)
        except Exception as e:  # noqa: BLE001
            self.send_ansi(31, "\n[error] %s\n" % e, device_id)
