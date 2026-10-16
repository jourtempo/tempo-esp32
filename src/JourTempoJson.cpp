// JourTempo - reading tempo.json with ArduinoJson 7.
// MIT License - Copyright (c) 2026 Jour Tempo (https://jourtempo.fr)
#include "JourTempoJson.h"

#include <string.h>

namespace jourtempo {

namespace {

void copyText(char* dst, size_t size, const char* src) {
  if (src == nullptr) src = "";
  strncpy(dst, src, size - 1);
  dst[size - 1] = '\0';
}

int16_t readCount(JsonVariantConst v) {
  if (!v.is<int>()) return -1;
  int n = v.as<int>();
  return (n < 0 || n > 366) ? -1 : (int16_t)n;
}

}  // namespace

void buildFilter(JsonDocument& filter) {
  filter.clear();
  filter["perime_apres"] = true;
  filter["collecte_le"] = true;
  filter["aujourdhui"]["date"] = true;
  filter["aujourdhui"]["code"] = true;
  filter["demain"]["date"] = true;
  filter["demain"]["code"] = true;
  filter["saison"]["compteurs"]["bleu"]["restants"] = true;
  filter["saison"]["compteurs"]["blanc"]["restants"] = true;
  filter["saison"]["compteurs"]["rouge"]["restants"] = true;
}

bool readDocument(JsonVariantConst root, Snapshot& out) {
  Snapshot s;
  const char* todayDate = root["aujourdhui"]["date"];
  const char* tomorrowDate = root["demain"]["date"];
  const char* expires = root["perime_apres"];
  const char* todayCode = root["aujourdhui"]["code"];
  const char* tomorrowCode = root["demain"]["code"];

  out = Snapshot();  // invalid until everything checks out
  if (!parseDate(todayDate, s.todayDay)) return false;
  if (!parseDate(tomorrowDate, s.tomorrowDay)) return false;
  if (s.tomorrowDay != s.todayDay + 1) return false;
  if (!parseIso8601(expires, s.expiresUtc)) return false;
  s.today = colorFromCode(todayCode);
  if (s.today == Color::Unknown) return false;  // today is always published: anything else is an error
  s.tomorrow = colorFromCode(tomorrowCode);     // "inconnu" before RTE's announcement

  JsonVariantConst counters = root["saison"]["compteurs"];
  s.remaining[(uint8_t)Color::Blue] = readCount(counters["bleu"]["restants"]);
  s.remaining[(uint8_t)Color::White] = readCount(counters["blanc"]["restants"]);
  s.remaining[(uint8_t)Color::Red] = readCount(counters["rouge"]["restants"]);

  copyText(s.todayDate, sizeof(s.todayDate), todayDate);
  copyText(s.tomorrowDate, sizeof(s.tomorrowDate), tomorrowDate);
  copyText(s.expiresAt, sizeof(s.expiresAt), expires);
  copyText(s.collectedAt, sizeof(s.collectedAt), root["collecte_le"].as<const char*>());
  s.valid = true;
  out = s;
  return true;
}

bool parseTempoJson(const char* json, size_t length, Snapshot& out) {
  JsonDocument filter;
  buildFilter(filter);
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json, length, DeserializationOption::Filter(filter));
  if (err) {
    out = Snapshot();
    return false;
  }
  return readDocument(doc.as<JsonVariantConst>(), out);
}

}  // namespace jourtempo
