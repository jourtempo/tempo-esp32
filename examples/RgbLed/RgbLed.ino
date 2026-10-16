// JourTempo example: an RGB LED shows tomorrow's Tempo colour (blue, white, red).
// While the colour is unknown (not announced yet, no network, expired data) the LED gives a short white flash
// every 2 seconds instead of a colour, so a guessed colour is never shown.
//
// Wiring (common cathode RGB LED, one 220 ohm resistor per colour):
//   red -> PIN_RED, green -> PIN_GREEN, blue -> PIN_BLUE, common -> GND.
//   For a common anode LED (common -> 3.3 V), set COMMON_ANODE to true.
// Data: RTE, via Jour Tempo (https://jourtempo.fr/api/).
#if defined(ESP32)
#include <WiFi.h>
const int PIN_RED = 25, PIN_GREEN = 26, PIN_BLUE = 27;
#else
#include <ESP8266WiFi.h>
const int PIN_RED = 14, PIN_GREEN = 12, PIN_BLUE = 13;  // GPIO14/12/13 = D5/D6/D7 on NodeMCU and Wemos D1 mini
#endif
#include <JourTempo.h>

const char* WIFI_SSID = "MyWifi";
const char* WIFI_PASSWORD = "MyPassword";
const bool COMMON_ANODE = false;
const bool SHOW_TOMORROW = true;  // false: show today's colour instead

JourTempo tempo;

void setLed(bool r, bool g, bool b) {
  digitalWrite(PIN_RED, (r != COMMON_ANODE) ? HIGH : LOW);
  digitalWrite(PIN_GREEN, (g != COMMON_ANODE) ? HIGH : LOW);
  digitalWrite(PIN_BLUE, (b != COMMON_ANODE) ? HIGH : LOW);
}

void showColor(JourTempo::Color c) {
  switch (c) {
    case JourTempo::Color::Blue: setLed(false, false, true); break;
    case JourTempo::Color::White: setLed(true, true, true); break;
    case JourTempo::Color::Red: setLed(true, false, false); break;
    default: {
      // Unknown: short white flash every 2 seconds.
      bool flash = (millis() % 2000) < 100;
      setLed(flash, flash, flash);
    }
  }
}

void setup() {
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  setLed(false, false, false);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // update() waits for the connection by itself
  configTime(0, 0, "pool.ntp.org");      // optional
  tempo.begin();
}

void loop() {
  tempo.update();  // sends a request only every 30 minutes
  showColor(SHOW_TOMORROW ? tempo.tomorrow() : tempo.today());
  delay(50);
}
