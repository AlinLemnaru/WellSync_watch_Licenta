#pragma once

void initChronoService();
void updateChronoService();

void startChrono();
void pauseChrono();
void resumeChrono();
void resetChrono();

bool isChronoRunning();
bool isChronoPaused();
uint32_t getChronoElapsedMs();