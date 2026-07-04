#include <M5Unified.h>
#include "../app/app_state.h"
#include "../sensors/imu/fall_detection.h"
#include "../services/alarm_service.h"
#include "../services/chrono_service.h"
#include "../services/rtc_service.h"
#include "../services/timer_service.h"
#include "../services/standby_service.h"
#include "../ui/ui.h"


static bool settingsBtnCLongPressHandled = false;


static bool pointInRect(int px, int py, int x, int y, int w, int h) {
  return px >= x && px <= (x + w) && py >= y && py <= (y + h);
}


static bool isLeapYear(int year) {
  return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}


static int daysInMonth(int month, int year) {
  switch (month) {
    case 1:  return 31;
    case 2:  return isLeapYear(year) ? 29 : 28;
    case 3:  return 31;
    case 4:  return 30;
    case 5:  return 31;
    case 6:  return 30;
    case 7:  return 31;
    case 8:  return 31;
    case 9:  return 30;
    case 10: return 31;
    case 11: return 30;
    case 12: return 31;
    default: return 30;
  }
}


static void triggerHomeAction(HomeAction action, const String& label) {
  app.homeSelectedAction = action;
  app.homeStatusMessage  = label + " opened";
  app.homeStatusUntilMs  = millis() + 1200;

  if (action == HOME_ACTION_ALARM) {
    app.alarmEditor.active  = true;
    app.alarmEditor.field   = ALARM_EDIT_HOUR;
    app.alarmEditor.hour    = app.alarm.hour;
    app.alarmEditor.minute  = app.alarm.minute;
    app.alarmEditor.enabled = app.alarm.enabled;
    app.homeStatusMessage   = "";
    app.homeStatusUntilMs   = 0;
  }

  if (action == HOME_ACTION_TIMER) {
    app.timerEditor.active = true;
    app.timerEditor.field  = TIMER_EDIT_MINUTES;

    uint32_t baseMs = app.timer.durationMs;
    if (app.timer.running || app.timer.paused || app.timer.finished) {
      baseMs = app.timer.remainingMs;
    }

    uint32_t totalSeconds   = baseMs / 1000;
    app.timerEditor.minutes = (totalSeconds / 60) % 100;
    app.timerEditor.seconds = totalSeconds % 60;

    app.homeStatusMessage = "";
    app.homeStatusUntilMs = 0;
  }

  if (action == HOME_ACTION_CHRONO) {
    app.homeStatusMessage = "";
    app.homeStatusUntilMs = 0;
  }

  requestRedraw();
}


static void changePage(PageId nextPage) {
  if (app.currentPage != nextPage) {
    app.previousPage       = app.currentPage;
    app.currentPage        = nextPage;
    app.pageChanged        = true;
    app.homeSelectedAction = HOME_ACTION_NONE;
    app.homeStatusMessage  = "";
    app.homeStatusUntilMs  = 0;
  }
}


static void goToNextPage() {
  int nextPage = static_cast<int>(app.currentPage) + 1;
  if (nextPage >= static_cast<int>(PAGE_COUNT)) nextPage = 0;
  changePage(static_cast<PageId>(nextPage));
}


static void goToPreviousPage() {
  int prevPage = static_cast<int>(app.currentPage) - 1;
  if (prevPage < 0) prevPage = static_cast<int>(PAGE_COUNT) - 1;
  changePage(static_cast<PageId>(prevPage));
}


static void closeAlarmEditor() {
  app.alarmEditor.active = false;
  app.alarmEditor.field  = ALARM_EDIT_NONE;
  app.homeSelectedAction = HOME_ACTION_NONE;
  requestRedraw();
}


static void closeTimerEditor() {
  app.timerEditor.active = false;
  app.timerEditor.field  = TIMER_EDIT_NONE;
  app.homeSelectedAction = HOME_ACTION_NONE;
  requestRedraw();
}


// ── Standby editor helpers ────────────────────────────────────────────────

static const uint32_t STANDBY_OPTIONS[]    = { 0, 15000, 30000, 60000, 120000, 300000 };
static const int      STANDBY_OPTION_COUNT = 6;

