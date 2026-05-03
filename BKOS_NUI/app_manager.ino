#include "app_manager.h"
#include "lua_runtime.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// SPIFFS heeft geen echte mappen — bestanden worden plat opgeslagen:
//   /app_<id>_manifest.json
//   /app_<id>_main.lua
//   /bkos_apps.json       ← lijst van geïnstalleerde app-IDs

AppManifest apps[APP_MAX];
int         apps_cnt = 0;

AppManifest winkel[WINKEL_MAX];
int         winkel_cnt    = 0;
bool        winkel_geladen = false;

// ─── Bestandspaden ───────────────────────────────────────────────────────────
static String _manifest_pad(const char* id) {
    return String("/app_") + id + "_manifest.json";
}

static String _lua_pad(const char* id) {
    return String("/app_") + id + "_main.lua";
}

static String _index_pad() { return "/bkos_apps.json"; }

// ─── JSON ↔ manifest ─────────────────────────────────────────────────────────
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

// ─── Index opslaan/laden ──────────────────────────────────────────────────────
static void _index_opslaan() {
    DynamicJsonDocument doc(512);
    JsonArray arr = doc.createNestedArray("ids");
    for (int i = 0; i < apps_cnt; i++) arr.add(apps[i].id);
    File f = SPIFFS.open(_index_pad(), "w");
    if (f) { serializeJson(doc, f); f.close(); }
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void app_setup() {
    app_manifesten_laden();
    lua_setup();
}

// ─── Manifesten laden ─────────────────────────────────────────────────────────
void app_manifesten_laden() {
    apps_cnt = 0;

    // Lees de index van geïnstalleerde app-IDs
    if (!SPIFFS.exists(_index_pad())) return;
    File idx = SPIFFS.open(_index_pad(), "r");
    if (!idx) return;

    DynamicJsonDocument idoc(512);
    if (deserializeJson(idoc, idx) != DeserializationError::Ok) { idx.close(); return; }
    idx.close();

    JsonArray ids = idoc["ids"].as<JsonArray>();
    for (const char* id : ids) {
        if (!id || apps_cnt >= APP_MAX) break;
        String pad = _manifest_pad(id);
        if (!SPIFFS.exists(pad)) continue;
        File mf = SPIFFS.open(pad, "r");
        if (!mf) continue;
        DynamicJsonDocument doc(512);
        if (deserializeJson(doc, mf) == DeserializationError::Ok)
            _json_naar_manifest(doc.as<JsonObject>(), apps[apps_cnt++]);
        mf.close();
    }
}

void app_manifest_opslaan(int idx) {
    if (idx < 0 || idx >= apps_cnt) return;
    File f = SPIFFS.open(_manifest_pad(apps[idx].id), "w");
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
    SPIFFS.remove(_manifest_pad(apps[idx].id));
    SPIFFS.remove(_lua_pad(apps[idx].id));
    for (int i = idx; i < apps_cnt - 1; i++) apps[i] = apps[i + 1];
    apps_cnt--;
    _index_opslaan();
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

    // Download main.lua
    String lua_url = String("https://raw.githubusercontent.com/brennyc86/BKOS-NUI/main/appstore/apps/")
                     + wm.id + "/main.lua";
    HTTPClient http;
    http.begin(lua_url);
    http.useHTTP10(true);
    http.setTimeout(15000);
    if (http.GET() != 200) { http.end(); return false; }

    File lf = SPIFFS.open(_lua_pad(wm.id), "w");
    if (!lf) { http.end(); return false; }
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buf[256];
    int len;
    while ((len = stream->readBytes(buf, sizeof(buf))) > 0) lf.write(buf, len);
    lf.close();
    http.end();

    // Manifest opslaan
    int bestaand = app_vindt(wm.id);
    int nieuw_idx = (bestaand >= 0) ? bestaand : apps_cnt;
    if (nieuw_idx >= APP_MAX) return false;
    if (bestaand < 0) apps_cnt++;

    apps[nieuw_idx] = wm;
    app_manifest_opslaan(nieuw_idx);
    _index_opslaan();
    return true;
}

// ─── Pad helper (geeft pad naar het Lua-script) ──────────────────────────────
String app_pad(const char* id) {
    return _lua_pad(id);
}
