# JourTempo

Arduino library for **ESP32** and **ESP8266** that reads the colour of the French **Tempo** electricity tariff
(blue, white or red) for today and tomorrow, in a few lines, from the free JSON API of
[Jour Tempo](https://jourtempo.fr/api/): no key, no account. Colours are published by RTE, the French transmission
system operator.

*Version française plus bas.*

```cpp
#include <JourTempo.h>
JourTempo tempo;

void setup() { /* connect Wi-Fi */ tempo.begin(); }

void loop() {
  tempo.update();                                         // one request every 30 minutes at most
  if (tempo.tomorrow() == JourTempo::Color::Red) { /* delay the water heater, dim the heating... */ }
}
```

## What it does

- `today()`, `tomorrow()`: `Blue`, `White`, `Red` or `Unknown`.
- `remainingDays(color)`: days of that colour left in the current season.
- `expiresAt()`, `isExpired()`: the data carries its own expiry date (`perime_apres`). Past it, every colour is `Unknown`.
- **Never a guessed colour.** `Unknown` is returned when tomorrow is not announced yet (RTE announces it around
  10:30, Paris time), when no data could be read, or when the data has expired.
- **Network loss**: a failed request keeps the previous data until it expires, then the colours become `Unknown`.
- **Tempo day**: a Tempo day runs from 06:00 to 06:00 (Paris time), as billed. Between 06:00 and the next read, the
  colour announced as "tomorrow" is returned as today's.
- **Polite by design**: one request every 30 minutes by default, **never less than 10 minutes** between two requests
  (also after an error). Calling `update()` in every `loop()` is fine.
- **Clock**: uses the system clock when set (NTP), otherwise the `Date` header of the last answer plus the board
  uptime. No NTP setup is required.

## Installation

- **Arduino IDE**: Library Manager, search for `JourTempo` (it also installs **ArduinoJson 7**).
- **PlatformIO**: `lib_deps = jourtempo/JourTempo`.
- Boards: ESP32 (Arduino core 2.x or 3.x) and ESP8266 (Arduino core 3.x).

## API

| Call | Returns |
| --- | --- |
| `begin(rootCA = nullptr)` | starts the client. With a PEM root certificate the server is verified; without, the link is encrypted but not verified. On ESP8266 the certificate check needs the clock set by NTP. |
| `setUpdateInterval(minutes)` | time between two successful requests (default 30, minimum 10). |
| `update()` | `true` when new data was read. Sends a request only when the interval is over and Wi-Fi is connected. |
| `today()`, `tomorrow()` | `JourTempo::Color` |
| `remainingDays(color)` | remaining days in the season, `-1` if unknown |
| `hasData()`, `isExpired()` | data state |
| `todayDate()`, `tomorrowDate()`, `expiresAt()`, `collectedAt()` | dates as published (`YYYY-MM-DD`, ISO 8601) |
| `lastResult()`, `lastHttpCode()`, `resultName(r)` | `None`, `Ok`, `NoWifi`, `ConnectionFailed`, `HttpError`, `ParseError`, `InvalidData` |
| `secondsSinceUpdate()`, `now()` | seconds since the last successful read; current Unix time used for the checks (0 if unknown) |
| `colorName(c)`, `colorCode(c)` | `"red"`, or the API code `"rouge"` |
| `JourTempo::ATTRIBUTION` | credit line to display next to the colours |

## Examples

- `SerialMonitor`: prints today's and tomorrow's colour, remaining days and expiry date.
- `RgbLed`: an RGB LED takes tomorrow's colour; a short white flash means "unknown".

## Terms of use of the data

- Free, for personal and commercial use. One request every 30 minutes is enough; **never more than one request every
  10 minutes per device**.
- If you display the colours publicly, credit the source: **"Couleurs Tempo : RTE, via Jour Tempo"** with a link to
  https://jourtempo.fr (text in `JourTempo::ATTRIBUTION`).
- Jour Tempo is an independent website, neither EDF nor RTE. Do not use their logos next to this data.
- To drive an expensive appliance, plan a default behaviour for `Unknown`.
- Full terms and field list: https://jourtempo.fr/api/

## Tests

`extras/test/test_core.cpp` checks the date, Paris time, Tempo day and expiry logic on a computer (no board needed).
Build commands are at the top of the file.

## Licence

Code: [MIT](LICENSE). Data: Tempo colours published by RTE (reuse allowed with the source credited, terms of use of
data.rte-france.com, article 9); Jour Tempo files under Licence Ouverte 2.0 (Etalab).

Questions, bugs: open an issue or write to contact@jourtempo.fr.

---

## En français

**JourTempo** lit la couleur Tempo du jour et du lendemain (bleu, blanc, rouge) sur un ESP32 ou un ESP8266, à partir de
l'API gratuite de [Jour Tempo](https://jourtempo.fr/api/), sans clé ni compte. Les couleurs sont publiées par RTE.

- `today()` et `tomorrow()` renvoient `Blue`, `White`, `Red` ou `Unknown` ; `remainingDays(couleur)` donne les jours
  restants de la saison.
- **Jamais de couleur devinée** : `Unknown` tant que la couleur de demain n'est pas annoncée (vers 10 h 30), sans
  donnée lue, ou quand la date `perime_apres` est dépassée.
- **Coupure réseau** : la dernière donnée reste utilisée jusqu'à sa péremption, puis tout passe à `Unknown`.
- **Jour Tempo** de 6 h à 6 h (heure de Paris), comme la facturation.
- **Bon usage** : une requête toutes les 30 minutes par défaut, **jamais moins de 10 minutes** entre deux requêtes,
  même après une erreur. Appeler `update()` dans chaque `loop()` ne pose pas de problème.
- Installation : gestionnaire de bibliothèques de l'IDE Arduino (recherche `JourTempo`, installe aussi ArduinoJson 7),
  ou `lib_deps = jourtempo/JourTempo` sous PlatformIO.
- Exemples : `SerialMonitor` (moniteur série) et `RgbLed` (LED RVB qui prend la couleur de demain ; un bref flash blanc
  signifie « inconnu »).
- Crédit à afficher si les couleurs sont montrées publiquement : **« Couleurs Tempo : RTE, via Jour Tempo »**, avec un
  lien vers https://jourtempo.fr.
- Jour Tempo est un site indépendant, ni EDF ni RTE. Conditions complètes : https://jourtempo.fr/api/

Code sous licence MIT. Une question, une erreur : une *issue* ou contact@jourtempo.fr.