static void beginStandbyEditor() {
  app.standbyEditor.active    = true;
  app.standbyEditor.field     = STANDBY_EDIT_TIMEOUT;
  app.standbyEditor.timeoutMs = app.standby.timeoutMs;
  app.homeStatusMessage       = "";
  app.homeStatusUntilMs       = 0;
  requestRedraw();
}

static void closeStandbyEditor() {
  app.standbyEditor.active = false;
  app.standbyEditor.field  = STANDBY_EDIT_NONE;
  requestRedraw();
}

static void saveStandbyEditor() {
  app.standby.timeoutMs      = app.standbyEditor.timeoutMs;
  app.standby.lastActivityMs = millis();
  app.standbyEditor.active   = false;
  app.standbyEditor.field    = STANDBY_EDIT_NONE;
  app.homeStatusMessage      = app.standby.timeoutMs == 0
                                 ? "Standby: Off"
                                 : "Standby saved";
  app.homeStatusUntilMs = millis() + 1200;
  requestRedraw();
}

static void changeStandbyValue(int delta) {
  int idx = 2;  // fallback 30s
  for (int i = 0; i < STANDBY_OPTION_COUNT; i++) {
    if (STANDBY_OPTIONS[i] == app.standbyEditor.timeoutMs) { idx = i; break; }
  }
  idx += delta;
  if (idx < 0) idx = STANDBY_OPTION_COUNT - 1;
  if (idx >= STANDBY_OPTION_COUNT) idx = 0;
  app.standbyEditor.timeoutMs = STANDBY_OPTIONS[idx];
  requestRedraw();
}


// ── Brightness editor helpers ─────────────────────────────────────────────

static void beginBrightnessEditor() {
  app.brightnessEditor.active  = true;
  app.brightnessEditor.field   = BRIGHTNESS_EDIT_VALUE;
  app.brightnessEditor.percent = app.settings.brightnessPercent;
  app.homeStatusMessage        = "";
  app.homeStatusUntilMs        = 0;
  requestRedraw();
}

static void closeBrightnessEditor() {
  app.brightnessEditor.active = false;
  app.brightnessEditor.field  = BRIGHTNESS_EDIT_NONE;
  requestRedraw();
}

static void saveBrightnessEditor() {
  app.settings.brightnessPercent = app.brightnessEditor.percent;
  M5.Display.setBrightness(
    static_cast<uint8_t>(app.brightnessEditor.percent * 255 / 100)
  );
  app.brightnessEditor.active = false;
  app.brightnessEditor.field  = BRIGHTNESS_EDIT_NONE;
  app.homeStatusMessage       = "Brightness saved";
  app.homeStatusUntilMs       = millis() + 1200;
  requestRedraw();
}

static void changeBrightnessValue(int delta) {
  int next = app.brightnessEditor.percent + delta * 10;
  if (next < 10)  next = 10;
  if (next > 100) next = 100;
  app.brightnessEditor.percent = next;
  // Preview live
  M5.Display.setBrightness(
    static_cast<uint8_t>(next * 255 / 100)
  );
  requestRedraw();
}

// ── Step target editor helpers ────────────────────────────────────────────

static void beginStepTargetEditor() {
  if (app.ble.state == CONNECTION_CONNECTED) {
    app.homeStatusMessage = "Phone sync active";
    app.homeStatusUntilMs = millis() + 1500;
    requestRedraw();
    return;
  }
  app.stepTargetEditor.active      = true;
  app.stepTargetEditor.field       = STEP_TARGET_EDIT_VALUE;
  app.stepTargetEditor.targetValue = app.health.stepTarget;
  app.homeStatusMessage            = "";
  app.homeStatusUntilMs            = 0;
  requestRedraw();
}

static void closeStepTargetEditor() {
  app.stepTargetEditor.active = false;
  app.stepTargetEditor.field  = STEP_TARGET_EDIT_NONE;
  requestRedraw();
}

static void saveStepTargetEditor() {
  app.health.stepTarget           = app.stepTargetEditor.targetValue;
  app.stepTargetEditor.active     = false;
  app.stepTargetEditor.field      = STEP_TARGET_EDIT_NONE;
  app.homeStatusMessage           = "Step target saved";
  app.homeStatusUntilMs           = millis() + 1200;
  requestRedraw();
}

