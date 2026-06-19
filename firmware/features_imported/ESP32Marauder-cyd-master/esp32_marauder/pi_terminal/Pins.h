// Pins.h  (Pi Terminal module)
#pragma once

// NOTE (CYD 3.5"): This board is touch driven and has no physical UP/OK/DOWN
// buttons, so the on-screen keyboard uses the touchscreen. The BTN_* defines
// below are only consumed when PT_USE_BUTTONS is defined (off by default) and
// are kept solely so the provided VirtualKeyboard sources compile unchanged.

// Buttons, wired to GND when pressed (unused on CYD touch builds)
#ifndef BTN_UP
  #define BTN_UP    32
#endif
#ifndef BTN_OK
  #define BTN_OK    33
#endif
#ifndef BTN_DOWN
  #define BTN_DOWN  25
#endif

// No buzzer on the CYD 3.5". Assign a free GPIO here to enable key clicks.
#ifndef BUZZER_PIN
  #define BUZZER_PIN -1
#endif
