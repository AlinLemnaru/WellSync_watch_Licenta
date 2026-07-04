#include <M5Unified.h>
#include "ble_service.h"
#include "../app/app_state.h"
#include "../ui/ui.h"
#include "../services/rtc_service.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ── UUIDs ──────────────────────────────────────────────────────────────────
#define SERVICE_UUID          ""
#define MEASUREMENT_CHAR_UUID ""
// Write
#define CONFIG_CHAR_UUID      ""
// Notify
#define ALERT_CHAR_UUID       ""

static constexpr uint32_t NOTIFY_INTERVAL_MS = 3000;

static BLEServer*         _server          = nullptr;
static BLECharacteristic* _measureChar     = nullptr;
static BLECharacteristic* _alertChar = nullptr;
static bool               _deviceConnected = false;
static uint32_t           _lastNotifyMs    = 0;
static bool               _bleStateChanged = false;

// ── JSON Parser ───────────────────────────────────────────────────────
static int parseJsonInt(const String& json, const char* key, int defaultVal) {
  String search = "\"";
  search += key;
  search += "\":";

  int idx = json.indexOf(search);

  if (idx < 0) return defaultVal;

  idx += search.length();

  while (idx < (int)json.length() && json[idx] == ' ') idx++;

  int start = idx;

  if (idx < (int)json.length() && json[idx] == '-') idx++;

  while (idx < (int)json.length() && isdigit(json[idx])) idx++;

  if (idx == start) return defaultVal;

  return x.substring(start, idx).toInt();
}

// ── Callback Write ────────────────────────────────────────────────────
// Expected JSON: {"year":2026,"month":6,"day":7,"hour":20,"minute":30,"second":0,"step_target":8000}
class ConfigCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) override {
    String val = pChar->getValue().c_str();
    if (val.length() < 5) return;

    int year    = parseJsonInt(val, "year",        2026);
    int month   = parseJsonInt(val, "month",          1);
    int day     = parseJsonInt(val, "day",             1);
    int hour    = parseJsonInt(val, "hour",            0);
    int minute  = parseJsonInt(val, "minute",          0);
    int second  = parseJsonInt(val, "second",          0);
    int stepTgt = parseJsonInt(val, "step_target",  8000);

    setBleTimeAndWeather(year, month, day, hour, minute, second, 0.0f, "WellSync App");

    if (stepTgt >= 500 && stepTgt <= 50000) {
      app.health.stepTarget = stepTgt;
    }

    _bleStateChanged = true;
  }
};

// ── Callback connect ──────────────────────────────────────────────────
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) override {
    _deviceConnected = true;
    app.ble.state      = CONNECTION_CONNECTED;
    app.ble.deviceName = "WellSync App";
    _bleStateChanged   = true;
  }

  void onDisconnect(BLEServer* server) override {
    _deviceConnected = false;
    disconnectBleFallback();
    _bleStateChanged = true;
    BLEDevice::startAdvertising();
  }
};

// ── Init ──────────────────────────────────────────────────────────────────────
void initBleService() {
  if (!app.settings.bluetoothEnabled) return;

  BLEDevice::init("WellSync Core2");

  _server = BLEDevice::createServer();
  _server->setCallbacks(new ServerCallbacks());

  BLEService* service = _server->createService(BLEUUID(SERVICE_UUID), 30);

  _measureChar = service->createCharacteristic(
    MEASUREMENT_CHAR_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  _measureChar->addDescriptor(new BLE2902());

  _alertChar = service->createCharacteristic(
    ALERT_CHAR_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  _alertChar->addDescriptor(new BLE2902());

  BLECharacteristic* configChar = service->createCharacteristic(
    CONFIG_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  configChar->setCallbacks(new ConfigCallbacks());

  service->start();

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  adv->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  app.ble.state = CONNECTION_DISCONNECTED;
}

// ── JSON sensor data ─────────────────────────────────────────────────────────

static String fmt(float val, int decimals) {
  if (isnan(val) || isinf(val)) return "0";
  if (val > 99999.0f || val < -9999.0f) return "0";
  return String(val, decimals);
}

static String buildJson() {
  float hr   = (app.health.availability == DATA_AVAILABLE)
                ? (float)app.health.heartRateBpm : 0.0f;
  float spo2 = (app.health.availability == DATA_AVAILABLE)
                ? (float)app.health.spo2Percent  : 0.0f;
  int steps  = app.health.steps;

  float temp     = (app.environment.availability == DATA_AVAILABLE)
                    ? app.environment.temperatureC    : 0.0f;
  float humidity = (app.environment.availability == DATA_AVAILABLE)
                    ? app.environment.humidityPercent : 0.0f;
  float pressure = (app.environment.availability == DATA_AVAILABLE)
                    ? app.environment.pressureHpa     : 0.0f;
  float iaq      = (app.environment.availability == DATA_AVAILABLE)
                    ? (float)app.environment.iaqScore : 0.0f;

  String json = "{";
  json += "\"steps_total_today\":"  + String(steps)         + ",";
  json += "\"heart_rate_bpm\":"     + fmt(hr,       0)      + ",";
  json += "\"spo2\":"               + fmt(spo2,     0)      + ",";
  json += "\"air_quality_index\":"  + fmt(iaq,      0)      + ",";
  json += "\"temperature_c\":"      + fmt(temp,     1)      + ",";
  json += "\"humidity_percent\":"   + fmt(humidity, 1)      + ",";
  json += "\"pressure_hpa\":"       + fmt(pressure, 1);
  json += "}";
  return json;
}

// ── Update — called in loop() ────────────────────────────────────────────────
void updateBleService() {
  if (_bleStateChanged) {
    _bleStateChanged = false;
    if (app.currentPage == PAGE_SETTINGS) requestRedraw();
  }

  if (!app.settings.bluetoothEnabled) return;
  if (!_deviceConnected)              return;
  if (_measureChar == nullptr)        return;

  uint32_t now = millis();
  if (now - _lastNotifyMs < NOTIFY_INTERVAL_MS) return;
  _lastNotifyMs = now;

  String json = buildJson();
  _measureChar->setValue(json.c_str());
  _measureChar->notify();
}

// ── Alert fall detection ─────────────────────────────────────────────────────
void sendFallAlertNotification() {
  if (!app.settings.bluetoothEnabled) return;
  if (!_deviceConnected)              return;
  if (_alertChar == nullptr)          return;

  String json = "{\"type\":\"fall_alert\"}";

  _alertChar->setValue(json.c_str());
  _alertChar->notify();
}
