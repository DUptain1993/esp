#!/bin/bash
set -e

echo "--- Uninstalling Cyberdeck Controller ---"

sudo systemctl stop cyberdeck || true
sudo systemctl disable cyberdeck || true
sudo rm -f /etc/systemd/system/cyberdeck.service
sudo systemctl daemon-reload

sudo rm -f /etc/udev/rules.d/99-cyberdeck.rules
sudo udevadm control --reload-rules

echo "Removing virtual environment..."
rm -rf "$(dirname "$0")/venv"

echo "--- Uninstallation Complete ---"
