#pragma once
#include "ui_draw.h"
#include "app_state.h"
#include "io.h"

// Vaarmodus knop layout (rechter paneel, x van CTRL_PANEL_X)
#define MKNOP_W   165
#define MKNOP_H    72
#define MKNOP_GAP   8
#define MKNOP_X1  (CTRL_PANEL_X + 10)
#define MKNOP_X2  (CTRL_PANEL_X + 10 + MKNOP_W + MKNOP_GAP)
#define MKNOP_Y1  (CONTENT_Y + 14)
#define MKNOP_Y2  (MKNOP_Y1 + MKNOP_H + MKNOP_GAP)

// Licht knoppen
#define LKNOP_W   110
#define LKNOP_H    60
#define LKNOP_Y   (MKNOP_Y2 + MKNOP_H + 20)
#define LKNOP_X1  (CTRL_PANEL_X + 10)
#define LKNOP_X2  (LKNOP_X1 + LKNOP_W + 6)
#define LKNOP_X3  (LKNOP_X2 + LKNOP_W + 6)

// Interieur status (onder licht knoppen)
#define INT_STATUS_Y  (LKNOP_Y + LKNOP_H + 18)

// Boot diagram area
#define BDX   5
#define BDY   (CONTENT_Y + 5)
#define BDW   (BOOT_PANEL_W - 10)
#define BDH   (CONTENT_H - 10)

// Navigatielichten posities (relatief aan bootpaneel)
// Bovenaanzicht boot: links=bakboord(rood), rechts=stuurboord(groen)
#define BL_ANKER_X  (BDX + BDW/2)
#define BL_ANKER_Y  (BDY + 55)
#define BL_MAST_X   (BDX + BDW/2)
#define BL_MAST_Y   (BDY + 120)
#define BL_BB_X     (BDX + 65)
#define BL_BB_Y     (BDY + 210)
#define BL_SB_X     (BDX + BDW - 65)
#define BL_SB_Y     (BDY + 210)
#define BL_HEK_X    (BDX + BDW/2)
#define BL_HEK_Y    (BDY + BDH - 60)
#define BL_STOOM_X  (BDX + BDW/2)
#define BL_STOOM_Y  (BDY + 165)

#define LICHT_R  14   // radius lichtindicatoren

void screen_main_teken();
void screen_main_run(int x, int y, bool aanraking);
void screen_main_update_boot();
void screen_main_update_controls();

// Boot tekening
void boot_teken();
void boot_lichten_teken();
void interieur_status_teken();
