#include "wifi.h"
#include "hw_scherm.h"
#include "ui_colors.h"

// Gedeclareerd in app_state.ino
bool wifi_aangesloten = false;

static unsigned long wifi_last_check = 0;

void wifi_setup() {
    WiFiManager wm;
    wm.setConfigPortalTimeout(60);
    wm.setConnectTimeout(20);
    if (!wm.autoConnect("BKOS-NUI-Setup")) {
        wifi_verbonden = false;
    } else {
        wifi_verbonden = true;
    }
}

void wifi_loop() {
    if (millis() - wifi_last_check < 5000) return;
    wifi_last_check = millis();
    wifi_verbonden = (WiFi.status() == WL_CONNECTED);
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
