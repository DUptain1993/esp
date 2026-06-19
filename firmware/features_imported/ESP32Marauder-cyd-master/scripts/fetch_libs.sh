#!/usr/bin/env bash
# ===========================================================================
#  Fetch the vendored Marauder libraries required to build the CYD 3.5" target.
#
#  These are the upstream submodules from .gitmodules plus a few extra runtime
#  dependencies, pinned to versions known to compile against arduino-esp32 2.x
#  (espressif32 6.9.0). Idempotent: existing, non-empty folders are skipped.
# ===========================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIBS="$ROOT/esp32_marauder/libraries"
ROOTLIBS="$ROOT/libraries"
mkdir -p "$LIBS" "$ROOTLIBS"

clone() {  # clone <dest_dir> <repo_url> [branch/tag]
  local dest="$1" url="$2" ref="${3:-}"
  if [ -n "$(ls -A "$dest" 2>/dev/null || true)" ]; then
    echo "skip   $dest (already present)"
    return
  fi
  echo "clone  $dest <- $url ${ref:+($ref)}"
  if [ -n "$ref" ]; then
    git clone --depth 1 -b "$ref" "$url" "$dest"
  else
    git clone --depth 1 "$url" "$dest"
  fi
  rm -rf "$dest/.git"
}

# Core Marauder submodules
clone "$LIBS/TFT_eSPI"          https://github.com/justcallmekoko/TFT_eSPI.git
clone "$LIBS/LinkedList"        https://github.com/ivanseidel/LinkedList.git
clone "$LIBS/JPEGDecoder"       https://github.com/Bodmer/JPEGDecoder.git
clone "$LIBS/Adafruit_NeoPixel" https://github.com/adafruit/Adafruit_NeoPixel.git
clone "$LIBS/SwitchLib"         https://github.com/justcallmekoko/SwitchLib.git
clone "$LIBS/ArduinoJson"       https://github.com/bblanchon/ArduinoJson.git        6.x
clone "$LIBS/NimBLE-Arduino"    https://github.com/h2zero/NimBLE-Arduino.git        1.4.3

# Extra runtime dependencies
clone "$LIBS/ESP32Ping"            https://github.com/marian-craciunescu/ESP32Ping.git
clone "$LIBS/MicroNMEA"            https://github.com/stevemarple/MicroNMEA.git
clone "$LIBS/XPT2046_Touchscreen"  https://github.com/PaulStoffregen/XPT2046_Touchscreen.git
clone "$LIBS/EspSoftwareSerial"    https://github.com/plerup/espsoftwareserial.git  8.1.0
clone "$ROOTLIBS/AsyncTCP"         https://github.com/me-no-dev/AsyncTCP.git

echo "All Marauder libraries are present."
