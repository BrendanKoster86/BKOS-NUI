# BKOS App Handleiding
**Versie:** 1.0 — BKOS-NUI v0.0.260503.3+

Deze handleiding beschrijft hoe je een BKOS app schrijft, test en publiceert.  
BKOS apps zijn Lua 5.4 scripts die draaien op het ESP32-S3 boordcomputer scherm.

---

## Inhoudsopgave

1. [Wat zijn BKOS Apps?](#1-wat-zijn-bkos-apps)
2. [Snelstart — minimale app](#2-snelstart--minimale-app)
3. [Bestandsstructuur](#3-bestandsstructuur)
4. [Manifest (manifest.json)](#4-manifest-manifestjson)
5. [Lua API — volledig overzicht](#5-lua-api--volledig-overzicht)
   - [Scherm tekenen](#51-scherm-tekenen)
   - [Kleuren](#52-kleuren)
   - [IO-kanalen](#53-io-kanalen)
   - [Data-opslag](#54-data-opslag)
   - [Systeem](#55-systeem)
6. [App callbacks](#6-app-callbacks)
7. [Schermresolutie en schalen](#7-schermresolutie-en-schalen)
8. [Scherm-override](#8-scherm-override)
9. [Data-opslag sleutelconventies](#9-data-opslag-sleutelconventies)
10. [Publiceren naar de app store](#10-publiceren-naar-de-app-store)
11. [Lokale installatie (zonder app store)](#11-lokale-installatie-zonder-app-store)
12. [Richtlijnen en beperkingen](#12-richtlijnen-en-beperkingen)
13. [Volledige voorbeeldapp](#13-volledige-voorbeeldapp)
14. [API-referentie voor AI-systemen](#14-api-referentie-voor-ai-systemen)

---

## 1. Wat zijn BKOS Apps?

Een BKOS app is een Lua 5.4 script dat:
- Draait op het 800×480 aanraakscherm van een ESP32-S3 boordcomputer
- Toegang heeft tot IO-kanalen (lichten, schakelaars, sensoren) via naam of poortnummer
- Gedeelde data-opslag kan lezen/schrijven (weer, getij, systeemstatus)
- Optioneel een ingebouwd scherm (paneel, meteo, IO-lijst, etc.) kan vervangen
- Beschikbaar is via de BKOS App Store (GitHub) of handmatig geïnstalleerd

Apps zijn **sandboxed**: ze hebben geen toegang tot het bestandssysteem, netwerk of OS-functies. Alle hardware-interactie gaat via de `bkos.*` API.

Beschikbare Lua-bibliotheken: `math`, `string`, `table`, `coroutine` en de ingebouwde basisfuncties (`print`, `type`, `pcall`, `pairs`, `ipairs`, `tostring`, `tonumber`, etc.).

---

## 2. Snelstart — minimale app

De kleinste werkende BKOS app:

```lua
-- Mijn eerste BKOS app

function bkos.teken()
    bkos.vul(0, 0, bkos.W, bkos.H, bkos.kleur.bg)
    bkos.tekst(100, 200, "Hallo BKOS!", 3, bkos.kleur.cyaan)
end

function bkos.aanraking(x, y)
    bkos.tekst(x, y, "!", 2, bkos.kleur.groen)
end
```

---

## 3. Bestandsstructuur

Elke app bestaat uit twee bestanden:

```
appstore/apps/<app-id>/
    manifest.json    ← metadata (naam, versie, schermgrootte, etc.)
    main.lua         ← het Lua-script
```

**App-ID** mag alleen kleine letters, cijfers en underscores bevatten (`[a-z0-9_]`).  
Gebruik een unieke, beschrijvende naam: `mijn_weer`, `anker_timer`, `motor_dashboard`.

Op het apparaat zelf worden apps opgeslagen als platte SPIFFS-bestanden:
```
/app_<id>_manifest.json
/app_<id>_main.lua
/bkos_apps.json          ← lijst van geïnstalleerde apps
```

---

## 4. Manifest (manifest.json)

```json
{
  "id":           "mijn_app",
  "naam":         "Mijn App",
  "versie":       "1.0.0",
  "auteur":       "Jouw Naam",
  "beschrijving": "Korte omschrijving — max 80 tekens.",
  "scherm_b":     800,
  "scherm_h":     480,
  "vervangt":     -1,
  "api_versie":   1,
  "actief":       true
}
```

### Velden

| Veld | Type | Verplicht | Beschrijving |
|---|---|---|---|
| `id` | string | ✅ | Unieke app-ID (`[a-z0-9_]`, max 23 tekens) |
| `naam` | string | ✅ | Zichtbare naam in de app-store (max 31 tekens) |
| `versie` | string | ✅ | Semantische versie `MAJOR.MINOR.PATCH` |
| `auteur` | string | ✅ | Naam van de maker (max 23 tekens) |
| `beschrijving` | string | — | Korte uitleg (max 79 tekens) |
| `scherm_b` | int | — | Ontwerp-breedte in pixels (standaard: 800) |
| `scherm_h` | int | — | Ontwerp-hoogte in pixels (standaard: 480) |
| `vervangt` | int | — | Screen-ID dat vervangen wordt, of `-1` voor geen override (zie §8) |
| `api_versie` | int | — | Minimale BKOS API versie (huidig: `1`) |
| `actief` | bool | — | Of de app standaard ingeschakeld is |

### Screen-IDs voor `vervangt`

| Waarde | Scherm |
|---|---|
| `-1` | Geen override (standalone app) |
| `0` | PANEEL (hoofdscherm met bootschema) |
| `1` | IO-lijst |
| `2` | METEO / Getij |
| `3` | CONFIGURATIE |
| `5` | INFO (boot & eigenaar) |

---

## 5. Lua API — volledig overzicht

Alle BKOS-functies zitten in de globale tabel `bkos`.

### 5.1 Scherm tekenen

Coördinaten zijn in de **ontwerp-ruimte** van de app (`scherm_b × scherm_h`).  
De runtime schaalt automatisch naar de werkelijke schermresolutie.

---

#### `bkos.vul(x, y, breedte, hoogte, kleur)`
Vul een rechthoek met een kleur.

```lua
bkos.vul(0, 0, bkos.W, bkos.H, bkos.kleur.bg)  -- wis het scherm
bkos.vul(10, 50, 200, 80, bkos.rgb(20, 60, 100))
```

| Parameter | Type | Beschrijving |
|---|---|---|
| `x`, `y` | int | Linker-bovenhoek |
| `breedte` | int | Breedte in pixels |
| `hoogte` | int | Hoogte in pixels |
| `kleur` | int | RGB565 kleurwaarde |

---

#### `bkos.lijn(x1, y1, x2, y2, kleur)`
Teken een rechte lijn.

```lua
bkos.lijn(0, 0, bkos.W - 1, bkos.H - 1, bkos.kleur.cyaan)  -- diagonaal
bkos.lijn(0, 100, bkos.W, 100, bkos.rgb(80, 80, 80))         -- horizontaal
```

---

#### `bkos.tekst(x, y, tekst, grootte, kleur)`
Teken tekst. Grootte 1 = 6×8 pixels per karakter, grootte 2 = 12×16, grootte 3 = 18×24, etc.

```lua
bkos.tekst(20, 30, "Temperatuur:", 1, bkos.kleur.tekst_dim)
bkos.tekst(20, 45, "22.5 C",       3, bkos.kleur.cyaan)

-- Tekst centreren (grootte 2 = 12px per karakter):
local s = "Goed zo!"
bkos.tekst(math.floor((bkos.W - #s * 12) / 2), 200, s, 2, bkos.kleur.groen)
```

> **Lettertypebreedte per grootte:** grootte N = N × 6 pixels per karakter.

---

#### `bkos.cirkel(cx, cy, straal, kleur, gevuld)`
Teken een cirkel.

```lua
bkos.cirkel(400, 240, 50, bkos.kleur.rood, false)  -- omtrek
bkos.cirkel(400, 240, 50, bkos.kleur.rood, true)   -- gevuld
```

| Parameter | Type | Beschrijving |
|---|---|---|
| `cx`, `cy` | int | Middelpunt |
| `straal` | int | Straal in pixels |
| `kleur` | int | RGB565 kleurwaarde |
| `gevuld` | bool | `true` = gevulde schijf, `false` = ring |

---

#### `bkos.rgb(rood, groen, blauw)` → int
Maak een RGB565 kleurwaarde van 8-bit R, G, B componenten.

```lua
local zeeblauw = bkos.rgb(0, 105, 148)
local geel     = bkos.rgb(255, 220, 0)
```

---

#### `bkos.W` en `bkos.H`
De **ontwerp-breedte** en **ontwerp-hoogte** van de app (uit het manifest). Gebruik dit voor positionering. De werkelijke schermresolutie is altijd 800×480 maar jouw code hoeft dat niet te weten.

```lua
-- Altijd precies in het midden:
bkos.tekst(math.floor(bkos.W / 2) - 30, math.floor(bkos.H / 2), "Midden!", 2, bkos.kleur.tekst)
```

---

### 5.2 Kleuren

#### `bkos.kleur.*`
Voorgedefinieerde kleuren van het actieve kleurenpalette van het apparaat. Gebruik deze voor een consistente look.

| Sleutel | Beschrijving |
|---|---|
| `bkos.kleur.bg` | Achtergrondkleur |
| `bkos.kleur.surface` | Kaartachtergrond (iets lichter dan bg) |
| `bkos.kleur.tekst` | Hoofdtekstkleur |
| `bkos.kleur.tekst_dim` | Gedimde tekst (labels, hints) |
| `bkos.kleur.cyaan` | Accentkleur (blauw-groen) |
| `bkos.kleur.groen` | Succes / AAN |
| `bkos.kleur.amber` | Waarschuwing |
| `bkos.kleur.rood` | Fout / ALARM |

```lua
-- Kleur afhankelijk van staat:
local kleur = lamp_aan and bkos.kleur.groen or bkos.kleur.tekst_dim
bkos.tekst(100, 200, lamp_aan and "AAN" or "UIT", 2, kleur)
```

---

### 5.3 IO-kanalen

Toegang tot de fysieke IO-kanalen van de ATtiny3217 IO-module.

---

#### `bkos.io.lees(kanaalnr)` → bool
Leest de invoer van een kanaal op nummer (0-gebaseerd).

```lua
local drukknop_ingedrukt = bkos.io.lees(3)
if drukknop_ingedrukt then
    bkos.tekst(100, 100, "Ingedrukt!", 2, bkos.kleur.groen)
end
```

---

#### `bkos.io.zet(kanaalnr, staat)`
Zet een uitvoerkanaal op nummer. Gebruik `bkos.IO_AAN` of `bkos.IO_UIT`.

```lua
bkos.io.zet(5, bkos.IO_AAN)   -- kanaal 5 aanzetten
bkos.io.zet(5, bkos.IO_UIT)   -- kanaal 5 uitzetten
```

---

#### `bkos.io.wissel(kanaalnr)`
Wisselt een uitvoerkanaal (AAN→UIT of UIT→AAN).

```lua
bkos.io.wissel(5)  -- toggle kanaal 5
```

---

#### `bkos.io.lees_naam(naam)` → bool of nil
Leest de invoer van een kanaal op naam. Geeft `nil` als de naam niet gevonden wordt.

```lua
local pomp_draait = bkos.io.lees_naam("bilgepomp")
if pomp_draait == nil then
    bkos.tekst(10, 10, "Kanaal niet gevonden", 1, bkos.kleur.amber)
elseif pomp_draait then
    bkos.tekst(10, 10, "Pomp AAN", 2, bkos.kleur.rood)
end
```

> **Tip:** Kanaalnamen worden ingesteld via CONFIG → IO CONFIGURATIE. Gebruik de namen die de eigenaar van het systeem heeft ingesteld.

---

#### `bkos.io.zet_naam(naam, staat)`
Zet een uitvoerkanaal op naam.

```lua
bkos.io.zet_naam("salon", bkos.IO_AAN)
bkos.io.zet_naam("boegschroef", bkos.IO_UIT)
```

---

#### `bkos.io.wissel_naam(naam)`
Toggle een kanaal op naam. Werkt op alle kanalen met die naam tegelijk (handig voor groepen).

```lua
bkos.io.wissel_naam("ankerlicht")
```

---

#### `bkos.io.naam(kanaalnr)` → string of nil
Geeft de naam van een kanaal op nummer.

```lua
local naam = bkos.io.naam(0)  -- bijv. "salon"
```

---

#### `bkos.io.kanalen()` → int
Geeft het aantal geconfigureerde kanalen.

```lua
local n = bkos.io.kanalen()
for i = 0, n - 1 do
    local naam = bkos.io.naam(i) or ("kanaal " .. i)
    local staat = bkos.io.lees(i)
    -- ... teken rij
end
```

---

#### Constanten

| Constante | Waarde | Beschrijving |
|---|---|---|
| `bkos.IO_AAN` | 1 | Uitvoer aanzetten |
| `bkos.IO_UIT` | 0 | Uitvoer uitzetten |

---

### 5.4 Data-opslag

Gedeelde sleutel-waarde opslag met tijdstempel. Zowel apps als het systeem schrijven hier data naartoe.

---

#### `bkos.data.lees(sleutel)` → string of nil
Leest een waarde als tekst. Geeft `nil` als de sleutel niet bestaat.

```lua
local station = bkos.data.lees("getij.station")
if station then
    bkos.tekst(10, 50, "Station: " .. station, 1, bkos.kleur.tekst)
end
```

---

#### `bkos.data.lees_f(sleutel, standaard)` → number
Leest een waarde als getal (float). Geeft `standaard` terug als de sleutel niet bestaat.

```lua
local temp  = bkos.data.lees_f("meteo.temp",     999)  -- 999 = niet beschikbaar
local wind  = bkos.data.lees_f("meteo.wind_kn",  0)
local druk  = bkos.data.lees_f("meteo.druk_hpa", 0)
```

---

#### `bkos.data.schrijf(sleutel, waarde)`
Schrijft een tekstwaarde. Overschrijft bestaande waarde. Tijdstempel wordt automatisch bijgewerkt.

```lua
bkos.data.schrijf("mijn_app.status", "actief")
bkos.data.schrijf("mijn_app.kleur",  "blauw")
```

---

#### `bkos.data.schrijf_f(sleutel, getal)`
Schrijft een getal (float). Opgeslagen als tekst met 2 decimalen.

```lua
bkos.data.schrijf_f("mijn_app.drempel", 22.5)
```

---

#### `bkos.data.leeftijd(sleutel)` → int
Geeft het aantal seconden geleden dat de waarde voor het laatst geschreven is.  
Geeft `-1` als de sleutel niet bestaat.  
`0` betekent dat de sleutel permanent is (geen tijdstempel).

```lua
local leeftijd = bkos.data.leeftijd("meteo.temp")
if leeftijd < 0 then
    bkos.tekst(10, 10, "Geen temperatuurdata", 1, bkos.kleur.amber)
elseif leeftijd > 3600 then
    bkos.tekst(10, 10, "Data is meer dan 1 uur oud", 1, bkos.kleur.amber)
else
    local temp = bkos.data.lees_f("meteo.temp", 0)
    bkos.tekst(10, 10, string.format("%.1f C", temp), 2, bkos.kleur.tekst)
end
```

---

### 5.5 Systeem

---

#### `bkos.sys.versie()` → string
Geeft de firmware-versie van de BKOS-NUI.

```lua
local v = bkos.sys.versie()  -- bijv. "0.0.260503.3"
bkos.tekst(10, 10, "BKOS " .. v, 1, bkos.kleur.tekst_dim)
```

---

#### `bkos.sys.millis()` → int
Geeft de uptime in milliseconden (loopt over na ~49 dagen).  
Gebruik dit voor animaties en tijdmetingen binnen een sessie.

```lua
-- Knipperend element (elke 500ms van kleur wisselen)
local kleur = (math.floor(bkos.sys.millis() / 500) % 2 == 0)
              and bkos.kleur.rood or bkos.kleur.bg
bkos.cirkel(400, 240, 10, kleur, true)
```

---

#### `bkos.sys.log(tekst)`
Schrijft een debugregel naar de seriële monitor (alleen zichtbaar als firmware met `#define DEBUG` is gebouwd).

```lua
bkos.sys.log("temp gelezen: " .. temp)
```

---

## 6. App Callbacks

Jouw app registreert functies die door het systeem aangeroepen worden.

```lua
-- Verplicht: teken het volledige scherm
function bkos.teken()
    -- Wordt aangeroepen:
    -- · Bij het openen van het scherm
    -- · Na elke teken() aanroep vanuit bkos.aanraking()
    -- · Na scherm-herstart (bijv. na inkomende touch op donker scherm)
end

-- Optioneel: verwerk een aanraking
function bkos.aanraking(x, y)
    -- x, y in ontwerp-coördinaten (al geschaald naar scherm_b × scherm_h)
    -- Wordt aangeroepen bij elke geldige touch-event (320ms debounce)
end

-- Optioneel: periodieke update zonder aanraking
function bkos.update()
    -- Wordt continu aangeroepen als er geen aanraking is.
    -- Gebruik bkos.sys.millis() om updates te vertragen.
    -- Roep bkos.teken() aan als je het scherm wilt bijwerken.
end
```

> **Belangrijk:** roep `bkos.teken()` alleen aan als het scherm echt veranderd is — elke aanroep wist en hertekent het volledige scherm.

Voorbeeld met update-interval:

```lua
local vorige_update = 0

function bkos.update()
    local nu = bkos.sys.millis()
    if nu - vorige_update > 5000 then  -- elke 5 seconden
        vorige_update = nu
        bkos.teken()  -- herlaad data en herteken
    end
end
```

---

## 7. Schermresolutie en schalen

Je geeft in het manifest op voor welke resolutie je app ontworpen is:

```json
{
  "scherm_b": 800,
  "scherm_h": 480
}
```

Als de app ontworpen is voor 400×240 maar het scherm is 800×480, schaalt het systeem alles automatisch ×2. Dit werkt voor:
- Coordinaten in `bkos.vul`, `bkos.lijn`, `bkos.tekst`, `bkos.cirkel`
- Aanraakcoördinaten in `bkos.aanraking(x, y)` — altijd in ontwerp-ruimte

**Aanbeveling:** ontwerp voor 800×480 (de standaard BKOS schermgrootte). Schalen is handig als je een app wilt porteren die voor een andere resolutie gemaakt is.

Gebruik `bkos.W` en `bkos.H` (de ontwerp-dimensies) voor alle positionering — nooit hardcoded 800 of 480.

---

## 8. Scherm-override

Een app kan een ingebouwd scherm vervangen door `vervangt` in het manifest in te stellen:

```json
{
  "vervangt": 2
}
```

Dit vervangt het METEO-scherm. Wanneer de gebruiker op METEO klikt in de navigatiebalk, wordt jouw Lua-app getoond in plaats van het ingebouwde weeroverzicht.

De **navigatiebalk blijft altijd bereikbaar** — de gebruiker kan altijd naar een ander scherm navigeren.

### Override via de interface

In de APPS-screen (tab INSTELLINGEN) kan de gebruiker zelf instellen welke app welk scherm vervangt — los van de waarde in het manifest. Hierdoor kan één app meerdere schermen vervangen (door meerdere keren te installeren met andere ID's), of kan de gebruiker de override tijdelijk uitschakelen.

---

## 9. Data-opslag sleutelconventies

Gebruik namespace-prefixes voor je sleutels om conflicten te vermijden.

### Systemsleutels (alleen lezen voor apps)

| Sleutel | Type | Beschrijving |
|---|---|---|
| `meteo.temp` | float (°C) | Actuele temperatuur |
| `meteo.temp_gevoeld` | float (°C) | Gevoelstemperatuur |
| `meteo.wind_kn` | float (kn) | Windsnelheid |
| `meteo.wind_windrichting` | int (°) | Windrichting |
| `meteo.code` | int | Open-Meteo weather code |
| `meteo.is_dag` | int (0/1) | 1 als het dag is |
| `getij.station` | string | Naam van het getijstation |
| `getij.hw_hoogte` | float (m) | Hoogte volgende hoogwater |
| `getij.hw_tijd` | int (unix) | Tijdstip volgende hoogwater |
| `getij.lw_hoogte` | float (m) | Hoogte volgende laagwater |
| `getij.lw_tijd` | int (unix) | Tijdstip volgende laagwater |
| `sys.tijd` | string | Huidige tijd "HH:MM" |
| `sys.datum` | string | Huidige datum |

### Eigen sleutels

Gebruik je app-ID als prefix:

```lua
bkos.data.schrijf("mijn_app.drempel_temp",  "22.5")
bkos.data.schrijf("mijn_app.laatste_alarm", "14:32")
```

---

## 10. Publiceren naar de app store

De BKOS app store is onderdeel van de GitHub repository `brennyc86/BKOS-NUI`.

### Stappen

1. **Fork** de repository op GitHub
2. Maak je app-map aan:
   ```
   appstore/apps/<jouw-app-id>/
       manifest.json
       main.lua
   ```
3. Voeg je app toe aan `appstore/index.json`:
   ```json
   [
     { ...bestaande apps... },
     {
       "id": "jouw_app_id",
       "naam": "Jouw App Naam",
       "versie": "1.0.0",
       "auteur": "Jouw naam",
       "beschrijving": "Korte beschrijving",
       "scherm_b": 800,
       "scherm_h": 480,
       "vervangt": -1,
       "api_versie": 1,
       "actief": true
     }
   ]
   ```
4. Maak een **Pull Request** naar de `main` branch
5. Na merge is de app direct beschikbaar in de APPS → WINKEL van alle apparaten

---

## 11. Lokale installatie (zonder app store)

Handmatig uploaden via een SPIFFS-flashtool (bijv. Arduino IDE SPIFFS Data Upload):

1. Maak een `data/` map in je Arduino-project
2. Zet daarin:
   - `app_<id>_manifest.json` (inhoud van manifest.json)
   - `app_<id>_main.lua` (inhoud van main.lua)
   - `bkos_apps.json`: `{"ids":["<id>"]}`
3. Upload via **Sketch → Upload SPIFFS/LittleFS Data**

Of via de seriële terminal (bijv. met `esptool.py`) een volledig SPIFFS-image flashen.

---

## 12. Richtlijnen en beperkingen

### Geheugen
- De Lua-runtime gebruikt **PSRAM** voor de heap (8MB SPI RAM op de ESP32-S3)
- Maximale stack-diepte: 500 niveaus
- Houd scripts compact — grote tabellen of strings kosten geheugen

### Timing
- `bkos.aanraking()` heeft een debounce van 320ms — te snel achter elkaar aanraken wordt genegeerd
- `bkos.update()` wordt elke loop-iteratie aangeroepen (~50ms) — gebruik zelf een timer
- `bkos.teken()` is duur (wist het hele scherm) — roep het alleen aan als de weergave verandert

### Veiligheid
- Apps kunnen **geen bestanden lezen of schrijven** — alleen via `bkos.data.*`
- Apps kunnen **geen netwerk** gebruiken — data komt via de systeemeigen weermodule
- Apps kunnen **geen andere apps** starten of stoppen
- Crashes worden afgevangen — foutmelding verschijnt op het scherm, systeem blijft stabiel

### Stijl
- Gebruik `bkos.kleur.*` kleuren voor een consistente uitstraling
- Houd de navigatiebalk (onderste 42px) leeg — die is voor het systeem
- Houd de statusbalk (bovenste 42px) leeg als je het scherm niet vervangt

---

## 13. Volledige voorbeeldapp

Hieronder staat een complete app die wertemperatuur toont, en een lamp via naam bedient:

```lua
-- ─────────────────────────────────────────────────────────────────────────────
-- BKOS App: Brugwacht Dashboard
-- Toont actuele temperatuur en wind, en bedient de brugwacht-lamp
-- ─────────────────────────────────────────────────────────────────────────────

local LAMP_NAAM = "brugwacht"     -- pas aan naar jouw kanaalnaam

local vorige_update  = 0
local UPDATE_INTERVAL = 10000   -- 10 seconden

local function scherm_bijwerken()
    bkos.vul(0, 0, bkos.W, bkos.H, bkos.kleur.bg)

    -- Titel
    bkos.vul(0, 0, bkos.W, 46, bkos.rgb(18, 28, 40))
    bkos.tekst(20, 14, "BRUGWACHT DASHBOARD", 2, bkos.kleur.cyaan)

    -- Temperatuur
    local temp = bkos.data.lees_f("meteo.temp", 999)
    local temp_kleur = bkos.kleur.tekst
    local temp_str
    if temp == 999 then
        temp_str  = "---"
        temp_kleur = bkos.kleur.tekst_dim
    else
        temp_str = string.format("%.1f C", temp)
        if temp < 5 then temp_kleur = bkos.kleur.cyaan end
        if temp > 30 then temp_kleur = bkos.kleur.rood  end
    end
    bkos.tekst(40, 80,  "Temperatuur", 1, bkos.kleur.tekst_dim)
    bkos.tekst(40, 96,  temp_str,      4, temp_kleur)

    -- Wind
    local wind = bkos.data.lees_f("meteo.wind_kn", -1)
    local wind_str = (wind >= 0) and string.format("%.0f kn", wind) or "---"
    bkos.tekst(40, 170, "Wind",     1, bkos.kleur.tekst_dim)
    bkos.tekst(40, 186, wind_str,   4, bkos.kleur.tekst)

    -- Data-leeftijd
    local leeftijd = bkos.data.leeftijd("meteo.temp")
    local leeftijd_str
    if leeftijd < 0 then
        leeftijd_str = "Geen weerdata beschikbaar"
    elseif leeftijd < 60 then
        leeftijd_str = "Bijgewerkt " .. leeftijd .. "s geleden"
    else
        leeftijd_str = "Bijgewerkt " .. math.floor(leeftijd / 60) .. " min geleden"
    end
    bkos.tekst(40, 260, leeftijd_str, 1, bkos.kleur.tekst_dim)

    -- Lamp-knop
    local lamp_aan  = bkos.io.lees_naam(LAMP_NAAM)
    local kl_knop   = (lamp_aan == true) and bkos.kleur.groen or bkos.rgb(50, 60, 80)
    local kl_tekst  = bkos.kleur.tekst
    bkos.vul(40, 300, 280, 80, kl_knop)
    bkos.tekst(70, 318, LAMP_NAAM,                      1, kl_tekst)
    bkos.tekst(70, 332, (lamp_aan == true) and "AAN" or "UIT",  3, kl_tekst)
end

function bkos.teken()
    scherm_bijwerken()
end

function bkos.aanraking(x, y)
    -- Lamp-knop aangeraakt?
    if x >= 40 and x <= 320 and y >= 300 and y <= 380 then
        bkos.io.wissel_naam(LAMP_NAAM)
        scherm_bijwerken()  -- direct hertekenen
    end
end

function bkos.update()
    local nu = bkos.sys.millis()
    if nu - vorige_update > UPDATE_INTERVAL then
        vorige_update = nu
        scherm_bijwerken()
    end
end
```

---

## 14. API-referentie voor AI-systemen

Dit gedeelte is gestructureerd als machine-leesbare referentie voor AI-codeertools.

```
BKOS LUA APP API — versie 1
Taal: Lua 5.4
Beschikbare standaard bibliotheken: base, math, string, table, coroutine

=== GLOBALE TABEL: bkos ===

--- CONSTANTEN ---
bkos.W          : integer  — ontwerp-breedte (uit manifest.scherm_b)
bkos.H          : integer  — ontwerp-hoogte  (uit manifest.scherm_h)
bkos.IO_AAN     : integer = 1
bkos.IO_UIT     : integer = 0

--- KLEUREN ---
bkos.kleur.bg         : uint16  — achtergrondkleur (RGB565)
bkos.kleur.surface    : uint16  — kaartachtergrond
bkos.kleur.tekst      : uint16  — hoofdtekst
bkos.kleur.tekst_dim  : uint16  — gedimde tekst
bkos.kleur.cyaan      : uint16  — accentkleur
bkos.kleur.groen      : uint16  — succes/AAN
bkos.kleur.amber      : uint16  — waarschuwing
bkos.kleur.rood       : uint16  — fout/alarm

--- SCHERM-FUNCTIES ---
bkos.rgb(r, g, b)
  r, g, b : integer 0-255
  return  : integer (RGB565 kleurwaarde)

bkos.vul(x, y, breedte, hoogte, kleur)
  x, y, breedte, hoogte : integer (ontwerp-coördinaten)
  kleur                 : integer (RGB565)

bkos.lijn(x1, y1, x2, y2, kleur)
  x1,y1, x2,y2 : integer
  kleur         : integer (RGB565)

bkos.tekst(x, y, tekst, grootte, kleur)
  x, y    : integer
  tekst   : string
  grootte : integer 1-8  (breedte per karakter = grootte * 6 px)
  kleur   : integer (RGB565)

bkos.cirkel(cx, cy, straal, kleur, gevuld)
  cx, cy  : integer (middelpunt)
  straal  : integer
  kleur   : integer (RGB565)
  gevuld  : boolean

--- IO-FUNCTIES ---
bkos.io.lees(kanaalnr)      → boolean     — leest invoer-kanaal
bkos.io.zet(kanaalnr, staat)              — zet uitvoer (IO_AAN/IO_UIT)
bkos.io.wissel(kanaalnr)                  — toggle uitvoer
bkos.io.lees_naam(naam)     → boolean|nil — leest op naam (nil=niet gevonden)
bkos.io.zet_naam(naam, staat)             — zet uitvoer op naam
bkos.io.wissel_naam(naam)                 — toggle alle kanalen met naam
bkos.io.naam(kanaalnr)      → string|nil  — naam van kanaal op nummer
bkos.io.kanalen()           → integer     — totaal aantal kanalen

--- DATA-OPSLAG ---
bkos.data.lees(sleutel)          → string|nil    — leest tekst, nil als afwezig
bkos.data.lees_f(sleutel, std)   → number        — leest als float
bkos.data.schrijf(sleutel, waarde)               — schrijft tekst (string)
bkos.data.schrijf_f(sleutel, waarde)             — schrijft getal (float)
bkos.data.leeftijd(sleutel)      → integer       — seconden oud; -1 afwezig; 0 permanent

Systeem-data sleutels (lezen):
  meteo.temp          float   temperatuur °C
  meteo.temp_gevoeld  float   gevoelstemperatuur °C
  meteo.wind_kn       float   windsnelheid knopen
  meteo.code          int     Open-Meteo weercode
  meteo.is_dag        int     1=dag, 0=nacht
  getij.station       string  naam getijstation
  getij.hw_hoogte     float   volgende HW hoogte m
  getij.hw_tijd       int     volgende HW unix-tijdstip
  getij.lw_hoogte     float   volgende LW hoogte m
  getij.lw_tijd       int     volgende LW unix-tijdstip
  sys.tijd            string  "HH:MM"
  sys.datum           string  datumstring

--- SYSTEEM-FUNCTIES ---
bkos.sys.versie()    → string    — firmware versie, bijv. "0.0.260503.3"
bkos.sys.millis()    → integer   — uptime milliseconden
bkos.sys.log(tekst)             — debug naar Serial (alleen bij DEBUG build)

--- CALLBACKS (registreer als functie) ---
bkos.teken            = function()        — herteken volledig scherm
bkos.aanraking        = function(x, y)   — touch-event (in ontwerp-coördinaten)
bkos.update           = function()        — periodiek (geen aanraking)

=== MANIFEST FORMAAT (manifest.json) ===
{
  "id"          : string  — uniek, [a-z0-9_], max 23 tekens
  "naam"        : string  — weergavenaam, max 31 tekens
  "versie"      : string  — "MAJOR.MINOR.PATCH"
  "auteur"      : string  — max 23 tekens
  "beschrijving": string  — max 79 tekens
  "scherm_b"    : int     — ontwerp-breedte  (standaard 800)
  "scherm_h"    : int     — ontwerp-hoogte   (standaard 480)
  "vervangt"    : int     — SCREEN_ID of -1 (geen override)
                            0=PANEEL, 1=IO, 2=METEO, 3=CONFIG, 5=INFO
  "api_versie"  : int     — huidig: 1
  "actief"      : bool    — standaard ingeschakeld
}

=== BEPERKINGEN ===
- Geen bestandssysteem-toegang (gebruik bkos.data.*)
- Geen netwerk-toegang
- Geen timers of interrupts
- Maximale stack-diepte: 500
- Touch-debounce: 320ms
- Navigatiebalk (onderste 42px) is systeemeigen — teken daar niet over heen
- Statusbalk (bovenste 42px) is systeemeigen tenzij je vervangt != -1 hebt
```
