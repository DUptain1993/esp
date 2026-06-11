Build a complete ESP32-based touchscreen control system using LVGL 8 for a 320x480 ST7796 SPI display (E32R35T with ESP32-WROOM-32E), designed as a high-performance modular control terminal communicating with one or more Raspberry Pi devices.

This must be a FULL implementation, not partial code.

----------------------------------------
1. HARDWARE + DISPLAY CONFIG
----------------------------------------

Display:
- Driver: ST7796
- Resolution: 320x480
- Color format: RGB565
- SPI speed: 40MHz (DMA enabled)

Pins:
- CS = GPIO15
- DC = GPIO2
- RST tied to EN (no dedicated pin)
- Backlight = GPIO27

SPI:
- SCLK = GPIO14
- MOSI = GPIO13
- MISO = GPIO12

Touch:
- Controller: XPT2046
- CS = GPIO33
- IRQ = GPIO36

----------------------------------------
2. LVGL UI SYSTEM (CYBERPUNK STEALTH HUD)
----------------------------------------

Theme:
- Background: #0A0A0F
- Panel: #12121A
- Accent: #00FFD1
- Warning: #FF3B3B
- Text: #E0E0E0
- Dim: #606060

Requirements:
- No gradients, shadows, or transparency
- Static styles only
- Minimize redraw regions

Screens:
1. Main Menu
   - USB
   - WiFi
   - System
   - Settings

2. Submenus:
   - Grid-aligned rectangular buttons
   - Minimal labels

3. Status HUD:
   - Bottom bar
   - CPU indicator
   - Connection status
   - Active device label

----------------------------------------
3. UI INTERACTION MODEL
----------------------------------------

Gestures:
- Swipe LEFT → previous screen
- Swipe RIGHT → next screen
- Swipe UP → quick action
- Long press → execute command

Use:
- LV_EVENT_GESTURE
- LV_EVENT_LONG_PRESSED

Additional:
- Idle timeout dimming
- Tap-to-wake

----------------------------------------
4. PERFORMANCE OPTIMIZATION (REQUIRED)
----------------------------------------

Must include:
- LV_COLOR_DEPTH = 16
- Double buffering (320x40 buffers)
- DMA SPI transfers
- Partial refresh only

Avoid:
- Full redraws
- Blocking delays
- Expensive UI effects

Loop:
- Non-blocking lv_timer_handler()
- ~5ms tick

Must run smoothly at 40MHz SPI.

----------------------------------------
5. FULL BINARY PROTOCOL (MULTI-CHANNEL + MULTI-DEVICE)
----------------------------------------

Packet:
[0xAA][CHANNEL][DEVICE_ID][LEN][PAYLOAD][CHK][0x55]

Channels:
0x01 = Control
0x02 = Status
0x03 = Stream
0x04 = OTA
0x05 = Routing

Payload:
- First byte = Command ID
- Remaining bytes = arguments

Checksum:
- XOR over CHANNEL + DEVICE_ID + LEN + PAYLOAD

----------------------------------------
6. BINARY PROTOCOL COMPRESSION (BANDWIDTH STEALTH)
----------------------------------------

Implement:
- Variable-length encoding
- Run-length encoding (RLE)
- Delta compression for repeated data

Requirements:
- Lightweight (ESP32-friendly)
- Automatic enable based on payload size
- Symmetric encode/decode

----------------------------------------
7. ENCRYPTION + SECURE KEY EXCHANGE
----------------------------------------

Implement:

1. Session key exchange:
   - Challenge-response handshake
   - Derive key from shared secret + nonce

2. Encryption:
   - Rolling XOR using session key
   - Key evolves per packet

3. Obfuscation:
   - Random padding (1–4 bytes)
   - Packet jitter (15–90ms)
   - Noise packets (CMD=0x00)

----------------------------------------
8. LIVE STREAMING TERMINAL (ANSI)
----------------------------------------

Channel: 0x03

Features:
- Real-time terminal view
- ANSI escape parsing:
    - Colors
    - Cursor movement
    - Line control
- Scrollable LVGL text area
- Ring buffer
- Auto-scroll + pause

----------------------------------------
9. REMOTE UI MIRRORING
----------------------------------------

Implement second stream mode:

Pi sends:
- Compressed framebuffer diffs OR
- Draw commands (rect/text/pixels)

ESP32:
- LVGL canvas rendering
- Delta updates only

Switchable modes:
- Terminal mode
- Graphics mirror mode

----------------------------------------
10. OTA UPDATE SYSTEM
----------------------------------------

Channel: 0x04

Features:
- Chunked firmware uploads
- Integrity verification
- Safe update with rollback

UI:
- Progress bar
- Status messages
- Auto reboot

----------------------------------------
11. MULTI-DEVICE CONTROL HUB
----------------------------------------

Implement:
- DEVICE_ID routing
- Active device selection UI
- Device registry

Features:
- Broadcast commands
- Per-device status
- Visual active selection

----------------------------------------
12. COMMAND SYSTEM
----------------------------------------

Compact IDs:

Examples:
0x01 = Navigation
0x02 = Execute action
0x03 = Status request
0x04 = Start stream
0x05 = Stop stream

Requirements:
- No strings
- Stateless parsing
- Fast execution

----------------------------------------
13. PI-SIDE IMPLEMENTATION (PYTHON)
----------------------------------------

Provide Python service:

- Packet receive (UART/WiFi)
- Decrypt + decompress
- Protocol parsing
- DEVICE_ID routing
- Dispatcher

Include:
- Terminal stream handler
- UI mirror handler
- OTA handler
- Device manager

Structure:
- comms.py
- protocol.py
- encryption.py
- compression.py
- dispatcher.py
- stream.py
- mirror.py
- ota.py
- device_manager.py

----------------------------------------
14. ESP32 CODE STRUCTURE
----------------------------------------

Modules:

- display.c
- ui.c
- theme.c
- touch.c
- gestures.c
- comms.c
- protocol.c
- encryption.c
- compression.c
- stream.c
- mirror.c
- ota.c
- device_manager.c
- main.cpp

----------------------------------------
15. GITHUB ACTIONS CI/CD WORKFLOW
----------------------------------------

Provide a complete GitHub Actions workflow to:

- Automatically build firmware on push
- Support ESP32 (Arduino or ESP-IDF)
- Cache dependencies for faster builds
- Output compiled firmware binaries (.bin)
- Upload artifacts

Include:

- .github/workflows/build.yml
- Steps:
  - Checkout repo
  - Setup toolchain (ESP-IDF or Arduino CLI)
  - Install dependencies
  - Compile project
  - Store build outputs

Optional:
- Tag-based release build
- Firmware artifact naming with version

----------------------------------------
16. OUTPUT REQUIREMENTS
----------------------------------------

- Complete compilable codebase
- Modular structure
- No placeholders
- Clean comments
- ESP32 compatible

----------------------------------------
17. SAFETY REQUIREMENT
----------------------------------------

Do NOT include harmful payloads or exploit scripts.

Provide only:
- UI system
- Communication framework
- Control infrastructure
- Extensible architecture

----------------------------------------

Goal:
A high-performance, stealthy, modular ESP32 touchscreen terminal with encrypted compressed binary protocol, secure key exchange, OTA updates, live ANSI terminal streaming, remote UI mirroring, multi-device control, and GitHub Actions-based automated firmware build pipeline.
