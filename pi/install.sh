#!/bin/bash
set -e

# Cyberdeck Controller Installer (Bookworm/PEP 668 safe)
# This script creates a venv, installs dependencies, and sets up systemd.

REPO_ROOT=$(cd "$(dirname "$0")/.." && pwd)
PI_DIR="$REPO_ROOT/pi"
VENV_PATH="$PI_DIR/venv"
USER_NAME=$(logname || echo $USER)

echo "--- Installing Cyberdeck Controller ---"
echo "Repo root: $REPO_ROOT"
echo "Target user: $USER_NAME"

# 1. System dependencies
echo "Installing system dependencies..."
sudo apt-get update && sudo apt-get install -y python3-venv python3-pip

# 2. Create venv
if [ ! -d "$VENV_PATH" ]; then
    echo "Creating virtual environment..."
    python3 -m venv "$VENV_PATH"
fi

# 3. Install Python dependencies
echo "Installing requirements into venv..."
"$VENV_PATH/bin/pip" install --upgrade pip
"$VENV_PATH/bin/pip" install -r "$PI_DIR/requirements.txt"

# 4. Udev Rule
echo "Installing udev rule for /dev/cyberdeck..."
sudo cp "$PI_DIR/99-cyberdeck.rules" /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger

# 5. User groups
echo "Adding $USER_NAME to dialout group..."
sudo usermod -a -G dialout "$USER_NAME"

# 6. Systemd Unit
echo "Configuring systemd service..."
sed "s|{{USER}}|$USER_NAME|g; s|{{WORKDIR}}|$REPO_ROOT|g" "$PI_DIR/cyberdeck.service.template" > cyberdeck.service
sudo mv cyberdeck.service /etc/systemd/system/cyberdeck.service
sudo systemctl daemon-reload
sudo systemctl enable cyberdeck

echo "--- Installation Complete ---"
echo "To start the service: sudo systemctl start cyberdeck"
echo "To check logs: journalctl -u cyberdeck -f"
echo "Note: You may need to logout and login for group changes to take effect."
