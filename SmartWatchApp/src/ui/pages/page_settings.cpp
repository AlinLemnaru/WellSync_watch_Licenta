#include <M5Unified.h>
#include "../../app/app_state.h"
#include "../../services/rtc_service.h"
#include "../ui_theme.h"
#include "page_settings.h"



static const int SCREEN_W  = 320;
static const int SCREEN_H  = 240;
static const int HEADER_H  = 24;
static const int CONTENT_Y = HEADER_H;
static const int CONTENT_H = SCREEN_H - HEADER_H;

static const int VISIBLE_ROWS = 6;
static const int ROW_H        = 24;
static const int ROW_GAP      = 6;
static const int ROW_STRIDE   = ROW_H + ROW_GAP;



// ── General elpers  ──────────────────────────────────────────────────────

static String twoDigits(int value) {
  if (value < 10) return "0" + String(value);
  return String(value);
}

static String buildTimeSourceLabel() {
  switch (app.timeSync.source) {
    case TIME_SOURCE_BLE:    return "Phone";
    case TIME_SOURCE_MANUAL: return "Manual";
    default:                 return "Default";
  }
}

static String buildManualSetLabel() {
  return isManualTimeSettingAllowed() ? "Allowed" : "Locked";
}

static String buildEditorDate() {
  return twoDigits(app.dateTimeEditor.day) + "/" +
         twoDigits(app.dateTimeEditor.month) + "/" +
         String(app.dateTimeEditor.year);
}

static String buildEditorTime() {
  return twoDigits(app.dateTimeEditor.hour) + ":" +
         twoDigits(app.dateTimeEditor.minute);
}

static const char* buildEditorFieldLabel() {
  switch (app.dateTimeEditor.field) {
    case EDIT_FIELD_DAY:    return "Day";
    case EDIT_FIELD_MONTH:  return "Month";
    case EDIT_FIELD_YEAR:   return "Year";
    case EDIT_FIELD_HOUR:   return "Hour";
    case EDIT_FIELD_MINUTE: return "Minute";
    case EDIT_FIELD_SAVE:   return "Save";
    default:                return "--";
  }
}



// ── Standby helpers ───────────────────────────────────────────────────────

static const uint32_t STANDBY_OPTIONS[]    = { 0, 15000, 30000, 60000, 120000, 300000 };
static const char*    STANDBY_LABELS[]     = { "Off", "15s", "30s", "1 min", "2 min", "5 min" };
static const int      STANDBY_OPTION_COUNT = 6;

static int standbyTimeoutIndex(uint32_t ms) {
  for (int i = 0; i < STANDBY_OPTION_COUNT; i++) {
    if (STANDBY_OPTIONS[i] == ms) return i;
  }
  return 2;
}

static String buildStandbyLabel() {
  return String(STANDBY_LABELS[standbyTimeoutIndex(app.standby.timeoutMs)]);
}

static String buildStandbyEditorLabel() {
  return String(STANDBY_LABELS[standbyTimeoutIndex(app.standbyEditor.timeoutMs)]);
}

static bool isSelected(SettingsOption option) {
  return app.settings.selectedOption == option;
}

static void drawRow(
  M5Canvas& canvas,
  int x, int y, int w, int h,
  const char* label,
  const String& value,
  bool highlighted = false
) {
  uint16_t bg = highlighted ? UITheme::COLOR_BUTTON_HL : UITheme::COLOR_SURFACE;

  canvas.fillRoundRect(x, y, w, h, 14, bg);

  int textY = y + (h / 2) + 1;

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(ML_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, bg);
  canvas.drawString(label, x + 12, textY);

  canvas.setTextDatum(MR_DATUM);
  canvas.setTextColor(UITheme::COLOR_TEXT, bg);
  canvas.drawString(value, x + w - 12, textY);
}



// ── Editor DateTime ───────────────────────────────────────────────────────

static void drawTimeSourceEditor(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);

  drawRow(canvas, 12, CONTENT_Y + 8,   296, 28, "Time Source", buildTimeSourceLabel(), true);
  drawRow(canvas, 12, CONTENT_Y + 44,  296, 28, "Manual Time", buildManualSetLabel());
  drawRow(canvas, 12, CONTENT_Y + 80,  296, 28, "Date",        buildEditorDate());
  drawRow(canvas, 12, CONTENT_Y + 116, 296, 28, "Time",        buildEditorTime());
  drawRow(canvas, 12, CONTENT_Y + 152, 296, 28, "Field",       buildEditorFieldLabel());

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_BG);
  canvas.drawString("A:-  B:next/save  C:+", 18, CONTENT_Y + 192);
}



// ── Editor Standby ────────────────────────────────────────────────────────

