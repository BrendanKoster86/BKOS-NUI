#pragma once
#include <Arduino.h>
#include <Preferences.h>

// Actief scherm
#define SCREEN_MAIN    0
#define SCREEN_IO      1
#define SCREEN_CONFIG  2
#define SCREEN_OTA     3
#define SCREEN_INFO    4
#define SCREEN_WIFI    5  // niet in nav bar, toegankelijk via OTA scherm

// Vaarmodi
#define MODE_HAVEN   0
#define MODE_ZEILEN  1
#define MODE_MOTOR   2
#define MODE_ANKER   3

// Verlichting instelling
#define LICHT_UIT   0
#define LICHT_AAN   1
#define LICHT_AUTO  2

// IO output stadia (per kanaal)
#define IO_UIT         0  // uit
#define IO_AAN         1  // aan
#define IO_INV_UIT     2  // inverteer uit (hoog = uit)
#define IO_INV_AAN     3  // inverteer aan (laag = aan)
#define IO_GEBLOKKEERD 4  // geblokkeerd laag
#define IO_INV_GEBLOKKEERD 5 // geblokkeerd hoog

// Max IO kanalen
#define MAX_IO_KANALEN 240  // 30 modules × 8
#define IO_NAAM_LEN    12

// Lichtstaat (visueel, gecombineerd output+input)
#define LSTATE_ECHT_UIT      0  // output=0, input=0 → echt uit
#define LSTATE_KOELT_AF      1  // output=0, input=1 → uit maar voelt nog aan
#define LSTATE_GEEN_SIGNAAL  2  // output=1, input=0 → aan maar geen signaal
#define LSTATE_ECHT_AAN      3  // output=1, input=1 → echt aan

extern int    actief_scherm;
extern bool   scherm_bouwen;

extern byte   vaar_modus;
extern byte   licht_instelling;
extern bool   ota_push_actief;

// IO state
extern int    io_kanalen_cnt;
extern byte   io_output[];
extern bool   io_input[];
extern bool   io_gewijzigd[];
extern char   io_namen[][IO_NAAM_LEN];

// OTA
extern bool   ota_wifi_actief;
extern bool   updaten;

// Klok
extern String klok_tijd;
extern bool   wifi_verbonden;

// Apparaat lokale staat (fallback als geen IO module)
extern bool dev_lokaal[5];

// Preferences opslaan
void state_save();
void state_load();
byte licht_staat(int kanaal);