static void changeStepTargetValue(int delta) {
  int next = app.stepTargetEditor.targetValue + delta * 500;
  if (next < 1000)  next = 1000;   // min 1000
  if (next > 20000) next = 20000;  // max 20000
  app.stepTargetEditor.targetValue = next;
  requestRedraw();
}

static bool handleFallDetectionInput() {
  if (!isFallOverlayActive()) return false;

  if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
    confirmUserIsOk();
    app.homeStatusMessage = "Fall alert dismissed";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return true;
  }

  auto touch = M5.Touch.getDetail();
  if (touch.wasPressed()) {
    confirmUserIsOk();
    app.homeStatusMessage = "Fall alert dismissed";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return true;
  }

  return true;
}


static bool isChronoVisible() {
  if (app.currentPage != PAGE_HOME) return false;
  if (app.alarm.ringing)            return false;
  if (app.timer.finished)           return false;
  if (app.alarmEditor.active)       return false;
  if (app.timerEditor.active)       return false;
  if (app.timer.running || app.timer.paused) return false;

  return app.homeSelectedAction == HOME_ACTION_CHRONO ||
         app.stopwatch.running  ||
         app.stopwatch.paused   ||
         app.stopwatch.elapsedMs > 0;
}


static void closeChronoView() {
  if (app.stopwatch.running || app.stopwatch.paused || app.stopwatch.elapsedMs > 0) {
    return;
  }
  app.homeSelectedAction = HOME_ACTION_NONE;
  requestRedraw();
}


static void handleChronoStartAction() {
  if (!app.stopwatch.running && !app.stopwatch.paused && app.stopwatch.elapsedMs == 0) {
    startChrono();
    app.homeSelectedAction = HOME_ACTION_CHRONO;
    app.homeStatusMessage  = "";
    app.homeStatusUntilMs  = 0;
    requestRedraw();
  }
}


static void handleChronoPrimaryAction() {
  if (app.stopwatch.running) {
    pauseChrono();
  } else if (app.stopwatch.paused) {
    resumeChrono();
  }
  app.homeSelectedAction = HOME_ACTION_CHRONO;
  app.homeStatusMessage  = "";
  app.homeStatusUntilMs  = 0;
  requestRedraw();
}


static void handleChronoResetAction() {
  resetChrono();
  app.homeSelectedAction = HOME_ACTION_NONE;
  app.homeStatusMessage  = "";
  app.homeStatusUntilMs  = 0;
  requestRedraw();
}


static void goBack() {
  if (app.alarm.ringing) {
    stopAlarmRinging();
    app.homeStatusMessage = "Alarm dismissed";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return;
  }

  if (app.timer.finished) {
    stopTimerAlert();
    app.homeStatusMessage = "Timer dismissed";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return;
  }

  if (app.brightnessEditor.active) {   
    M5.Display.setBrightness(
      static_cast<uint8_t>(app.settings.brightnessPercent * 255 / 100)
    );
    closeBrightnessEditor();
    app.homeStatusMessage = "Brightness cancelled";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return;
  }

  if (app.stepTargetEditor.active) {
  closeStepTargetEditor();
  app.homeStatusMessage = "Step target cancelled";
  app.homeStatusUntilMs = millis() + 1200;
  requestRedraw();
  return;
  }

  if (app.standbyEditor.active) {
    closeStandbyEditor();
    app.homeStatusMessage = "Standby edit cancelled";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return;
  }

  if (app.alarmEditor.active) {
    closeAlarmEditor();
    app.homeStatusMessage = "Alarm edit cancelled";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return;
  }

  if (app.timerEditor.active) {
    closeTimerEditor();
    app.homeStatusMessage = "Timer edit cancelled";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return;
  }

  if (isChronoVisible()) {
    closeChronoView();
    return;
  }

  if (app.dateTimeEditor.active) {
    app.dateTimeEditor.active = false;
    app.dateTimeEditor.field  = EDIT_FIELD_NONE;
    app.homeStatusMessage     = "Edit cancelled";
    app.homeStatusUntilMs     = millis() + 1000;
    requestRedraw();
    return;
  }

  PageId temp            = app.currentPage;
  app.currentPage        = app.previousPage;
  app.previousPage       = temp;
  app.pageChanged        = true;
  app.homeSelectedAction = HOME_ACTION_NONE;
  app.homeStatusMessage  = "";
  app.homeStatusUntilMs  = 0;
}


