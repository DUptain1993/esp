import os
import pty
import subprocess
import select
import threading
import time

class PtyShell:
    def __init__(self, command=["/bin/bash"]):
        self.master_fd, self.slave_fd = pty.openpty()
        self.process = subprocess.Popen(
            command,
            stdin=self.slave_fd,
            stdout=self.slave_fd,
            stderr=self.slave_fd,
            preexec_fn=os.setsid,
            env={**os.environ, "TERM": "linux", "COLUMNS": "40", "LINES": "25"}
        )
        self.lock = threading.Lock()
        self.output_queue = b""

    def write(self, data: bytes):
        if self.process.poll() is None:
            os.write(self.master_fd, data)

    def read(self, timeout=0.1) -> bytes:
        r, _, _ = select.select([self.master_fd], [], [], timeout)
        if r:
            try:
                data = os.read(self.master_fd, 1024)
                return data
            except OSError:
                pass
        return b""

    def is_alive(self):
        return self.process.poll() is None

    def close(self):
        if self.process:
            self.process.terminate()
            os.close(self.master_fd)
            os.close(self.slave_fd)
