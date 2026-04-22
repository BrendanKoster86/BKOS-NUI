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

static Preferences prefs_state;

void state_save() {
    prefs_state.begin("bkos_state", false);
    prefs_state.putUChar("modus",   vaar_modus);
    prefs_state.putUChar("licht",   licht_instelling);
    prefs_state.end();
}

void state_load() {
    prefs_state.begin("bkos_state", true);
    vaar_modus       = prefs_state.getUChar("modus", MODE_HAVEN);
    licht_instelling = prefs_state.getUChar("licht", LICHT_UIT);
    prefs_state.end();
}

byte licht_staat(int kanaal) {
    bool uit = (io_output[kanaal] == IO_UIT || io_output[kanaal] == IO_INV_UIT);
    bool sig = io_input[kanaal];
    if (uit && !sig)  return LSTATE_ECHT_UIT;
    if (uit && sig)   return LSTATE_KOELT_AF;
    if (!uit && !sig) return LSTATE_GEEN_SIGNAAL;
    return LSTATE_ECHT_AAN;
}
