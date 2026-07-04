#pragma once

void initAlarmService();
void updateAlarmService();

void setAlarmTime(int hour, int minute);
void enableAlarm(bool enabled);
void stopAlarmRinging();

bool isAlarmEnabled();
bool isAlarmRinging();