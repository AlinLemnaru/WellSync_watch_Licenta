#include "../app/app_state.h"
#include "rtc_service.h"

static uint32_t lastClockUpdateMs = 0;

static bool isLeapYear(int year) {
  return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

static int daysInMonth(int month, int year) {
  switch (month) {
    case 1:  return 31;
    case 2:  return isLeapYear(year) ? 29 : 28;
    case 3:  return 31;
    case 4:  return 30;
    case 5:  return 31;
    case 6:  return 30;
    case 7:  return 31;
    case 8:  return 31;
    case 9:  return 30;
    case 10: return 31;
    case 11: return 30;
    case 12: return 31;
    default: return 30;
  }
}

static bool isValidDateTime(int year, int month, int day, int hour, int minute, int second) {
  if (year < 2026 || year > 2099) return false;
  if (month < 1   || month > 12)  return false;
  if (day < 1     || day > daysInMonth(month, year)) return false;
  if (hour < 0    || hour > 23)   return false;
  if (minute < 0  || minute > 59) return false;
  if (second < 0  || second > 59) return false;
  return true;
}

static void incrementOneSecond() {
  app.time.second++;
  if (app.time.second >= 60) { app.time.second = 0; app.time.minute++; }
  if (app.time.minute >= 60) { app.time.minute = 0; app.time.hour++;   }
  if (app.time.hour   >= 24) { app.time.hour   = 0; app.time.day++;    }

  int dim = daysInMonth(app.time.month, app.time.year);
  if (app.time.day > dim) { app.time.day = 1; app.time.month++; }
  if (app.time.month > 12) { app.time.month = 1; app.time.year++; }
}

void initRtcService() {
  lastClockUpdateMs = millis();
}

void updateRtcService() {
  uint32_t now = millis();
  while (now - lastClockUpdateMs >= 1000) {
    lastClockUpdateMs += 1000;
    incrementOneSecond();
    app.timeSync.hasValidTime = true;
  }
}

void setBleTimeAndWeather(
  int year, int month, int day,
  int hour, int minute, int second,
  float temperatureC,
  const String& deviceName) {
  if (!isValidDateTime(year, month, day, hour, minute, second)) return;

  app.time.year   = year;
  app.time.month  = month;
  app.time.day    = day;
  app.time.hour   = hour;
  app.time.minute = minute;
  app.time.second = second;
  app.time.syncedFromPhone = true;

  app.timeSync.source          = TIME_SOURCE_BLE;
  app.timeSync.manualSetAllowed = false;
  app.timeSync.hasValidTime    = true;

  app.ble.state      = CONNECTION_CONNECTED;
  app.ble.deviceName = deviceName.length() ? deviceName : "Phone";

  app.weather.temperatureC  = temperatureC;
  app.weather.conditionText = "From phone";
  app.weather.availability  = DATA_AVAILABLE;
  app.weather.fromBleDevice = true;

  lastClockUpdateMs = millis();
}

void disconnectBleFallback() {
  app.ble.state      = CONNECTION_DISCONNECTED;
  app.ble.deviceName = "No Device Connection";

  app.weather.temperatureC  = 0.0f;
  app.weather.conditionText = "No Device Connection";
  app.weather.availability  = DATA_UNAVAILABLE;
  app.weather.fromBleDevice = false;

  app.time.syncedFromPhone = false;

  if (app.timeSync.source == TIME_SOURCE_BLE) {
    app.timeSync.source = app.timeSync.hasValidTime
      ? TIME_SOURCE_MANUAL
      : TIME_SOURCE_DEFAULT;
  }

  app.timeSync.manualSetAllowed = true;
  lastClockUpdateMs = millis();
}

bool setManualDateTime(int year, int month, int day, int hour, int minute, int second) {
  if (!app.timeSync.manualSetAllowed)                          return false;
  if (!isValidDateTime(year, month, day, hour, minute, second)) return false;

  app.time.year   = year;
  app.time.month  = month;
  app.time.day    = day;
  app.time.hour   = hour;
  app.time.minute = minute;
  app.time.second = second;
  app.time.syncedFromPhone = false;

  app.timeSync.source          = TIME_SOURCE_MANUAL;
  app.timeSync.manualSetAllowed = true;
  app.timeSync.hasValidTime    = true;

  lastClockUpdateMs = millis();
  return true;
}

bool isManualTimeSettingAllowed() { return app.timeSync.manualSetAllowed; }
bool hasValidDateTime()           { return app.timeSync.hasValidTime; }
bool isBleTimeActive()            { return app.timeSync.source == TIME_SOURCE_BLE; }