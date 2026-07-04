#include <M5Unified.h>
#include "../app/app_state.h"
#include "chrono_service.h"

void initChronoService() {
  app.stopwatch.running = false;
  app.stopwatch.paused = false;
  app.stopwatch.startedAtMs = 0;
  app.stopwatch.elapsedMs = 0;
  app.stopwatch.pausedElapsedMs = 0;
}

void updateChronoService() {
  if (app.stopwatch.running) {
    uint32_t now = millis();
    app.stopwatch.elapsedMs = app.stopwatch.pausedElapsedMs + (now - app.stopwatch.startedAtMs);
  }
}

void startChrono() {
  app.stopwatch.running = true;
  app.stopwatch.paused = false;
  app.stopwatch.elapsedMs = 0;
  app.stopwatch.pausedElapsedMs = 0;
  app.stopwatch.startedAtMs = millis();
}

void pauseChrono() {
  if (!app.stopwatch.running) return;

  uint32_t now = millis();
  app.stopwatch.elapsedMs = app.stopwatch.pausedElapsedMs + (now - app.stopwatch.startedAtMs);
  app.stopwatch.pausedElapsedMs = app.stopwatch.elapsedMs;
  app.stopwatch.running = false;
  app.stopwatch.paused = true;
}

void resumeChrono() {
  if (!app.stopwatch.paused) return;

  app.stopwatch.running = true;
  app.stopwatch.paused = false;
  app.stopwatch.startedAtMs = millis();
}

void resetChrono() {
  app.stopwatch.running = false;
  app.stopwatch.paused = false;
  app.stopwatch.startedAtMs = 0;
  app.stopwatch.elapsedMs = 0;
  app.stopwatch.pausedElapsedMs = 0;
}

bool isChronoRunning() {
  return app.stopwatch.running;
}

bool isChronoPaused() {
  return app.stopwatch.paused;
}

uint32_t getChronoElapsedMs() {
  return app.stopwatch.elapsedMs;
}