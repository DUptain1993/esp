#pragma once

// ===========================================================================
//  Pi Terminal  ·  isolated Marauder add-on
//
//  A self-contained module that adds an encrypted/compressed UART link to a
//  Raspberry Pi plus an on-screen terminal page. It does NOT touch Marauder
//  internals — the only hooks are these three free functions, called from the
//  sketch (init + pump) and from a single new main-menu node (open).
//
//  Wiring (defaults, override with -D build flags):
//      ESP32 GPIO PI_UART_TX_PIN (26) -> Pi UART RX
//      ESP32 GPIO PI_UART_RX_PIN (35) <- Pi UART TX
//      GND <-> GND
//  These pins avoid the CYD 3.5" TFT/touch/SD/I2C/GPS/LED assignments.
// ===========================================================================

// Bring up the UART, cipher streams and message handler. Call once in setup()
// AFTER the display has been initialised. Safe to call on any board; if the
// link is never used it just idles (and emits background traffic if obfuscated).
void pi_terminal_init();

// Non-blocking service routine: drains the UART, parses/decrypts/decompresses
// frames and appends streamed output to the scrollback. Call every loop().
void pi_terminal_pump();

// Modal terminal page. Blocks until the user taps EXIT, pumping the link the
// whole time so output keeps streaming live. Intended as a Marauder menu node
// callback. On return the caller should redraw the current menu.
void pi_terminal_open();
