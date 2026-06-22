"""Distributed Cyberdeck - Raspberry Pi controller service.

Speaks the encrypted/compressed binary protocol to the ESP32-32E frontend
over USB serial. Receives input/commands/status from the touchscreen and
streams terminal output, UI mirror updates and OTA data back.

Run:
    python3 -m pi.main
Environment:
    CYBERDECK_PORT   serial device (default /dev/ttyUSB0)
    CYBERDECK_BAUD   baud rate     (default 115200)
    CYBERDECK_SHELL  "1" to allow inline shell execution (default 1)
"""

import os
import time
import logging
import argparse

from .comms import Comms
from .stream import StreamHandler
from .mirror import MirrorHandler
from .ota import OTAHandler
from .device_manager import DeviceManager
from .dispatcher import Dispatcher, Config
from . import commands as C

PI_DEVICE_ID = 0x02  # this controller's address on the bus


def build_banner():
    try:
        with open("/proc/device-tree/model", "r") as f:
            model = f.read().strip("\x00").strip()
    except Exception:
        model = "Raspberry Pi"
    return ("\033[36mCYBERDECK CONTROLLER ONLINE\033[0m\n"
            "host: %s\n"
            "link: encrypted + compressed\n"
            "ready for commands\n" % model)


def main():
    ap = argparse.ArgumentParser(description="Cyberdeck Pi controller")
    ap.add_argument("--port", default=os.environ.get("CYBERDECK_PORT", "/dev/ttyUSB0"))
    ap.add_argument("--baud", type=int, default=int(os.environ.get("CYBERDECK_BAUD", "115200")))
    ap.add_argument("--ota", metavar="FIRMWARE.BIN", help="push a firmware image then exit")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(name)-8s %(levelname)s %(message)s",
        datefmt="%H:%M:%S")
    log = logging.getLogger("main")

    comms = Comms(port=args.port, baud=args.baud, obfuscate=True)
    stream = StreamHandler(comms)
    mirror = MirrorHandler(comms)
    ota = OTAHandler(comms)
    devices = DeviceManager(comms)
    cfg = Config(allow_shell=os.environ.get("CYBERDECK_SHELL", "1") == "1")
    dispatcher = Dispatcher(comms, stream, mirror, ota, devices, cfg)
    comms.set_handler(dispatcher.handle)

    log.info("connected on %s @ %d baud", args.port, args.baud)

    # Establish the encrypted session.
    comms.handshake()
    time.sleep(0.3)

    # One-shot OTA mode.
    if args.ota:
        for _ in range(50):
            comms.update()
            time.sleep(0.02)
        ota.push_firmware(args.ota, device_id=C.DEVICE_SELF)
        comms.close()
        return

    # Greet the terminal and select terminal mirror mode.
    stream.send_text(build_banner(), device_id=C.DEVICE_SELF)
    mirror.set_mode(C.MIRROR_TERMINAL, device_id=C.DEVICE_SELF)

    last_heartbeat = 0.0
    last_expire = 0.0
    last_handshake = 0.0
    try:
        while True:
            comms.update()
            stream.shell_pump()

            now = time.time()
            if len(devices.devices) == 0:
                dispatcher.link_established = False

            if not dispatcher.link_established:
                if now - last_handshake >= 2.0:
                    last_handshake = now
                    comms.handshake()
            else:
                if now - last_heartbeat >= 5.0:
                    last_heartbeat = now
                    comms.send_cmd(C.CH_STATUS, PI_DEVICE_ID, C.ST_HEARTBEAT)

            if now - last_expire >= 10.0:
                last_expire = now
                devices.expire()

            time.sleep(0.005)
    except KeyboardInterrupt:
        log.info("shutting down")
    finally:
        comms.close()


if __name__ == "__main__":
    main()
