// JourTempo - French Tempo electricity tariff colour on ESP32 / ESP8266.
// MIT License - Copyright (c) 2026 Jour Tempo (https://jourtempo.fr)
#include "JourTempo.h"

#include <time.h>

#include "JourTempoJson.h"

#if defined(ESP32)
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#elif defined(ESP8266)
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#else
#error "JourTempo supports ESP32 and ESP8266 boards only."
#endif

#define JOURTEMPO_VERSION "1.0.0"

const char* const JourTempo::URL = "https://jourtempo.fr/api/v1/tempo.json";
const char* const JourTempo::ATTRIBUTION = "Couleurs Tempo : RTE, via Jour Tempo (jourtempo.fr)";

namespace {
const uint32_t MIN_INTERVAL_MS = JourTempo::MIN_INTERVAL_MINUTES * 60UL * 1000UL;
const int64_t CLOCK_SET_AFTER = 1700000000;  // 2023-11-14: below this the system clock is not set
}  // namespace

void JourTempo::begin(const char* rootCA) {
  _rootCA = rootCA;
#if defined(ESP8266)
  if (_rootCA != nullptr && _trustAnchors == nullptr) _trustAnchors = new BearSSL::X509List(_rootCA);
#endif
}

void JourTempo::setUpdateInterval(uint32_t minutes) {
  if (minutes < MIN_INTERVAL_MINUTES) minutes = MIN_INTERVAL_MINUTES;
  if (minutes > 24UL * 60UL) minutes = 24UL * 60UL;
  _intervalMs = minutes * 60UL * 1000UL;
}

uint64_t JourTempo::uptimeMs() {
  uint32_t m = millis();
  if (m < _lastMillis) _millisHigh++;  // millis() wrapped (every ~49.7 days)
  _lastMillis = m;
  return ((uint64_t)_millisHigh << 32) | m;
}

int64_t JourTempo::now() {
  uint64_t up = uptimeMs();
  time_t sys = time(nullptr);
  if ((int64_t)sys > CLOCK_SET_AFTER) return (int64_t)sys;
  if (_serverTime > 0) return _serverTime + (int64_t)((up - _serverTimeMs) / 1000ULL);
  return 0;
}

bool JourTempo::update() {
  uint64_t up = uptimeMs();
  if (_attempted && up < _nextAttemptMs) return false;  // too early: lastResult() keeps the last attempt
  if (WiFi.status() != WL_CONNECTED) {
    _lastResult = Result::NoWifi;  // nothing sent: no waiting needed
    return false;
  }
  _attempted = true;
  _lastResult = fetch();
  up = uptimeMs();
  if (_lastResult == Result::Ok) {
    _nextAttemptMs = up + _intervalMs;
    return true;
  }
  _nextAttemptMs = up + MIN_INTERVAL_MS;  // retry later, never sooner than 10 minutes
  return false;
}

JourTempo::Result JourTempo::fetch() {
  WiFiClientSecure client;
#if defined(ESP32)
  if (_rootCA != nullptr) client.setCACert(_rootCA);
  else client.setInsecure();
#else
  if (_trustAnchors != nullptr) client.setTrustAnchors(static_cast<BearSSL::X509List*>(_trustAnchors));
  else client.setInsecure();
#endif

  HTTPClient http;
  http.useHTTP10(true);  // plain body (no chunks): ArduinoJson reads it straight from the stream
  http.setTimeout(10000);
#if defined(ESP32)
  http.setConnectTimeout(10000);
#endif
  http.setUserAgent("JourTempo-Arduino/" JOURTEMPO_VERSION);
  const char* headers[] = {"Date"};
  http.collectHeaders(headers, 1);

  if (!http.begin(client, URL)) return Result::ConnectionFailed;
  int code = http.GET();
  _lastHttpCode = code;
  if (code <= 0) {
    http.end();
    return Result::ConnectionFailed;
  }
  if (code != 200) {
    http.end();
    return Result::HttpError;
  }

  int64_t serverTime = 0;
  bool haveServerTime = jourtempo::parseHttpDate(http.header("Date").c_str(), serverTime);

  JsonDocument filter;
  jourtempo::buildFilter(filter);
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) return Result::ParseError;

  jourtempo::Snapshot fresh;
  if (!jourtempo::readDocument(doc.as<JsonVariantConst>(), fresh)) return Result::InvalidData;

  _snapshot = fresh;
  _lastSuccessMs = uptimeMs();
  if (haveServerTime) {
    _serverTime = serverTime;
    _serverTimeMs = _lastSuccessMs;
  }
  return Result::Ok;
}

JourTempo::Color JourTempo::today() { return jourtempo::resolveToday(_snapshot, now()); }

JourTempo::Color JourTempo::tomorrow() { return jourtempo::resolveTomorrow(_snapshot, now()); }

int JourTempo::remainingDays(Color color) { return jourtempo::resolveRemaining(_snapshot, color, now()); }

bool JourTempo::isExpired() { return jourtempo::isExpired(_snapshot, now()); }

long JourTempo::secondsSinceUpdate() {
  if (!_snapshot.valid) return -1;
  return (long)((uptimeMs() - _lastSuccessMs) / 1000ULL);
}

const char* JourTempo::resultName(Result result) {
  switch (result) {
    case Result::None: return "none";
    case Result::Ok: return "ok";
    case Result::NoWifi: return "no Wi-Fi";
    case Result::ConnectionFailed: return "connection failed";
    case Result::HttpError: return "HTTP error";
    case Result::ParseError: return "invalid JSON";
    case Result::InvalidData: return "invalid data";
  }
  return "?";
}
