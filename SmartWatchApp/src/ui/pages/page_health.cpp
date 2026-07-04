#include <M5Unified.h>
#include "../../app/app_state.h"
#include "../ui_theme.h"
#include "page_health.h"

static const int SCREEN_W  = 320;
static const int SCREEN_H  = 240;
static const int HEADER_H  = 24;
static const int CONTENT_Y = HEADER_H;
static const int CONTENT_H = SCREEN_H - HEADER_H;

static uint16_t hrArcColor(int bpm) {
  if (bpm < 60)   return 0x001F;  
  if (bpm <= 100) return 0x3666;  
  if (bpm <= 130) return 0xFD20;  
  return 0xF800;                   
}

static float hrArcAngle(int bpm) {
  if (bpm <= 40)  return 0.0f;
  if (bpm >= 200) return 320.0f;
  return (bpm - 40) * 320.0f / 160.0f;
}

// ── Helpers text ──────────────────────────────────────────────────────────

static String buildHeartRateString() {
  if (app.health.availability == DATA_AVAILABLE)
    return String(app.health.heartRateBpm);
  return "--";
}

static String buildSpo2String() {
  if (app.health.availability == DATA_AVAILABLE)
    return String(app.health.spo2Percent) + "%";
  return "--";
}

static String buildStepsString() {
  return String(app.health.steps);
}

static String buildStepsPctString() {
  if (app.health.stepTarget > 0) {
    int pct = app.health.steps * 100 / app.health.stepTarget;
    if (pct > 100) pct = 100;
    return String(pct) + "%";
  }
  return "--%";
}

static void drawMetricCard(
  M5Canvas& canvas,
  int x, int y, int w, int h,
  const String& title,
  const String& value
) {
  canvas.fillRoundRect(x, y, w, h, 16, UITheme::COLOR_SURFACE);

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  canvas.setTextDatum(TL_DATUM);
  canvas.drawString(title, x + 14, y + 12);

  UITheme::applyFontBody(canvas);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(value, x + 14, y + 42);
}

static void drawHealthHeroCard(M5Canvas& canvas, int x, int y, int w, int h) {
  const int gaugeCx = x + 58;
  const int gaugeCy = y + (h / 2);

  int      bpm   = app.health.heartRateBpm;
  uint16_t col   = hrArcColor(bpm);
  float    angle = hrArcAngle(bpm);

  canvas.fillRoundRect(x, y, w, h, 18, UITheme::COLOR_SURFACE);

  canvas.drawArc(gaugeCx, gaugeCy, 34, 24, 0, 360, UITheme::COLOR_SUBTEXT);

  if (angle > 0.0f && app.health.availability == DATA_AVAILABLE) {
    canvas.fillArc(gaugeCx, gaugeCy, 34, 24, 220, 220 + angle, col);
  }

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  canvas.setTextDatum(TL_DATUM);
  canvas.drawString("Heart rate", x + 104, y + 18);

  UITheme::applyFontBody(canvas);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(buildHeartRateString(), x + 104, y + 52);

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  canvas.drawString("BPM", x + 158, y + 56);
}

// ── Entry point ───────────────────────────────────────────────────────────
void drawPageHealth(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);

  drawHealthHeroCard(canvas, 12, CONTENT_Y + 8, 296, 102);
  drawMetricCard(canvas, 12, CONTENT_Y + 120, 142, 84, "SpO2", buildSpo2String());

  {
    int x = 166, y = CONTENT_Y + 120, w = 142, h = 84;
    canvas.fillRoundRect(x, y, w, h, 16, UITheme::COLOR_SURFACE);

    UITheme::applyFontHeader(canvas);
    canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
    canvas.setTextDatum(TL_DATUM);
    canvas.drawString("Steps", x + 14, y + 12);

    UITheme::applyFontBody(canvas);
    canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
    canvas.drawString(buildStepsString(), x + 14, y + 38);

    UITheme::applyFontHeader(canvas);
    canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
    canvas.drawString(buildStepsPctString(), x + 14, y + 62);
  }
}