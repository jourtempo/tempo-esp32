// JourTempo - French Tempo electricity tariff colour (blue, white, red) on ESP32 / ESP8266.
// Reads the free, keyless JSON published by Jour Tempo (https://jourtempo.fr/api/), colours published by RTE.
// MIT License - Copyright (c) 2026 Jour Tempo (https://jourtempo.fr)
#pragma once

#include <Arduino.h>

#include "JourTempoCore.h"

class JourTempo {
 public:
  using Color = jourtempo::Color;

  // Result of the last request attempt (calls skipped because the interval is not over do not change it).
  enum class Result : uint8_t {
    None = 0,          // update() never ran
    Ok,                // data read and checked
    NoWifi,            // Wi-Fi not connected, nothing sent
    ConnectionFailed,  // DNS, TLS or TCP error
    HttpError,         // server answered with a status other than 200 (see lastHttpCode())
    ParseError,        // the body is not valid JSON
    InvalidData        // JSON is valid but a required field is missing or malformed
  };

  static const char* const URL;           // https://jourtempo.fr/api/v1/tempo.json
  static const char* const ATTRIBUTION;   // credit to display next to the colours
  static constexpr uint32_t MIN_INTERVAL_MINUTES = 10;
  static constexpr uint32_t DEFAULT_INTERVAL_MINUTES = 30;

  // rootCA: optional PEM certificate to verify the server. nullptr = encrypted but not verified.
  // On ESP8266 a certificate check also needs the clock to be set (NTP) beforehand.
  void begin(const char* rootCA = nullptr);

  // Time between two successful requests, in minutes. Values under 10 are raised to 10.
  void setUpdateInterval(uint32_t minutes);

  // Call it often (in loop()): it only sends a request when the interval is over.
  // Returns true when new data was read. On failure the previous data is kept until it expires,
  // and the next try waits at least 10 minutes.
  bool update();

  // Colours of the current and next Tempo day (a Tempo day runs 06:00 to 06:00, Paris time).
  // Unknown when tomorrow is not announced yet, when no data was ever read, or when the data has expired.
  Color today();
  Color tomorrow();
  // Remaining days of a colour in the current season, -1 if unknown.
  int remainingDays(Color color);

  bool hasData() const { return _snapshot.valid; }
  // True when there is no usable data: never read, or past its expiry date (perime_apres).
  bool isExpired();
  const char* todayDate() const { return _snapshot.todayDate; }        // "YYYY-MM-DD" of the last read
  const char* tomorrowDate() const { return _snapshot.tomorrowDate; }
  const char* expiresAt() const { return _snapshot.expiresAt; }        // perime_apres, ISO 8601
  const char* collectedAt() const { return _snapshot.collectedAt; }    // collecte_le, ISO 8601

  Result lastResult() const { return _lastResult; }
  int lastHttpCode() const { return _lastHttpCode; }
  // Seconds since the last successful read, or -1.
  long secondsSinceUpdate();
  // Current time used for the checks (system clock if set by NTP, otherwise HTTP Date of the last answer), 0 if unknown.
  int64_t now();

  static const char* colorName(Color color) { return jourtempo::colorName(color); }  // "blue", "white", "red", "unknown"
  static const char* colorCode(Color color) { return jourtempo::colorCode(color); }  // "bleu", "blanc", "rouge", "inconnu"
  static const char* resultName(Result result);

 private:
  uint64_t uptimeMs();
  Result fetch();

  jourtempo::Snapshot _snapshot;
  const char* _rootCA = nullptr;
  void* _trustAnchors = nullptr;  // ESP8266 only (BearSSL::X509List)
  uint32_t _intervalMs = DEFAULT_INTERVAL_MINUTES * 60UL * 1000UL;
  uint64_t _nextAttemptMs = 0;
  bool _attempted = false;
  uint64_t _lastSuccessMs = 0;
  int64_t _serverTime = 0;      // HTTP Date of the last successful answer
  uint64_t _serverTimeMs = 0;   // uptime at that moment
  uint32_t _lastMillis = 0;
  uint32_t _millisHigh = 0;
  Result _lastResult = Result::None;
  int _lastHttpCode = 0;
};
