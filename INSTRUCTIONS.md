You are a senior embedded firmware engineer.

Your task is to extend an existing ESP32 Marauder CYD firmware WITHOUT removing or altering its core functionality.

---

## CRITICAL RULE (NON-NEGOTIABLE)

ESP32 Marauder MUST remain fully intact.

- Do NOT remove features
- Do NOT rewrite Marauder logic
- Do NOT break existing menus, scanning, attacks, or UI
- Do NOT replace its architecture

You are ONLY allowed to:
- ADD new functionality
- HOOK into existing systems cleanly
- EXTEND the UI with a new module/page

If anything breaks Marauder functionality, your solution is invalid.

---

## OBJECTIVE

Augment Marauder with:

1. Real-time UART communication with a Raspberry Pi Zero W
2. Encryption + compression using provided core modules
3. A new CLI Terminal UI page (separate from existing menus)

---

## FIRMWARE BASE

Use the CYD 3.5-inch Marauder configuration as-is.

Modify ONLY what is necessary in:

- firmware/features_imported/ESP32Marauder-cyd-master/User_Setup_cyd_3_5_inch.h
- firmware/features_imported/ESP32Marauder-cyd-master/User_Setup_Select.h

Ensure display works exactly as before.

---

## NEW FEATURE: TERMINAL MODE (ISOLATED MODULE)

You must add a NEW menu entry inside Marauder:

"Pi Terminal"

This must NOT interfere with existing UI logic.

---

## TERMINAL UI REQUIREMENTS

Create a dedicated UI module:

- Real-time terminal display (scrolling output)
- Virtual keyboard input
- Command submission over UART
- Streaming response rendering (line-by-line, real-time)

You MUST use:

```cpp
#include "VirtualKeyboard.h"
#include "DisplayTFT.h"
#include "PepeDraw.h"
#include "Pins.h"
#include "SoundUtils.h"
```

Behavior:

1. User opens "Pi Terminal"
2. A terminal screen appears
3. User types command via virtual keyboard
4. Command is sent over UART
5. Raspberry Pi executes it
6. Output streams back live and displays continuously

---

## UART COMMUNICATION LAYER (MANDATORY)

Integrate the following core modules:

- firmware/src/core/comms.*
- firmware/src/core/protocol.*
- firmware/src/core/encryption.*
- firmware/src/core/compression.*
- firmware/src/core/commands.h

You MUST use this pipeline:

INPUT → commands → protocol → encryption → compression → UART TX  
UART RX → decompression → decryption → protocol parse → output to UI

---

## REAL-TIME REQUIREMENTS

- Non-blocking UART (no delays)
- Interrupt or buffered RX handling
- Streaming output (NOT full-response buffering)
- UI must update continuously while receiving data

---

## RASPBERRY PI COMPATIBILITY

Ensure compatibility with the existing Python system:

- comms.py
- protocol.py
- encryption.py
- compression.py
- dispatcher.py
- shell.py
- stream.py

Do NOT modify Pi-side logic.

Match:
- Packet structure
- Command format
- Streaming behavior

---

## ARCHITECTURE RULES

- All new functionality must be modular
- No tight coupling with Marauder core logic
- Use clean interfaces/hooks
- Keep Marauder upgrade-safe

---

## BUILD SYSTEM

Convert to PlatformIO:

- Create platformio.ini
- Ensure everything compiles cleanly
- Include all dependencies

Add GitHub Actions:

- Build firmware automatically
- Fail on warnings/errors

---

## STRICT EXCLUSIONS

DO NOT USE OR MODIFY:

- firmware/src/security

---

## OUTPUT REQUIREMENTS

You must produce:

1. New module for "Pi Terminal"
2. UART communication implementation
3. Integration of core modules
4. Minimal, safe hooks into Marauder menu system
5. PlatformIO configuration
6. GitHub Actions workflow
7. Fully compiling codebase

---

## FINAL BEHAVIOR REQUIREMENT

The final system must allow:

- User navigates Marauder normally (ALL features still work)
- User opens "Pi Terminal"
- Types command
- ESP sends command over UART
- Raspberry Pi executes it
- Output streams back live to ESP display

NO simulation  
NO feature loss  
NO breaking changes  

---

Proceed carefully and preserve ALL existing Marauder functionality.
