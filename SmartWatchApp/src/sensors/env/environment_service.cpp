#include <M5Unified.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include "environment_service.h"
#include "../../app/app_state.h"
#include "../../ui/ui.h"


// ── Intervals ─────────────────────────────────────────────────────────────────
static constexpr uint32_t READ_INTERVAL_MS         = 8000;
static constexpr uint32_t READ_INTERVAL_STARTUP_MS = 3000;
static constexpr uint32_t STARTUP_PERIOD_MS        = 5UL * 60UL * 1000UL;
static constexpr uint32_t READING_TIMEOUT_MS       = 5000;
static constexpr uint32_t WARMUP_MS                = 15000;  


// ── TH IAQ ────────────────────────────────────────────────────────────────────
static constexpr float GAS_FLOOR_INVALID = 1500.0f;
static constexpr float HUM_IDEAL         =   40.0f;


// ── Baseline rolling (30 min) ─────────────────────────────────────────────────
static constexpr uint32_t BASELINE_WINDOW_MS = 30UL * 60UL * 1000UL;
static constexpr int      BASELINE_SLOTS     = 15;
static constexpr uint32_t BASELINE_SLOT_MS   = BASELINE_WINDOW_MS / BASELINE_SLOTS;

static float    _baselineSlots[BASELINE_SLOTS];
static uint32_t _baselineSlotTs[BASELINE_SLOTS];
static int      _baselineSlotIdx = 0;
static float    _gasBaseline     = 0.0f;


// ── State ─────────────────────────────────────────────────────────────────────
static Adafruit_BME680 _bme;
static bool     _bmeReady       = false;
static bool     _readingPending = false;
static uint32_t _readingStartMs = 0;
static uint32_t _readingDoneMs  = 0;
static uint32_t _lastReadMs     = 0;
static uint32_t _initMs         = 0;

static float    _lastGoodTemp     = 0.0f;
static float    _lastGoodHumidity = 0.0f;
static float    _lastGoodPressure = 0.0f;
static float    _lastGoodGas      = 0.0f;
static int      _lastGoodIaq      = -1;
static bool     _hasGoodData      = false;


// ── Baseline ──────────────────────────────────────────────────────────────────
static void updateGasBaseline(float gasOhms, uint32_t now) {
  if (now - _baselineSlotTs[_baselineSlotIdx] >= BASELINE_SLOT_MS) {
    _baselineSlotIdx = (_baselineSlotIdx + 1) % BASELINE_SLOTS;
    _baselineSlots[_baselineSlotIdx]  = gasOhms;
    _baselineSlotTs[_baselineSlotIdx] = now;
  } else {
    if (gasOhms > _baselineSlots[_baselineSlotIdx])
      _baselineSlots[_baselineSlotIdx] = gasOhms;
  }

  float maxGas = 0.0f;
  for (int i = 0; i < BASELINE_SLOTS; i++) {
    if (_baselineSlotTs[i] > 0 &&
        (now - _baselineSlotTs[i]) <= BASELINE_WINDOW_MS &&
        _baselineSlots[i] > maxGas) {
      maxGas = _baselineSlots[i];
    }
  }
  if (maxGas > GAS_FLOOR_INVALID) _gasBaseline = maxGas;
}


// ── IAQ ───────────────────────────────────────────────────────────────────────
static int estimateIaq(float gasOhms, float humidity, uint32_t now) {
  if (now - _initMs < WARMUP_MS)        return -1;
  if (gasOhms < GAS_FLOOR_INVALID)      return -1;
  if (_gasBaseline < GAS_FLOOR_INVALID) return -1;

  float ratio = gasOhms / _gasBaseline;
  if (ratio > 1.0f) ratio = 1.0f;

  float gasScore;
  if      (ratio >= 0.90f) gasScore = 75.0f;
  else if (ratio >= 0.70f) gasScore = 55.0f + (ratio - 0.70f) / 0.20f * 20.0f;
  else if (ratio >= 0.45f) gasScore = 30.0f + (ratio - 0.45f) / 0.25f * 25.0f;
  else if (ratio >= 0.20f) gasScore =  5.0f + (ratio - 0.20f) / 0.25f * 25.0f;
  else                     gasScore = 0.0f;

  float humScore;
  float humOffset = humidity - HUM_IDEAL;
  if (humOffset >= 0.0f) {
    humScore = (100.0f - HUM_IDEAL - humOffset) / (100.0f - HUM_IDEAL) * 25.0f;
  } else {
    humScore = (HUM_IDEAL + humOffset) / HUM_IDEAL * 25.0f;
  }
  if (humScore < 0.0f)  humScore = 0.0f;
  if (humScore > 25.0f) humScore = 25.0f;

  int iaq = (int)(gasScore + humScore);
  return iaq > 100 ? 100 : iaq < 0 ? 0 : iaq;
}


static const char* iaqToLabel(int iaq) {
  if (iaq >= 75) return "Good";
  if (iaq >= 55) return "Moderate";
  if (iaq >= 35) return "Poor";
  return "Bad";
}


