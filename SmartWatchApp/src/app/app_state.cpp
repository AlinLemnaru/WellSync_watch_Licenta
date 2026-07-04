#include "app_state.h"
#include <M5Unified.h>

static float  _battEma       = -1.0f;
static int    _battDisplayed = -1;
static int    _battReadCount = 0;
static constexpr float BATT_EMA_ALPHA = 0.15f;

static int readBatteryLevel() {
  for (int i = 0; i < 4; i++) {
    int lvl = M5.Power.getBatteryLevel();
    if (lvl >= 0 && lvl <= 100) return lvl;
    delay(5);
  }
  return -1;
}

// ─────────────────────────────────────────────────────────────────────────────

AppState app;

void initAppState() {
  app.currentPage  = PAGE_HOME;
  app.previousPage = PAGE_HOME;
  app.pageChanged  = true;

  app.time.hour            = 0;
  app.time.minute          = 0;
  app.time.second          = 0;
  app.time.day             = 1;
  app.time.month           = 1;
  app.time.year            = 2026;
  app.time.syncedFromPhone = false;

  app.timeSync.source           = TIME_SOURCE_DEFAULT;
  app.timeSync.manualSetAllowed = true;
  app.timeSync.hasValidTime     = false;

  app.weather.temperatureC  = 0.0f;
  app.weather.conditionText = "No Device Connection";
  app.weather.availability  = DATA_UNAVAILABLE;
  app.weather.fromBleDevice = false;

  app.ble.state      = CONNECTION_DISCONNECTED;
  app.ble.deviceName = "No Device Connection";

  app.health.heartRateBpm = 74;
  app.health.spo2Percent  = 98;
  app.health.steps        = 0;
  app.health.stepTarget   = 10000;
  app.health.measuring    = false;
  app.health.availability = DATA_AVAILABLE;

  app.environment.temperatureC      = 24.3f;
  app.environment.humidityPercent   = 46.0f;
  app.environment.pressureHpa       = 1008.6f;
  app.environment.gasResistanceOhms = 18250;
  app.environment.iaqScore          = 82;
  app.environment.iaqLabel          = "Good";
  app.environment.availability      = DATA_AVAILABLE;

  app.sleep.sleepScore       = 84;
  app.sleep.durationHours    = 7;
  app.sleep.durationMinutes  = 32;
  app.sleep.bedTime          = "23:18";
  app.sleep.wakeTime         = "06:50";
  app.sleep.deepSleepMinutes = 96;
  app.sleep.availability     = DATA_AVAILABLE;

  app.stress.stressScore   = 38;
  app.stress.hrvMs         = 52;
  app.stress.restingHr     = 61;
  app.stress.statusLabel   = "Low";
  app.stress.recoveryScore = 79;
  app.stress.availability  = DATA_AVAILABLE;

  app.alarm.enabled = false;
  app.alarm.ringing = false;
  app.alarm.hour    = 7;
  app.alarm.minute  = 0;

  app.alarmEditor.active  = false;
  app.alarmEditor.field   = ALARM_EDIT_NONE;
  app.alarmEditor.hour    = app.alarm.hour;
  app.alarmEditor.minute  = app.alarm.minute;
  app.alarmEditor.enabled = app.alarm.enabled;

  app.timer.running           = false;
  app.timer.paused            = false;
  app.timer.finished          = false;
  app.timer.durationMs        = 0;
  app.timer.remainingMs       = 0;
  app.timer.startedAtMs       = 0;
  app.timer.pausedRemainingMs = 0;

  app.timerEditor.active  = false;
  app.timerEditor.field   = TIMER_EDIT_NONE;
  app.timerEditor.minutes = 1;
  app.timerEditor.seconds = 0;

  app.standbyEditor.active    = false;
  app.standbyEditor.field     = STANDBY_EDIT_NONE;
  app.standbyEditor.timeoutMs = 30000;

  // ── brightness editor ─────────────────────────────────────────────────────
  app.brightnessEditor.active  = false;
  app.brightnessEditor.field   = BRIGHTNESS_EDIT_NONE;
  app.brightnessEditor.percent = 70;

  app.stepTargetEditor.active      = false;
  app.stepTargetEditor.field       = STEP_TARGET_EDIT_NONE;
  app.stepTargetEditor.targetValue = 10000;

  app.stopwatch.running         = false;
  app.stopwatch.paused          = false;
  app.stopwatch.startedAtMs     = 0;
  app.stopwatch.elapsedMs       = 0;
  app.stopwatch.pausedElapsedMs = 0;

  app.fallDetection.enabled                 = true;
  app.fallDetection.phase                   = FALL_PHASE_IDLE;
  app.fallDetection.popupVisible            = false;
  app.fallDetection.alertSoundActive        = false;
  app.fallDetection.alertVisualActive       = false;
  app.fallDetection.confirmationStartedAtMs = 0;
  app.fallDetection.confirmationTimeoutMs   = 10000;
  app.fallDetection.lastAccelMagnitude      = 1.0f;
  app.fallDetection.lastGyroMagnitude       = 0.0f;
  app.fallDetection.lastTiltDegrees         = 0.0f;
  app.fallDetection.impactDetectedAtMs      = 0;
  app.fallDetection.inactivityStartedAtMs   = 0;
  app.fallDetection.impactDetected          = false;
  app.fallDetection.orientationChanged      = false;
  app.fallDetection.inactivityDetected      = false;
  app.fallDetection.freeFallDetected        = false;
  app.fallDetection.confirmSoundActive      = false;
  app.fallDetection.lastConfirmBeepAtMs     = 0;

  app.settings.bluetoothEnabled  = true;
  app.settings.brightnessPercent = 70;

  app.settings.batteryPercent = -1;

  app.settings.deviceName      = "SmartWatch";
  app.settings.firmwareVersion = "v1.0";
  app.settings.selectedOption  = SETTINGS_OPTION_BLUETOOTH;
  app.settings.scrollOffset    = 0;

  app.dateTimeEditor.active = false;
  app.dateTimeEditor.field  = EDIT_FIELD_NONE;
  app.dateTimeEditor.day    = app.time.day;
  app.dateTimeEditor.month  = app.time.month;
  app.dateTimeEditor.year   = app.time.year;
  app.dateTimeEditor.hour   = app.time.hour;
  app.dateTimeEditor.minute = app.time.minute;

  app.standby.active         = false;
  app.standby.lastActivityMs = 0;
  app.standby.timeoutMs      = 30000;

  app.homeSelectedAction = HOME_ACTION_NONE;
  app.homeStatusMessage  = "";
  app.homeStatusUntilMs  = 0;
}

