#include <M5Unified.h>
#include "../../app/app_state.h"
#include "../ui_theme.h"
#include "page_home.h"

static const int SCREEN_W = 320;
static const int SCREEN_H = 240;

static const int HEADER_H = 24;
static const int CONTENT_Y = HEADER_H;
static const int CONTENT_H = SCREEN_H - HEADER_H;

static String twoDigits(int value) {
  if (value < 10) return "0" + String(value);
  return String(value);
}

static String buildTimeString() {
  return twoDigits(app.time.hour) + ":" + twoDigits(app.time.minute);
}

static String buildDateString() {
  return twoDigits(app.time.day) + "/" + twoDigits(app.time.month) + "/" + String(app.time.year);
}

static String buildWeatherString() {
  if (app.weather.availability == DATA_AVAILABLE) {
    return String(app.weather.temperatureC, 1) + " C " + app.weather.conditionText;
  }
  return "No weather data";
}

static String buildAlarmTimeString() {
  return twoDigits(app.alarmEditor.hour) + ":" + twoDigits(app.alarmEditor.minute);
}

static String buildAlarmEnabledString() {
  return app.alarmEditor.enabled ? "On" : "Off";
}

static const char* buildAlarmFieldLabel() {
  switch (app.alarmEditor.field) {
    case ALARM_EDIT_HOUR: return "Hour";
    case ALARM_EDIT_MINUTE: return "Minute";
    case ALARM_EDIT_ENABLED: return "Enabled";
    case ALARM_EDIT_SAVE: return "Save";
    case ALARM_EDIT_NONE:
    default: return "--";
  }
}

static String buildTimerEditorTimeString() {
  return twoDigits(app.timerEditor.minutes) + ":" + twoDigits(app.timerEditor.seconds);
}

static const char* buildTimerFieldLabel() {
  switch (app.timerEditor.field) {
    case TIMER_EDIT_MINUTES: return "Minutes";
    case TIMER_EDIT_SECONDS: return "Seconds";
    case TIMER_EDIT_START: return "Start";
    case TIMER_EDIT_NONE:
    default: return "--";
  }
}

static String formatClockMs(uint32_t ms) {
  uint32_t totalSeconds = ms / 1000;
  uint32_t minutes = totalSeconds / 60;
  uint32_t seconds = totalSeconds % 60;
  return twoDigits(static_cast<int>(minutes)) + ":" + twoDigits(static_cast<int>(seconds));
}

static String formatChronoMs(uint32_t ms) {
  uint32_t totalCentiseconds = ms / 10;
  uint32_t minutes = totalCentiseconds / 6000;
  uint32_t seconds = (totalCentiseconds / 100) % 60;
  uint32_t centiseconds = totalCentiseconds % 100;

  return twoDigits(static_cast<int>(minutes)) + ":" +
         twoDigits(static_cast<int>(seconds)) + "." +
         twoDigits(static_cast<int>(centiseconds));
}

static const char* buildTimerStateLabel() {
  if (app.timer.running) return "Running";
  if (app.timer.paused) return "Paused";
  if (app.timer.finished) return "Done";
  return "Ready";
}

static const char* buildChronoStateLabel() {
  if (app.stopwatch.running) return "Running";
  if (app.stopwatch.paused) return "Paused";
  return "Ready";
}

static void drawCenteredTopText(M5Canvas& canvas, int centerX, int topY, const String& text) {
  int w = UITheme::textWidth(canvas, text);
  canvas.setTextDatum(TL_DATUM);
  canvas.drawString(text, centerX - (w / 2), topY);
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

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, bg);
  canvas.drawString(label, x + 12, y + 7);

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TR_DATUM);
  canvas.setTextColor(UITheme::COLOR_TEXT, bg);
  canvas.drawString(value, x + w - 12, y + 7);
}

