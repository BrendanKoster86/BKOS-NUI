#include "app_state.h"
#include "hw_io.h"

int   actief_scherm    = SCREEN_MAIN;
bool  scherm_bouwen    = true;
byte  vaar_modus       = MODE_HAVEN;
byte  licht_instelling = LICHT_UIT;
bool  ota_push_actief  = false;
bool  updaten          = false;
String klok_tijd       = "--:--";
bool  wifi_verbonden   = false;
bool  dev_lokaal[5]    = {false, false, false, false, false};

static Preferences prefs_state;

void state_save() {
    prefs_state.begin("bkos_state", false);
    prefs_state.putUChar("modus",  vaar_modus);
    prefs_state.putUChar("licht",  licht_instelling);
    prefs_state.putInt("helderh",  tft_helderheid);
    prefs_state.putLong("timer",   scherm_timer);
    for (int i = 0; i < 5; i++) {
        char k[6]; snprintf(k, sizeof(k), "dev%d", i);
        prefs_state.putBool(k, dev_lokaal[i]);
    }
    prefs_state.end();
}

void state_load() {
    prefs_state.begin("bkos_state", true);
    vaar_modus       = prefs_state.getUChar("modus",   MODE_HAVEN);
    licht_instelling = prefs_state.getUChar("licht",   LICHT_UIT);
    tft_helderheid   = prefs_state.getInt("helderh",   75);
    scherm_timer     = prefs_state.getLong("timer",    30);
    for (int i = 0; i < 5; i++) {
        char k[6]; snprintf(k, sizeof(k), "dev%d", i);
        dev_lokaal[i] = prefs_state.getBool(k, false);
    }
    prefs_state.end();
}
