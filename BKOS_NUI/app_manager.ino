#include "app_manager.h"
#include "lua_runtime.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

AppManifest apps[APP_MAX];
int         apps_cnt = 0;

AppManifest winkel[WINKEL_MAX];
int         winkel_cnt   = 0;
bool        winkel_geladen = false;

// ─── Hulpfunctie: manifest JSON → struct ─────────────────────────────────────
static void _json_naar_manifest(JsonObject obj, AppManifest& m) {
    strncpy(m.id,           obj["id"]          | "", APP_ID_LEN    - 1);
    strncpy(m.naam,         obj["naam"]        | "", APP_NAAM_LEN  - 1);
    strncpy(m.versie,       obj["versie"]      | "", APP_VERSIE_LEN- 1);
    strncpy(m.auteur,       obj["auteur"]      | "", APP_AUTEUR_LEN- 1);
    strncpy(m.beschrijving, obj["beschrijving"]| "", APP_DESC_LEN  - 1);
    m.id[APP_ID_LEN-1] = m.naam[APP_NAAM_LEN-1] = m.versie[APP_VERSIE_LEN-1] = '\0';
    m.auteur[APP_AUTEUR_LEN-1] = m.beschrijving[APP_DESC_LEN-1] = '\0';
    m.scherm_b   = obj["scherm_b"]   | 800;
    m.scherm_h   = obj["scherm_h"]   | 480;
    m.vervangt   = obj["vervangt"]   | APP_VERVANGT_GEEN;
    m.api_versie = obj["api_versie"] | 1;
    m.actief     = obj["actief"]     | true;
}

static void _manifest_naar_json(AppManifest& m, JsonObject obj) {
    obj["id"]          = m.id;
    obj["naam"]        = m.naam;
    obj["versie"]      = m.versie;
    obj["auteur"]      = m.auteur;
    obj["beschrijving"]= m.beschrijving;
    obj["scherm_b"]    = m.scherm_b;
    obj["scherm_h"]    = m.scherm_h;
    obj["vervangt"]    = m.vervangt;
    obj["api_versie"]  = m.api_versie;
    obj["actief"]      = m.actief;
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void app_setup() {
    app_manifesten_laden();
    lua_setup();
}

// ─── Manifesten laden ─────────────────────────────────────────────────────────
void app_manifesten_laden() {
    apps_cnt = 0;

    // Loop door /apps/<id>/manifest.json bestanden
    File root = SPIFFS.open("/apps");
    if (!root || !root.isDirectory()) return;

    File app_dir = root.openNextFile();
    while (app_dir && apps_cnt < APP_MAX) {
        if (app_dir.isDirectory()) {
            String manifest_pad = String(app_dir.name()) + "/manifest.json";
            if (SPIFFS.exists(manifest_pad)) {
                File mf = SPIFFS.open(manifest_pad, "r");
                if (mf) {
                    DynamicJsonDocument doc(512);
                    if (deserializeJson(doc, mf) == DeserializationError::Ok) {
                        _json_naar_manifest(doc.as<JsonObject>(), apps[apps_cnt]);
                        apps_cnt++;
                    }
                    mf.close();
                }
            }
        }
        app_dir = root.openNextFile();
    }
}

void app_manifest_opslaan(int idx) {
    if (idx < 0 || idx >= apps_cnt) return;
    String pad = app_pad(apps[idx].id) + "/manifest.json";
    File f = SPIFFS.open(pad, "w");
    if (!f) return;
    DynamicJsonDocument doc(512);
    _manifest_naar_json(apps[idx], doc.as<JsonObject>());
    serializeJson(doc, f);
    f.close();
}

// ─── Zoekfuncties ────────────────────────────────────────────────────────────
int app_vindt(const char* id) {
    for (int i = 0; i < apps_cnt; i++)
        if (strcmp(apps[i].id, id) == 0) return i;
    return -1;
}

int app_voor_scherm(int scherm_id) {
    for (int i = 0; i < apps_cnt; i++)
        if (apps[i].actief && apps[i].vervangt == scherm_id) return i;
    return -1;
}

// ─── App beheer ──────────────────────────────────────────────────────────────
void app_zet_actief(int idx, bool actief) {
    if (idx < 0 || idx >= apps_cnt) return;
    apps[idx].actief = actief;
    app_manifest_opslaan(idx);
}

void app_verwijder(int idx) {
    if (idx < 0 || idx >= apps_cnt) return;

    String pad = app_pad(apps[idx].id);
    // Verwijder alle bestanden in de app-map
    File dir = SPIFFS.open(pad);
    if (dir && dir.isDirectory()) {
        File f = dir.openNextFile();
        while (f) {
            SPIFFS.remove(f.name());
            f = dir.openNextFile();
        }
    }
    SPIFFS.rmdir(pad);

    // Verschuif array
    for (int i = idx; i < apps_cnt - 1; i++)
        apps[i] = apps[i + 1];
    apps_cnt--;
}

// ─── Winkel: laden vanuit GitHub ─────────────────────────────────────────────
void app_winkel_laden() {
    winkel_cnt    = 0;
    winkel_geladen = false;

    HTTPClient http;
    http.begin(APPSTORE_INDEX_URL);
    http.useHTTP10(true);
    http.setTimeout(10000);
    int code = http.GET();
    if (code != 200) { http.end(); return; }

    DynamicJsonDocument doc(4096);
    if (deserializeJson(doc, http.getStream()) != DeserializationError::Ok) {
        http.end(); return;
    }
    http.end();

    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject obj : arr) {
        if (winkel_cnt >= WINKEL_MAX) break;
        _json_naar_manifest(obj, winkel[winkel_cnt]);
        winkel[winkel_cnt].actief = true;
        winkel_cnt++;
    }
    winkel_geladen = true;
}

// ─── App installeren vanuit winkel ───────────────────────────────────────────
bool app_installeer_uit_winkel(int winkel_idx) {
    if (winkel_idx < 0 || winkel_idx >= winkel_cnt) return false;
    AppManifest& wm = winkel[winkel_idx];

    // Maak map aan
    String pad = app_pad(wm.id);
    SPIFFS.mkdir(pad);

    // Download main.lua
    String lua_url = String("https://raw.githubusercontent.com/brennyc86/BKOS-NUI/main/appstore/apps/")
                     + wm.id + "/main.lua";
    HTTPClient http;
    http.begin(lua_url);
    http.useHTTP10(true);
    http.setTimeout(15000);
    if (http.GET() != 200) { http.end(); return false; }

    File lf = SPIFFS.open(pad + "/main.lua", "w");
    if (!lf) { http.end(); return false; }

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buf[256];
    int len;
    while ((len = stream->readBytes(buf, sizeof(buf))) > 0)
        lf.write(buf, len);
    lf.close();
    http.end();

    // Sla manifest op (kopie van winkel-entry)
    int nieuw_idx = apps_cnt;
    if (nieuw_idx >= APP_MAX) return false;

    int bestaand = app_vindt(wm.id);
    if (bestaand >= 0) nieuw_idx = bestaand;
    else apps_cnt++;

    apps[nieuw_idx] = wm;
    app_manifest_opslaan(nieuw_idx);
    return true;
}

String app_pad(const char* id) {
    return String("/apps/") + id;
}
