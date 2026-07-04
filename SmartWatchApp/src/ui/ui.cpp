#include <M5Unified.h>
#include "../app/app_state.h"
#include "ui.h"
#include "ui_pages.h"
#include "ui_theme.h"

static const int SCREEN_W = 320;
static const int SCREEN_H = 240;
static const int HEADER_H = 24;

static M5Canvas canvas(&M5.Display);
static uint32_t lastAutoRefreshMs = 0;
static const uint32_t AUTO_REFRESH_INTERVAL_MS = 250;

static uint16_t blend565(uint16_t c1, uint16_t c2, float t) {
  if (t <= 0.0f) return c1;
  if (t >= 1.0f) return c2;

  uint8_t r1 = (c1 >> 11) & 0x1F;
  uint8_t g1 = (c1 >> 5) & 0x3F;
  uint8_t b1 = c1 & 0x1F;

  uint8_t r2 = (c2 >> 11) & 0x1F;
  uint8_t g2 = (c2 >> 5) & 0x3F;
  uint8_t b2 = c2 & 0x1F;

  uint8_t r = static_cast<uint8_t>(r1 + (r2 - r1) * t);
  uint8_t g = static_cast<uint8_t>(g1 + (g2 - g1) * t);
  uint8_t b = static_cast<uint8_t>(b1 + (b2 - b1) * t);

  return (r << 11) | (g << 5) | b;
}

static bool pageNeedsAutoRefresh(PageId page) {
  switch (page) {
    case PAGE_HOME:
    case PAGE_SETTINGS:
      return true;

    default:
      return false;
  }
}

static bool fallOverlayNeedsRefresh() {
  return app.fallDetection.popupVisible;
}

static String buildBatteryLabel() {
  int level = app.settings.batteryPercent;
  if (M5.Power.isCharging()) return String(level) + "%+";
  return String(level) + "%";
}

static void drawHeader() {
  canvas.fillRect(0, 0, SCREEN_W, HEADER_H, UITheme::COLOR_HEADER);

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_HEADER);

  canvas.setTextDatum(TL_DATUM);
  canvas.drawString(getPageTitle(app.currentPage), 8, 6);

  canvas.setTextDatum(TR_DATUM);
  canvas.drawString(buildBatteryLabel(), SCREEN_W - 8, 6);
}

static int getFallCountdownSeconds() {
  if (app.fallDetection.phase != FALL_PHASE_CONFIRMATION) return 0;

  uint32_t now = millis();
  uint32_t elapsed = now - app.fallDetection.confirmationStartedAtMs;
  uint32_t remainingMs = 0;

  if (elapsed < app.fallDetection.confirmationTimeoutMs) {
    remainingMs = app.fallDetection.confirmationTimeoutMs - elapsed;
  }

  return static_cast<int>((remainingMs + 999) / 1000);
}

static void drawOverlayScrim(uint16_t color, int step) {
  for (int y = HEADER_H; y < SCREEN_H; y += step) {
    canvas.drawFastHLine(0, y, SCREEN_W, color);
  }
}

static void drawFallConfirmationOverlay() {
  drawOverlayScrim(TFT_BLACK, 2);

  const int cardX = 14;
  const int cardY = 44;
  const int cardW = 292;
  const int cardH = 162;

  canvas.fillRoundRect(cardX + 3, cardY + 4, cardW, cardH, 18, TFT_BLACK);
  canvas.fillRoundRect(cardX, cardY, cardW, cardH, 18, UITheme::COLOR_SURFACE);

  uint16_t warningColor = TFT_ORANGE;
  uint16_t accentColor  = blend565(UITheme::COLOR_BUTTON_HL, warningColor, 0.55f);

  // Badge
  const int badgeW = 110;
  const int badgeH = 24;
  const int badgeX = cardX + (cardW - badgeW) / 2;
  canvas.fillRoundRect(badgeX, cardY + 10, badgeW, badgeH, 12, accentColor);

  UITheme::applyFontHeader(canvas);   
  canvas.setTextColor(UITheme::COLOR_TEXT, accentColor);
  canvas.setTextDatum(MC_DATUM);
  canvas.drawString("Fall Alert", cardX + cardW / 2, cardY + 10 + badgeH / 2 + 1);

  // "Are you OK?" 
  UITheme::applyFontBody(canvas);     
  canvas.setTextColor(TFT_RED, UITheme::COLOR_SURFACE);
  canvas.drawString("Are you OK?", cardX + cardW / 2, cardY + 54);

  // Countdown
  UITheme::applyFontHeader(canvas);   
  String countdownText = "Alert in " + String(getFallCountdownSeconds()) + "s";
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(countdownText, cardX + cardW / 2, cardY + 82);

  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  canvas.drawString("Tap or press any button", cardX + cardW / 2, cardY + 100);

  // Buton OK 
  uint16_t okBg  = blend565(UITheme::COLOR_BUTTON_HL, UITheme::COLOR_SUCCESS, 0.35f);
  const int btnH = 28;
  const int btnY = cardY + cardH - btnH - 10;  
  canvas.fillRoundRect(cardX + 66, btnY, 160, btnH, 13, okBg);

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_TEXT, okBg);
  canvas.drawString("I'M OKAY", cardX + cardW / 2, btnY + btnH / 2 + 1);
}

