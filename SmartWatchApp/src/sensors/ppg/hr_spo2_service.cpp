#include <M5Unified.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include "hr_spo2_service.h"
#include "../../app/app_state.h"
#include "../../ui/ui.h"

// ── Constants ─────────────────────────────────────────────────────────────────
static constexpr uint32_t MEASURE_DURATION_MS  = 25000;
static constexpr uint32_t INTERVAL_AWAKE_MS    = 2000;
static constexpr uint32_t INTERVAL_STANDBY_MS  = 8000;
static constexpr uint32_t SAMPLE_INTERVAL_MS   = 1000;
static constexpr uint32_t STALE_DATA_MS        = 15000;
static constexpr int      BUFFER_SIZE          = 8;
static constexpr int      MIN_VALID_READS      = 3;   
static constexpr int      INVALID_RESET_COUNT  = 6;

static constexpr int      ACCEL_AVG_SIZE       = 4;
static constexpr float    MOTION_THRESHOLD_AVG = 0.20f;
static constexpr uint32_t MOTION_COOLDOWN_MS   = 8000;

// HR Plausibility
static constexpr float PLAUS_LOW        = 0.65f;
static constexpr float PLAUS_HIGH       = 1.40f;
static constexpr int   PLAUS_REJECT_MAX = 5;

// Doubling
static constexpr float DOUBLING_MAX_HR    = 90.0f;
static constexpr float DOUBLING_TOLERANCE = 20.0f;

// Physiological limits
static constexpr float HR_MIN   = 40.0f;
static constexpr float HR_MAX   = 180.0f;
static constexpr float SPO2_MIN = 70.0f;
static constexpr float SPO2_MAX = 99.0f;

// Maximum consecutive SpO2 variation
static constexpr float SPO2_MAX_JUMP = 10.0f;

// RR intervals validation - anti-artifact
static constexpr int      BEAT_VALID_COUNT  = 3;     
static constexpr float    BEAT_RR_MIN_MS    = 300.0f;
static constexpr float    BEAT_RR_MAX_MS    = 1500.0f;
static constexpr float    BEAT_RR_JITTER_MS = 500.0f; 


// ── State ─────────────────────────────────────────────────────────────────────
static PulseOximeter _pox;
static bool     _poxReady     = false;
static bool     _active       = false;
static uint32_t _sessionStart = 0;
static uint32_t _pauseStart   = 0;
static uint32_t _lastSampleMs = 0;
static uint32_t _lastBeatMs   = 0;
static int      _invalidCount = 0;

static float _hrBuffer[BUFFER_SIZE]   = {0};
static float _spo2Buffer[BUFFER_SIZE] = {0};
static int   _bufIdx   = 0;
static int   _bufCount = 0;

static float    _accelAvgBuf[ACCEL_AVG_SIZE] = {1.0f, 1.0f, 1.0f, 1.0f};
static int      _accelAvgIdx = 0;
static float    _accelAvgSum = 4.0f;

static uint32_t _lastMotionMs = 0;
static bool     _wasMoving    = false;
static float    _stableHr     = 0.0f;
static int      _plausRejects = 0;

static float    _prevSpo2 = 0.0f;

// RR validation
static uint32_t _lastBeatTimestamp = 0;
static float    _rrBuffer[BEAT_VALID_COUNT] = {0};
static int      _rrBufCount = 0;
static bool     _beatSignalValid = false;


// ── Helpers ───────────────────────────────────────────────────────────────────
static void onBeatDetected() {
  uint32_t now = millis();

  if (_lastBeatTimestamp == 0) {
    _lastBeatTimestamp = now;
    return;
  }

  float rr = (float)(now - _lastBeatTimestamp);
  _lastBeatTimestamp = now;

  if (rr < BEAT_RR_MIN_MS || rr > BEAT_RR_MAX_MS) {
    return;
  }

  if (_rrBufCount > 0) {
    float prevRr = _rrBuffer[(_rrBufCount - 1) % BEAT_VALID_COUNT];
    if (fabsf(rr - prevRr) > BEAT_RR_JITTER_MS) {

      _rrBuffer[(_rrBufCount - 1) % BEAT_VALID_COUNT] = rr;
      return;
    }
  }

  _rrBuffer[_rrBufCount % BEAT_VALID_COUNT] = rr;
  _rrBufCount++;

  if (_rrBufCount >= BEAT_VALID_COUNT) {
    _beatSignalValid = true;
    _lastBeatMs = now;
  }
}


static float median(float* buf, int n) {
  float tmp[BUFFER_SIZE];
  for (int i = 0; i < n; i++) tmp[i] = buf[i];
  for (int i = 0; i < n - 1; i++)
    for (int j = 0; j < n - i - 1; j++)
      if (tmp[j] > tmp[j + 1]) {
        float t = tmp[j]; tmp[j] = tmp[j + 1]; tmp[j + 1] = t;
      }
  return tmp[n / 2];
}