const char* getPageTitle(PageId page) {
  switch (page) {
    case PAGE_HOME:        return "Home";
    case PAGE_HEALTH:      return "Health";
    case PAGE_STRESS:      return "Stress";
    case PAGE_ENVIRONMENT: return "Environment";
    case PAGE_SLEEP:       return "Sleep";
    case PAGE_SETTINGS:    return "Settings";
    default:               return "SmartWatch";
  }
}

const char* getConnectionLabel(ConnectionState state) {
  switch (state) {
    case CONNECTION_CONNECTED:    return "Connected";
    case CONNECTION_CONNECTING:   return "Connecting";
    case CONNECTION_DISCONNECTED:
    default:                      return "Disconnected";
  }
}

// ─────────────────────────────────────────────────────────────────────────────
void updateBatteryState() {
  static uint32_t lastBattUpdateMs = 0;
  uint32_t now = millis();

  if (now < 3000) return;

  uint32_t interval = (_battReadCount < 3) ? 3000 : 30000;
  if (now - lastBattUpdateMs < interval) return;
  lastBattUpdateMs = now;

  int lvl = readBatteryLevel();
  if (lvl < 0) return;

  _battReadCount++;

  if (_battReadCount <= 3) {
    _battEma                    = (float)lvl;
    _battDisplayed              = lvl;
    app.settings.batteryPercent = lvl;
    return;
  }

  _battEma = _battEma * (1.0f - BATT_EMA_ALPHA) + (float)lvl * BATT_EMA_ALPHA;
  int smoothed = (int)(_battEma + 0.5f);

  bool shouldUpdate = (smoothed < _battDisplayed) ||
                      (smoothed >= _battDisplayed + 2);

  if (shouldUpdate && smoothed != _battDisplayed) {
    _battDisplayed              = smoothed;
    app.settings.batteryPercent = smoothed;
  }
}