#include <M5Unified.h>
#include "stress_estimation_service.h"
#include "../app/app_state.h"
#include "../ui/ui.h"

// ── Constants ─────────────────────────────────────────────────────────────────
static constexpr int      HR_WINDOW_SIZE         = 16;
static constexpr int      BASELINE_SAMPLES       = 12;    
static constexpr uint32_t SAMPLE_MS              = 2000;
static constexpr uint32_t SCORE_UPDATE_MS        = 5000;
static constexpr float    MOTION_THRESHOLD       = 0.20f; 
static constexpr int      ACCEL_AVG_SIZE         = 4;
static constexpr uint32_t MOTION_COOLDOWN_MS     = 10000; 

// Resting HR TH
static constexpr float    HR_RESTING_RELAXED     = 60.0f; 
static constexpr float    HR_RESTING_STRESS      = 100.0f;
static constexpr float    HR_RESTING_NORMAL_LOW  = 60.0f;
static constexpr float    HR_RESTING_NORMAL_HIGH = 100.0f;

// ── State ─────────────────────────────────────────────────────────────────────
static float    _hrWindow[HR_WINDOW_SIZE] = {0};
static int      _hrWinIdx    = 0;
static int      _hrWinCount  = 0;

static float    _baselineSum   = 0.0f;
static int      _baselineCount = 0;
static float    _baselineHr    = 0.0f;
static bool     _baselineReady = false;

static float    _accelAvgBuf[ACCEL_AVG_SIZE] = {1.0f, 1.0f, 1.0f, 1.0f};
static int      _accelAvgIdx = 0;
static float    _accelAvgSum = 4.0f;

static uint32_t _lastMotionMs  = 0;
static uint32_t _lastSampleMs  = 0;
static uint32_t _lastScoreMs   = 0;
static bool     _initialized   = false;

// ── Motion detection ──────────────────────────────────────────────────────────
static float updateAccelAvg(float newVal) {
  _accelAvgSum -= _accelAvgBuf[_accelAvgIdx];
  _accelAvgBuf[_accelAvgIdx] = newVal;
  _accelAvgSum += newVal;
  _accelAvgIdx = (_accelAvgIdx + 1) % ACCEL_AVG_SIZE;
  return _accelAvgSum / (float)ACCEL_AVG_SIZE;
}

static bool isMoving() {
  float avg = updateAccelAvg(app.fallDetection.lastAccelMagnitude);
  return fabsf(avg - 1.0f) > MOTION_THRESHOLD;
}

// ── Stress estimation ─────────────────────────────────────────────────────────
static int computeStressScore(float meanHr) {
  float baseline = _baselineReady ? _baselineHr : 72.0f; 

  if (meanHr <= HR_RESTING_RELAXED) {
    float t = meanHr / HR_RESTING_RELAXED;  
    return (int)(t * 20.0f);
  }

  if (meanHr <= baseline) {
    float range = baseline - HR_RESTING_RELAXED;
    if (range < 1.0f) range = 1.0f;
    float t = (meanHr - HR_RESTING_RELAXED) / range;
    return 20 + (int)(t * 20.0f);
  }

  float midPoint = 85.0f;
  if (meanHr <= midPoint) {
    float range = midPoint - baseline;
    if (range < 1.0f) range = 1.0f;
    float t = (meanHr - baseline) / range;
    return 40 + (int)(t * 25.0f);
  }

  if (meanHr < HR_RESTING_STRESS) {
    float t = (meanHr - midPoint) / (HR_RESTING_STRESS - midPoint);
    return 65 + (int)(t * 15.0f);
  }

  float excess = meanHr - HR_RESTING_STRESS;
  int score = 80 + (int)(excess / 40.0f * 20.0f);
  return score > 100 ? 100 : score;
}

static const char* stressToLabel(int score) {
  if (score <= 20) return "Resting";
  if (score <= 40) return "Low";
  if (score <= 65) return "Medium";
  if (score <= 80) return "High";
  return "Very High";
}

static int computeRecoveryScore(int stressScore) {
  int r = 100 - (int)(stressScore * 0.9f);
  return r > 100 ? 100 : r < 0 ? 0 : r;
}

static float windowMean(float* buf, int n) {
  float s = 0;
  for (int i = 0; i < n; i++) s += buf[i];
  return s / (float)n;
}

