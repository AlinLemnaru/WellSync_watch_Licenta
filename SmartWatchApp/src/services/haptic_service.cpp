#include <M5Unified.h>
#include "haptic_service.h"

// ─── Pattern Definition ────────────────────────────────────────────
struct HapticStep {
    uint8_t  intensity;   
    uint16_t durationMs;
};

// Alarm
static const HapticStep PATTERN_ALARM[] = {
    { 180, 500 },
    {   0, 300 },
};
static const uint8_t PATTERN_ALARM_LEN = 2;

// Timer
static const HapticStep PATTERN_TIMER[] = {
    { 160, 400 },
    {   0, 200 },
};
static const uint8_t PATTERN_TIMER_LEN = 2;

// Fall confirmation
static const HapticStep PATTERN_FALL_CONFIRM[] = {
    {  150, 200 },
    {   0, 800 },
};
static const uint8_t PATTERN_FALL_CONFIRM_LEN = 2;

// Fall alert
static const HapticStep PATTERN_FALL_ALERT[] = {
    { 220, 150 },
    {   0, 100 },
    { 220, 150 },
    {   0, 300 },
};
static const uint8_t PATTERN_FALL_ALERT_LEN = 4;

// ─── State intern ─────────────────────────────────────────────────────────
static HapticPattern   currentPattern   = HAPTIC_OFF;
static uint8_t         currentStep      = 0;
static uint32_t        stepStartedAtMs  = 0;
static bool            hapticActive     = false;

static const HapticStep* getPatternSteps(HapticPattern p) {
    switch (p) {
        case HAPTIC_ALARM:        return PATTERN_ALARM;
        case HAPTIC_TIMER:        return PATTERN_TIMER;
        case HAPTIC_FALL_CONFIRM: return PATTERN_FALL_CONFIRM;
        case HAPTIC_FALL_ALERT:   return PATTERN_FALL_ALERT;
        default:                  return nullptr;
    }
}

static uint8_t getPatternLen(HapticPattern p) {
    switch (p) {
        case HAPTIC_ALARM:        return PATTERN_ALARM_LEN;
        case HAPTIC_TIMER:        return PATTERN_TIMER_LEN;
        case HAPTIC_FALL_CONFIRM: return PATTERN_FALL_CONFIRM_LEN;
        case HAPTIC_FALL_ALERT:   return PATTERN_FALL_ALERT_LEN;
        default:                  return 0;
    }
}

// ─── API public ───────────────────────────────────────────────────────────
void initHaptic() {
    M5.Power.setVibration(0);
    currentPattern  = HAPTIC_OFF;
    currentStep     = 0;
    stepStartedAtMs = 0;
    hapticActive    = false;
}

void startHaptic(HapticPattern pattern) {
    if (pattern == HAPTIC_OFF) {
        stopHaptic();
        return;
    }
    currentPattern  = pattern;
    currentStep     = 0;
    stepStartedAtMs = millis();
    hapticActive    = true;

    const HapticStep* steps = getPatternSteps(pattern);
    if (steps) {
        M5.Power.setVibration(steps[0].intensity);
    }
}

void stopHaptic() {
    M5.Power.setVibration(0);
    currentPattern  = HAPTIC_OFF;
    currentStep     = 0;
    stepStartedAtMs = 0;
    hapticActive    = false;
}

void updateHaptic() {
    if (!hapticActive || currentPattern == HAPTIC_OFF) return;

    const HapticStep* steps = getPatternSteps(currentPattern);
    uint8_t           len   = getPatternLen(currentPattern);
    if (!steps || len == 0) return;

    uint32_t now = millis();
    if ((now - stepStartedAtMs) < steps[currentStep].durationMs) return;

    currentStep     = (currentStep + 1) % len;
    stepStartedAtMs = now;
    M5.Power.setVibration(steps[currentStep].intensity);
}