#include <M5Unified.h>
#include "standby_service.h"
#include "../app/app_state.h"
#include "../sensors/imu/fall_detection.h"
#include "../sensors/ppg/hr_spo2_service.h"   
#include "../ui/ui.h"

// ── Helper intern ─────────────────────────────────────────────────────────
static uint8_t normalBrightness() {
  return static_cast<uint8_t>(app.settings.brightnessPercent * 255 / 100);
}

void initStandby() {
  app.standby.active         = false;
  app.standby.lastActivityMs = millis();
  M5.Display.setBrightness(normalBrightness());
}

void notifyUserActivity() {
  app.standby.lastActivityMs = millis();
  if (app.standby.active) {
    wakeFromStandby();
  }
}

void wakeFromStandby() {
  if (!app.standby.active) return;
  app.standby.active         = false;
  app.standby.lastActivityMs = millis();
  M5.Display.setBrightness(normalBrightness());
  startHrSpo2Measurement();  
  requestRedraw();
}

void wakeForAlert() {
  if (app.standby.active) {
    app.standby.active         = false;
    app.standby.lastActivityMs = millis();
    startHrSpo2Measurement();  
    requestRedraw();
  }
  M5.Display.setBrightness(255);
}

void restoreNormalBrightness() {
  M5.Display.setBrightness(normalBrightness());
}

bool isStandbyActive() {
  return app.standby.active;
}

static void enterStandby() {
  if (app.standby.active) return;

  if (app.alarm.ringing)     return;
  if (app.timer.finished)    return;
  if (isFallOverlayActive()) return;

  app.standby.active = true;
  stopHrSpo2Measurement();   
  M5.Display.setBrightness(0);
  M5.Speaker.stop();
}

void updateStandby() {
  if (app.standby.active)         return;
  if (app.standby.timeoutMs == 0) return;

  uint32_t now = millis();
  if ((now - app.standby.lastActivityMs) >= app.standby.timeoutMs) {
    enterStandby();
  }
}