// ── Write to app.environment ──────────────────────────────────────────────────
static void commitToAppState(float temp, float humidity, float pressure,
                             float gas, int iaq, bool iaqValid) {
  bool metaChanged =
    (int)(temp     * 10) != (int)(app.environment.temperatureC    * 10) ||
    (int)(humidity * 10) != (int)(app.environment.humidityPercent * 10) ||
    (int)(pressure * 10) != (int)(app.environment.pressureHpa     * 10);

  bool iaqChanged = iaqValid && (iaq != app.environment.iaqScore);

  app.environment.temperatureC      = temp;
  app.environment.humidityPercent   = humidity;
  app.environment.pressureHpa       = pressure;
  app.environment.gasResistanceOhms = (uint32_t)gas;
  app.environment.availability      = DATA_AVAILABLE;

  if (iaqValid) {
    app.environment.iaqScore = iaq;
    app.environment.iaqLabel = iaqToLabel(iaq);
  } else {
    app.environment.iaqScore = 0;
    app.environment.iaqLabel = "Warming up";
  }

  if ((metaChanged || iaqChanged) && app.currentPage == PAGE_ENVIRONMENT)
    requestRedraw();
}


// ── Init ──────────────────────────────────────────────────────────────────────
void initEnvironmentService() {
  app.environment.availability      = DATA_UNAVAILABLE;
  app.environment.iaqScore          = 0;
  app.environment.iaqLabel          = "--";
  app.environment.temperatureC      = 0.0f;
  app.environment.humidityPercent   = 0.0f;
  app.environment.pressureHpa       = 0.0f;
  app.environment.gasResistanceOhms = 0;

  bool found = false;
  if (_bme.begin(0x76, &Wire)) {
    Serial.println("[BME] Found at 0x76");
    found = true;
  } else if (_bme.begin(0x77, &Wire)) {
    Serial.println("[BME] Found at 0x77");
    found = true;
  }

  if (!found) {
    Serial.println("[BME] ERROR: not found on 0x76 or 0x77");
    return;
  }

  _bme.setTemperatureOversampling(BME680_OS_8X);
  _bme.setHumidityOversampling(BME680_OS_2X);
  _bme.setPressureOversampling(BME680_OS_4X);
  _bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  _bme.setGasHeater(320, 300);

  _bmeReady       = true;
  _readingPending = false;
  _readingStartMs = 0;
  _readingDoneMs  = 0;
  _lastReadMs     = 0;
  _initMs         = millis();
  _hasGoodData    = false;
  _lastGoodIaq    = -1;
  _lastGoodGas    = 0.0f;

  memset(_baselineSlots,  0, sizeof(_baselineSlots));
  memset(_baselineSlotTs, 0, sizeof(_baselineSlotTs));
  _baselineSlotIdx = 0;
  _gasBaseline     = 0.0f;

  Serial.printf("[BME] Init OK, warming up %lus...\n", WARMUP_MS / 1000);
}


// ── Update ────────────────────────────────────────────────────────────────────
void updateEnvironmentService() {
  if (!_bmeReady) return;

  uint32_t now = millis();

  uint32_t interval = (now - _initMs < STARTUP_PERIOD_MS)
                      ? READ_INTERVAL_STARTUP_MS
                      : READ_INTERVAL_MS;

  if (!_readingPending && (now - _lastReadMs < interval)) return;

  // ── Faze 1: start new reading ───────────────────────────────────────────────
  if (!_readingPending) {
    uint32_t raw = _bme.beginReading();

    if (raw == 0) {
      Serial.println("[BME] beginReading failed");
      _lastReadMs = now;
      return;
    }

    uint32_t dur = (raw > now) ? (raw - now) : raw;
    if (dur < 100)  dur = 100;
    if (dur > 5000) dur = 500;

    Serial.printf("[BME] beginReading dur=%lu ms\n", dur);

    _readingStartMs = now;
    _readingDoneMs  = now + dur;
    _readingPending = true;
    return;
  }

  // ── Faze 2: get data ────────────────────────────────────────────────────────
  if (now - _readingStartMs > READING_TIMEOUT_MS) {
    Serial.println("[BME] timeout, resetting");
    _readingPending = false;
    _lastReadMs     = now;
    return;
  }

  if (now < _readingDoneMs) return;

  if (!_bme.endReading()) {
    Serial.println("[BME] endReading failed");
    _readingPending = false;
    _lastReadMs     = now;
    return;
  }

  _readingPending = false;
  _lastReadMs     = now;

  float temp     = _bme.temperature;
  float humidity = _bme.humidity;
  float pressure = _bme.pressure / 100.0f;
  float gas      = (float)_bme.gas_resistance;

  Serial.printf("[BME] temp=%.1f hum=%.1f pres=%.1f gas=%.0f baseline=%.0f\n",
                temp, humidity, pressure, gas, _gasBaseline);

  if (temp < -40.0f || temp > 100.0f)          return;
  if (humidity < 0.0f || humidity > 100.0f)    return;
  if (pressure < 200.0f || pressure > 1200.0f) return;

  _lastGoodTemp     = temp;
  _lastGoodHumidity = humidity;
  _lastGoodPressure = pressure;
  _lastGoodGas      = gas;
  _hasGoodData      = true;

  updateGasBaseline(gas, now);

  if (_gasBaseline < GAS_FLOOR_INVALID && gas > GAS_FLOOR_INVALID) {
    _gasBaseline = gas;
  }

  int  iaq      = estimateIaq(gas, humidity, now);
  bool iaqValid = (iaq >= 0);

  if (!iaqValid && _lastGoodIaq >= 0) {
    iaq      = _lastGoodIaq;
    iaqValid = true;
  }

  if (iaqValid && iaq >= 0) _lastGoodIaq = iaq;

  commitToAppState(temp, humidity, pressure, gas, iaq, iaqValid);
}