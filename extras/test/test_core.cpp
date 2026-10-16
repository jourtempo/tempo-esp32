// Host test of the JourTempo logic (no board, no network).
//
// Core only:
//   g++ -std=c++17 -Wall -Wextra -DJOURTEMPO_NO_JSON -I../../src test_core.cpp ../../src/JourTempoCore.cpp -o test_core
// With JSON parsing (ArduinoJson 7 sources in ARDUINOJSON_DIR):
//   g++ -std=c++17 -Wall -Wextra -I../../src -I$ARDUINOJSON_DIR/src test_core.cpp ../../src/JourTempoCore.cpp \
//       ../../src/JourTempoJson.cpp -o test_core
// Run: ./test_core tempo-sample.json
#include <stdio.h>
#include <string.h>

#include <string>

#include "JourTempoCore.h"
#ifndef JOURTEMPO_NO_JSON
#include "JourTempoJson.h"
#endif

using namespace jourtempo;

static int failures = 0;
#define CHECK(cond)                                         \
  do {                                                      \
    if (!(cond)) {                                          \
      printf("FAIL line %d: %s\n", __LINE__, #cond);        \
      failures++;                                           \
    }                                                       \
  } while (0)

static int64_t iso(const char* s) {
  int64_t t = 0;
  bool ok = parseIso8601(s, t);
  CHECK(ok);
  return t;
}

static void testDates() {
  int32_t d = 0;
  CHECK(parseDate("1970-01-01", d) && d == 0);
  CHECK(parseDate("2026-10-16", d) && d == 20742);
  CHECK(!parseDate("2026-02-29", d));
  CHECK(parseDate("2028-02-29", d));
  CHECK(!parseDate("2026-13-01", d));
  CHECK(!parseDate("26-10-16", d));
  CHECK(!parseDate(nullptr, d));

  CHECK(iso("2026-10-17T12:00:00+02:00") == 1792231200);
  CHECK(iso("2026-10-16T15:12:00Z") == 1792163520);
  CHECK(iso("2026-10-16T15:12:00.250+00:00") == 1792163520);
  CHECK(iso("2026-12-31T23:30:00-01:30") == 1798765200);
  int64_t t = 0;
  CHECK(!parseIso8601("2026-10-17T12:00:00", t));        // no time zone: refused
  CHECK(!parseIso8601("2026-10-17T25:00:00+02:00", t));
  CHECK(!parseIso8601("2026-10-17T12:00:00+02:00x", t));

  CHECK(parseHttpDate("Fri, 16 Oct 2026 15:12:00 GMT", t) && t == 1792163520);
  CHECK(!parseHttpDate("Fri, 16 Foo 2026 15:12:00 GMT", t));
  CHECK(!parseHttpDate("", t));
}

static void testParisTime() {
  CHECK(parisOffset(1774746000 - 1) == 3600);  // 2026-03-29 00:59:59 UTC
  CHECK(parisOffset(1774746000) == 7200);      // 2026-03-29 01:00:00 UTC: summer time
  CHECK(parisOffset(1792890000 - 1) == 7200);  // 2026-10-25 00:59:59 UTC
  CHECK(parisOffset(1792890000) == 3600);      // 2026-10-25 01:00:00 UTC: winter time
  CHECK(tempoDay(iso("2026-10-17T05:59:59+02:00")) == 20742);  // still the Tempo day of 16 October
  CHECK(tempoDay(iso("2026-10-17T06:00:00+02:00")) == 20743);
  CHECK(tempoDay(iso("2027-01-01T05:00:00+01:00")) == tempoDay(iso("2026-12-31T12:00:00+01:00")));
}

static Snapshot sample() {
  Snapshot s;
  s.valid = true;
  s.today = Color::White;
  s.tomorrow = Color::Red;
  parseDate("2026-10-16", s.todayDay);
  parseDate("2026-10-17", s.tomorrowDay);
  s.expiresUtc = iso("2026-10-17T12:00:00+02:00");
  s.remaining[(int)Color::Blue] = 254;
  s.remaining[(int)Color::White] = 42;
  s.remaining[(int)Color::Red] = 21;
  return s;
}

static void testResolve() {
  Snapshot s = sample();
  int64_t evening = iso("2026-10-16T20:00:00+02:00");
  CHECK(resolveToday(s, evening) == Color::White);
  CHECK(resolveTomorrow(s, evening) == Color::Red);
  CHECK(resolveRemaining(s, Color::Red, evening) == 21);

  int64_t night = iso("2026-10-17T03:00:00+02:00");  // before 06:00: same Tempo day
  CHECK(resolveToday(s, night) == Color::White);
  CHECK(resolveTomorrow(s, night) == Color::Red);

  int64_t morning = iso("2026-10-17T07:00:00+02:00");  // the announced "tomorrow" is now today
  CHECK(resolveToday(s, morning) == Color::Red);
  CHECK(resolveTomorrow(s, morning) == Color::Unknown);

  int64_t expired = iso("2026-10-17T12:00:00+02:00");
  CHECK(isExpired(s, expired));
  CHECK(resolveToday(s, expired) == Color::Unknown);
  CHECK(resolveTomorrow(s, expired) == Color::Unknown);
  CHECK(resolveRemaining(s, Color::Red, expired) == -1);

  CHECK(resolveToday(s, 0) == Color::Unknown);  // clock unknown
  CHECK(resolveToday(s, iso("2026-10-15T20:00:00+02:00")) == Color::Unknown);  // clock before the data

  Snapshot none;
  CHECK(isExpired(none, evening));
  CHECK(resolveToday(none, evening) == Color::Unknown);

  CHECK(colorFromCode("rouge") == Color::Red && colorFromCode("inconnu") == Color::Unknown);
  CHECK(strcmp(colorCode(Color::White), "blanc") == 0 && strcmp(colorName(Color::Blue), "blue") == 0);
}

#ifndef JOURTEMPO_NO_JSON
static void testJson(const char* path) {
  const char* inline_json =
      "{\"perime_apres\":\"2026-12-04T12:00:00+01:00\",\"collecte_le\":\"2026-12-03T11:02:41+01:00\","
      "\"aujourdhui\":{\"date\":\"2026-12-03\",\"code\":\"blanc\"},\"demain\":{\"date\":\"2026-12-04\",\"code\":"
      "\"inconnu\"},\"saison\":{\"compteurs\":{\"bleu\":{\"restants\":210},\"blanc\":{\"restants\":40},"
      "\"rouge\":{\"restants\":20}}},\"calendrier\":{\"2026-09-01\":\"bleu\"}}";
  Snapshot s;
  CHECK(parseTempoJson(inline_json, strlen(inline_json), s));
  CHECK(s.valid && s.today == Color::White && s.tomorrow == Color::Unknown);
  CHECK(s.remaining[(int)Color::Red] == 20 && strcmp(s.expiresAt, "2026-12-04T12:00:00+01:00") == 0);
  CHECK(resolveTomorrow(s, iso("2026-12-03T09:00:00+01:00")) == Color::Unknown);

  const char* broken = "{\"aujourdhui\":{\"date\":\"2026-12-03\",\"code\":\"blanc\"}";
  CHECK(!parseTempoJson(broken, strlen(broken), s) && !s.valid);
  const char* missing = "{\"aujourdhui\":{\"date\":\"2026-12-03\",\"code\":\"blanc\"},\"demain\":{\"date\":\"2026-12-04\"}}";
  CHECK(!parseTempoJson(missing, strlen(missing), s) && !s.valid);  // no perime_apres
  const char* badToday = "{\"perime_apres\":\"2026-12-04T12:00:00+01:00\",\"aujourdhui\":{\"date\":\"2026-12-03\","
                         "\"code\":\"vert\"},\"demain\":{\"date\":\"2026-12-04\",\"code\":\"rouge\"}}";
  CHECK(!parseTempoJson(badToday, strlen(badToday), s));

  if (path == nullptr) return;
  FILE* f = fopen(path, "rb");
  CHECK(f != nullptr);
  if (f == nullptr) return;
  std::string text;
  char buf[1024];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) text.append(buf, n);
  fclose(f);
  CHECK(parseTempoJson(text.data(), text.size(), s));
  CHECK(s.valid && s.today != Color::Unknown && s.tomorrowDay == s.todayDay + 1);
  printf("sample: today %s %s, tomorrow %s %s, expires %s, red left %d\n", s.todayDate, colorCode(s.today),
         s.tomorrowDate, colorCode(s.tomorrow), s.expiresAt, s.remaining[(int)Color::Red]);
}
#endif

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  testDates();
  testParisTime();
  testResolve();
#ifndef JOURTEMPO_NO_JSON
  testJson(argc > 1 ? argv[1] : nullptr);
#endif
  if (failures == 0) printf("All tests passed\n");
  else printf("%d test(s) failed\n", failures);
  return failures == 0 ? 0 : 1;
}
