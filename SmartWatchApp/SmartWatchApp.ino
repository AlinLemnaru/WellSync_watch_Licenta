#include <M5Unified.h>

#include "src/app/app_state.h"
#include "src/board/board_init.h"
#include "src/input/input.h"
#include "src/sensors/imu/fall_detection.h"
#include "src/services/alarm_service.h"
#include "src/services/chrono_service.h"
#include "src/services/rtc_service.h"
#include "src/services/timer_service.h"
#include "src/services/haptic_service.h"
#include "src/services/standby_service.h"   
#include "src/services/stress_estimation_service.h"
#include "src/sensors/imu/step_counter.h"
#include "src/sensors/ppg/hr_spo2_service.h"
#include "src/sensors/env/environment_service.h"
#include "src/services/ble_service.h"
#include "src/ui/ui_splash.h"
#include "src/ui/ui.h"

void setup() {
  initBoard();
  initAppState();

  Wire.begin(32, 33);

  // Brightness Fade-in before splash
  {
    uint8_t target = static_cast<uint8_t>(app.settings.brightnessPercent * 255 / 100);
    for (uint8_t b = 0; b <= target; b += 5) {
    M5.Display.setBrightness(b);
    delay(8);   
    }

    M5.Display.setBrightness(target);  
  }

  drawSplashScreen();
  delay(3000);

  initRtcService();
  initAlarmService();
  initTimerService();
  initChronoService();
  initFallDetection();
  initHaptic();
  initStandby();          

  initHrSpo2Service();
  initEnvironmentService(); 
  initStressEstimationService();
  initBleService(); 
  initUI();
  drawCurrentPage(true);
}


void loop() {
  M5.update();

  updateBatteryState(); 
  updateInput();
  updateRtcService();
  updateAlarmService();
  updateTimerService();
  updateChronoService();
  updateFallDetection();
  updateStepCounterService(); 
  updateHrSpo2Service();
  updateEnvironmentService();
  updateStressEstimationService();
  updateBleService();
  updateHaptic();
  updateStandby();        
  updateUI();

  delay(16);
}