static void drawAlarmRingingView(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);
  canvas.fillRoundRect(12, CONTENT_Y + 10, 296, 186, 18, UITheme::COLOR_SURFACE);

  UITheme::applyFontClock(canvas);
  canvas.setTextColor(UITheme::COLOR_WARNING, UITheme::COLOR_SURFACE);
  drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 28, "ALARM");

  UITheme::applyFontClock(canvas);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 84,
                      twoDigits(app.alarm.hour) + ":" + twoDigits(app.alarm.minute));

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 138, "Press any button to dismiss");
}

static void drawTimerFinishedView(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);
  canvas.fillRoundRect(12, CONTENT_Y + 10, 296, 186, 18, UITheme::COLOR_SURFACE);

  UITheme::applyFontClock(canvas);
  canvas.setTextColor(UITheme::COLOR_WARNING, UITheme::COLOR_SURFACE);
  drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 28, "TIMER");

  UITheme::applyFontClock(canvas);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 84, "00:00");

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_SUCCESS, UITheme::COLOR_SURFACE);
  drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 138, "Time is up");

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 164, "Press any button");
}

static void drawAlarmEditorView(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);

  drawRow(canvas, 12, CONTENT_Y + 8,   296, 30, "Alarm", buildAlarmTimeString(), false);
  drawRow(canvas, 12, CONTENT_Y + 46,  296, 30, "Hour", String(app.alarmEditor.hour),
          app.alarmEditor.field == ALARM_EDIT_HOUR);
  drawRow(canvas, 12, CONTENT_Y + 84,  296, 30, "Minute", String(app.alarmEditor.minute),
          app.alarmEditor.field == ALARM_EDIT_MINUTE);
  drawRow(canvas, 12, CONTENT_Y + 122, 296, 30, "Enabled", buildAlarmEnabledString(),
          app.alarmEditor.field == ALARM_EDIT_ENABLED);
  drawRow(canvas, 12, CONTENT_Y + 160, 296, 30, "Field", buildAlarmFieldLabel(),
          app.alarmEditor.field == ALARM_EDIT_SAVE);

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_BG);
  canvas.drawString("A:-  B:next/save  C:+", 18, CONTENT_Y + 198);
}

static void drawTimerEditorView(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);

  drawRow(canvas, 12, CONTENT_Y + 8,   296, 30, "Timer", buildTimerEditorTimeString(), false);
  drawRow(canvas, 12, CONTENT_Y + 46,  296, 30, "Minutes", String(app.timerEditor.minutes),
          app.timerEditor.field == TIMER_EDIT_MINUTES);
  drawRow(canvas, 12, CONTENT_Y + 84,  296, 30, "Seconds", String(app.timerEditor.seconds),
          app.timerEditor.field == TIMER_EDIT_SECONDS);
  drawRow(canvas, 12, CONTENT_Y + 122, 296, 30, "Action", "Start",
          app.timerEditor.field == TIMER_EDIT_START);
  drawRow(canvas, 12, CONTENT_Y + 160, 296, 30, "Field", buildTimerFieldLabel(), false);

  UITheme::applyFontHeader(canvas);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_BG);
  canvas.drawString("A:-  B:next/start  C:+", 18, CONTENT_Y + 198);
}

static void drawTimerRunningView(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);
  canvas.fillRoundRect(12, CONTENT_Y + 10, 296, 186, 18, UITheme::COLOR_SURFACE);

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 24, buildTimerStateLabel());

  UITheme::applyFontClock(canvas);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 62, formatClockMs(app.timer.remainingMs));

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_WEATHER, UITheme::COLOR_SURFACE);

  if (app.timer.running) {
    drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 126, "A Pause  C Reset");
  } else if (app.timer.paused) {
    drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 126, "A Resume  C Reset");
  } else {
    drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 126, "Timer ready");
  }

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 154, "B Home");
}

static void drawChronoView(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);
  canvas.fillRoundRect(12, CONTENT_Y + 10, 296, 186, 18, UITheme::COLOR_SURFACE);

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 24, buildChronoStateLabel());

  UITheme::applyFontClock(canvas);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 62, formatChronoMs(app.stopwatch.elapsedMs));

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_WEATHER, UITheme::COLOR_SURFACE);

  if (app.stopwatch.running) {
    drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 126, "A Pause  C Reset");
  } else if (app.stopwatch.paused) {
    drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 126, "A Resume  C Reset");
  } else {
    drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 126, "B Start");
  }

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  drawCenteredTopText(canvas, SCREEN_W / 2, CONTENT_Y + 154, "Chronometer");
}