static void goHome() {
  if (app.alarm.ringing) {
    stopAlarmRinging();
    app.homeStatusMessage = "Alarm dismissed";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return;
  }

  if (app.timer.finished) {
    stopTimerAlert();
    app.homeStatusMessage = "Timer dismissed";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return;
  }

  if (app.standbyEditor.active    || app.dateTimeEditor.active ||
    app.alarmEditor.active      || app.timerEditor.active    ||
    app.brightnessEditor.active || app.stepTargetEditor.active) {
    return;
  }

  changePage(PAGE_HOME);
}


static void clampEditorDay() {
  int maxDay = daysInMonth(app.dateTimeEditor.month, app.dateTimeEditor.year);
  if (app.dateTimeEditor.day > maxDay) app.dateTimeEditor.day = maxDay;
  if (app.dateTimeEditor.day < 1)      app.dateTimeEditor.day = 1;
}


static void beginDateTimeEditor() {
  app.dateTimeEditor.active = true;
  app.dateTimeEditor.field  = EDIT_FIELD_DAY;
  app.dateTimeEditor.day    = app.time.day;
  app.dateTimeEditor.month  = app.time.month;
  app.dateTimeEditor.year   = app.time.year;
  app.dateTimeEditor.hour   = app.time.hour;
  app.dateTimeEditor.minute = app.time.minute;
  app.homeStatusMessage     = "";
  app.homeStatusUntilMs     = 0;
  requestRedraw();
}


static void changeAlarmEditorValue(int delta) {
  switch (app.alarmEditor.field) {
    case ALARM_EDIT_HOUR:
      app.alarmEditor.hour += delta;
      if (app.alarmEditor.hour < 0)  app.alarmEditor.hour = 23;
      if (app.alarmEditor.hour > 23) app.alarmEditor.hour = 0;
      break;
    case ALARM_EDIT_MINUTE:
      app.alarmEditor.minute += delta;
      if (app.alarmEditor.minute < 0)  app.alarmEditor.minute = 59;
      if (app.alarmEditor.minute > 59) app.alarmEditor.minute = 0;
      break;
    case ALARM_EDIT_ENABLED:
      app.alarmEditor.enabled = !app.alarmEditor.enabled;
      break;
    default:
      break;
  }
  requestRedraw();
}


static void saveAlarmEditor() {
  setAlarmTime(app.alarmEditor.hour, app.alarmEditor.minute);
  enableAlarm(app.alarmEditor.enabled);

  app.alarmEditor.active = false;
  app.alarmEditor.field  = ALARM_EDIT_NONE;
  app.homeSelectedAction = HOME_ACTION_NONE;
  app.homeStatusMessage  = "Alarm saved";
  app.homeStatusUntilMs  = millis() + 1200;
  requestRedraw();
}


static void advanceAlarmEditorField() {
  switch (app.alarmEditor.field) {
    case ALARM_EDIT_HOUR:    app.alarmEditor.field = ALARM_EDIT_MINUTE;  break;
    case ALARM_EDIT_MINUTE:  app.alarmEditor.field = ALARM_EDIT_ENABLED; break;
    case ALARM_EDIT_ENABLED: app.alarmEditor.field = ALARM_EDIT_SAVE;    break;
    case ALARM_EDIT_SAVE:    saveAlarmEditor(); return;
    default:                 app.alarmEditor.field = ALARM_EDIT_HOUR;    break;
  }
  requestRedraw();
}


static void changeTimerEditorValue(int delta) {
  switch (app.timerEditor.field) {
    case TIMER_EDIT_MINUTES:
      app.timerEditor.minutes += delta;
      if (app.timerEditor.minutes < 0)  app.timerEditor.minutes = 99;
      if (app.timerEditor.minutes > 99) app.timerEditor.minutes = 0;
      break;
    case TIMER_EDIT_SECONDS:
      app.timerEditor.seconds += delta;
      if (app.timerEditor.seconds < 0)  app.timerEditor.seconds = 59;
      if (app.timerEditor.seconds > 59) app.timerEditor.seconds = 0;
      break;
    default:
      break;
  }
  requestRedraw();
}