static void drawFallAlertOverlay() {
  uint32_t phaseTick  = (millis() / 220) % 2;
  uint16_t alertBg    = phaseTick == 0 ? TFT_RED     : TFT_MAROON;
  uint16_t panelColor = phaseTick == 0 ? TFT_DARKRED : TFT_RED;

  drawOverlayScrim(alertBg, 1);

  const int panelX = 14;
  const int panelY = 34;
  const int panelW = 292;
  const int panelH = 174;

  canvas.fillRoundRect(panelX + 4, panelY + 4, panelW, panelH, 20, TFT_BLACK);
  canvas.fillRoundRect(panelX, panelY, panelW, panelH, 20, panelColor);

  // Title
  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(TFT_WHITE, panelColor);
  canvas.setTextDatum(MC_DATUM);
  canvas.drawString("EMERGENCY ALERT", SCREEN_W / 2, panelY + 16 + 1);

  // "FALL"
  UITheme::applyFontClock(canvas);
  canvas.drawString("FALL", SCREEN_W / 2, panelY + 74);

  UITheme::applyFontHeader(canvas);
  canvas.drawString("No confirmation received", SCREEN_W / 2, panelY + 112);
  canvas.drawString("Touch or press button",    SCREEN_W / 2, panelY + 132);
  canvas.drawString("to dismiss alert",          SCREEN_W / 2, panelY + 152);

  canvas.drawRoundRect(panelX,     panelY,     panelW,     panelH,     20, TFT_WHITE);
  canvas.drawRoundRect(panelX + 3, panelY + 3, panelW - 6, panelH - 6, 18, TFT_YELLOW);
}

static void drawFallDetectionOverlay() {
  if (!app.fallDetection.popupVisible) return;

  if (app.fallDetection.phase == FALL_PHASE_CONFIRMATION) {
    drawFallConfirmationOverlay();
    return;
  }

  if (app.fallDetection.phase == FALL_PHASE_ALERT) {
    drawFallAlertOverlay();
    return;
  }
}

void initUI() {
  UITheme::beginFonts();

  canvas.setColorDepth(16);
  canvas.createSprite(SCREEN_W, SCREEN_H);

  lastAutoRefreshMs = millis();
  requestRedraw();
}

void updateUI() {
  uint32_t now = millis();

  if (app.pageChanged) {
    drawCurrentPage(true);
    lastAutoRefreshMs = now;
    return;
  }

  if ((pageNeedsAutoRefresh(app.currentPage) || fallOverlayNeedsRefresh()) &&
      (now - lastAutoRefreshMs >= AUTO_REFRESH_INTERVAL_MS)) {
    drawCurrentPage(true);
    lastAutoRefreshMs = now;
  }
}

void drawCurrentPage(bool forceRedraw) {
  if (!forceRedraw && !app.pageChanged) return;

  canvas.fillScreen(UITheme::COLOR_BG);

  drawHeader();
  drawActivePage(canvas);
  drawFallDetectionOverlay();

  canvas.pushSprite(0, 0);

  app.pageChanged = false;
}

void requestRedraw() {
  app.pageChanged = true;
}