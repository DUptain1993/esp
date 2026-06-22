#!/bin/bash
# ==============================================================================
#  Raspberry Pi Automated Setup Script for Cyberdeck UART link
#  Run this script on the Pi (via SSH, local terminal, or chroot)
#  Usage: sudo bash setup_pi.sh
# ==============================================================================

set -e

# Ensure script is run as root
if [ "$EUID" -ne 0 ]; then
  echo "[-] Please run this script as root (e.g., sudo bash setup_pi.sh)"
  exit 1
fi

echo "[+] Starting automated Pi Cyberdeck UART configuration..."

# 1. Determine boot partition path (Bookworm uses /boot/firmware, older use /boot)
BOOT_DIR="/boot"
if [ -d "/boot/firmware" ]; then
  BOOT_DIR="/boot/firmware"
fi
echo "[+] Using boot configuration directory: $BOOT_DIR"

CONFIG_FILE="$BOOT_DIR/config.txt"
CMDLINE_FILE="$BOOT_DIR/cmdline.txt"

# 2. Configure config.txt for UART & disable Bluetooth
echo "[+] Configuring config.txt for UART..."

# Backup config.txt
cp "$CONFIG_FILE" "${CONFIG_FILE}.bak"

# Add enable_uart=1 if not present
if ! grep -q "^enable_uart=1" "$CONFIG_FILE"; then
  echo "enable_uart=1" >> "$CONFIG_FILE"
  echo "[+] Added enable_uart=1 to config.txt"
else
  echo "[*] enable_uart=1 already present in config.txt"
fi

# Add dtoverlay=disable-bt if not present
if ! grep -q "^dtoverlay=disable-bt" "$CONFIG_FILE"; then
  echo "dtoverlay=disable-bt" >> "$CONFIG_FILE"
  echo "[+] Added dtoverlay=disable-bt to config.txt"
else
  echo "[*] dtoverlay=disable-bt already present in config.txt"
fi

# 3. Disable serial login console in cmdline.txt
echo "[+] Disabling serial getty console in cmdline.txt..."

# Backup cmdline.txt
cp "$CMDLINE_FILE" "${CMDLINE_FILE}.bak"

# Strip console=serial0,115200 and console=ttyAMA0,115200 from cmdline
# We use sed to edit the single line in place
sed -i 's/console=serial0,[0-9]*\s*//g' "$CMDLINE_FILE"
sed -i 's/console=ttyAMA0,[0-9]*\s*//g' "$CMDLINE_FILE"
echo "[+] Cleaned cmdline.txt console mappings"

# 4. Stop and Disable getty services in systemd
echo "[+] Disabling systemd serial getty services..."
systemctl stop serial-getty@ttyAMA0.service 2>/dev/null || true
systemctl disable serial-getty@ttyAMA0.service 2>/dev/null || true
systemctl mask serial-getty@ttyAMA0.service 2>/dev/null || true

systemctl stop serial-getty@serial0.service 2>/dev/null || true
systemctl disable serial-getty@serial0.service 2>/dev/null || true
systemctl mask serial-getty@serial0.service 2>/dev/null || true
echo "[+] Serial getty services disabled and masked"

# 5. Enable SSH service
echo "[+] Enabling SSH daemon..."
systemctl enable ssh 2>/dev/null || true
systemctl start ssh 2>/dev/null || true
echo "[+] SSH service enabled"

# 6. Configure Wi-Fi using NetworkManager (if available)
if command -v nmcli >/dev/null 2>&1; then
  echo "[+] Connecting to Wi-Fi network 'SETUP-5188' using NetworkManager..."
  nmcli device wifi connect "SETUP-5188" password "MaggotMan88!" || {
    echo "[-] Warning: Wi-Fi connection attempt failed. Check signal or credentials."
  }
else
  echo "[*] nmcli not found, skipping immediate Wi-Fi association (using system profiles)"
fi

# 7. Run the cyberdeck installer to compile virtual environment and services
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -f "$SCRIPT_DIR/install.sh" ]; then
  echo "[+] Running Cyberdeck Controller service installer..."
  # Run install.sh as regular user (logname) to avoid creating virtualenv under root ownership
  REAL_USER=$(logname || echo $SUDO_USER || echo $USER)
  echo "[+] Running installation as user: $REAL_USER"
  su -c "bash $SCRIPT_DIR/install.sh" "$REAL_USER"
else
  echo "[-] Warning: install.sh not found in $SCRIPT_DIR. Run it manually later."
fi

echo "[+] Configuration finished! Please reboot the Pi for UART changes to take effect."
