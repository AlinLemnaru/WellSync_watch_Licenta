#include <M5Unified.h>
#include "../../app/app_state.h"
#include "../ui_theme.h"
#include "page_sleep.h"

static const int SCREEN_W = 320;
static const int HEADER_H = 24;
static const int CONTENT_Y = HEADER_H;
static const int CONTENT_H = 240 - HEADER_H;

static String formatSleepScore() {
  if (app.sleep.availability == DATA_AVAILABLE) {
    return String(app.sleep.sleepScore);
  }
  return "--";
}

static String formatDuration() {
  if (app.sleep.availability == DATA_AVAILABLE) {
    return String(app.sleep.durationHours) + "h " + String(app.sleep.durationMinutes) + "m";
  }
  return "--";
}

static String formatDeepSleep() {
  if (app.sleep.availability == DATA_AVAILABLE) {
    return String(app.sleep.deepSleepMinutes) + " min";
  }
  return "--";
}

static String formatBedTime() {
  if (app.sleep.availability == DATA_AVAILABLE) {
    return app.sleep.bedTime;
  }
  return "--";
}

static String formatWakeTime() {
  if (app.sleep.availability == DATA_AVAILABLE) {
    return app.sleep.wakeTime;
  }
  return "--";
}

static void drawMetricCard(M5Canvas& canvas, int x, int y, int w, int h, const char* title, const String& value) {
  canvas.fillRoundRect(x, y, w, h, 16, UITheme::COLOR_SURFACE);

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(title, x + 14, y + 10);

  UITheme::applyFontBody(canvas);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(value, x + 14, y + 30);
}

static void drawBottomInfoCard(M5Canvas& canvas, int x, int y, int w, int h) {
  canvas.fillRoundRect(x, y, w, h, 16, UITheme::COLOR_SURFACE);

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);

  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  canvas.drawString("Duration", x + 14, y + 10);

  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(formatDuration(), x + 14, y + 30);

  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  canvas.drawString("Deep sleep", x + 160, y + 10);

  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(formatDeepSleep(), x + 160, y + 30);
}

static void drawSleepHeroCard(M5Canvas& canvas, int x, int y, int w, int h) {
  uint16_t accent = 0x3666;
  int gaugeCx = x + 58;
  int gaugeCy = y + h / 2;

  if (app.sleep.sleepScore < 60) accent = 0xFD20;
  else if (app.sleep.sleepScore < 80) accent = 0xE8A3;

  canvas.fillRoundRect(x, y, w, h, 18, UITheme::COLOR_SURFACE);

  canvas.drawArc(gaugeCx, gaugeCy, 34, 24, 0, 360, UITheme::COLOR_SUBTEXT);
  canvas.fillArc(gaugeCx, gaugeCy, 34, 24, 220, 220 + (app.sleep.sleepScore * 2), accent);

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  canvas.drawString("Sleep score", x + 104, y + 12);

  UITheme::applyFontBody(canvas);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(formatSleepScore(), x + 104, y + 34);

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);

  const char* label = "Good";
  if (app.sleep.sleepScore < 60) label = "Low";
  else if (app.sleep.sleepScore >= 85) label = "Great";

  canvas.drawString(label, x + 144, y + 38);
}

void drawPageSleep(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);

  drawSleepHeroCard(canvas, 12, CONTENT_Y + 8, 296, 72);
  drawMetricCard(canvas, 12,  CONTENT_Y + 90, 142, 52, "Bedtime", formatBedTime());
  drawMetricCard(canvas, 166, CONTENT_Y + 90, 142, 52, "Wake",    formatWakeTime());
  drawBottomInfoCard(canvas, 12, CONTENT_Y + 152, 296, 58);
}