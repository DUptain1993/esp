"""Routes decoded packets from the ESP32 to controller actions."""

import logging

from . import commands as C

log = logging.getLogger("dispatch")


class Config:
    def __init__(self, allow_shell=True,
                 quick_command="uptime",
                 execute_command="uname -a; uptime"):
        self.allow_shell = allow_shell
        self.quick_command = quick_command
        self.execute_command = execute_command


class Dispatcher:
    def __init__(self, comms, stream, mirror, ota, devices, config=None):
        self.comms = comms
        self.stream = stream
        self.mirror = mirror
        self.ota = ota
        self.devices = devices
        self.config = config or Config()
        self.link_established = False

    def handle(self, channel, device_id, msg):
        if not msg:
            return
        if self.devices.seen(device_id):
            log.info("new device 0x%02X", device_id)

        if channel == C.CH_CONTROL:
            self._control(device_id, msg)
        elif channel == C.CH_STATUS:
            self._status(device_id, msg)
        elif channel == C.CH_STREAM:
            log.debug("inbound stream from 0x%02X: %r", device_id, msg[:32])
        elif channel == C.CH_OTA:
            self.ota.handle(device_id, msg)
        elif channel == C.CH_ROUTING:
            self._routing(device_id, msg)

    # -- per-channel handlers ---------------------------------------------
    def _control(self, device_id, msg):
        cmd = msg[0]
        if cmd == C.CMD_PONG:
            if not self.link_established:
                self.link_established = True
                log.info("link established (PONG)")
        elif cmd == C.CMD_PING:
            self.comms.send_cmd(C.CH_CONTROL, device_id, C.CMD_PONG)
        elif cmd == C.CMD_QUICK_ACTION:
            log.info("quick action -> %s", self.config.quick_command)
            if self.link_established:
                self.stream.run_and_stream(self.config.quick_command, device_id)
        elif cmd == C.CMD_EXECUTE:
            self._execute(device_id, msg)
        elif cmd == C.CMD_INPUT:
            if self.config.allow_shell:
                text = msg[1:].decode("utf-8", "replace")
                self.stream.shell_write(text, device_id)
        elif cmd == C.CMD_PAGE_NEXT:
            log.debug("page next")
        elif cmd == C.CMD_PAGE_PREV:
            log.debug("page prev")

    def _execute(self, device_id, msg):
        if not self.link_established:
            return
        # Optional inline command string after the opcode.
        if len(msg) > 1 and self.config.allow_shell:
            command = msg[1:].decode("utf-8", "replace").strip()
        else:
            command = self.config.execute_command
        log.info("execute -> %s", command)
        self.stream.run_and_stream(command, device_id)

    def _status(self, device_id, msg):
        cmd = msg[0]
        if cmd == C.ST_HEARTBEAT:
            log.debug("heartbeat from 0x%02X", device_id)
        elif cmd == C.ST_INFO:
            log.info("info from 0x%02X: %r", device_id, msg[1:])

    def _routing(self, device_id, msg):
        cmd = msg[0]
        if cmd == C.RT_LIST:
            self.devices.send_list_to(self.stream, device_id)
        elif cmd == C.RT_SELECT and len(msg) >= 2:
            self.devices.select(msg[1])
            log.info("active device -> 0x%02X", msg[1])