static void startTimerFromEditor() {
  uint32_t totalMs =
    static_cast<uint32_t>(app.timerEditor.minutes) * 60000UL +
    static_cast<uint32_t>(app.timerEditor.seconds)  * 1000UL;

  if (totalMs == 0) {
    app.homeStatusMessage = "Set timer first";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return;
  }

  setTimerDurationMs(totalMs);
  startTimer();

  app.timerEditor.active = false;
  app.timerEditor.field  = TIMER_EDIT_NONE;
  app.homeSelectedAction = HOME_ACTION_NONE;
  app.homeStatusMessage  = "Timer started";
  app.homeStatusUntilMs  = millis() + 1200;
  requestRedraw();
}


static void advanceTimerEditorField() {
  switch (app.timerEditor.field) {
    case TIMER_EDIT_MINUTES: app.timerEditor.field = TIMER_EDIT_SECONDS; break;
    case TIMER_EDIT_SECONDS: app.timerEditor.field = TIMER_EDIT_START;   break;
    case TIMER_EDIT_START:   startTimerFromEditor(); return;
    default:                 app.timerEditor.field = TIMER_EDIT_MINUTES; break;
  }
  requestRedraw();
}


static void changeEditorValue(int delta) {
  switch (app.dateTimeEditor.field) {
    case EDIT_FIELD_DAY:
      app.dateTimeEditor.day += delta;
      if (app.dateTimeEditor.day < 1)  app.dateTimeEditor.day = 31;
      if (app.dateTimeEditor.day > 31) app.dateTimeEditor.day = 1;
      clampEditorDay();
      break;
    case EDIT_FIELD_MONTH:
      app.dateTimeEditor.month += delta;
      if (app.dateTimeEditor.month < 1)  app.dateTimeEditor.month = 12;
      if (app.dateTimeEditor.month > 12) app.dateTimeEditor.month = 1;
      clampEditorDay();
      break;
    case EDIT_FIELD_YEAR:
      app.dateTimeEditor.year += delta;
      if (app.dateTimeEditor.year < 2026) app.dateTimeEditor.year = 2099;
      if (app.dateTimeEditor.year > 2099) app.dateTimeEditor.year = 2026;
      clampEditorDay();
      break;
    case EDIT_FIELD_HOUR:
      app.dateTimeEditor.hour += delta;
      if (app.dateTimeEditor.hour < 0)  app.dateTimeEditor.hour = 23;
      if (app.dateTimeEditor.hour > 23) app.dateTimeEditor.hour = 0;
      break;
    case EDIT_FIELD_MINUTE:
      app.dateTimeEditor.minute += delta;
      if (app.dateTimeEditor.minute < 0)  app.dateTimeEditor.minute = 59;
      if (app.dateTimeEditor.minute > 59) app.dateTimeEditor.minute = 0;
      break;
    default:
      break;
  }
  requestRedraw();
}


static void advanceEditorField() {
  switch (app.dateTimeEditor.field) {
    case EDIT_FIELD_DAY:    app.dateTimeEditor.field = EDIT_FIELD_MONTH;  break;
    case EDIT_FIELD_MONTH:  app.dateTimeEditor.field = EDIT_FIELD_YEAR;   break;
    case EDIT_FIELD_YEAR:   app.dateTimeEditor.field = EDIT_FIELD_HOUR;   break;
    case EDIT_FIELD_HOUR:   app.dateTimeEditor.field = EDIT_FIELD_MINUTE; break;
    case EDIT_FIELD_MINUTE: app.dateTimeEditor.field = EDIT_FIELD_SAVE;   break;

    case EDIT_FIELD_SAVE: {
      bool ok = setManualDateTime(
        app.dateTimeEditor.year,
        app.dateTimeEditor.month,
        app.dateTimeEditor.day,
        app.dateTimeEditor.hour,
        app.dateTimeEditor.minute,
        0
      );
      app.dateTimeEditor.active = false;
      app.dateTimeEditor.field  = EDIT_FIELD_NONE;
      app.homeStatusMessage     = ok ? "Time updated" : "Time locked";
      app.homeStatusUntilMs     = millis() + 1500;
      requestRedraw();
      return;
    }

    default:
      app.dateTimeEditor.field = EDIT_FIELD_DAY;
      break;
  }
  requestRedraw();
}