static void drawStandbyEditor(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_BG);
  canvas.drawString("Standby Timeout", 18, CONTENT_Y + 10);

  int centerY = CONTENT_Y + 68;

  canvas.setTextDatum(ML_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_BG);
  canvas.drawString("<", 20, centerY);

  canvas.fillRoundRect(60, centerY - 18, 200, 36, 14, UITheme::COLOR_BUTTON_HL);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_BUTTON_HL);
  canvas.drawString(buildStandbyEditorLabel(), 160, centerY);

  canvas.setTextDatum(MR_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_BG);
  canvas.drawString(">", 300, centerY);

  int currentIdx = standbyTimeoutIndex(app.standbyEditor.timeoutMs);
  int dotsTotalW = STANDBY_OPTION_COUNT * 14;
  int dotsStartX = (SCREEN_W - dotsTotalW) / 2;
  int dotsY      = centerY + 30;

  for (int i = 0; i < STANDBY_OPTION_COUNT; i++) {
    uint16_t dotColor = (i == currentIdx) ? UITheme::COLOR_BUTTON : UITheme::COLOR_SURFACE;
    int radius        = (i == currentIdx) ? 4 : 2;
    canvas.fillCircle(dotsStartX + i * 14 + 7, dotsY, radius, dotColor);
  }

  canvas.drawFastHLine(12, centerY + 52, 296, UITheme::COLOR_SURFACE);

  bool saveHL = (app.standbyEditor.field == STANDBY_EDIT_SAVE);
  drawRow(canvas, 12, centerY + 60, 296, 28,
          "Save", buildStandbyEditorLabel(), saveHL);

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_BG);
  canvas.drawString("A:-  B:save  C:+", 18, CONTENT_Y + 185);
}



// ── Editor Brightness ─────────────────────────────────────────────────────

static void drawBrightnessEditor(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_BG);
  canvas.drawString("Brightness", 18, CONTENT_Y + 10);

  int centerY = CONTENT_Y + 68;

  canvas.setTextDatum(ML_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_BG);
  canvas.drawString("<", 20, centerY);

  String valStr = String(app.brightnessEditor.percent) + "%";
  canvas.fillRoundRect(60, centerY - 18, 200, 36, 14, UITheme::COLOR_BUTTON_HL);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_BUTTON_HL);
  canvas.drawString(valStr, 160, centerY);

  canvas.setTextDatum(MR_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_BG);
  canvas.drawString(">", 300, centerY);

  int barX      = 18;
  int barY      = centerY + 30;
  int barW      = 284;
  int segCount  = 10;
  int segGap    = 3;
  int segW      = (barW - segGap * (segCount - 1)) / segCount;
  int activeSegs = app.brightnessEditor.percent / 10;

  for (int i = 0; i < segCount; i++) {
    int sx       = barX + i * (segW + segGap);
    bool active  = (i < activeSegs);
    uint16_t col = active ? UITheme::COLOR_BUTTON : UITheme::COLOR_SURFACE;
    canvas.fillRoundRect(sx, barY, segW, 8, 3, col);
  }

  canvas.drawFastHLine(12, centerY + 52, 296, UITheme::COLOR_SURFACE);

  bool saveHL = (app.brightnessEditor.field == BRIGHTNESS_EDIT_SAVE);
  drawRow(canvas, 12, centerY + 60, 296, 28,
          "Save", valStr, saveHL);

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_BG);
  canvas.drawString("A:-10%  B:save  C:+10%", 18, CONTENT_Y + 185);
}



// ── Editor Step Target ────────────────────────────────────────────────────

static void drawStepTargetEditor(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_BG);
  canvas.drawString("Step Target", 18, CONTENT_Y + 10);

  int centerY = CONTENT_Y + 68;

  canvas.setTextDatum(ML_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_BG);
  canvas.drawString("<", 20, centerY);

  String valStr = String(app.stepTargetEditor.targetValue);
  canvas.fillRoundRect(60, centerY - 18, 200, 36, 14, UITheme::COLOR_BUTTON_HL);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_BUTTON_HL);
  canvas.drawString(valStr, 160, centerY);

  canvas.setTextDatum(MR_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_BG);
  canvas.drawString(">", 300, centerY);

  int barX      = 18;
  int barY      = centerY + 30;
  int barW      = 284;
  int segCount  = 10;
  int segGap    = 3;
  int segW      = (barW - segGap * (segCount - 1)) / segCount;
  int activeSegs = app.stepTargetEditor.targetValue / 2000;
  if (activeSegs < 1)  activeSegs = 1;
  if (activeSegs > 10) activeSegs = 10;

  for (int i = 0; i < segCount; i++) {
    int sx       = barX + i * (segW + segGap);
    bool active  = (i < activeSegs);
    uint16_t col = active ? UITheme::COLOR_BUTTON : UITheme::COLOR_SURFACE;
    canvas.fillRoundRect(sx, barY, segW, 8, 3, col);
  }

  canvas.drawFastHLine(12, centerY + 52, 296, UITheme::COLOR_SURFACE);

  bool saveHL = (app.stepTargetEditor.field == STEP_TARGET_EDIT_SAVE);
  drawRow(canvas, 12, centerY + 60, 296, 28, "Save", valStr, saveHL);

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_BG);
  canvas.drawString("A:-500  B:save  C:+500", 18, CONTENT_Y + 185);
}



