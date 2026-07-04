#include <M5Unified.h>
#include <math.h>
#include "../../app/app_state.h"
#include "../../ui/ui.h"
#include "../../services/haptic_service.h"
#include "../../services/standby_service.h"
#include "../../services/ble_service.h"


// ─── TH — Pre-impact ──────────────────────────────────────────────────────
static const float    FREE_FALL_THRESHOLD_G     = 0.85f;
static const float    FREE_FALL_MIN_DURATION_MS = 300.0f;
static const float    FREE_FALL_WINDOW_MS       = 1000.0f;
static const float    FREE_FALL_GYRO_MIN_DPS    = 60.0f;

// ─── TH — Impact ──────────────────────────────────────────────────────────
static const float    IMPACT_THRESHOLD_G        = 2.2f;
static const uint32_t IMPACT_WINDOW_MS          = 2500;

// ─── TH — Post-impact ─────────────────────────────────────────────────────
static const float    POST_IMPACT_GYRO_MIN_DPS  = 80.0f;
static const float    TILT_CHANGE_DEG           = 20.0f;

static const float    INACTIVITY_ACCEL_DELTA_G      = 0.20f;
static const uint32_t INACTIVITY_FAST_CONFIRM_MS    = 800;   
static const uint32_t FALL_CONFIRM_TIMEOUT_MS       = 2000;

// ─── BLE Alert ────────────────────────────────────────────────────────────
static const uint32_t FALL_BLE_ALERT_DELAY_MS       = 10000;


// ─── Circular Buffer ──────────────────────────────────────────────────────
static const int ACCEL_BUF_SIZE = 40;
struct AccelSample {
  float    accelMagnitude;
  float    gyroMagnitude;
  uint32_t timestampMs;
};
static AccelSample accelHistory[ACCEL_BUF_SIZE];
static int         accelBufHead  = 0;
static int         accelBufCount = 0;

// ─── State intern ─────────────────────────────────────────────────────────
static uint32_t alertPhaseStartedAtMs    = 0;
static bool     fallBleSentForThisAlert  = false;
static uint32_t lastAlertPatternStepAtMs = 0;
static uint8_t  alertPatternStep         = 0;


static void accelBufPush(float accel, float gyro, uint32_t ts) {
  accelHistory[accelBufHead] = { accel, gyro, ts };
  accelBufHead = (accelBufHead + 1) % ACCEL_BUF_SIZE;
  if (accelBufCount < ACCEL_BUF_SIZE) accelBufCount++;
}


static bool wasFreeFallBeforeImpact(uint32_t impactMs) {
  float    consecutiveFreeFallMs = 0.0f;
  float    gyroSumDuringFreeFall = 0.0f;
  int      gyroSamples           = 0;
  uint32_t prevTs                = impactMs;
  bool     freeFallFound         = false;

  for (int i = 0; i < accelBufCount; i++) {
    int idx = ((accelBufHead - 1 - i) + ACCEL_BUF_SIZE) % ACCEL_BUF_SIZE;
    AccelSample& s = accelHistory[idx];

    if (impactMs - s.timestampMs > (uint32_t)FREE_FALL_WINDOW_MS) break;

    uint32_t dt = prevTs - s.timestampMs;

    if (s.accelMagnitude < FREE_FALL_THRESHOLD_G) {
      consecutiveFreeFallMs += (float)dt;
      gyroSumDuringFreeFall += s.gyroMagnitude;
      gyroSamples++;
      if (consecutiveFreeFallMs >= FREE_FALL_MIN_DURATION_MS) {
        freeFallFound = true;
        break;
      }
    } else {
      consecutiveFreeFallMs = 0.0f;
      gyroSumDuringFreeFall = 0.0f;
      gyroSamples           = 0;
    }
    prevTs = s.timestampMs;
  }

  if (!freeFallFound) return false;
  float gyroMean = (gyroSamples > 0)
                   ? (gyroSumDuringFreeFall / (float)gyroSamples)
                   : 0.0f;
  return gyroMean >= FREE_FALL_GYRO_MIN_DPS;
}


static float absF(float v) { return v < 0.0f ? -v : v; }


