#include "fout_log.h"
#include "app_state.h"
#include "ota.h"
#include <Preferences.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <mbedtls/sha256.h>

static char          _token[120] = "";
static unsigned long _laatste_ms = 0;
static volatile bool _bezig      = false;

#define FLOG_COOLDOWN 60000UL   // max 1 issue per minuut

struct _FlogPakket {
    FoutType type;
    char     bericht[80];
    char     context[80];
};
static _FlogPakket _pakket;

void fout_log_setup() {
    Preferences prefs;
    prefs.begin("foutlog", true);
    String tok = prefs.getString("token", "");
    prefs.end();
    strncpy(_token, tok.c_str(), sizeof(_token) - 1);
    _token[sizeof(_token) - 1] = '\0';
}

void fout_log_token_zet(const char* token) {
    strncpy(_token, token, sizeof(_token) - 1);
    _token[sizeof(_token) - 1] = '\0';
    Preferences prefs;
    prefs.begin("foutlog", false);
    prefs.putString("token", token);
    prefs.end();
}

bool fout_log_token_aanwezig() {
    return _token[0] != '\0';
}

// SHA-256 van chip eFuse MAC → geanonimiseerde device ID (niet omkeerbaar)
static String _device_id() {
    uint64_t chipid = ESP.getEfuseMac();
    uint8_t  hash[32];
    mbedtls_sha256((const unsigned char*)&chipid, 8, hash, 0);
    char hex[17];
    for (int i = 0; i < 8; i++) snprintf(hex + i * 2, 3, "%02x", hash[i]);
    hex[16] = '\0';
    return String(hex);
}

static const char* _type_naam(FoutType t) {
    switch (t) {
        case FOUT_LUA_RUNTIME: return "LUA_RUNTIME";
        case FOUT_APP_CRASH:   return "APP_CRASH";
        case FOUT_WIFI:        return "WIFI";
        case FOUT_OTA:         return "OTA";
        case FOUT_IO:          return "IO";
        case FOUT_SPIFFS:      return "SPIFFS";
        default:               return "ALGEMEEN";
    }
}

static void _flog_taak(void* param) {
    _FlogPakket* p = (_FlogPakket*)param;

    // Wacht op WiFi (max 30s)
    for (int i = 0; i < 30 && !wifi_verbonden; i++) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    if (!wifi_verbonden) { _bezig = false; vTaskDelete(NULL); return; }

    WiFiClientSecure sc;
    sc.setInsecure();
    HTTPClient http;
    if (!http.begin(sc, FOUT_LOG_API)) { _bezig = false; vTaskDelete(NULL); return; }

    http.setTimeout(15000);
    http.addHeader("Authorization",        String("Bearer ") + _token);
    http.addHeader("Accept",               "application/vnd.github+json");
    http.addHeader("Content-Type",         "application/json");
    http.addHeader("X-GitHub-Api-Version", "2022-11-28");

    const char* tnaam  = _type_naam(p->type);
    String      dev_id = _device_id();

    char body[1400];
    snprintf(body, sizeof(body),
        "{"
          "\"title\":\"[BKOS] %s | v%s\","
          "\"body\":\"## Fouttype\\n%s\\n\\n"
                    "## Bericht\\n%s\\n\\n"
                    "## Context\\n%s\\n\\n"
                    "## Apparaatinformatie\\n"
                    "| Veld | Waarde |\\n"
                    "|------|--------|\\n"
                    "| Firmware versie | %s |\\n"
                    "| Uptime (s) | %lu |\\n"
                    "| Vrij heap (bytes) | %lu |\\n"
                    "| Tijd | %s |\\n"
                    "| Device ID | %s |\","
          "\"labels\":[\"automatisch\",\"onbeoordeeld\"]"
        "}",
        tnaam, BKOS_NUI_VERSIE,
        tnaam,
        p->bericht[0] ? p->bericht : "(geen)",
        p->context[0] ? p->context : "(geen)",
        BKOS_NUI_VERSIE,
        millis() / 1000,
        (unsigned long)ESP.getFreeHeap(),
        klok_tijd.c_str(),
        dev_id.c_str()
    );

    http.POST(body);
    http.end();

    _bezig = false;
    vTaskDelete(NULL);
}

void fout_log_stuur(FoutType type, const char* bericht, const char* context) {
    if (!fout_rapportage)           return;
    if (!fout_log_token_aanwezig()) return;
    if (_bezig)                     return;
    if (millis() - _laatste_ms < FLOG_COOLDOWN) return;

    _laatste_ms  = millis();
    _pakket.type = type;
    strncpy(_pakket.bericht, bericht ? bericht : "", sizeof(_pakket.bericht) - 1);
    _pakket.bericht[sizeof(_pakket.bericht) - 1] = '\0';
    strncpy(_pakket.context, context ? context : "", sizeof(_pakket.context) - 1);
    _pakket.context[sizeof(_pakket.context) - 1] = '\0';

    _bezig = true;
    xTaskCreatePinnedToCore(_flog_taak, "fout_log", 12288, &_pakket, 1, NULL, 0);
}
