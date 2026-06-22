### 1. PI-OS AUTOMATED INSTALLATION
  Follow these steps to configure your Raspberry Pi OS thumbdrive:
  1. Boot your Pi:
  Power on your Raspberry Pi Zero W with the SD card (containing 
  bootcode.bin ) and your OS USB thumbdrive inserted, along with a
  monitor/keyboard (or via your existing network link if it's already
  connected).
  2. Copy the script to the Pi:
  Move the folder  /home/mrfkry/esp/pi  (including setup_pi.sh,
  comms.py, main.py, etc.) onto the Pi's file system.
  3. Run the Setup Script:
  Open a terminal on the Pi and execute:
    cd /path/to/copied/pi/folder
    sudo ./setup_pi.sh
    

  #### What this script does automatically:

  • Boot Config: Adjusts  /boot/firmware/config.txt  (or  /boot/config.
  txt ) to add  enable_uart=1  and  dtoverlay=disable-bt  (disabling
  Bluetooth to dedicate the high-quality PL011 hardware UART 
  /dev/ttyAMA0  to pins 8 & 10).
  • Console Getty: Edits  /boot/firmware/cmdline.txt  (or 
  /boot/cmdline.txt ) to strip the kernel serial console mappings,
  stops/disables the serial getty log-in service, and masks them so
  systemd releases  /dev/serial0 .
  • SSH Enable: Enables and starts the system SSH daemon.
  • Wi-Fi Association: Uses NetworkManager ( nmcli ) to connect to your
  Wi-Fi network  SETUP-5188  with the password  MaggotMan88! .
  • Cyberdeck Daemon: Invokes the  install.sh  routine to compile the
  virtual environment, link dependencies, template the systemd unit
  using  /dev/serial0  at 115200 baud, register it with systemd, and
  start the service.

  4. Reboot the Pi:
  Once the script completes, reboot the Pi to apply the kernel and UART
  changes:
    sudo reboot
    
  ──────
  ### 2. FLASH ESP32 FIRMWARE

  To flash the ESP32 CYD 3.5" firmware with the correct menu
  configurations, run these commands on your host computer:

    cd /home/mrfkry/esp/firmware/features_imported/ESP32Marauder-cyd-
  master
    pio run -t upload
    ──────
  ### 3. HARDWARE CONNECTION

  Once both devices are configured, wire them together:

   ESP32 CYD 3.5" Pin | Raspberry Pi Pin      | Connection Description
  --------------------|-----------------------|------------------------
   GPIO 26 (TX)       | GPIO 15 / Pin 10 (RX) | ESP32 Transmit → Pi
                      |                       | Receive
   GPIO 35 (RX)       | GPIO 14 / Pin 8 (TX)  | ESP32 Receive ← Pi
                      |                       | Transmit
   GND                | GND / Pin 6 (Ground)  | Shared ground
                      |                       | reference

  │ [!IMPORTANT]
  │ A common ground (GND) is required. Do not connect the 5V or 3.3V
  │ power lines between the ESP32 and the Pi if they are powered by
  │ separate power sources (e.g., ESP32 connected to your PC's USB and
  │ Pi running off a wall adapter).
  ──────
  ### 4. LIVE INTERACTION SUMMARY

  1. Turn on the Pi and open the "Pi Terminal" screen on the ESP32
  Marauder.
  2. The Pi automatically runs the self-healing connection loop. It
  will send a cleartext handshake over  /dev/serial0  once every 2
  seconds.
  3. The ESP32 catches the handshake, synchronizes rolling XOR keys,
  and returns an ACK.
  4. The link status switches to LINK OK. You can now press "TYPE CMD"
  to execute commands on the Pi interactively, streaming output back to
  the ESP32 screen in real-time.
