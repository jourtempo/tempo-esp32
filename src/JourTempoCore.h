// JourTempo - core logic, plain C++ (no Arduino, no network).
// Dates, Paris time, Tempo day and expiry rules used by the JourTempo class.
// MIT License - Copyright (c) 2026 Jour Tempo (https://jourtempo.fr)
#pragma once

#include <stdint.h>
#include <stddef.h>

namespace jourtempo {

// Tempo colour of a day. Unknown = not announced yet, no data, or data expired.
enum class Color : uint8_t { Unknown = 0, Blue = 1, White = 2, Red = 3 };

// Data kept from one successful read of tempo.json.
struct Snapshot {
  bool valid = false;
  Color today = Color::Unknown;
  Color tomorrow = Color::Unknown;
  int32_t todayDay = 0;       // days since 1970-01-01 of aujourdhui.date
  int32_t tomorrowDay = 0;    // days since 1970-01-01 of demain.date
  int64_t expiresUtc = 0;     // perime_apres, Unix time (UTC)
  int16_t remaining[4] = {-1, -1, -1, -1};  // indexed by Color (Blue, White, Red)
  char todayDate[11] = "";    // "YYYY-MM-DD"
  char tomorrowDate[11] = "";
  char expiresAt[26] = "";    // perime_apres as published (ISO 8601, Paris time)
  char collectedAt[26] = "";  // collecte_le as published
};

// "bleu" / "blanc" / "rouge" -> Color ; anything else (including "inconnu") -> Unknown.
Color colorFromCode(const char* code);
// Color -> "bleu" / "blanc" / "rouge" / "inconnu" (codes used by the API).
const char* colorCode(Color c);
// Color -> "blue" / "white" / "red" / "unknown".
const char* colorName(Color c);

// Days since 1970-01-01 for a civil date (proleptic Gregorian).
int32_t daysFromCivil(int32_t y, uint32_t m, uint32_t d);
// "YYYY-MM-DD" -> days since 1970-01-01. Returns false if malformed.
bool parseDate(const char* s, int32_t& days);
// "YYYY-MM-DDTHH:MM:SS(.fff)(Z|+HH:MM|-HH:MM)" -> Unix time. Returns false if malformed.
bool parseIso8601(const char* s, int64_t& utc);
// HTTP "Date" header, e.g. "Fri, 16 Oct 2026 15:12:00 GMT" -> Unix time. Returns false if malformed.
bool parseHttpDate(const char* s, int64_t& utc);

// Offset of Europe/Paris from UTC at a given instant, in seconds (3600 or 7200).
// EU rule: summer time from the last Sunday of March 01:00 UTC to the last Sunday of October 01:00 UTC.
int32_t parisOffset(int64_t utc);
// Tempo day (days since 1970-01-01) at a given instant: a Tempo day runs from 06:00 to 06:00, Paris time.
int32_t tempoDay(int64_t utc);

// True when the snapshot can no longer be trusted at time nowUtc (no data, unknown clock, or past perime_apres).
bool isExpired(const Snapshot& s, int64_t nowUtc);
// Colour of the current Tempo day at nowUtc, or Unknown. Never guessed: it only comes from the snapshot.
Color resolveToday(const Snapshot& s, int64_t nowUtc);
// Colour of the next Tempo day at nowUtc, or Unknown.
Color resolveTomorrow(const Snapshot& s, int64_t nowUtc);
// Remaining days of a colour in the current season, or -1 if unknown / expired.
int resolveRemaining(const Snapshot& s, Color c, int64_t nowUtc);

}  // namespace jourtempo
