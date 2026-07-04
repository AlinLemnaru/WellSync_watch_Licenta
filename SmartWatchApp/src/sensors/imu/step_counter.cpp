#include <M5Unified.h>
#include "../../app/app_state.h"
#include "../../ui/ui.h"

static int      _lastResetHour = -1;
static bool     _stepPending   = false;
static uint32_t _lastStepMs    = 0;
static uint32_t _peakStartMs   = 0;

static constexpr float    STEP_THRESHOLD_HIGH  = 1.5f;
static constexpr float    STEP_THRESHOLD_LOW   = 0.85f;
static constexpr uint32_t STEP_MIN_INTERVAL_MS = 300;
static constexpr uint32_t STEP_PEAK_MIN_MS     = 60;
static constexpr uint32_t STEP_PEAK_MAX_MS     = 700;

void initStepCounterService() {
  _lastResetHour = app.time.hour;
}

void updateStepCounterService() {
  if (app.time.hour == 0 && _lastResetHour != 0) {
    app.health.steps = 0;
  }
  _lastResetHour = app.time.hour;

  float ax, ay, az;
  M5.Imu.getAccel(&ax, &ay, &az);
  float mag = sqrtf(ax * ax + ay * ay + az * az);

  if (!_stepPending && mag > STEP_THRESHOLD_HIGH) {
    _stepPending = true;
    _peakStartMs = millis();
  }

  if (_stepPending && mag < STEP_THRESHOLD_LOW) {
    uint32_t now          = millis();
    uint32_t peakDuration = now - _peakStartMs;
    uint32_t interval     = now - _lastStepMs;

    // Valid peak durtation
    bool peakOk = (peakDuration >= STEP_PEAK_MIN_MS &&
                   peakDuration <= STEP_PEAK_MAX_MS);

    // Valid interval
    bool intervalOk = (_lastStepMs == 0)          ||  // first step
                      (interval > 2500)            ||  // pause -> reset, accept
                      (interval >= STEP_MIN_INTERVAL_MS);  // normal rithm

    if (peakOk && intervalOk) {
      app.health.steps++;
      _lastStepMs = now;
      requestRedraw();
    }

    _stepPending = false;
  }
}