#ifndef PEPE_DRAW_H
#define PEPE_DRAW_H

#include <Arduino.h>
#include "DisplayTFT.h"

// ===========================================================================
//  PepeDraw  ·  small text rendering helpers used by the virtual keyboard and
//  the Pi terminal. Backed by TFT_eSPI's transparent text engine so glyphs
//  overlay key backgrounds cleanly.
// ===========================================================================

// UI palette
#define UI_MAIN    TFT_WHITE
#define UI_BG      TFT_BLACK
#define UI_ACCENT  0x7BEF   // grey
#define UI_CURSOR  TFT_WHITE
#define UI_SELECT  0xFA20   // orange-red highlight

extern DisplayTFT tft;

enum FontType {
    FONT_SMALL = 0,   // GLCD 6x8 cell
    FONT_BIG   = 1    // 16px font for headers
};

// Back-compat API (use FONT_SMALL)
void drawCharCustom(int x, int y, char c, uint16_t color, int size);
void drawStringCustom(int x, int y, String txt, uint16_t color, int size);

// Header font
void drawStringBig(int x, int y, const String& txt, uint16_t color, int size);

// Pixel width of a string (for centering / right-align)
int  getTextWidth(const String& txt, int size, FontType font = FONT_SMALL);

// Pixel height of the font
int  getFontHeight(int size, FontType font = FONT_SMALL);

// Centered on a 320px-wide canvas
void drawStringCentered(int y, const String& txt, uint16_t color,
                        int size, FontType font = FONT_SMALL);

// Right-aligned (xRight = right edge of text)
void drawStringRight(int xRight, int y, const String& txt, uint16_t color,
                     int size, FontType font = FONT_SMALL);

#endif
