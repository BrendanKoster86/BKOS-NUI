#pragma once
#include "ui_draw.h"
#include "app_state.h"
#include "io.h"

// Boot paneel (linker helft)
#define BDX   5
#define BDY   (CONTENT_Y + 5)
#define BDW   (BOOT_PANEL_W - 10)   // 380
#define BDH   (CONTENT_H - 10)      // 386

// Boot tekening schaal: raw x 0..120, y 0..165 → display (schaal 1.75 = *7/4)
// Geeft 210px breed, ~289px hoog — vrije ruimte rondom boot
#define BOOT_BX_OFF  90   // BDX + (BDW-210)/2 = 5+85
#define BOOT_BY_OFF  67   // BDY + top marge 20px
#define BOOT_BX(x)   (BOOT_BX_OFF + ((x)*7)/4)
#define BOOT_BY(y)   (BOOT_BY_OFF + ((y)*7)/4)
#define BOOT_LICHT_R  8   // iets kleiner bij schaal 1.75

// Licht indicator posities (ruwe bootcoördinaten)
#define BL_ANKER_RX  67
#define BL_ANKER_RY   2
#define BL_STOOM_RX  67
#define BL_STOOM_RY  50
#define BL_NAVI_RX    2
#define BL_NAVI_RY  148
#define BL_HEK_RX   113
#define BL_HEK_RY   142

// Vaarmodus knoppen (rechter paneel, 2x2 grid)
#define MKNOP_W   186
#define MKNOP_H    68
#define MKNOP_GAP   8
#define MKNOP_X1  (CTRL_PANEL_X + 10)
#define MKNOP_X2  (MKNOP_X1 + MKNOP_W + MKNOP_GAP)
#define MKNOP_Y1  (CONTENT_Y + 8)
#define MKNOP_Y2  (MKNOP_Y1 + MKNOP_H + MKNOP_GAP)

// Verlichting knoppen (3 naast elkaar)
#define LKNOP_W   122
#define LKNOP_H    52
#define LKNOP_X1  (CTRL_PANEL_X + 11)
#define LKNOP_X2  (LKNOP_X1 + LKNOP_W + 6)
#define LKNOP_X3  (LKNOP_X2 + LKNOP_W + 6)
#define LKNOP_Y   (MKNOP_Y2 + MKNOP_H + 14)

// Apparaat knoppen (USB/230V/TV/water/deklicht)
#define DKNOP_W   122
#define DKNOP_H    52
#define DKNOP_X1  (CTRL_PANEL_X + 11)
#define DKNOP_X2  (DKNOP_X1 + DKNOP_W + 6)
#define DKNOP_X3  (DKNOP_X2 + DKNOP_W + 6)
#define DKNOP2_X1 (CTRL_PANEL_X + 75)
#define DKNOP2_X2 (DKNOP2_X1 + DKNOP_W + 6)
#define DKNOP_Y1  (LKNOP_Y + LKNOP_H + 16)
#define DKNOP_Y2  (DKNOP_Y1 + DKNOP_H + 6)

// Interieur status balk
#define INT_STATUS_Y  (DKNOP_Y2 + DKNOP_H + 8)

#if PLATFORM_PICO
// ─── Pico portret-layout (240×320) ────────────────────────────────────────
// Content: CONTENT_Y=24 .. NAV_Y=284, hoogte=260
#define PICO_LEFT_W   120   // linker paneel (boot)
#define PICO_RIGHT_X  120   // rechter paneel start
#define PICO_RIGHT_W  120   // rechter paneel breedte

// Boot tekening op Pico: schaal ×7/10, raw 0..120 × 0..165
// Uitkomst: 84px breed, 115px hoog — gecentreerd in linker paneel
#define PICO_BOOT_BX_OFF  18                           // (120-84)/2
#define PICO_BOOT_BY_OFF  (CONTENT_Y + 10)
#define PICO_BOOT_BX(x)   (PICO_BOOT_BX_OFF + ((x)*7)/10)
#define PICO_BOOT_BY(y)   (PICO_BOOT_BY_OFF + ((y)*7)/10)
#define PICO_BOOT_LICHT_R  4

// Vaarmodus knoppen: 4 gestapeld in rechter kolom
#define PICO_MKNOP_X    (PICO_RIGHT_X + 4)
#define PICO_MKNOP_W    112
#define PICO_MKNOP_H    38
#define PICO_MKNOP_Y0   (CONTENT_Y + 4)
#define PICO_MKNOP_Y(i) (PICO_MKNOP_Y0 + (i) * (PICO_MKNOP_H + 4))

// Verlichting: 1 cyclische knop
#define PICO_LKNOP_X    (PICO_RIGHT_X + 4)
#define PICO_LKNOP_W    112
#define PICO_LKNOP_H    24
#define PICO_LKNOP_Y    (PICO_MKNOP_Y(4) + 6)         // = 24+4+4*42+6 = 198

// Apparaat knoppen: 2×2 grid (WATER, TV, USB, 230V)
#define PICO_DKNOP_W    54
#define PICO_DKNOP_H    24
#define PICO_DKNOP_X1   (PICO_RIGHT_X + 4)
#define PICO_DKNOP_X2   (PICO_DKNOP_X1 + PICO_DKNOP_W + 4)
#define PICO_DKNOP_Y1   (PICO_LKNOP_Y + PICO_LKNOP_H + 6)
#define PICO_DKNOP_Y2   (PICO_DKNOP_Y1 + PICO_DKNOP_H + 4)
#endif

void screen_main_teken();
void screen_main_run(int x, int y, bool aanraking);
void screen_main_update_boot();
void screen_main_update_controls();
void screen_main_lang_indruk(int x, int y);
void boot_teken();
void boot_lichten_teken();