static void cycleSettingsSelection() {
  const int VISIBLE_ROWS = 6;
  int next = static_cast<int>(app.settings.selectedOption) + 1;

  if (next >= static_cast<int>(SETTINGS_OPTION_COUNT)) {
    next = 0;
    app.settings.scrollOffset = 0;
  } else {
    if (next >= app.settings.scrollOffset + VISIBLE_ROWS) {
      app.settings.scrollOffset++;
    }
  }

  app.settings.selectedOption = static_cast<SettingsOption>(next);
  requestRedraw();
}


static void openSelectedSettingsOption() {
  switch (app.settings.selectedOption) {

    case SETTINGS_OPTION_TIME_SOURCE:
      if (app.ble.state == CONNECTION_CONNECTED) {
        app.homeStatusMessage = "Phone sync active";
        app.homeStatusUntilMs = millis() + 1500;
        requestRedraw();
      } else if (isManualTimeSettingAllowed()) {
        beginDateTimeEditor();
      } else {
        app.homeStatusMessage = "Time is locked";
        app.homeStatusUntilMs = millis() + 1500;
        requestRedraw();
      }
      break;

    case SETTINGS_OPTION_BRIGHTNESS:
      beginBrightnessEditor();
      break;

    case SETTINGS_OPTION_STANDBY_TIMEOUT:
      beginStandbyEditor();
      break;

    case SETTINGS_OPTION_STEP_TARGET:
      beginStepTargetEditor();  
      break;

    default:
      app.homeStatusMessage = "Not available yet";
      app.homeStatusUntilMs = millis() + 1000;
      requestRedraw();
      break;
  }
}

static bool handleAlarmRingingInput() {
  if (!app.alarm.ringing) return false;

  if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
    stopAlarmRinging();
    app.homeStatusMessage = "Alarm dismissed";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return true;
  }

  auto touch = M5.Touch.getDetail();
  if (touch.wasPressed()) {
    stopAlarmRinging();
    app.homeStatusMessage = "Alarm dismissed";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return true;
  }

  return false;
}


static bool handleTimerFinishedInput() {
  if (!app.timer.finished) return false;

  if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
    stopTimerAlert();
    app.homeStatusMessage = "Timer dismissed";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return true;
  }

  auto touch = M5.Touch.getDetail();
  if (touch.wasPressed()) {
    stopTimerAlert();
    app.homeStatusMessage = "Timer dismissed";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return true;
  }

  return true;
}


static bool handleAlarmEditorInput() {
  if (!app.alarmEditor.active) return false;

  if (M5.BtnA.wasPressed()) { changeAlarmEditorValue(-1); return true; }
  if (M5.BtnC.wasPressed()) { changeAlarmEditorValue(+1); return true; }
  if (M5.BtnB.wasPressed()) { advanceAlarmEditorField();  return true; }

  auto touch = M5.Touch.getDetail();
  if (touch.wasPressed()) {
    closeAlarmEditor();
    app.homeStatusMessage = "Alarm edit cancelled";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return true;
  }

  return true;
}


static bool handleTimerEditorInput() {
  if (!app.timerEditor.active) return false;

  if (M5.BtnA.wasPressed()) { changeTimerEditorValue(-1); return true; }
  if (M5.BtnC.wasPressed()) { changeTimerEditorValue(+1); return true; }
  if (M5.BtnB.wasPressed()) { advanceTimerEditorField();  return true; }

  auto touch = M5.Touch.getDetail();
  if (touch.wasPressed()) {
    closeTimerEditor();
    app.homeStatusMessage = "Timer edit cancelled";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return true;
  }

  return true;
}