static void resetFallDetectionFlags() {
  app.fallDetection.impactDetected        = false;
  app.fallDetection.orientationChanged    = false;
  app.fallDetection.inactivityDetected    = false;
  app.fallDetection.impactDetectedAtMs    = 0;
  app.fallDetection.inactivityStartedAtMs = 0;
  app.fallDetection.freeFallDetected      = false;
}


static void resetAlertSoundPattern() {
  lastAlertPatternStepAtMs = 0;
  alertPatternStep         = 0;
}


static void updateAlertSoundPattern(uint32_t now) {
  if (!app.fallDetection.alertSoundActive) return;

  static const uint16_t patternDurationsMs[] = { 90, 90, 110, 110, 140, 260 };
  static const uint16_t patternFrequencies[] = { 3200, 4200, 3200, 4600, 3600, 0 };
  static const uint8_t  patternLength =
    sizeof(patternDurationsMs) / sizeof(patternDurationsMs[0]);

  if (lastAlertPatternStepAtMs != 0 &&
      (now - lastAlertPatternStepAtMs) < patternDurationsMs[alertPatternStep]) return;

  lastAlertPatternStepAtMs = now;
  alertPatternStep = (alertPatternStep + 1) % patternLength;

  uint16_t freq = patternFrequencies[alertPatternStep];
  uint16_t dur  = patternDurationsMs[alertPatternStep];
  if (freq == 0) { M5.Speaker.stop(); return; }
  M5.Speaker.tone(freq, dur);
}


static void beginConfirmationPhase() {
  wakeForAlert();
  app.fallDetection.phase                   = FALL_PHASE_CONFIRMATION;
  app.fallDetection.popupVisible            = true;
  app.fallDetection.alertSoundActive        = false;
  app.fallDetection.alertVisualActive       = false;
  app.fallDetection.confirmationStartedAtMs = millis();
  M5.Speaker.stop();
  startHaptic(HAPTIC_FALL_CONFIRM);
  resetAlertSoundPattern();
  requestRedraw();
}


static void beginAlertPhase() {
  app.fallDetection.phase             = FALL_PHASE_ALERT;
  app.fallDetection.popupVisible      = true;
  app.fallDetection.alertSoundActive  = true;
  app.fallDetection.alertVisualActive = true;
  resetAlertSoundPattern();
  startHaptic(HAPTIC_FALL_ALERT);
  alertPhaseStartedAtMs   = millis();
  fallBleSentForThisAlert = false;
  requestRedraw();
}


bool isFallConfirmationActive() { return app.fallDetection.phase == FALL_PHASE_CONFIRMATION; }
bool isFallAlertActive()        { return app.fallDetection.phase == FALL_PHASE_ALERT; }
bool isFallOverlayActive()      { return app.fallDetection.phase == FALL_PHASE_CONFIRMATION ||
                                         app.fallDetection.phase == FALL_PHASE_ALERT; }


void confirmUserIsOk() {
  app.fallDetection.phase                   = FALL_PHASE_IDLE;
  app.fallDetection.popupVisible            = false;
  app.fallDetection.alertSoundActive        = false;
  app.fallDetection.alertVisualActive       = false;
  app.fallDetection.confirmationStartedAtMs = 0;
  M5.Speaker.stop();
  stopHaptic();
  resetAlertSoundPattern();
  resetFallDetectionFlags();
  alertPhaseStartedAtMs   = 0;
  fallBleSentForThisAlert = false;

  if (app.timer.running) {
    app.timer.startedAtMs = millis();
    app.timer.durationMs  = app.timer.remainingMs;
  }

  restoreNormalBrightness();
  requestRedraw();
}


void triggerFallDetectedForTest() {
  if (!app.fallDetection.enabled) return;
  if (isFallOverlayActive()) return;
  beginConfirmationPhase();
}


void initFallDetection() {
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
  app.fallDetection.freeFallDetected        = false;
  M5.Speaker.stop();
  resetAlertSoundPattern();
  resetFallDetectionFlags();
  accelBufHead            = 0;
  accelBufCount           = 0;
  alertPhaseStartedAtMs   = 0;
  fallBleSentForThisAlert = false;
}


