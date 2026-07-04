#include <M5Unified.h>
#include "../app/app_state.h"
#include "../ui/ui.h"
#include "alarm_service.h"
#include "haptic_service.h"
#include "standby_service.h"
#include "../sensors/imu/fall_detection.h"   


static uint32_t lastAlarmCheckMinuteKey = 0;
static uint32_t lastBeepMs = 0;
static bool speakerReady = false;


static uint32_t buildMinuteKey() {
  return static_cast<uint32_t>(app.time.year) * 100000000UL +
         static_cast<uint32_t>(app.time.month) * 1000000UL +
         static_cast<uint32_t>(app.time.day) * 10000UL +
         static_cast<uint32_t>(app.time.hour) * 100UL +
         static_cast<uint32_t>(app.time.minute);
}


static void triggerAlarmRinging() {
  wakeFromStandby();
  app.alarm.ringing = true;
  startHaptic(HAPTIC_ALARM);
  app.currentPage = PAGE_HOME;
  app.pageChanged = true;
  app.homeSelectedAction = HOME_ACTION_NONE;
  app.homeStatusMessage = "";
  app.homeStatusUntilMs = 0;
  app.alarmEditor.active = false;
  app.alarmEditor.field = ALARM_EDIT_NONE;
  lastBeepMs = 0;
  requestRedraw();
}


void initAlarmService() {
  app.alarm.ringing = false;
  lastAlarmCheckMinuteKey = 0;
  lastBeepMs = 0;

  auto spkCfg = M5.Speaker.config();
  M5.Speaker.config(spkCfg);
  M5.Speaker.begin();
  M5.Speaker.setVolume(255);
  speakerReady = true;
}


void updateAlarmService() {
  if (app.alarm.enabled && !app.alarm.ringing) {
    if (isFallOverlayActive()) return;   // ← NOU

    uint32_t currentMinuteKey = buildMinuteKey();

    if (currentMinuteKey != lastAlarmCheckMinuteKey) {
      lastAlarmCheckMinuteKey = currentMinuteKey;

      if (app.time.hour == app.alarm.hour &&
          app.time.minute == app.alarm.minute) {
        triggerAlarmRinging();
      }
    }
  }

  if (app.alarm.ringing) {
    if (app.currentPage != PAGE_HOME) {
      app.currentPage = PAGE_HOME;
      app.pageChanged = true;
      requestRedraw();
    }

    uint32_t now = millis();
    if (now - lastBeepMs >= 650) {
      lastBeepMs = now;

      if (speakerReady) {
        M5.Speaker.setVolume(255);
        M5.Speaker.tone(2200, 220);
      }
    }
  }
}


void setAlarmTime(int hour, int minute) {
  if (hour < 0)    hour = 0;
  if (hour > 23)   hour = 23;
  if (minute < 0)  minute = 0;
  if (minute > 59) minute = 59;

  app.alarm.hour   = hour;
  app.alarm.minute = minute;
}


void enableAlarm(bool enabled) {
  app.alarm.enabled = enabled;

  if (!enabled) {
    app.alarm.ringing = false;
  }
}


void stopAlarmRinging() {
  app.alarm.ringing = false;
  if (speakerReady) {
    M5.Speaker.stop();
    stopHaptic();
  }
}


bool isAlarmEnabled()  { return app.alarm.enabled; }
bool isAlarmRinging()  { return app.alarm.ringing; }