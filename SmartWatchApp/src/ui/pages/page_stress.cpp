#include <M5Unified.h>
#include "../../app/app_state.h"
#include "../ui_theme.h"
#include "page_stress.h"

static const int SCREEN_W = 320;
static const int HEADER_H = 24;
static const int CONTENT_Y = HEADER_H;
static const int CONTENT_H = 240 - HEADER_H;

static String formatStressScore() {
  if (app.stress.availability == DATA_AVAILABLE) {
    return String(app.stress.stressScore);
  }
  return "--";
}

static String formatHrv() {
  if (app.stress.availability == DATA_AVAILABLE) {
    return String(app.stress.hrvMs) + " ms";
  }
  return "--";
}

static String formatRestingHr() {
  if (app.stress.availability == DATA_AVAILABLE) {
    return String(app.stress.restingHr) + " bpm";
  }
  return "--";
}

static String formatRecovery() {
  if (app.stress.availability == DATA_AVAILABLE) {
    return String(app.stress.recoveryScore);
  }
  return "--";
}

static const char* deriveStressLabel() {
  if (app.stress.availability != DATA_AVAILABLE) return "--";
  if (app.stress.stressScore <= 25) return "Resting";
  if (app.stress.stressScore <= 50) return "Low";
  if (app.stress.stressScore <= 75) return "Medium";
  return "High";
}

static uint16_t deriveStressAccent() {
  if (app.stress.availability != DATA_AVAILABLE) return UITheme::COLOR_SUBTEXT;
  if (app.stress.stressScore <= 25) return 0x3666;
  if (app.stress.stressScore <= 50) return 0xAEDC;
  if (app.stress.stressScore <= 75) return 0xE8A3;
  return 0xFD20;
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
  canvas.drawString("Status", x + 14, y + 10);

  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(deriveStressLabel(), x + 14, y + 30);

  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  canvas.drawString("Recovery", x + 160, y + 10);

  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(formatRecovery(), x + 160, y + 30);
}

static void drawStressHeroCard(M5Canvas& canvas, int x, int y, int w, int h) {
  uint16_t accent = deriveStressAccent();
  int gaugeCx = x + 58;
  int gaugeCy = y + h / 2;

  canvas.fillRoundRect(x, y, w, h, 18, UITheme::COLOR_SURFACE);

  canvas.drawArc(gaugeCx, gaugeCy, 34, 24, 0, 360, UITheme::COLOR_SUBTEXT);
  canvas.fillArc(gaugeCx, gaugeCy, 34, 24, 220, 220 + (app.stress.stressScore * 2), accent);

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  canvas.drawString("Stress score", x + 104, y + 12);

  UITheme::applyFontBody(canvas);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(formatStressScore(), x + 104, y + 34);

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(accent, UITheme::COLOR_SURFACE);
  canvas.drawString(deriveStressLabel(), x + 144, y + 38);
}

void drawPageStress(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);

  drawStressHeroCard(canvas, 12, CONTENT_Y + 8, 296, 72);
  drawMetricCard(canvas, 12,  CONTENT_Y + 90, 142, 52, "HRV",        formatHrv());
  drawMetricCard(canvas, 166, CONTENT_Y + 90, 142, 52, "Resting HR", formatRestingHr());
  drawBottomInfoCard(canvas, 12, CONTENT_Y + 152, 296, 58);
}