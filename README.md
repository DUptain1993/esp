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

## Controller (Raspberry Pi)
Requires Python 3.11+ (Bookworm recommended).
1. `cd pi`
2. `./install.sh`
3. The service will be managed by systemd as `cyberdeck.service`.

## CI/CD
Automated builds are configured via GitHub Actions in `.github/workflows/build.yml`.
