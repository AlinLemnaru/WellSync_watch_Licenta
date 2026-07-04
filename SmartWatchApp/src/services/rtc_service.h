#pragma once
#include <Arduino.h>

void initRtcService();
void updateRtcService();

void setBleTimeAndWeather(
  int year, int month, int day,
  int hour, int minute, int second,
  float temperatureC,
  const String& deviceName
);

void disconnectBleFallback();

bool setManualDateTime(
  int year, int month, int day,
  int hour, int minute, int second
);

bool isManualTimeSettingAllowed();
bool hasValidDateTime();
bool isBleTimeActive();