// ── Standby editor input ──────────────────────────────────────────────────
static bool handleStandbyEditorInput() {
  if (!app.standbyEditor.active) return false;

  if (M5.BtnA.wasPressed()) { changeStandbyValue(-1); return true; }
  if (M5.BtnC.wasPressed()) { changeStandbyValue(+1); return true; }
  if (M5.BtnB.wasPressed()) { saveStandbyEditor();    return true; }

  auto touch = M5.Touch.getDetail();
  if (touch.wasPressed()) {
    closeStandbyEditor();
    app.homeStatusMessage = "Standby edit cancelled";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return true;
  }

  return true;
}


// ── Brightness editor input ───────────────────────────────────────────────
static bool handleBrightnessEditorInput() {
  if (!app.brightnessEditor.active) return false;

  if (M5.BtnA.wasPressed()) { changeBrightnessValue(-1); return true; }
  if (M5.BtnC.wasPressed()) { changeBrightnessValue(+1); return true; }
  if (M5.BtnB.wasPressed()) { saveBrightnessEditor();    return true; }

  auto touch = M5.Touch.getDetail();
  if (touch.wasPressed()) {
    // Restaurează luminozitatea veche la cancel
    M5.Display.setBrightness(
      static_cast<uint8_t>(app.settings.brightnessPercent * 255 / 100)
    );
    closeBrightnessEditor();
    app.homeStatusMessage = "Brightness cancelled";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return true;
  }

  return true;
}

static bool handleStepTargetEditorInput() {
  if (!app.stepTargetEditor.active) return false;

  if (M5.BtnA.wasPressed()) { changeStepTargetValue(-1); return true; }
  if (M5.BtnC.wasPressed()) { changeStepTargetValue(+1); return true; }
  if (M5.BtnB.wasPressed()) { saveStepTargetEditor();    return true; }

  auto touch = M5.Touch.getDetail();
  if (touch.wasPressed()) {
    closeStepTargetEditor();
    app.homeStatusMessage = "Step target cancelled";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return true;
  }
  return true;
}

static bool handleTimerRunningInput() {
  if (!app.timer.running) return false;

  if (M5.BtnA.wasPressed()) {
    pauseTimer();
    app.homeStatusMessage = "Timer paused";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return true;
  }
  if (M5.BtnB.wasPressed()) { goHome(); return true; }
  if (M5.BtnC.wasPressed()) {
    resetTimer();
    app.homeStatusMessage = "Timer reset";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return true;
  }

  return false;
}


static bool handleTimerPausedInput() {
  if (!app.timer.paused) return false;

  if (M5.BtnA.wasPressed()) {
    resumeTimer();
    app.homeStatusMessage = "Timer resumed";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return true;
  }
  if (M5.BtnB.wasPressed()) { goHome(); return true; }
  if (M5.BtnC.wasPressed()) {
    resetTimer();
    app.homeStatusMessage = "Timer reset";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return true;
  }

  return false;
}


static bool handleChronoInput() {
  if (!isChronoVisible()) return false;

  if (M5.BtnA.wasPressed()) {
    if (app.stopwatch.running || app.stopwatch.paused) {
      handleChronoPrimaryAction();
    }
    return true;
  }

  if (M5.BtnB.wasPressed()) {
    if (!app.stopwatch.running && !app.stopwatch.paused && app.stopwatch.elapsedMs == 0) {
      handleChronoStartAction();
    } else if (!app.stopwatch.running && !app.stopwatch.paused) {
      closeChronoView();
    }
    return true;
  }

  if (M5.BtnC.wasPressed()) { handleChronoResetAction(); return true; }

  auto touch = M5.Touch.getDetail();
  if (touch.wasPressed()) { closeChronoView(); return true; }

  return true;
}


static bool handleSettingsEditorInput() {
  if (app.currentPage != PAGE_SETTINGS) return false;
  if (!app.dateTimeEditor.active)        return false;

  if (M5.BtnA.wasPressed()) { changeEditorValue(-1); return true; }
  if (M5.BtnC.wasPressed()) { changeEditorValue(+1); return true; }
  if (M5.BtnB.wasPressed()) { advanceEditorField();  return true; }

  return true;
}


