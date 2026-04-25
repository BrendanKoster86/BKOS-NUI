#include "hardware.h"
#include "screen_main.h"
#include "screen_io.h"
#include "screen_config.h"
#include "screen_ota.h"
#include "nav_bar.h"

static bool vorige_touch   = false;
static bool touch_verwerkt = false;

void hw_setup() {
    Serial.begin(115200);
    tft_setup();
    ts_setup();
    hw_io_setup();
    state_load();
    tft_helderheid_zet(tft_helderheid);  // herstel opgeslagen helderheid

    // Welkomstscherm
    tft.fillScreen(C_BG);
    tft_logo(TFT_W / 2 - 100, TFT_H / 2 - 50, 1, C_CYAN);
    tft.setTextSize(2);
    tft.setTextColor(C_TEXT);
    tft.setCursor(TFT_W / 2 - 100, TFT_H / 2 + 40);
    tft.print("BKOS-NUI  ");
    tft.print(BKOS_NUI_VERSIE);
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(TFT_W / 2 - 80, TFT_H / 2 + 62);
    tft.print("WiFi verbinden...");

    wifi_setup();
    ntp_setup();
    ota_setup();
    io_boot();

    scherm_bouwen = true;
    actief_scherm = SCREEN_MAIN;
}

void hw_loop() {
    io_loop();
    wifi_loop();
    ota_loop();

    bool aanraking = ts_touched();

    // Scherm (her)bouwen
    if (scherm_bouwen) {
        scherm_bouwen = false;
        touch_verwerkt = false;
        switch (actief_scherm) {
            case SCREEN_MAIN:   screen_main_teken();   break;
            case SCREEN_IO:     screen_io_teken();     break;
            case SCREEN_CONFIG: screen_config_teken(); break;
            case SCREEN_OTA:    screen_ota_teken();    break;
            case SCREEN_INFO:   screen_info_teken();   break;
            case SCREEN_WIFI:   screen_wifi_teken();   break;
        }
    }

    // Touch: eerste touch na scherm-wake overslaan
    if (aanraking && !vorige_touch) {
        touch_verwerkt = false;
    }
    if (scherm_net_gewekt && aanraking) {
        scherm_net_gewekt = false;
        touch_verwerkt = true;  // consumeer wake-touch
    } else if (aanraking && !touch_verwerkt) {
        touch_verwerkt = true;
        switch (actief_scherm) {
            case SCREEN_MAIN:   screen_main_run(ts_x, ts_y, true);   break;
            case SCREEN_IO:     screen_io_run(ts_x, ts_y, true);     break;
            case SCREEN_CONFIG: screen_config_run(ts_x, ts_y, true); break;
            case SCREEN_OTA:    screen_ota_run(ts_x, ts_y, true);    break;
            case SCREEN_INFO:   screen_info_run(ts_x, ts_y, true);   break;
            case SCREEN_WIFI:   screen_wifi_run(ts_x, ts_y, true);   break;
        }
    }

    // Geen aanraking: periodieke updates
    if (!aanraking) {
        touch_verwerkt = false;
        switch (actief_scherm) {
            case SCREEN_MAIN:   screen_main_run(0, 0, false);   break;
            case SCREEN_IO:     screen_io_run(0, 0, false);     break;
            case SCREEN_OTA:    screen_ota_run(0, 0, false);    break;
            default: break;
        }
    }

    vorige_touch = aanraking;
    tft_loop();
}
