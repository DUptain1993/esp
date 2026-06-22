# ESP32 Marauder Cyberdeck Terminal System (CYD 3.5")

A high-performance modular command terminal built on the ESP32 Marauder framework (CYD 3.5-inch board) communicating with a Raspberry Pi Zero W backend over a secure, self-healing, encrypted/compressed UART connection.

---

## Key Features

* **Display & Graphics**: ST7796 320x480 TFT display using direct HSPI IOMUX connections.
* **Calibrated Touch Input**: Custom XPT2046 touch digitizer mapping that bypasses standard `TFT_eSPI` calibration errors.
* **Landscape Touch Alignment**: Fixed touch scaling for all Marauder scans and attack screens when rotated to landscape mode.
* **Secure UART Transport**: Custom protocol featuring:
  * **Rolling XOR stream cipher** using a 32-bit rolling key (Shared Secret: `0xA5C3F19E`).
  * **Dual compression pipeline**: Delta coding combined with Run-Length Encoding (RLE) to maximize throughput.
  * **Background Obfuscation**: Jittered noise packets sent during idle times to mask command traffic.
* **Self-Healing Connection**:
  * Unencrypted handshake packets prevent rolling cipher key alignment deadlocks if devices reboot out of order.
  * Automatic retry loop: The Pi re-broadcasts the handshake every 2 seconds if the client connection drops or times out.
* **Automated OS Setup**: Setup scripts to configure headless Raspberry Pi OS (Bookworm) booting from a USB thumbdrive with Wi-Fi, SSH, and UART console mappings.

---

## Hardware Pin Connections

Connect the ESP32 CYD 3.5" to the Raspberry Pi Zero W GPIO pins:

| ESP32 CYD 3.5" Pin | Raspberry Pi GPIO Pin | Description |
| :--- | :--- | :--- |
| **GPIO 26** (TX) | **GPIO 15** / Pin 10 (RXD) | ESP32 Transmit $\rightarrow$ Pi Receive |
| **GPIO 35** (RX) | **GPIO 14** / Pin 8 (TXD) | ESP32 Receive $\leftarrow$ Pi Transmit |
| **GND** | **GND** / Pin 6 (Ground) | Shared ground reference |

*Note: A shared ground is mandatory. Do not connect power rails if both boards have independent power supplies.*

---

## Setup & Installation

### 1. ESP32 Firmware Compilation & Flashing
The firmware targets the CYD 3.5" layout and compiles with PlatformIO:
1. Ensure PlatformIO Core is installed on your computer.
2. Navigate to the ESP32 Marauder repository root:
   ```bash
   cd firmware/features_imported/ESP32Marauder-cyd-master
   ```
3. Compile and upload the binary:
   ```bash
   pio run -t upload
   ```

### 2. Raspberry Pi OS Controller Configuration
The Pi controller operates as a background systemd service (`cyberdeck.service`) running inside a Python virtual environment.

#### Automated Configuration:
1. Boot your Raspberry Pi from your SD/USB drive and copy the `pi` folder to the system.
2. Open a shell on the Pi, navigate to the `pi` folder, and execute the configuration helper:
   ```bash
   cd pi/
   sudo ./setup_pi.sh
   ```
   
#### What the script configures:
* **UART Configuration**: Sets `enable_uart=1` and `dtoverlay=disable-bt` in `/boot/firmware/config.txt` to allocate the PL011 hardware UART (`/dev/ttyAMA0`/`serial0`) to pins 8/10.
* **Console Disabling**: Strips `console=serial0` console loggers from `/boot/firmware/cmdline.txt` and disables/masks the conflicting systemd `serial-getty` services.
* **SSH Activation**: Activates and starts the SSH server.
* **Wi-Fi Profiles**: Configures NetworkManager to automatically connect to your local network.
* **Systemd Service**: Resolves python dependencies in a virtual environment (`venv`) and starts the `cyberdeck.service` to poll `/dev/serial0` at `115200` baud.

3. **Reboot the Pi** to apply the serial changes:
   ```bash
   sudo reboot
   ```

---

## Usage

1. Open the **"Pi Terminal"** screen on the ESP32 touch interface.
2. The Pi will automatically handshake and establish an encrypted session. The indicator in the top-right corner will update to **`LINK OK`**.
3. Tap **"TYPE CMD"** in the footer to open the virtual keyboard.
4. Enter shell commands (e.g. `ls -la`, `ifconfig`, `uptime`). Press **OK** to send them over the UART.
5. Command outputs stream back to the ESP32 display in real-time. Use the **DEL** button as a backspace to correct typed commands.
