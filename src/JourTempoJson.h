// JourTempo - reading tempo.json with ArduinoJson 7 (works on a board and on a computer).
// MIT License - Copyright (c) 2026 Jour Tempo (https://jourtempo.fr)
#pragma once

#include <ArduinoJson.h>

#include "JourTempoCore.h"

namespace jourtempo {

// Keeps only the fields the library needs: small memory use, even when the season calendar grows.
void buildFilter(JsonDocument& filter);
// Fills `out` from a parsed tempo.json document. Returns false (and leaves `out` invalid)
// if a required field is missing or malformed: dates, perime_apres, aujourdhui.code.
bool readDocument(JsonVariantConst root, Snapshot& out);
// Parses a tempo.json text (with the same filter). Returns false on invalid JSON or data.
bool parseTempoJson(const char* json, size_t length, Snapshot& out);

}  // namespace jourtempo
