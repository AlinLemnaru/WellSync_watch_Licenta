#include <M5Unified.h>
#include "../app/app_state.h"
#include "../ui/ui.h"
#include "timer_service.h"
#include "haptic_service.h"
#include "standby_service.h"
#include "../sensors/imu/fall_detection.h"   


static bool     timerSpeakerReady = false;
static uint32_t lastTimerBeepMs   = 0;


static void triggerTimerFinished() {
  app.timer.running  = false;
  app.timer.paused   = false;
  app.timer.finished = true;
  startHaptic(HAPTIC_TIMER);
  app.timer.remainingMs       = 0;
  app.timer.pausedRemainingMs = 0;

  app.currentPage        = PAGE_HOME;
  app.pageChanged        = true;
  app.homeSelectedAction = HOME_ACTION_NONE;
  app.homeStatusMessage  = "";
  app.homeStatusUntilMs  = 0;

  app.timerEditor.active = false;
  app.timerEditor.field  = TIMER_EDIT_NONE;

  lastTimerBeepMs = 0;

  wakeFromStandby();
  requestRedraw();
}


void initTimerService() {
  app.timer.running           = false;
  app.timer.paused            = false;
  app.timer.finished          = false;
  app.timer.durationMs        = 0;
  app.timer.remainingMs       = 0;
  app.timer.startedAtMs       = 0;
  app.timer.pausedRemainingMs = 0;

  auto spkCfg = M5.Speaker.config();
  M5.Speaker.config(spkCfg);
  M5.Speaker.begin();
  M5.Speaker.setVolume(255);
  timerSpeakerReady = true;
  lastTimerBeepMs   = 0;
}


void updateTimerService() {
  if (app.timer.running) {
    if (isFallOverlayActive()) return;   

    uint32_t now     = millis();
    uint32_t elapsed = now - app.timer.startedAtMs;

    if (elapsed >= app.timer.durationMs) {
      triggerTimerFinished();
    } else {
      app.timer.remainingMs = app.timer.durationMs - elapsed;
    }
  }

  if (app.timer.finished) {
    if (app.currentPage != PAGE_HOME) {
      app.currentPage = PAGE_HOME;
      app.pageChanged = true;
      requestRedraw();
    }

    uint32_t now = millis();
    if (now - lastTimerBeepMs >= 550) {
      lastTimerBeepMs = now;

      if (timerSpeakerReady) {
        M5.Speaker.setVolume(255);
        M5.Speaker.tone(2600, 180);
      }
    }
  }
}


void setTimerDurationMs(uint32_t durationMs) {
  app.timer.durationMs        = durationMs;
  app.timer.remainingMs       = durationMs;
  app.timer.pausedRemainingMs = durationMs;
  app.timer.finished          = false;
}


void startTimer() {
  if (app.timer.durationMs == 0) return;

  app.timer.running           = true;
  app.timer.paused            = false;
  app.timer.finished          = false;
  app.timer.startedAtMs       = millis();
  app.timer.remainingMs       = app.timer.durationMs;
  app.timer.pausedRemainingMs = app.timer.durationMs;
}


void pauseTimer() {
  if (!app.timer.running) return;

  uint32_t now     = millis();
  uint32_t elapsed = now - app.timer.startedAtMs;

  if (elapsed >= app.timer.durationMs) {
    triggerTimerFinished();
    return;
  }

  app.timer.running           = false;
  app.timer.paused            = true;
  app.timer.remainingMs       = app.timer.durationMs - elapsed;
  app.timer.pausedRemainingMs = app.timer.remainingMs;
}


void resumeTimer() {
  if (!app.timer.paused || app.timer.pausedRemainingMs == 0) return;

  app.timer.running     = true;
  app.timer.paused      = false;
  app.timer.finished    = false;
  app.timer.durationMs  = app.timer.pausedRemainingMs;
  app.timer.remainingMs = app.timer.pausedRemainingMs;
  app.timer.startedAtMs = millis();
}


void resetTimer() {
  app.timer.running           = false;
  app.timer.paused            = false;
  app.timer.finished          = false;
  app.timer.remainingMs       = app.timer.durationMs;
  app.timer.pausedRemainingMs = app.timer.durationMs;

  if (timerSpeakerReady) {
    M5.Speaker.stop();
  }
}


void stopTimerAlert() {
  app.timer.finished          = false;
  app.timer.running           = false;
  app.timer.paused            = false;
  app.timer.remainingMs       = app.timer.durationMs;
  app.timer.pausedRemainingMs = app.timer.durationMs;

  if (timerSpeakerReady) {
    M5.Speaker.stop();
    stopHaptic();
  }
}


bool     isTimerRunning()       { return app.timer.running; }
bool     isTimerPaused()        { return app.timer.paused; }
bool     isTimerFinished()      { return app.timer.finished; }
uint32_t getTimerRemainingMs()  { return app.timer.remainingMs; }