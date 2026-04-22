#pragma once
#include "ui_draw.h"
#include "app_state.h"
#include "hw_io.h"

#define CFG_RIJEN_PER_PAG  6
#define CFG_RIJ_H          52

extern int cfg_scroll;
extern int cfg_geselecteerd;
extern bool cfg_toetsenbord_actief;
extern char cfg_invoer[IO_NAAM_LEN];

void screen_config_teken();
void screen_config_run(int x, int y, bool aanraking);
void screen_config_rijen_teken();
void screen_config_toetsenbord_teken();
bool screen_config_toetsenbord_run(int x, int y);
