# Pi Terminal — Marauder CYD 3.5" add-on

An **isolated, upgrade-safe** module that gives the ESP32 Marauder an encrypted /
compressed UART link to a Raspberry Pi Zero W, plus a new on-screen **"Pi
Terminal"** page. Marauder's existing features, menus, scanning, attacks and UI
are untouched — this module only *adds* code and hooks in at three points.

## What it does

1. A new **"Pi Terminal"** entry appears on the Marauder main menu.
2. Opening it shows a scrolling terminal screen with `TYPE CMD` / `EXIT` buttons.
3. `TYPE CMD` opens the on-screen virtual keyboard.
4. The typed command is framed → compressed → encrypted → sent over UART.
5. The Pi executes it and streams stdout/stderr back **live**, line by line.
6. Output renders continuously; `EXIT` returns to Marauder (menu is redrawn).

The link is serviced every main-loop iteration, so the Pi's banner and any
streamed output are captured into the scrollback even while the page is closed.

## Pipeline (matches the Pi `pi/` Python stack byte-for-byte)

```
INPUT → commands → protocol → encryption → compression → UART TX
UART RX → decompression → decryption → protocol parse → output to UI
```

The core modules (`comms`, `protocol`, `encryption`, `compression`,
`commands.h`) are verbatim copies of `firmware/src/core/*` and share the same
`SHARED_SECRET`, frame format and opcodes as `pi/*.py`. No Pi-side changes are
required.

## Wiring (defaults, override with `-D` build flags)

| ESP32 (CYD 3.5") | Pi Zero W |
|------------------|-----------|
| GPIO `PI_UART_TX_PIN` = **26** | UART RX |
| GPIO `PI_UART_RX_PIN` = **35** | UART TX |
| GND | GND |

Pins 26 (out) and 35 (input-only) avoid the panel's TFT / touch / SD / I²C /
GPS / RGB-LED assignments. UART0 stays free for Marauder's USB CLI; the Pi link
uses **UART1 (`Serial1`)**.

## Files

| File | Purpose |
|------|---------|
| `pi_terminal_app.{h,cpp}` | Glue: UART bring-up, message handler, terminal model + UI |
| `VirtualKeyboard.{h,cpp}` | Touch-driven on-screen keyboard |
| `DisplayTFT.h` | `TFT_eSPI` wrapper (native 320×480) |
| `PepeDraw.{h,cpp}` | Text helpers over TFT_eSPI fonts |
| `Pins.h`, `SoundUtils.{h,cpp}` | Pin map + key-click helper |
| `comms`, `protocol`, `encryption`, `compression`, `commands.h` | Core link pipeline |

## Integration hooks (the only edits to Marauder)

- `esp32_marauder.ino`: `pi_terminal_init()` in `setup()`, `pi_terminal_pump()` in `loop()`.
- `MenuFunctions.cpp`: one `addNodes(...)` call adding the "Pi Terminal" main-menu node.
- `EvilPortal.h` / `GpsInterface.cpp`: tiny, behaviour-preserving compile shims
  (duplicate-symbol fixes) so the legacy code links under modern GCC.

## Build

```bash
cd firmware/features_imported/ESP32Marauder-cyd-master
bash scripts/fetch_libs.sh        # one-time: fetch vendored libraries
pio run -e marauder_cyd_3_5       # build
pio run -e marauder_cyd_3_5 -t upload
```