static int computeHrvDisplay(float* buf, int n) {
  if (n < 2) return 0;
  float mn = buf[0], mx = buf[0];
  for (int i = 1; i < n; i++) {
    if (buf[i] < mn) mn = buf[i];
    if (buf[i] > mx) mx = buf[i];
  }
  float rangeBpm = mx - mn;
  if (rangeBpm < 0.5f) return 18;
  float meanHr = windowMean(buf, n);
  if (meanHr <= 0) return 0;
  float hrv = rangeBpm * (60000.0f / (meanHr * meanHr)) * 3.0f;
  int val = (int)hrv;
  return val > 120 ? 120 : val < 10 ? 10 : val;
}

// ── Reset at moving state ─────────────────────────────────────────────────────
static void applyMovingState() {
  bool changed = (app.stress.stressScore   != 0)  ||
                 (app.stress.hrvMs         != 0)  ||
                 (app.stress.restingHr     != 0)  ||
                 (app.stress.recoveryScore != 0)  ||
                 (app.stress.availability  != DATA_UNAVAILABLE);

  app.stress.stressScore   = 0;
  app.stress.hrvMs         = 0;
  app.stress.restingHr     = 0;
  app.stress.recoveryScore = 0;
  app.stress.statusLabel   = "--";
  app.stress.availability  = DATA_UNAVAILABLE;

  if (changed && app.currentPage == PAGE_STRESS) requestRedraw();
}

// ── Public API ────────────────────────────────────────────────────────────────
void initStressEstimationService() {
  app.stress.stressScore   = 0;
  app.stress.hrvMs         = 0;
  app.stress.restingHr     = 0;
  app.stress.statusLabel   = "--";
  app.stress.recoveryScore = 0;
  app.stress.availability  = DATA_UNAVAILABLE;

  _hrWinIdx      = 0;
  _hrWinCount    = 0;
  _baselineSum   = 0.0f;
  _baselineCount = 0;
  _baselineHr    = 0.0f;
  _baselineReady = false;
  _lastMotionMs  = 0;
  _lastSampleMs  = 0;
  _lastScoreMs   = 0;

  for (int i = 0; i < ACCEL_AVG_SIZE; i++) _accelAvgBuf[i] = 1.0f;
  _accelAvgSum = (float)ACCEL_AVG_SIZE;
  _accelAvgIdx = 0;

  _initialized = true;
}

void updateStressEstimationService() {
  if (!_initialized)      return;
  if (app.standby.active) return;

  uint32_t now    = millis();
  bool     moving = isMoving();

  if (moving) {
    _lastMotionMs = now;
    applyMovingState();
    return;
  }

  if (_lastMotionMs > 0 && (now - _lastMotionMs) < MOTION_COOLDOWN_MS) {
    applyMovingState();
    return;
  }

  if (app.health.availability != DATA_AVAILABLE) return;
  int rawHr = app.health.heartRateBpm;
  if (rawHr <= 0) return;

  if (now - _lastSampleMs >= SAMPLE_MS) {
    _lastSampleMs = now;

    _hrWindow[_hrWinIdx] = (float)rawHr;
    _hrWinIdx = (_hrWinIdx + 1) % HR_WINDOW_SIZE;
    if (_hrWinCount < HR_WINDOW_SIZE) _hrWinCount++;

    if (!_baselineReady) {
      _baselineSum += (float)rawHr;
      _baselineCount++;
      if (_baselineCount >= BASELINE_SAMPLES) {
        _baselineHr    = _baselineSum / (float)_baselineCount;
        _baselineReady = true;
      }
    }
  }

  // ── Recalculate score ───────────────────────────────────────────────────────
  if (now - _lastScoreMs < SCORE_UPDATE_MS) return;
  _lastScoreMs = now;

  if (_hrWinCount < 4) return;

  float meanHr   = windowMean(_hrWindow, _hrWinCount);
  int   stress   = computeStressScore(meanHr);
  int   recovery = computeRecoveryScore(stress);
  int   restHr   = _baselineReady ? (int)_baselineHr : rawHr;
  int   hrv      = computeHrvDisplay(_hrWindow, _hrWinCount);

  bool changed =
    stress   != app.stress.stressScore   ||
    hrv      != app.stress.hrvMs         ||
    restHr   != app.stress.restingHr     ||
    recovery != app.stress.recoveryScore ||
    app.stress.availability != DATA_AVAILABLE;

  app.stress.stressScore   = stress;
  app.stress.hrvMs         = hrv;
  app.stress.restingHr     = restHr;
  app.stress.statusLabel   = stressToLabel(stress);
  app.stress.recoveryScore = recovery;
  app.stress.availability  = DATA_AVAILABLE;

  if (changed && app.currentPage == PAGE_STRESS) requestRedraw();
}