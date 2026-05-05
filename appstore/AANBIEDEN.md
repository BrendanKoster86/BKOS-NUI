# Een app aanbieden voor de BKOS App Store

Wil je een app maken voor het BKOS boordcomputer systeem en deze beschikbaar stellen voor andere gebruikers? Dit document legt uit hoe het werkt.

---

## Hoe het werkt

Apps komen via een **Pull Request** in de app store. Elke PR wordt eerst automatisch gecontroleerd, daarna handmatig beoordeeld door de beheerder. Alleen goedgekeurde apps verschijnen in de winkel.

---

## Stap 1 — Fork de repository

Ga naar [github.com/brennyc86/BKOS-NUI](https://github.com/brennyc86/BKOS-NUI) en klik op **Fork** (rechtsboven).

---

## Stap 2 — Maak je app aan

In jouw fork, maak je de volgende bestanden aan:

```
appstore/apps/<jouw-app-id>/
    manifest.json
    main.lua
```

**App-ID regels:**
- Alleen kleine letters, cijfers en underscores (`[a-z0-9_]`)
- Minimaal 3, maximaal **18** tekens
- Beschrijvend en uniek, bijv. `anker_timer`, `motor_uren`, `vloot_chat`

### manifest.json

```json
{
  "id":           "jouw_app_id",
  "naam":         "Jouw App Naam",
  "versie":       "1.0.0",
  "auteur":       "Jouw Naam",
  "beschrijving": "Korte omschrijving, max 79 tekens.",
  "scherm_b":     800,
  "scherm_h":     480,
  "vervangt":     -1,
  "api_versie":   1,
  "actief":       true
}
```

Zie de [BKOS App Handleiding](../docs/BKOS_APP_HANDLEIDING.md) voor een volledig overzicht van alle manifest-velden en de Lua-API.

### main.lua

Minimale app:

```lua
function bkos.teken()
    bkos.vul(0, 0, bkos.W, bkos.H, bkos.kleur.bg)
    bkos.tekst(100, 200, "Hallo BKOS!", 3, bkos.kleur.cyaan)
end
```

---

## Stap 3 — Voeg toe aan index.json

Voeg een entry toe aan `appstore/index.json`:

```json
{
  "id":           "jouw_app_id",
  "naam":         "Jouw App Naam",
  "versie":       "1.0.0",
  "auteur":       "Jouw Naam",
  "beschrijving": "Korte omschrijving, max 79 tekens.",
  "scherm_b":     800,
  "scherm_h":     480,
  "vervangt":     -1,
  "api_versie":   1,
  "grootte_kb":   5,
  "actief":       true
}
```

Voeg de entry toe aan het einde van de bestaande array, vóór de sluitende `]`.

---

## Stap 4 — Maak een Pull Request

Ga naar jouw fork op GitHub en klik op **Contribute → Open pull request**.

Geef je PR een duidelijke titel, bijv.: `App: Anker Timer v1.0.0`

Beschrijf kort wat de app doet en wie hem heeft getest.

---

## Automatische controles

Zodra je PR is ingediend, voert GitHub Actions automatisch een reeks checks uit:

| Check | Wat er gecontroleerd wordt |
|---|---|
| JSON geldigheid | `manifest.json` is geldige JSON |
| Verplichte velden | `id`, `naam`, `versie`, `auteur` aanwezig |
| App-ID formaat | `[a-z0-9_]`, 3–18 tekens |
| ID ↔ mapnaam | `id` in manifest gelijk aan mapnaam |
| Veldlengtes | `naam` ≤ 31, `beschrijving` ≤ 79 tekens |
| SPIFFS padlengtes | Bestandsnamen passen binnen 31 tekens |
| Lua syntaxcheck | `main.lua` compileert foutloos |
| Verboden patronen | Geen `io.*`, `os.*`, `dofile`, `require`, etc. |
| index.json | App aanwezig in de winkelindex |

Als een check mislukt zie je de fout direct bij je PR. Herstel het probleem en push — de checks worden dan automatisch opnieuw uitgevoerd.

---

## Handmatige beoordeling

Na succesvolle automatische checks beoordeelt de beheerder de PR op:

- **Functionaliteit** — doet de app wat het manifest belooft?
- **Stijl** — past de app bij het marine thema van BKOS?
- **Stabiliteit** — zijn er oneindige loops, geheugenlekken of crashes?
- **Zinvolheid** — is de app nuttig voor gebruikers van een scheepscomputer?
- **Scherm-overschrijving** — als `vervangt != -1`: is dit goed onderbouwd?

---

## Veiligheidseisen

BKOS apps worden uitgevoerd in een beveiligde sandbox. Hieronder staat wat wel en niet is toegestaan, en waarom.

### Wat apps NIET mogen

| Beperking | Technisch | Reden |
|---|---|---|
| BKOS-systeem aanpassen | Onmogelijk (geen bestandstoegang) | Systeemintegriteit |
| Andere apps aanpassen | Onmogelijk (geen bestandstoegang) | Systeemintegriteit |
| Opgeslagen data verwijderen | Onmogelijk (geen delete-API) | Betrouwbaarheid dataopslag |
| Internetverbinding maken | Onmogelijk (geen netwerk-API) | Privacy en veiligheid |
| Bestandssysteem lezen/schrijven | Onmogelijk (`io`/`os` niet geladen) | Sandbox-isolatie |
| Andere modules laden | Onmogelijk (`require`/`package` niet beschikbaar) | Sandbox-isolatie |

### Wat apps WEL mogen

- Lezen en schrijven van eigen data via `bkos.data.schrijf("mijn_app.sleutel", waarde)`
- Lezen van systeemdata zoals weer, getij en tijd (alleen lezen)
- IO-kanalen lezen en aansturen via naam of poortnummer
- Het scherm volledig hertekenen binnen de toegewezen ruimte

### Uitzondering: weer- en getijapps

Apps die weersvoorspellingen of getijdata berekenen of ophalen mogen **bestaande meteo-sleutels overschrijven** (`meteo.*`, `getij.*`). Ze mogen géén andere data-sleutels verwijderen of overschrijven.

---

## Versie-update van een bestaande app

Push gewoon een nieuwe PR met het bijgewerkte versienummer in `manifest.json` en `index.json`. Dezelfde checks gelden.

---

## Vragen?

Open een [GitHub Issue](https://github.com/brennyc86/BKOS-NUI/issues) met het label `app-vraag`.