void updateFallDetection() {
  if (!app.fallDetection.enabled) return;

  uint32_t now = millis();

  if (!M5.Imu.update()) {
    if (app.fallDetection.phase == FALL_PHASE_CONFIRMATION &&
        (now - app.fallDetection.confirmationStartedAtMs) >=
        app.fallDetection.confirmationTimeoutMs) {
      beginAlertPhase();
    }
    if (app.fallDetection.phase == FALL_PHASE_ALERT) {
      updateAlertSoundPattern(now);
      if (!fallBleSentForThisAlert &&
          alertPhaseStartedAtMs > 0 &&
          (now - alertPhaseStartedAtMs) >= FALL_BLE_ALERT_DELAY_MS) {
        fallBleSentForThisAlert = true;
        sendFallAlertNotification();
      }
    }
    return;
  }

  if (app.fallDetection.phase == FALL_PHASE_CONFIRMATION) {
    if ((now - app.fallDetection.confirmationStartedAtMs) >=
        app.fallDetection.confirmationTimeoutMs) {
      beginAlertPhase();
    }
    return;
  }

  if (app.fallDetection.phase == FALL_PHASE_ALERT) {
    updateAlertSoundPattern(now);
    if (!fallBleSentForThisAlert &&
        alertPhaseStartedAtMs > 0 &&
        (now - alertPhaseStartedAtMs) >= FALL_BLE_ALERT_DELAY_MS) {
      fallBleSentForThisAlert = true;
      sendFallAlertNotification();
    }
    return;
  }

  auto  imuData        = M5.Imu.getImuData();
  float ax             = imuData.accel.x, ay = imuData.accel.y, az = imuData.accel.z;
  float gx             = imuData.gyro.x,  gy = imuData.gyro.y,  gz = imuData.gyro.z;
  float accelMagnitude = sqrtf(ax*ax + ay*ay + az*az);
  float gyroMagnitude  = sqrtf(gx*gx + gy*gy + gz*gz);
  float horizontal     = sqrtf(ax*ax + ay*ay);
  float tiltDegrees    = atan2f(horizontal, absF(az)) * 57.2958f;
  float tiltDelta      = absF(tiltDegrees - app.fallDetection.lastTiltDegrees);

  app.fallDetection.lastAccelMagnitude = accelMagnitude;
  app.fallDetection.lastGyroMagnitude  = gyroMagnitude;
  app.fallDetection.lastTiltDegrees    = tiltDegrees;
  accelBufPush(accelMagnitude, gyroMagnitude, now);

  // ─── STEP 1: Pre-impact + impact ───────────────────────────────────────
  if (!app.fallDetection.impactDetected) {
    if (accelMagnitude >= IMPACT_THRESHOLD_G) {
      if (wasFreeFallBeforeImpact(now)) {
        app.fallDetection.impactDetected        = true;
        app.fallDetection.impactDetectedAtMs    = now;
        app.fallDetection.inactivityStartedAtMs = 0;
        app.fallDetection.freeFallDetected      = true;
      }
    }
    return;
  }

  // ─── Timeout  ──────────────────────────────────────────────────────────
  if ((now - app.fallDetection.impactDetectedAtMs) > IMPACT_WINDOW_MS) {
    resetFallDetectionFlags();
    return;
  }

  // ─── STEP 2: orientation change ────────────────────────────────────────
  if (!app.fallDetection.orientationChanged) {
    if (tiltDelta >= TILT_CHANGE_DEG || gyroMagnitude >= POST_IMPACT_GYRO_MIN_DPS) {
      app.fallDetection.orientationChanged = true;
    }
    return;
  }

  // ─── STEP 3: Post-impact ───────────────────────────────────────────────
  float accelDeltaFrom1G = absF(accelMagnitude - 1.0f);
  if (accelDeltaFrom1G < INACTIVITY_ACCEL_DELTA_G) {
    if (app.fallDetection.inactivityStartedAtMs == 0) {
      app.fallDetection.inactivityStartedAtMs = now;
    } else if ((now - app.fallDetection.inactivityStartedAtMs) >= INACTIVITY_FAST_CONFIRM_MS) {
      app.fallDetection.inactivityDetected = true;
      beginConfirmationPhase();
      resetFallDetectionFlags();
      return;
    }
  } else {
    app.fallDetection.inactivityStartedAtMs = 0;
  }

  if ((now - app.fallDetection.impactDetectedAtMs) >= FALL_CONFIRM_TIMEOUT_MS) {
    app.fallDetection.inactivityDetected = true;
    beginConfirmationPhase();
    resetFallDetectionFlags();
  }
}