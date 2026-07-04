#include <M5Unified.h>
#include "../../app/app_state.h"
#include "../ui_theme.h"
#include "page_environment.h"

static const int SCREEN_W = 320;
static const int HEADER_H = 24;
static const int CONTENT_Y = HEADER_H;
static const int CONTENT_H = 240 - HEADER_H;

static String formatTemperature() {
  if (app.environment.availability == DATA_AVAILABLE) {
    return String(app.environment.temperatureC, 1) + " C";
  }
  return "--";
}

static String formatHumidity() {
  if (app.environment.availability == DATA_AVAILABLE) {
    return String(app.environment.humidityPercent, 0) + "%";
  }
  return "--";
}

static String formatPressure() {
  if (app.environment.availability == DATA_AVAILABLE) {
    return String(app.environment.pressureHpa, 1) + " hPa";
  }
  return "--";
}

static String formatGasResistance() {
  if (app.environment.availability == DATA_AVAILABLE) {
    return String(app.environment.gasResistanceOhms) + " ohm";
  }
  return "--";
}

static String formatIAQScore() {
  if (app.environment.availability == DATA_AVAILABLE) {
    return String(app.environment.iaqScore);
  }
  return "--";
}

static void drawMetricCard(M5Canvas& canvas, int x, int y, int w, int h, const char* title, const String& value) {
  canvas.fillRoundRect(x, y, w, h, 16, UITheme::COLOR_SURFACE);

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(title, x + 14, y + 10);

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(value, x + 14, y + 30);
}

static void drawBottomInfoCard(M5Canvas& canvas, int x, int y, int w, int h) {
  canvas.fillRoundRect(x, y, w, h, 16, UITheme::COLOR_SURFACE);

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);

  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  canvas.drawString("Pressure", x + 14, y + 10);

  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(formatPressure(), x + 14, y + 30);

  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  canvas.drawString("Gas", x + 160, y + 10);

  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(formatGasResistance(), x + 160, y + 30);
}

static void drawIAQHeroCard(M5Canvas& canvas, int x, int y, int w, int h) {
  uint16_t accent = 0x3666;
  int gaugeCx = x + 58;
  int gaugeCy = y + h / 2;

  if (app.environment.iaqScore < 60) accent = 0xFD20;
  else if (app.environment.iaqScore < 80) accent = 0xE8A3;

  canvas.fillRoundRect(x, y, w, h, 18, UITheme::COLOR_SURFACE);

  canvas.drawArc(gaugeCx, gaugeCy, 34, 24, 0, 360, UITheme::COLOR_SUBTEXT);
  canvas.fillArc(gaugeCx, gaugeCy, 34, 24, 220, 220 + (app.environment.iaqScore * 2), accent);

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  canvas.drawString("Air quality", x + 104, y + 12);

  UITheme::applyFontBody(canvas);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(formatIAQScore(), x + 104, y + 34);

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(app.environment.iaqLabel, x + 150, y + 38);
}

void drawPageEnvironment(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);

  drawIAQHeroCard(canvas, 12, CONTENT_Y + 8, 296, 72);
  drawMetricCard(canvas, 12,  CONTENT_Y + 90, 142, 52, "Temp",     formatTemperature());
  drawMetricCard(canvas, 166, CONTENT_Y + 90, 142, 52, "Humidity", formatHumidity());
  drawBottomInfoCard(canvas, 12, CONTENT_Y + 152, 296, 58);
}