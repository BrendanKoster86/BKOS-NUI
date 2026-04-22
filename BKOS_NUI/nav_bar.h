#pragma once
#include "ui_draw.h"
#include "app_state.h"

#define NAV_ITEMS 4
static const char* nav_labels[NAV_ITEMS] = {"PANEEL", "IO", "CONFIG", "OTA"};

void nav_bar_teken();
int  nav_bar_klik(int x, int y);  // geeft scherm index of -1
