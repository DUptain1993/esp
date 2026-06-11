# ESP32 Touchscreen Terminal System

High-performance modular control terminal with encrypted compressed binary protocol.

## Features
- **Display**: ST7796 320x480 SPI with DMA.
- **UI**: LVGL 8 Cyberpunk Stealth HUD.
- **Security**: Rolling XOR encryption + Challenge-Response handshake.
- **Optimization**: RLE/Delta compression, jitter, noise packets.
- **Apps**: ANSI Terminal, UI Mirroring, OTA Updates.
- **Backend**: Python-based service for Raspberry Pi.

## Hardware Pins
- CS = GPIO15, DC = GPIO2, RST = EN, BL = GPIO27
- SCLK = GPIO14, MOSI = GPIO13, MISO = GPIO12
- Touch: CS = GPIO33, IRQ = GPIO36

## Firmware (ESP32)
Built with PlatformIO.
1. Install PlatformIO.
2. `cd firmware`
3. `pio run -t upload`

## Service (Raspberry Pi)
Requires Python 3.9+.
1. `cd service`
2. `pip install -r requirements.txt`
3. `python3 -m service.main`

## CI/CD
Automated builds are configured via GitHub Actions in `.github/workflows/build.yml`.
