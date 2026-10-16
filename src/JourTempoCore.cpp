// JourTempo - core logic, plain C++ (no Arduino, no network).
// MIT License - Copyright (c) 2026 Jour Tempo (https://jourtempo.fr)
#include "JourTempoCore.h"

#include <string.h>

namespace jourtempo {

namespace {

// Reads exactly n digits at s; returns -1 if one of them is not a digit.
int32_t readDigits(const char* s, int n) {
  int32_t v = 0;
  for (int i = 0; i < n; i++) {
    char c = s[i];
    if (c < '0' || c > '9') return -1;
    v = v * 10 + (c - '0');
  }
  return v;
}

bool validDate(int32_t y, int32_t m, int32_t d) {
  static const uint8_t len[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (y < 1970 || y > 2200 || m < 1 || m > 12 || d < 1) return false;
  if (d > len[m - 1]) return false;
  if (m == 2 && d == 29) {
    bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    if (!leap) return false;
  }
  return true;
}

// Floor division / modulo for negative values.
int64_t floorDiv(int64_t a, int64_t b) { return (a >= 0) ? a / b : -((-a + b - 1) / b); }

// Unix time of the last Sunday of a month at 01:00 UTC (EU summer time switch).
int64_t lastSunday0100(int32_t year, uint32_t month, uint32_t lastDay) {
  int32_t days = daysFromCivil(year, month, lastDay);
  int32_t weekday = (int32_t)((days % 7 + 7 + 4) % 7);  // 0 = Sunday ; 1970-01-01 was a Thursday
  days -= weekday;
  return (int64_t)days * 86400 + 3600;
}

}  // namespace

Color colorFromCode(const char* code) {
  if (code == nullptr) return Color::Unknown;
  if (strcmp(code, "bleu") == 0) return Color::Blue;
  if (strcmp(code, "blanc") == 0) return Color::White;
  if (strcmp(code, "rouge") == 0) return Color::Red;
  return Color::Unknown;
}

const char* colorCode(Color c) {
  switch (c) {
    case Color::Blue: return "bleu";
    case Color::White: return "blanc";
    case Color::Red: return "rouge";
    default: return "inconnu";
  }
}

const char* colorName(Color c) {
  switch (c) {
    case Color::Blue: return "blue";
    case Color::White: return "white";
    case Color::Red: return "red";
    default: return "unknown";
  }
}

int32_t daysFromCivil(int32_t y, uint32_t m, uint32_t d) {
  y -= (m <= 2) ? 1 : 0;
  const int32_t era = (y >= 0 ? y : y - 399) / 400;
  const uint32_t yoe = (uint32_t)(y - era * 400);
  const uint32_t doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
  const uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (int32_t)doe - 719468;
}

bool parseDate(const char* s, int32_t& days) {
  if (s == nullptr || strlen(s) < 10) return false;
  if (s[4] != '-' || s[7] != '-') return false;
  int32_t y = readDigits(s, 4), m = readDigits(s + 5, 2), d = readDigits(s + 8, 2);
  if (!validDate(y, m, d)) return false;
  days = daysFromCivil(y, (uint32_t)m, (uint32_t)d);
  return true;
}

bool parseIso8601(const char* s, int64_t& utc) {
  int32_t days;
  if (!parseDate(s, days)) return false;
  if (strlen(s) < 19 || (s[10] != 'T' && s[10] != ' ') || s[13] != ':' || s[16] != ':') return false;
  int32_t h = readDigits(s + 11, 2), mi = readDigits(s + 14, 2), se = readDigits(s + 17, 2);
  if (h < 0 || h > 23 || mi < 0 || mi > 59 || se < 0 || se > 60) return false;
  const char* p = s + 19;
  if (*p == '.') {  // fractional seconds: ignored
    p++;
    while (*p >= '0' && *p <= '9') p++;
  }
  int32_t offset = 0;
  if (*p == 'Z') {
    p++;
  } else if (*p == '+' || *p == '-') {
    int32_t oh = readDigits(p + 1, 2);
    if (oh < 0 || p[3] != ':') return false;
    int32_t om = readDigits(p + 4, 2);
    if (om < 0 || oh > 14 || om > 59) return false;
    offset = (oh * 3600 + om * 60) * (*p == '-' ? -1 : 1);
    p += 6;
  } else {
    return false;  // no time zone: refuse rather than guess
  }
  if (*p != '\0') return false;
  utc = (int64_t)days * 86400 + h * 3600 + mi * 60 + se - offset;
  return true;
}

bool parseHttpDate(const char* s, int64_t& utc) {
  // IMF-fixdate (RFC 9110): "Sun, 06 Nov 1994 08:49:37 GMT"
  static const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  if (s == nullptr || strlen(s) < 29) return false;
  if (s[3] != ',' || s[4] != ' ' || s[7] != ' ' || s[11] != ' ' || s[16] != ' ' || s[19] != ':' || s[22] != ':' ||
      strncmp(s + 25, " GMT", 4) != 0)
    return false;
  int32_t d = readDigits(s + 5, 2), y = readDigits(s + 12, 4);
  int32_t m = 0;
  for (int i = 0; i < 12; i++) {
    if (strncmp(s + 8, months + i * 3, 3) == 0) {
      m = i + 1;
      break;
    }
  }
  int32_t h = readDigits(s + 17, 2), mi = readDigits(s + 20, 2), se = readDigits(s + 23, 2);
  if (!validDate(y, m, d) || h < 0 || h > 23 || mi < 0 || mi > 59 || se < 0 || se > 60) return false;
  utc = (int64_t)daysFromCivil(y, (uint32_t)m, (uint32_t)d) * 86400 + h * 3600 + mi * 60 + se;
  return true;
}

int32_t parisOffset(int64_t utc) {
  int32_t days = (int32_t)floorDiv(utc, 86400);
  // Year of the UTC date (the switches happen in March and October, far from new year).
  int32_t y = 1970 + days / 365;
  while (daysFromCivil(y, 1, 1) > days) y--;
  while (daysFromCivil(y + 1, 1, 1) <= days) y++;
  int64_t start = lastSunday0100(y, 3, 31);
  int64_t end = lastSunday0100(y, 10, 31);
  return (utc >= start && utc < end) ? 7200 : 3600;
}

int32_t tempoDay(int64_t utc) {
  return (int32_t)floorDiv(utc + parisOffset(utc) - 6 * 3600, 86400);
}

bool isExpired(const Snapshot& s, int64_t nowUtc) {
  return !s.valid || nowUtc <= 0 || nowUtc >= s.expiresUtc;
}

Color resolveToday(const Snapshot& s, int64_t nowUtc) {
  if (isExpired(s, nowUtc)) return Color::Unknown;
  int32_t day = tempoDay(nowUtc);
  if (day == s.todayDay) return s.today;
  if (day == s.tomorrowDay) return s.tomorrow;  // past 06:00 the announced "tomorrow" is today
  return Color::Unknown;
}

Color resolveTomorrow(const Snapshot& s, int64_t nowUtc) {
  if (isExpired(s, nowUtc)) return Color::Unknown;
  if (tempoDay(nowUtc) == s.todayDay) return s.tomorrow;
  return Color::Unknown;  // the day after "tomorrow" is not in the snapshot
}

int resolveRemaining(const Snapshot& s, Color c, int64_t nowUtc) {
  if (c == Color::Unknown || isExpired(s, nowUtc)) return -1;
  return s.remaining[(uint8_t)c];
}

}  // namespace jourtempo