static bool handleSettingsMenuInput() {
  if (app.currentPage != PAGE_SETTINGS) {
    settingsBtnCLongPressHandled = false;
    return false;
  }

  if (app.dateTimeEditor.active || app.standbyEditor.active ||
    app.brightnessEditor.active || app.stepTargetEditor.active) {   
    settingsBtnCLongPressHandled = false;
    return false;
  }

  if (!M5.BtnC.isPressed()) {
    settingsBtnCLongPressHandled = false;
  }

  if (!settingsBtnCLongPressHandled && M5.BtnC.pressedFor(2000)) {
    settingsBtnCLongPressHandled = true;
    openSelectedSettingsOption();
    return true;
  }

  if (!settingsBtnCLongPressHandled && M5.BtnC.wasReleased()) {
    cycleSettingsSelection();
    return true;
  }

  return false;
}


static void contextAction() {
  if (app.currentPage == PAGE_HOME) {
    app.homeStatusMessage = "Quick actions";
    app.homeStatusUntilMs = millis() + 1200;
    requestRedraw();
    return;
  }
  app.homeStatusMessage = "";
  app.homeStatusUntilMs = 0;
}


void updateInput() {
  if (app.homeStatusUntilMs > 0 && millis() > app.homeStatusUntilMs) {
    app.homeStatusUntilMs = 0;
    app.homeStatusMessage = "";
    if (!app.alarmEditor.active && !app.timerEditor.active && !isChronoVisible()) {
      app.homeSelectedAction = HOME_ACTION_NONE;
    }
    requestRedraw();
  }

  // ── Standby wake ──────────────────────────────────────────────────────
  if (isStandbyActive()) {
    if (M5.BtnB.wasPressed()) { wakeFromStandby(); return; }
    auto touch = M5.Touch.getDetail();
    if (touch.wasPressed()) { wakeFromStandby(); return; }
    return;
  }

  // ── Notify activity ───────────────────────────────────────────────────
  if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
    notifyUserActivity();
  } else {
    auto touchActivity = M5.Touch.getDetail();
    if (touchActivity.wasPressed() || touchActivity.wasFlicked()) {
      notifyUserActivity();
    }
  }

  if (handleFallDetectionInput())    return;
  if (handleAlarmRingingInput())     return;
  if (handleTimerFinishedInput())    return;
  if (handleAlarmEditorInput())      return;
  if (handleTimerEditorInput())      return;
  if (handleStandbyEditorInput())    return;
  if (handleBrightnessEditorInput()) return;   
  if (handleStepTargetEditorInput()) return;
  if (handleTimerRunningInput())     return;
  if (handleTimerPausedInput())      return;
  if (handleChronoInput())           return;
  if (handleSettingsEditorInput())   return;
  if (handleSettingsMenuInput())     return;

  if (M5.BtnA.wasPressed()) { goBack();        return; }
  if (M5.BtnB.wasPressed()) { goHome();        return; }
  if (M5.BtnC.wasPressed()) { contextAction(); return; }

  auto touch = M5.Touch.getDetail();

  if (!app.alarm.ringing && !app.timer.finished && !isFallOverlayActive() && touch.wasFlicked()) {
    int dx = touch.distanceX();
    int dy = touch.distanceY();
    if (abs(dx) > 30 && abs(dx) > abs(dy)) {
      if (dx < 0) goToNextPage();
      else        goToPreviousPage();
      return;
    }
  }

  if (!touch.wasPressed())  return;
  if (app.currentPage != PAGE_HOME) return;
  if (app.alarmEditor.active || app.timerEditor.active) return;
  if (app.timer.running || app.timer.paused || app.timer.finished) return;
  if (app.stopwatch.running || app.stopwatch.paused || app.stopwatch.elapsedMs > 0) return;
  if (isFallOverlayActive()) return;

  const int btnY = 24 + 142;
  const int btnW = 92;
  const int btnH = 44;

  if (pointInRect(touch.x, touch.y, 12,  btnY, btnW, btnH)) {
    triggerHomeAction(HOME_ACTION_ALARM,  "Alarm");
    return;
  }
  if (pointInRect(touch.x, touch.y, 114, btnY, btnW, btnH)) {
    triggerHomeAction(HOME_ACTION_CHRONO, "Chrono");
    return;
  }
  if (pointInRect(touch.x, touch.y, 216, btnY, btnW, btnH)) {
    triggerHomeAction(HOME_ACTION_TIMER,  "Timer");
    return;
  }
}