#include "hardware.h"
#include "screen_main.h"
#include "screen_io.h"
#include "screen_meteo.h"
#include "screen_config.h"
#include "screen_ota.h"
#include "screen_info.h"
#include "screen_apps.h"
#include "meteo.h"
#include "nav_bar.h"
#include "data_store.h"
#include "app_manager.h"
#include "lua_runtime.h"
#include "fout_log.h"

static bool          vorige_touch        = false;
static bool          touch_verwerkt      = false;
static unsigned long laatste_touch_ms    = 0;
#define TOUCH_DEBOUNCE_MS  320   // minimale tijd tussen twee aparte aanrakingen

void hw_setup() {
    tft_setup();
    ts_setup();
    hw_io_setup();
    state_load();
    palette_toepassen(kleurenschema);
    tft_helderheid_zet(tft_helderheid);

    // Splash scherm
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
    tft.print("Opstarten...");

    info_laden();       // boot naam en eigenaar uit SPIFFS (voor status bar)
    data_setup();       // gestructureerde data-opslag laden
    meteo_setup();      // laadt NVS-instellingen (snel, geen netwerk)
    ota_setup();        // init OTA (snel)
    fout_log_setup();   // laad foutrapportage token uit Preferences
    io_boot();          // BKOSS check + UART IO discovery
    app_setup();        // app-manifesten laden + Lua runtime initialiseren

    // Splash: BKOSS status tonen
    tft.setTextSize(1);
    if (bkoss_actief) {
        tft.setTextColor(C_GREEN);
        tft.setCursor(TFT_W / 2 - 80, TFT_H / 2 + 80);
        tft.print("BKOSS ");
        tft.print(bkoss_versie);
        tft.print(" — ");
        tft.print(io_aparaten_cnt);
        tft.print(" module(s), ");
        tft.print(io_kanalen_cnt);
        tft.print(" kanalen");
    } else {
        tft.setTextColor(C_RED_BRIGHT);
        tft.setCursor(TFT_W / 2 - 100, TFT_H / 2 + 80);
        tft.print("! BKOSS module niet gevonden");
    }

    // Start netwerk taak op Core 0 (niet-blokkerend)
    wifi_taak_start();

    delay(1000);     // splash tonen

    scherm_bouwen = true;
    actief_scherm = SCREEN_MAIN;
}

void hw_loop() {
    // Touch en scherm-wake altijd EERST lezen — vóór blokkerende IO/WiFi calls
    // zodat een korte tap op het donkere scherm nooit gemist wordt
    bool aanraking = ts_touched();
    tft_loop();

    io_loop();
    ntp_loop();
    ota_loop();

    // Scherm (her)bouwen
    if (scherm_bouwen) {
        scherm_bouwen = false;
        touch_verwerkt = false;
        int app_idx = app_voor_scherm(actief_scherm);
        if (app_idx >= 0) {
            // Lua-app vervangt dit scherm: laad alleen bij schermwissel
            static int lua_geladen_voor = -1;
            static int lua_geladen_app  = -1;
            if (actief_scherm != lua_geladen_voor || app_idx != lua_geladen_app) {
                lua_app_laden(app_idx);
                lua_geladen_voor = actief_scherm;
                lua_geladen_app  = app_idx;
            }
            lua_app_teken(app_idx);
        } else {
            switch (actief_scherm) {
                case SCREEN_MAIN:   screen_main_teken();   break;
                case SCREEN_IO:     screen_io_teken();     break;
                case SCREEN_METEO:  screen_meteo_teken();  break;
                case SCREEN_CONFIG: screen_config_teken(); break;
                case SCREEN_OTA:    screen_ota_teken();    break;
                case SCREEN_INFO:   screen_info_teken();   break;
                case SCREEN_WIFI:   screen_wifi_teken();   break;
                case SCREEN_IO_CFG: screen_io_cfg_teken(); break;
                case SCREEN_APPS:   screen_apps_teken();   break;
            }
        }
    }

    // Nieuwe aanraking: reset verwerkt-vlag
    if (aanraking && !vorige_touch) {
        touch_verwerkt = false;
    }

    // Wake-touch consumeren (eerste touch na donker scherm)
    if (scherm_net_gewekt && aanraking) {
        scherm_net_gewekt = false;
        touch_verwerkt = true;
        laatste_touch_ms = millis();  // debounce zodat vastgehouden vinger ook genegeerd wordt
    } else if (aanraking && !touch_verwerkt) {
        // Debounce: minimale tijd tussen twee aparte aanrakingen
        if (millis() - laatste_touch_ms >= TOUCH_DEBOUNCE_MS) {
            touch_verwerkt = true;
            laatste_touch_ms = millis();
            {
                int app_idx = app_voor_scherm(actief_scherm);
                if (app_idx >= 0) {
                    // Nav bar altijd bereikbaar (bovenste schermhelft = app, onder = nav)
                    int nav = nav_bar_klik(ts_x, ts_y);
                    if (nav >= 0 && nav != actief_scherm) {
                        lua_app_sluiten();
                        actief_scherm = nav; scherm_bouwen = true;
                    } else {
                        lua_app_run(app_idx, ts_x, ts_y, true);
                    }
                } else {
                    switch (actief_scherm) {
                        case SCREEN_MAIN:   screen_main_run(ts_x, ts_y, true);   break;
                        case SCREEN_IO:     screen_io_run(ts_x, ts_y, true);     break;
                        case SCREEN_METEO:  screen_meteo_run(ts_x, ts_y, true);  break;
                        case SCREEN_CONFIG: screen_config_run(ts_x, ts_y, true); break;
                        case SCREEN_OTA:    screen_ota_run(ts_x, ts_y, true);    break;
                        case SCREEN_INFO:   screen_info_run(ts_x, ts_y, true);   break;
                        case SCREEN_WIFI:   screen_wifi_run(ts_x, ts_y, true);   break;
                        case SCREEN_IO_CFG: screen_io_cfg_run(ts_x, ts_y, true); break;
                        case SCREEN_APPS:   screen_apps_run(ts_x, ts_y, true);   break;
                    }
                }
            }
        } else {
            touch_verwerkt = true;  // te snel na vorige touch — negeren
        }
    }

    // Geen aanraking: periodieke updates
    if (!aanraking) {
        touch_verwerkt = false;
        int app_upd = app_voor_scherm(actief_scherm);
        if (app_upd >= 0) {
            lua_app_run(app_upd, 0, 0, false);
        } else {
            switch (actief_scherm) {
                case SCREEN_MAIN:   screen_main_run(0, 0, false);   break;
                case SCREEN_IO:     screen_io_run(0, 0, false);     break;
                case SCREEN_OTA:    screen_ota_run(0, 0, false);    break;
                default: break;
            }
        }
    }

    // Periodieke data-opslag (elke 60s als er wijzigingen zijn)
    static unsigned long data_opgeslagen_ms = 0;
    if (millis() - data_opgeslagen_ms >= 60000) {
        data_opgeslagen_ms = millis();
        data_opslaan();
    }

    // WiFi OTA modus aan/uit o.b.v. actief scherm (SCREEN_OTA = 6, niet meer in nav bar)
    static int vorig_scherm = -1;
    if (actief_scherm != vorig_scherm) {
        vorig_scherm = actief_scherm;
        bool ota_scherm = (actief_scherm == SCREEN_OTA);
        wifi_ota_zet(ota_scherm || ota_push_actief);
    }

    vorige_touch = aanraking;
}