// ── Overview ──────────────────────────────────────────────────────────────

struct SettingsRowDef {
  SettingsOption option;
  const char*    label;
};

static const SettingsRowDef ALL_ROWS[] = {
  { SETTINGS_OPTION_BLUETOOTH,       "Bluetooth"    },
  { SETTINGS_OPTION_TIME_SOURCE,     "Time Source"  },
  { SETTINGS_OPTION_BRIGHTNESS,      "Brightness"   },
  { SETTINGS_OPTION_STANDBY_TIMEOUT, "Standby"      },
  { SETTINGS_OPTION_STEP_TARGET,     "Step Target"  },  
  { SETTINGS_OPTION_BATTERY,         "Battery"      },
  { SETTINGS_OPTION_DEVICE,          "Device"       },
  { SETTINGS_OPTION_FIRMWARE,        "Firmware"     },
};
static const int TOTAL_ROWS = sizeof(ALL_ROWS) / sizeof(ALL_ROWS[0]);

static String buildRowValue(SettingsOption option) {
  switch (option) {
    case SETTINGS_OPTION_BLUETOOTH:
      return app.ble.state == CONNECTION_CONNECTED ? "Connected" : "Disconnected";
    case SETTINGS_OPTION_TIME_SOURCE:
      return buildTimeSourceLabel();
    case SETTINGS_OPTION_BRIGHTNESS:
      return String(app.settings.brightnessPercent) + "%";
    case SETTINGS_OPTION_STANDBY_TIMEOUT:
      return buildStandbyLabel();
    case SETTINGS_OPTION_STEP_TARGET:       
      return String(app.health.stepTarget);
    case SETTINGS_OPTION_BATTERY: {
      int level = app.settings.batteryPercent;
      if (M5.Power.isCharging()) return String(level) + "%+";
      return String(level) + "%";
    }
    case SETTINGS_OPTION_DEVICE:
      return app.settings.deviceName;
    case SETTINGS_OPTION_FIRMWARE:
      return app.settings.firmwareVersion;
    default:
      return "--";
  }
}

static void drawSettingsOverview(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);

  int offset = app.settings.scrollOffset;
  if (offset > TOTAL_ROWS - VISIBLE_ROWS) offset = TOTAL_ROWS - VISIBLE_ROWS;
  if (offset < 0) offset = 0;

  for (int i = 0; i < VISIBLE_ROWS; i++) {
    int rowIdx = offset + i;
    if (rowIdx >= TOTAL_ROWS) break;

    drawRow(
      canvas,
      12, CONTENT_Y + 8 + i * ROW_STRIDE, 296, ROW_H,
      ALL_ROWS[rowIdx].label,
      buildRowValue(ALL_ROWS[rowIdx].option),
      isSelected(ALL_ROWS[rowIdx].option)
    );
  }

  if (TOTAL_ROWS > VISIBLE_ROWS) {
    int dotX = SCREEN_W - 10;
    for (int i = 0; i < TOTAL_ROWS; i++) {
      int dotY   = CONTENT_Y + 8 + i * 10;
      bool active = (i >= offset && i < offset + VISIBLE_ROWS);
      canvas.fillCircle(dotX, dotY, 2,
        active ? UITheme::COLOR_BUTTON : UITheme::COLOR_SURFACE);
    }
  }

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_BG);
  canvas.drawString("C: next   hold C: open", 18, CONTENT_Y + 192);
}



// ── Entry point ───────────────────────────────────────────────────────────

void drawPageSettings(M5Canvas& canvas) {
  if (app.brightnessEditor.active) {
    drawBrightnessEditor(canvas);
    return;
  }

  if (app.stepTargetEditor.active) {    
    drawStepTargetEditor(canvas);
    return;
  }

  if (app.standbyEditor.active) {
    drawStandbyEditor(canvas);
    return;
  }

  if (app.dateTimeEditor.active) {
    drawTimeSourceEditor(canvas);
    return;
  }

  drawSettingsOverview(canvas);
}