static float updateAccelAvg(float newVal) {
  _accelAvgSum -= _accelAvgBuf[_accelAvgIdx];
  _accelAvgBuf[_accelAvgIdx] = newVal;
  _accelAvgSum += newVal;
  _accelAvgIdx = (_accelAvgIdx + 1) % ACCEL_AVG_SIZE;
  return _accelAvgSum / (float)ACCEL_AVG_SIZE;
}


static float applyDoublingCorrection(float hr) {
  if (_stableHr <= 0.0f) return hr;
  if (hr >= DOUBLING_MAX_HR) return hr;
  float doubled      = hr * 2.0f;
  float deltaDoubled = fabsf(doubled - _stableHr);
  float deltaDirect  = fabsf(hr - _stableHr);
  if (deltaDoubled < deltaDirect && deltaDoubled < DOUBLING_TOLERANCE)
    return doubled;
  return hr;
}


static bool isPlausible(float hr) {
  if (_stableHr <= 0.0f) return true;
  float lo = _stableHr * PLAUS_LOW;
  float hi = _stableHr * PLAUS_HIGH;
  return (hr >= lo && hr <= hi);
}


static void resetBeatState() {
  _lastBeatTimestamp = 0;
  _rrBufCount        = 0;
  _beatSignalValid   = false;
  for (int i = 0; i < BEAT_VALID_COUNT; i++) _rrBuffer[i] = 0.0f;
}


static void applyMovingState() {
  bool changed = (app.health.heartRateBpm != 0)  ||
                 (app.health.spo2Percent  != 0)  ||
                 (app.health.availability != DATA_UNAVAILABLE);

  app.health.heartRateBpm = 0;
  app.health.spo2Percent  = 0;
  app.health.availability = DATA_UNAVAILABLE;

  _bufCount     = 0;
  _bufIdx       = 0;
  _plausRejects = 0;
  _invalidCount = 0;
  _stableHr     = 0.0f;
  _prevSpo2     = 0.0f;
  resetBeatState();

  if (changed && app.currentPage == PAGE_HEALTH) requestRedraw();
}


static void beginSession() {
  _pox.begin();
  _pox.setIRLedCurrent(MAX30100_LED_CURR_27_1MA);
  _pox.setOnBeatDetectedCallback(onBeatDetected);
  _active       = true;
  _sessionStart = millis();
  _invalidCount = 0;
  _bufCount     = 0;
  _bufIdx       = 0;
  _plausRejects = 0;
  _prevSpo2     = 0.0f;
  resetBeatState();
  app.health.measuring = true;
}


static void endSession() {
  _pox.shutdown();
  _active     = false;
  _pauseStart = millis();
  _invalidCount = 0;
  app.health.measuring = false;
}


// ── Public API ────────────────────────────────────────────────────────────────
void initHrSpo2Service() {
  app.health.heartRateBpm = 0;
  app.health.spo2Percent  = 0;
  app.health.availability = DATA_UNAVAILABLE;
  app.health.measuring    = false;
  _poxReady     = false;
  _active       = false;
  _invalidCount = 0;
  _lastBeatMs   = 0;
  _pauseStart   = 0;
  _bufCount     = 0;
  _bufIdx       = 0;
  _stableHr     = 0.0f;
  _plausRejects = 0;
  _lastMotionMs = 0;
  _wasMoving    = false;
  _prevSpo2     = 0.0f;

  for (int i = 0; i < ACCEL_AVG_SIZE; i++) _accelAvgBuf[i] = 1.0f;
  _accelAvgSum = (float)ACCEL_AVG_SIZE;
  _accelAvgIdx = 0;
  resetBeatState();

  if (!_pox.begin()) return;
  _pox.shutdown();
  _poxReady = true;
}


void startHrSpo2Measurement() {
  if (!_poxReady || _active) return;
  beginSession();
}


void stopHrSpo2Measurement() {
  if (!_poxReady || !_active) return;
  endSession();
}


