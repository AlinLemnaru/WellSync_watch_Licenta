#pragma once
#include <M5Unified.h>

namespace UITheme {
  constexpr uint16_t COLOR_BG         = TFT_BLACK;
  constexpr uint16_t COLOR_HEADER     = 0x2BB0;  
  constexpr uint16_t COLOR_SURFACE    = 0x0945;  
  constexpr uint16_t COLOR_BUTTON     = 0x2BB0;  
  constexpr uint16_t COLOR_BUTTON_HL  = 0x2B4D;  
  constexpr uint16_t COLOR_TEXT       = TFT_WHITE;
  constexpr uint16_t COLOR_SUBTEXT    = 0x7574;  
  constexpr uint16_t COLOR_WEATHER    = 0x7574;  
  constexpr uint16_t COLOR_SUCCESS    = 0x3666;  
  constexpr uint16_t COLOR_WARNING    = 0xFD20;  

  void beginFonts();

  void applyFontHeader(M5Canvas& canvas);
  void applyFontWeather(M5Canvas& canvas);
  void applyFontBody(M5Canvas& canvas);
  void applyFontClock(M5Canvas& canvas);

  void applyFontHeader(M5GFX& display);
  void applyFontWeather(M5GFX& display);
  void applyFontBody(M5GFX& display);
  void applyFontClock(M5GFX& display);

  int textWidth(M5Canvas& canvas, const String& text);
  int fontHeight(M5Canvas& canvas);
}