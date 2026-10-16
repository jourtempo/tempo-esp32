// JourTempo example: print the Tempo colour of today and tomorrow on the serial monitor (115200 baud).
// Data: RTE, via Jour Tempo (https://jourtempo.fr/api/). One request every 30 minutes, never less than 10.
#if defined(ESP32)
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <JourTempo.h>

const char* WIFI_SSID = "MyWifi";
const char* WIFI_PASSWORD = "MyPassword";

JourTempo tempo;
unsigned long lastPrint = 0;

void printStatus() {
  if (tempo.isExpired()) {
    // No data yet, or data past its expiry date: never show a guessed colour.
    Serial.printf("Tempo: unknown (last result: %s)\n", JourTempo::resultName(tempo.lastResult()));
    return;
  }
  Serial.printf("Tempo today: %s, tomorrow: %s\n",
                JourTempo::colorName(tempo.today()), JourTempo::colorName(tempo.tomorrow()));
  Serial.printf("Remaining this season: %d blue, %d white, %d red\n",
                tempo.remainingDays(JourTempo::Color::Blue), tempo.remainingDays(JourTempo::Color::White),
                tempo.remainingDays(JourTempo::Color::Red));
  Serial.printf("Data valid until %s (read %ld s ago)\n", tempo.expiresAt(), tempo.secondsSinceUpdate());
  Serial.println(JourTempo::ATTRIBUTION);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println(" connected");

  // Optional: set the clock by NTP. Without it, the library uses the server's Date header.
  configTime(0, 0, "pool.ntp.org");

  tempo.begin();                 // or tempo.begin(rootCA) to check the server certificate
  tempo.setUpdateInterval(30);   // minutes (10 at least)
}

void loop() {
  if (tempo.update()) {          // true only when new data was read
    printStatus();
    lastPrint = millis();
  } else if (millis() - lastPrint > 60UL * 60UL * 1000UL) {
    printStatus();               // once an hour, even without new data
    lastPrint = millis();
  }
  delay(1000);
}
