#include "app_manager.h"
#include "lua_runtime.h"
#include "wifi.h"
#include <SPIFFS.h>
#include <SD_MMC.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

static bool _sd_geinitialiseerd = false;
static bool _sd_aanwezig        = false;

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
    m.grootte_kb = obj["grootte_kb"] | 0;
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
    obj["grootte_kb"]  = m.grootte_kb;
    obj["actief"]      = m.actief;
}

// ─── Index opslaan/laden ──────────────────────────────────────────────────────
static void _index_opslaan() {
    JsonDocument doc;
    JsonArray arr = doc["ids"].to<JsonArray>();
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

    JsonDocument idoc;
    if (deserializeJson(idoc, idx) != DeserializationError::Ok) { idx.close(); return; }
    idx.close();

    JsonArray ids = idoc["ids"].as<JsonArray>();
    for (const char* id : ids) {
        if (!id || apps_cnt >= APP_MAX) break;
        String pad = _manifest_pad(id);
        if (!SPIFFS.exists(pad)) continue;
        File mf = SPIFFS.open(pad, "r");
        if (!mf) continue;
        JsonDocument doc;
        if (deserializeJson(doc, mf) == DeserializationError::Ok)
            _json_naar_manifest(doc.as<JsonObject>(), apps[apps_cnt++]);
        mf.close();
    }
}

void app_manifest_opslaan(int idx) {
    if (idx < 0 || idx >= apps_cnt) return;
    File f = SPIFFS.open(_manifest_pad(apps[idx].id), "w");
    if (!f) return;
    JsonDocument doc;
    _manifest_naar_json(apps[idx], doc.to<JsonObject>());
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

    if (!wifi_verbonden) {
        wifi_verbind_aanvragen();
        unsigned long t = millis();
        while (!wifi_verbonden && millis() - t < 10000) delay(100);
    }
    if (!wifi_verbonden) return;

    WiFiClientSecure sc;
    sc.setInsecure();
    HTTPClient http;
    http.begin(sc, APPSTORE_INDEX_URL);
    http.useHTTP10(true);
    http.setTimeout(15000);
    int code = http.GET();
    if (code != 200) { http.end(); return; }

    JsonDocument doc;
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

// ─── App installeren op SPIFFS ────────────────────────────────────────────────
bool app_installeer_op_spiffs(int winkel_idx) {
    if (winkel_idx < 0 || winkel_idx >= winkel_cnt) return false;
    AppManifest& wm = winkel[winkel_idx];

    if (!wifi_verbonden) {
        wifi_verbind_aanvragen();
        unsigned long t = millis();
        while (!wifi_verbonden && millis() - t < 10000) delay(100);
    }
    if (!wifi_verbonden) return false;

    String lua_url = String("https://raw.githubusercontent.com/brennyc86/BKOS-NUI/main/appstore/apps/")
                     + wm.id + "/main.lua";
    WiFiClientSecure sc;
    sc.setInsecure();
    HTTPClient http;
    http.begin(sc, lua_url);
    http.useHTTP10(true);
    http.setTimeout(20000);
    int code = http.GET();
    if (code != 200) { http.end(); return false; }

    File lf = SPIFFS.open(_lua_pad(wm.id), "w");
    if (!lf) { http.end(); return false; }

    // Stream in blokken schrijven totdat verbinding sluit
    WiFiClientSecure* stream = (WiFiClientSecure*)http.getStreamPtr();
    uint8_t buf[512];
    int len;
    unsigned long deadline = millis() + 20000;
    while (millis() < deadline) {
        len = stream->readBytes(buf, sizeof(buf));
        if (len > 0) { lf.write(buf, len); deadline = millis() + 5000; }
        else if (!stream->connected()) break;
        else delay(10);
    }
    lf.close();
    http.end();

    int bestaand = app_vindt(wm.id);
    int nieuw_idx = (bestaand >= 0) ? bestaand : apps_cnt;
    if (nieuw_idx >= APP_MAX) return false;
    if (bestaand < 0) apps_cnt++;

    apps[nieuw_idx] = wm;
    apps[nieuw_idx].actief = true;
    app_manifest_opslaan(nieuw_idx);
    _index_opslaan();
    return true;
}

bool app_installeer_uit_winkel(int winkel_idx) {
    return app_installeer_op_spiffs(winkel_idx);
}

// ─── Pad helper ───────────────────────────────────────────────────────────────
String app_pad(const char* id) {
    return _lua_pad(id);
}

// ─── Storage info ─────────────────────────────────────────────────────────────
size_t app_spiffs_vrij() {
    return SPIFFS.totalBytes() - SPIFFS.usedBytes();
}

size_t app_spiffs_totaal() {
    return SPIFFS.totalBytes();
}

bool app_sd_aanwezig() {
    if (!_sd_geinitialiseerd) {
        _sd_geinitialiseerd = true;
        // 1-bit modus, geen pin-remapping — lukt alleen als kaart aanwezig is
        _sd_aanwezig = SD_MMC.begin("/sdcard", true);
        if (!_sd_aanwezig) SD_MMC.end();
    }
    return _sd_aanwezig;
}

size_t app_sd_vrij() {
    if (!app_sd_aanwezig()) return 0;
    return (size_t)SD_MMC.totalBytes() - (size_t)SD_MMC.usedBytes();
}