static void drawNormalHome(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);
  canvas.fillRoundRect(12, CONTENT_Y + 8, 296, 122, 18, UITheme::COLOR_SURFACE);

  const int centerX = 12 + 296 / 2;

  UITheme::applyFontClock(canvas);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  drawCenteredTopText(canvas, centerX, CONTENT_Y + 18, buildTimeString());

  UITheme::applyFontBody(canvas);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  drawCenteredTopText(canvas, centerX, CONTENT_Y + 72, buildDateString());

  UITheme::applyFontWeather(canvas);
  canvas.setTextColor(UITheme::COLOR_WEATHER, UITheme::COLOR_SURFACE);
  drawCenteredTopText(canvas, centerX, CONTENT_Y + 102, buildWeatherString());

  if (app.homeStatusMessage.length() > 0) {
    UITheme::applyFontHeader(canvas);
    canvas.setTextColor(UITheme::COLOR_SUCCESS, UITheme::COLOR_SURFACE);
    drawCenteredTopText(canvas, centerX, CONTENT_Y + 118, app.homeStatusMessage);
  }

  auto drawActionButton = [&](int x, int y, int w, int h, const char* label, bool highlighted) {
    uint16_t bg = highlighted ? UITheme::COLOR_BUTTON_HL : UITheme::COLOR_BUTTON;
    canvas.fillRoundRect(x, y, w, h, 14, bg);

    UITheme::applyFontHeader(canvas);
    canvas.setTextColor(UITheme::COLOR_TEXT, bg);
    canvas.setTextDatum(MC_DATUM);
    canvas.drawString(label, x + w / 2, y + h / 2);
  };

  drawActionButton(12, CONTENT_Y + 142, 92, 44, "Alarm", app.homeSelectedAction == HOME_ACTION_ALARM);
  drawActionButton(114, CONTENT_Y + 142, 92, 44, "Chrono", app.homeSelectedAction == HOME_ACTION_CHRONO);
  drawActionButton(216, CONTENT_Y + 142, 92, 44, "Timer", app.homeSelectedAction == HOME_ACTION_TIMER);
}

void drawPageHome(M5Canvas& canvas) {
  if (app.alarm.ringing) {
    drawAlarmRingingView(canvas);
    return;
  }

  if (app.timer.finished) {
    drawTimerFinishedView(canvas);
    return;
  }

  if (app.alarmEditor.active) {
    drawAlarmEditorView(canvas);
    return;
  }

  if (app.timerEditor.active) {
    drawTimerEditorView(canvas);
    return;
  }

  if (app.timer.running || app.timer.paused) {
    drawTimerRunningView(canvas);
    return;
  }

  if (app.homeSelectedAction == HOME_ACTION_CHRONO ||
      app.stopwatch.running ||
      app.stopwatch.paused ||
      app.stopwatch.elapsedMs > 0) {
    drawChronoView(canvas);
    return;
  }

  drawNormalHome(canvas);
}

void drawPagePlaceholder(M5Canvas& canvas) {
  canvas.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UITheme::COLOR_BG);
  canvas.fillRoundRect(12, CONTENT_Y + 12, SCREEN_W - 24, CONTENT_H - 24, 14, UITheme::COLOR_SURFACE);

  UITheme::applyFontBody(canvas);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(UITheme::COLOR_TEXT, UITheme::COLOR_SURFACE);
  canvas.drawString(getPageTitle(app.currentPage), SCREEN_W / 2, CONTENT_Y + 50);

  UITheme::applyFontHeader(canvas);
  canvas.setTextColor(UITheme::COLOR_SUBTEXT, UITheme::COLOR_SURFACE);
  canvas.drawString("Page coming next", SCREEN_W / 2, CONTENT_Y + 80);
}