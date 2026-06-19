#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

// ───────────────────────────────────────────────────────────────────────────
//  DisplayTFT  ·  thin TFT_eSPI wrapper used by the Pi Terminal UI.
//
//  The upstream virtual-keyboard project shipped a scaled DisplayTFT (logical
//  320x240 -> larger panel). On the CYD 3.5" we render natively in the panel's
//  own coordinate space (portrait 320x480, matching Marauder's orientation), so
//  this is an identity wrapper. Keeping the class/name preserves the provided
//  PepeDraw / VirtualKeyboard API (extern DisplayTFT tft) verbatim, while
//  avoiding TFT_eSPI's size-1 text fast-path being mis-positioned by a scaler.
//
//  All drawing primitives used by the UI (drawPixel, drawFastH/VLine, fillRect,
//  drawRect, fillScreen, drawCircle, fillCircle, getTouch, text helpers) are
//  inherited unchanged from TFT_eSPI.
// ───────────────────────────────────────────────────────────────────────────
class DisplayTFT : public TFT_eSPI {
public:
    DisplayTFT() : TFT_eSPI() {}
};

extern DisplayTFT tft;
