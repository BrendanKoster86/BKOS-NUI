#include "wifi.h"
#include "hw_scherm.h"
#include "ui_colors.h"
#include "app_state.h"

bool wifi_aangesloten = false;

static unsigned long wifi_last_check = 0;
static unsigned long ntp_last_sync   = 0;
static bool          ntp_gesync      = false;

void wifi_setup() {
    // Probeer opgeslagen credentials
    Preferences wprefs;
    wprefs.begin("wifi_creds", true);
    String opgeslagen_ssid = wprefs.getString("ssid", "");
    String opgeslagen_pass = wprefs.getString("pass", "");
    wprefs.end();

    if (opgeslagen_ssid.length() > 0) {
        WiFi.begin(opgeslagen_ssid.c_str(), opgeslagen_pass.c_str());
        unsigned long t = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t < 12000) delay(300);
        if (WiFi.status() == WL_CONNECTED) { wifi_verbonden = true; return; }
    }

    // WiFiManager als fallback
    WiFiManager wm;
    wm.setConfigPortalTimeout(90);
    wm.setConnectTimeout(20);
    if (wm.autoConnect("BKOS-NUI-Setup")) {
        wifi_verbonden = true;
        wprefs.begin("wifi_creds", false);
        wprefs.putString("ssid", WiFi.SSID());
        wprefs.putString("pass", WiFi.psk());
        wprefs.end();
    } else {
        wifi_verbonden = false;
    }
}

bool wifi_verbind(const char* ssid, const char* wachtwoord) {
    WiFi.disconnect(true);
    delay(200);
    WiFi.begin(ssid, wachtwoord);
    unsigned long t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 15000) delay(300);
    wifi_verbonden = (WiFi.status() == WL_CONNECTED);
    if (wifi_verbonden) {
        Preferences wprefs;
        wprefs.begin("wifi_creds", false);
        wprefs.putString("ssid", ssid);
        wprefs.putString("pass", wachtwoord);
        wprefs.end();
        ntp_gesync = false;
    }
    return wifi_verbonden;
}

void wifi_loop() {
    if (millis() - wifi_last_check < 5000) return;
    wifi_last_check = millis();
    wifi_verbonden = (WiFi.status() == WL_CONNECTED);
    if (wifi_verbonden && !ntp_gesync) ntp_setup();
    ntp_loop();
}

void wifi_reset() {
    Preferences wprefs;
    wprefs.begin("wifi_creds", false);
    wprefs.clear();
    wprefs.end();
    WiFiManager wm;
    wm.resetSettings();
    ESP.restart();
}

bool wifi_check() {
    wifi_verbonden = (WiFi.status() == WL_CONNECTED);
    if (!wifi_verbonden) {
        WiFi.reconnect();
        unsigned long t = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t < 10000) delay(200);
        wifi_verbonden = (WiFi.status() == WL_CONNECTED);
    }
    return wifi_verbonden;
}

void ntp_setup() {
    if (!wifi_verbonden) return;
    configTime(NTP_GMT_OFFSET, NTP_DST_OFFSET, NTP_SERVER1, NTP_SERVER2);
    ntp_gesync = false;
}

void ntp_loop() {
    if (!wifi_verbonden) return;
    if (millis() - ntp_last_sync < 30000) return;
    ntp_last_sync = millis();
    struct tm t;
    if (getLocalTime(&t, 1000)) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
        klok_tijd  = String(buf);
        ntp_gesync = true;
    }
}
