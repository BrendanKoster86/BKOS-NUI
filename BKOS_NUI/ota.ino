#include "ota.h"
#include "hw_scherm.h"
#include "ui_colors.h"

bool   ota_wifi_actief   = false;
// ota_push_actief en updaten gedeclareerd in app_state.ino
String ota_versie_github = "";
String ota_status_tekst  = "Niet gecontroleerd";

static unsigned long ota_last_git_check = 0;
#define OTA_GIT_INTERVAL  300000UL  // 5 minuten

void ota_setup() {
    // ArduinoOTA push is standaard uitgeschakeld
    // Activeer alleen via scherm_ota als de gebruiker het inschakelt
}

void ota_loop() {
    if (ota_push_actief) ArduinoOTA.handle();
    if (millis() - ota_last_git_check > OTA_GIT_INTERVAL) {
        ota_last_git_check = millis();
        if (wifi_verbonden && strlen(OTA_GITHUB_VERSIE_URL) > 5) {
            ota_git_check();
        }
    }
}

void ota_push_inschakelen(bool aan) {
    ota_push_actief = aan;
    if (aan) {
        ArduinoOTA.setHostname("BKOS-NUI");
        ArduinoOTA.setPassword("bkos2025");
        ArduinoOTA.begin();
        ota_status_tekst = "Push OTA actief";
    } else {
        ota_status_tekst = "Push OTA uit";
    }
}

void ota_git_check() {
    if (!wifi_verbonden) { ota_status_tekst = "Geen WiFi"; return; }
    HTTPClient http;
    http.begin(OTA_GITHUB_VERSIE_URL);
    int code = http.GET();
    if (code == HTTP_CODE_OK) {
        ota_versie_github = http.getString();
        ota_versie_github.trim();
        if (ota_versie_github == BKOS_NUI_VERSIE) {
            ota_status_tekst = "Up to date (" + ota_versie_github + ")";
        } else {
            ota_status_tekst = "Update beschikbaar: " + ota_versie_github;
        }
    } else {
        ota_status_tekst = "GitHub fout " + String(code);
    }
    http.end();
}

void ota_git_update() {
    if (!wifi_verbonden) return;
    ota_status_tekst = "Downloaden...";
    ota_download_toepassen(String(OTA_GITHUB_FIRMWARE_URL));
}

bool ota_download_toepassen(String url) {
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(url);
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        ota_status_tekst = "Download fout " + String(code);
        http.end();
        return false;
    }
    int len = http.getSize();
    if (len <= 0) { http.end(); return false; }

    if (!Update.begin(len)) { http.end(); return false; }

    WiFiClient* stream = http.getStreamPtr();
    size_t written = 0;
    uint8_t buf[512];
    unsigned long last_data = millis();

    while (written < (size_t)len) {
        if (stream->available()) {
            size_t rd = stream->read(buf, sizeof(buf));
            if (rd > 0) { Update.write(buf, rd); written += rd; last_data = millis(); }
        }
        if (millis() - last_data > 15000) { Update.abort(); http.end(); return false; }
        yield();
    }
    http.end();
    if (!Update.end()) return false;
    ESP.restart();
    return true;
}
