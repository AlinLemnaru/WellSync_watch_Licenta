#pragma once

void initTimerService();
void updateTimerService();

void setTimerDurationMs(uint32_t durationMs);
void startTimer();
void pauseTimer();
void resumeTimer();
void resetTimer();
void stopTimerAlert();

bool isTimerRunning();
bool isTimerPaused();
bool isTimerFinished();
uint32_t getTimerRemainingMs();