#ifndef VIRTUAL_KEYBOARD_H
#define VIRTUAL_KEYBOARD_H

#include <Arduino.h>

// ===========================================================================
//  VIRTUAL KEYBOARD · 10 cols x 4 alphanumeric rows + 5 special keys
//  · QWERTY layout, SHIFT toggles upper/symbols
//  · CYD 3.5" build: driven by the touchscreen (no physical buttons)
//  · Returns the entered string, or "" if cancelled
// ===========================================================================

// Show the keyboard and return the entered string. Returns "" if cancelled (X).
//
//   title:     header title (e.g. "PI TERMINAL")
//   subtitle:  line under the title (e.g. "Type a command")
//   maxLen:    maximum characters allowed
//   maskInput: true = render as asterisks (passwords)
String virtualKeyboardInput(const String& title,
                             const String& subtitle,
                             int maxLen = 62,
                             bool maskInput = false);

#endif
