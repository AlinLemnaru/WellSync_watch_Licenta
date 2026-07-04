#include "ui_theme.h"

namespace UITheme {

  static const lgfx::IFont* FONT_HEADER  = &fonts::FreeSans9pt7b;
  static const lgfx::IFont* FONT_WEATHER = &fonts::FreeSans9pt7b;
  static const lgfx::IFont* FONT_BODY    = &fonts::FreeSans12pt7b;
  static const lgfx::IFont* FONT_CLOCK   = &fonts::FreeSansBold24pt7b;

  void beginFonts() {
    M5.Display.setFont(FONT_HEADER);
  }

  void applyFontHeader(M5Canvas& canvas) {
    canvas.setFont(FONT_HEADER);
  }

  void applyFontWeather(M5Canvas& canvas) {
    canvas.setFont(FONT_WEATHER);
  }

  void applyFontBody(M5Canvas& canvas) {
    canvas.setFont(FONT_BODY);
  }

  void applyFontClock(M5Canvas& canvas) {
    canvas.setFont(FONT_CLOCK);
  }

  void applyFontHeader(M5GFX& display) {
    display.setFont(FONT_HEADER);
  }

  void applyFontWeather(M5GFX& display) {
    display.setFont(FONT_WEATHER);
  }

  void applyFontBody(M5GFX& display) {
    display.setFont(FONT_BODY);
  }

  void applyFontClock(M5GFX& display) {
    display.setFont(FONT_CLOCK);
  }

  int textWidth(M5Canvas& canvas, const String& text) {
    return canvas.textWidth(text);
  }

  int fontHeight(M5Canvas& canvas) {
    return canvas.fontHeight();
  }
}