void updateHrSpo2Service() {
  if (!_poxReady) return;

  uint32_t now      = millis();
  uint32_t interval = app.standby.active ? INTERVAL_STANDBY_MS : INTERVAL_AWAKE_MS;

  // ── Stale data check ───────────────────────────────────────────────────────
  if (_lastBeatMs > 0 && app.health.availability == DATA_AVAILABLE) {
    if ((now - _lastBeatMs) > STALE_DATA_MS) {
      app.health.heartRateBpm = 0;
      app.health.spo2Percent  = 0;
      app.health.availability = DATA_UNAVAILABLE;
      _lastBeatMs   = 0;
      _invalidCount = 0;
      _bufCount     = 0;
      _bufIdx       = 0;
      _stableHr     = 0.0f;
      _plausRejects = 0;
      _prevSpo2     = 0.0f;
      resetBeatState();
      if (app.currentPage == PAGE_HEALTH) requestRedraw();
    }
  }

  // ── Motion detection ──────────────────────────────────────────────────────
  float avgAccel   = updateAccelAvg(app.fallDetection.lastAccelMagnitude);
  float accelDelta = fabsf(avgAccel - 1.0f);
  bool  moving     = (accelDelta > MOTION_THRESHOLD_AVG);

  if (moving) {
    _lastMotionMs = now;
    _wasMoving    = true;
    applyMovingState();
  }

  bool inCooldown = (_wasMoving && (now - _lastMotionMs) < MOTION_COOLDOWN_MS);

  if (inCooldown) {
    applyMovingState();
    if (_active) {
      if (now - _sessionStart >= MEASURE_DURATION_MS) endSession();
      else _pox.update();
    } else {
      if (now - _pauseStart >= interval) beginSession();
    }
    return;
  }

  if (_wasMoving) _wasMoving = false;

  // ── Standard session management ────────────────────────────────────────────
  if (_active) {
    if (now - _sessionStart >= MEASURE_DURATION_MS) {
      endSession();
      return;
    }
    _pox.update();
  } else {
    if (now - _pauseStart >= interval) beginSession();
    return;
  }

  // ── Sample read ────────────────────────────────────────────────────────────
  if (now - _lastSampleMs < SAMPLE_INTERVAL_MS) return;
  _lastSampleMs = now;

  // ── Gating beat signal ─────────────────────────────────────────────────────
  if (!_beatSignalValid) {
    if (app.health.availability == DATA_AVAILABLE) {
      _invalidCount++;
      if (_invalidCount >= INVALID_RESET_COUNT) {
        app.health.heartRateBpm = 0;
        app.health.spo2Percent  = 0;
        app.health.availability = DATA_UNAVAILABLE;
        _invalidCount = 0;
        _stableHr     = 0.0f;
        _plausRejects = 0;
        _prevSpo2     = 0.0f;
        if (app.currentPage == PAGE_HEALTH) requestRedraw();
      }
    }
    return;
  }

  float hr   = _pox.getHeartRate();
  float spo2 = _pox.getSpO2();

  // ── Physiological validation of HR and SpO2 ────────────────────────────────
  if (hr < HR_MIN || hr > HR_MAX || spo2 < SPO2_MIN || spo2 > SPO2_MAX) {
    _invalidCount++;
    if (_invalidCount >= INVALID_RESET_COUNT &&
        app.health.availability == DATA_AVAILABLE) {
      app.health.heartRateBpm = 0;
      app.health.spo2Percent  = 0;
      app.health.availability = DATA_UNAVAILABLE;
      _invalidCount = 0;
      _stableHr     = 0.0f;
      _plausRejects = 0;
      _prevSpo2     = 0.0f;
      resetBeatState();
      if (app.currentPage == PAGE_HEALTH) requestRedraw();
    }
    return;
  }

  // ── Chaotic SpO2 variation ─────────────────────────────────────────────────
  if (_prevSpo2 > 0.0f && fabsf(spo2 - _prevSpo2) > SPO2_MAX_JUMP) {
    _prevSpo2 = spo2;
    return;
  }
  _prevSpo2 = spo2;

  // ── Doubling correction ────────────────────────────────────────────────────
  float correctedHr = applyDoublingCorrection(hr);

  // ── Plausibility filter ────────────────────────────────────────────────────
  if (!isPlausible(correctedHr)) {
    _plausRejects++;
    if (_plausRejects >= PLAUS_REJECT_MAX) {
      _stableHr     = correctedHr;
      _plausRejects = 0;
      _bufCount     = 0;
      _bufIdx       = 0;
    }
    return;
  }

  _plausRejects = 0;
  _invalidCount = 0;

  _hrBuffer[_bufIdx]   = correctedHr;
  _spo2Buffer[_bufIdx] = spo2;
  _bufIdx = (_bufIdx + 1) % BUFFER_SIZE;
  if (_bufCount < BUFFER_SIZE) _bufCount++;

  if (_bufCount < MIN_VALID_READS) return;

  int filteredHr   = (int)median(_hrBuffer,   _bufCount);
  int filteredSpo2 = (int)median(_spo2Buffer, _bufCount);

  if (_bufCount >= BUFFER_SIZE) _stableHr = (float)filteredHr;

  bool changed = (filteredHr   != app.health.heartRateBpm) ||
                 (filteredSpo2 != app.health.spo2Percent);

  app.health.heartRateBpm = filteredHr;
  app.health.spo2Percent  = filteredSpo2;
  app.health.availability = DATA_AVAILABLE;

  if (changed && app.currentPage == PAGE_HEALTH) requestRedraw();
}