#pragma once
#include <stdint.h>

enum HapticPattern {
    HAPTIC_OFF = 0,
    HAPTIC_ALARM,           
    HAPTIC_TIMER,           
    HAPTIC_FALL_CONFIRM,    
    HAPTIC_FALL_ALERT       
};

void initHaptic();
void startHaptic(HapticPattern pattern);
void stopHaptic();
void updateHaptic();         