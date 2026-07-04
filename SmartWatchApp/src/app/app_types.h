#pragma once
#include <Arduino.h>


enum PageId {
  PAGE_HOME = 0,
  PAGE_HEALTH,
  PAGE_STRESS,
  PAGE_ENVIRONMENT,
  PAGE_SLEEP,
  PAGE_SETTINGS,
  PAGE_COUNT
};


enum ConnectionState {
  CONNECTION_DISCONNECTED = 0,
  CONNECTION_CONNECTING,
  CONNECTION_CONNECTED
};


enum DataAvailability {
  DATA_UNAVAILABLE = 0,
  DATA_AVAILABLE
};


enum HomeAction {
  HOME_ACTION_NONE = 0,
  HOME_ACTION_ALARM,
  HOME_ACTION_CHRONO,
  HOME_ACTION_TIMER
};


enum TimeSource {
  TIME_SOURCE_DEFAULT = 0,
  TIME_SOURCE_MANUAL,
  TIME_SOURCE_BLE
};


enum DateTimeEditField {
  EDIT_FIELD_NONE = 0,
  EDIT_FIELD_DAY,
  EDIT_FIELD_MONTH,
  EDIT_FIELD_YEAR,
  EDIT_FIELD_HOUR,
  EDIT_FIELD_MINUTE,
  EDIT_FIELD_SAVE
};


enum SettingsOption {
  SETTINGS_OPTION_BLUETOOTH = 0,
  SETTINGS_OPTION_TIME_SOURCE,
  SETTINGS_OPTION_BRIGHTNESS,
  SETTINGS_OPTION_STANDBY_TIMEOUT,
  SETTINGS_OPTION_STEP_TARGET,
  SETTINGS_OPTION_BATTERY,
  SETTINGS_OPTION_DEVICE,
  SETTINGS_OPTION_FIRMWARE,
  SETTINGS_OPTION_COUNT
};


enum AlarmEditField {
  ALARM_EDIT_NONE = 0,
  ALARM_EDIT_HOUR,
  ALARM_EDIT_MINUTE,
  ALARM_EDIT_ENABLED,
  ALARM_EDIT_SAVE
};


enum TimerEditField {
  TIMER_EDIT_NONE = 0,
  TIMER_EDIT_MINUTES,
  TIMER_EDIT_SECONDS,
  TIMER_EDIT_START
};


enum StandbyEditField {
  STANDBY_EDIT_NONE = 0,
  STANDBY_EDIT_TIMEOUT,
  STANDBY_EDIT_SAVE
};


// ── Brightness editor ────────────────────────────────────────────────
enum BrightnessEditField {
  BRIGHTNESS_EDIT_NONE = 0,
  BRIGHTNESS_EDIT_VALUE,
  BRIGHTNESS_EDIT_SAVE
};

enum StepTargetEditField {
  STEP_TARGET_EDIT_NONE = 0,
  STEP_TARGET_EDIT_VALUE,
  STEP_TARGET_EDIT_SAVE
};

enum FallAlertPhase {
  FALL_PHASE_IDLE = 0,
  FALL_PHASE_CONFIRMATION,
  FALL_PHASE_ALERT
};


struct TimeData {
  int hour;
  int minute;
  int second;
  int day;
  int month;
  int year;
  bool syncedFromPhone;
};


struct TimeSyncState {
  TimeSource source;
  bool manualSetAllowed;
  bool hasValidTime;
};


struct WeatherData {
  float temperatureC;
  String conditionText;
  DataAvailability availability;
  bool fromBleDevice;
};


struct BleState {
  ConnectionState state;
  String deviceName;
};


struct HealthData {
  int heartRateBpm;
  int spo2Percent;
  int steps;
  int stepTarget; 
  bool measuring;
  DataAvailability availability;
};


struct EnvironmentData {
  float temperatureC;
  float humidityPercent;
  float pressureHpa;
  uint32_t gasResistanceOhms;
  int iaqScore;
  String iaqLabel;
  DataAvailability availability;
};


struct SleepData {
  int sleepScore;
  int durationHours;
  int durationMinutes;
  String bedTime;
  String wakeTime;
  int deepSleepMinutes;
  DataAvailability availability;
};


struct StressData {
  int stressScore;
  int hrvMs;
  int restingHr;
  String statusLabel;
  int recoveryScore;
  DataAvailability availability;
};


struct AlarmState {
  bool enabled;
  bool ringing;
  int hour;
  int minute;
};


struct AlarmEditorState {
  bool active;
  AlarmEditField field;
  int hour;
  int minute;
  bool enabled;
};


struct TimerState {
  bool running;
  bool paused;
  bool finished;
  uint32_t durationMs;
  uint32_t remainingMs;
  uint32_t startedAtMs;
  uint32_t pausedRemainingMs;
};


struct TimerEditorState {
  bool active;
  TimerEditField field;
  int minutes;
  int seconds;
};


struct StandbyEditorState {
  bool             active;
  StandbyEditField field;
  uint32_t         timeoutMs;
};


// ── Brightness editor ────────────────────────────────────────────────
struct BrightnessEditorState {
  bool                active;
  BrightnessEditField field;
  int                 percent;   
};

struct StepTargetEditorState {
  bool               active;
  StepTargetEditField field;
  int                targetValue;  
};

struct StopwatchState {
  bool running;
  bool paused;
  uint32_t startedAtMs;
  uint32_t elapsedMs;
  uint32_t pausedElapsedMs;
};


struct FallDetectionState {
  bool enabled;

  FallAlertPhase phase;
  bool popupVisible;
  bool alertSoundActive;
  bool alertVisualActive;

  uint32_t confirmationStartedAtMs;
  uint32_t confirmationTimeoutMs;

  float lastAccelMagnitude;
  float lastGyroMagnitude;
  float lastTiltDegrees;

  uint32_t impactDetectedAtMs;
  uint32_t inactivityStartedAtMs;

  bool impactDetected;
  bool orientationChanged;
  bool inactivityDetected;
  bool freeFallDetected;

  bool     confirmSoundActive;
  uint32_t lastConfirmBeepAtMs;
};


struct SettingsData {
  bool bluetoothEnabled;
  int brightnessPercent;
  int batteryPercent;
  String deviceName;
  String firmwareVersion;
  SettingsOption selectedOption;
  int scrollOffset;
};


struct StandbyState {
  bool     active;
  uint32_t lastActivityMs;
  uint32_t timeoutMs;
};


struct DateTimeEditorState {
  bool active;
  DateTimeEditField field;
  int day;
  int month;
  int year;
  int hour;
  int minute;
};


struct AppState {
  PageId currentPage;
  PageId previousPage;
  bool pageChanged;

  TimeData time;
  TimeSyncState timeSync;
  WeatherData weather;
  BleState ble;
  HealthData health;
  EnvironmentData environment;
  SleepData sleep;
  StressData stress;
  AlarmState alarm;
  AlarmEditorState alarmEditor;
  TimerState timer;
  TimerEditorState timerEditor;
  StandbyEditorState standbyEditor;
  BrightnessEditorState brightnessEditor;   
  StepTargetEditorState stepTargetEditor; 
  StopwatchState stopwatch;
  FallDetectionState fallDetection;
  SettingsData settings;
  DateTimeEditorState dateTimeEditor;
  StandbyState standby;

  HomeAction homeSelectedAction;
  String homeStatusMessage;
  uint32_t homeStatusUntilMs;
};