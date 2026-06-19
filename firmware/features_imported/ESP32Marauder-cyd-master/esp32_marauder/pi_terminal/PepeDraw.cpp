#include "PepeDraw.h"

// All helpers render through the shared `tft` instance using TFT_eSPI's
// built-in fonts. Single-argument setTextColor() yields transparent text, so
// glyphs overlay the pre-filled key/cell backgrounds.
//
//  - FONT_SMALL -> GLCD font (font 1): 6 x 8 px cell, scaled by `size`.
//  - FONT_BIG   -> font 2 (16 px tall), scaled by `size`.

static const int GLCD_W = 6;   // cell width  (5 + 1 spacing)
static const int GLCD_H = 8;   // cell height
static const int BIG_H  = 16;  // font 2 height

void drawCharCustom(int x, int y, char c, uint16_t color, int size) {
    char buf[2] = { c, 0 };
    tft.setTextColor(color);
    tft.setTextSize(size < 1 ? 1 : size);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(buf, x, y, 1);
}

void drawStringCustom(int x, int y, String txt, uint16_t color, int size) {
    tft.setTextColor(color);
    tft.setTextSize(size < 1 ? 1 : size);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(txt, x, y, 1);
}

void drawStringBig(int x, int y, const String& txt, uint16_t color, int size) {
    tft.setTextColor(color);
    tft.setTextSize(size < 1 ? 1 : size);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(txt, x, y, 2);
}

int getTextWidth(const String& txt, int size, FontType font) {
    if (size < 1) size = 1;
    int cell = (font == FONT_BIG) ? (GLCD_W + 4) : GLCD_W;  // big font ~10px
    return (int)txt.length() * cell * size;
}

int getFontHeight(int size, FontType font) {
    if (size < 1) size = 1;
    return ((font == FONT_BIG) ? BIG_H : GLCD_H) * size;
}

void drawStringCentered(int y, const String& txt, uint16_t color,
                        int size, FontType font) {
    int w = getTextWidth(txt, size, font);
    int x = (320 - w) / 2;
    if (x < 0) x = 0;
    if (font == FONT_BIG) drawStringBig(x, y, txt, color, size);
    else                  drawStringCustom(x, y, txt, color, size);
}

void drawStringRight(int xRight, int y, const String& txt, uint16_t color,
                     int size, FontType font) {
    int w = getTextWidth(txt, size, font);
    int x = xRight - w;
    if (x < 0) x = 0;
    if (font == FONT_BIG) drawStringBig(x, y, txt, color, size);
    else                  drawStringCustom(x, y, txt, color, size);
}
