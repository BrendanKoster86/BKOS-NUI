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
// Links 180px: boot   |   Rechts 60px: vaarmodus + licht knoppen
// Onderaan (volle breedte): 1 rij apparaat knoppen

#define PICO_LEFT_W   180   // linker paneel (boot)
#define PICO_RIGHT_X  180   // rechter paneel start
#define PICO_RIGHT_W  60    // rechter paneel breedte

// Boot tekening: 1:1 schaal (raw 0..120 × 0..165), gecentreerd in 180px
#define PICO_BOOT_BX_OFF  30                       // (180-120)/2
#define PICO_BOOT_BY_OFF  (CONTENT_Y + 10)
#define PICO_BOOT_BX(x)   (PICO_BOOT_BX_OFF + (x))
#define PICO_BOOT_BY(y)   (PICO_BOOT_BY_OFF + (y))

// Vaarmodus knoppen: 4 gestapeld in rechter kolom (60px breed)
#define PICO_MKNOP_X    (PICO_RIGHT_X + 4)            // 184
#define PICO_MKNOP_W    (PICO_RIGHT_W - 8)             // 52
#define PICO_MKNOP_H    38
#define PICO_MKNOP_Y(i) (CONTENT_Y + 4 + (i) * (PICO_MKNOP_H + 4))

// Verlichting cycling knop (onder vaarmodus, zelfde breedte)
#define PICO_LKNOP_X    (PICO_RIGHT_X + 4)            // 184
#define PICO_LKNOP_W    (PICO_RIGHT_W - 8)             // 52
#define PICO_LKNOP_H    22
#define PICO_LKNOP_Y    (PICO_MKNOP_Y(4) + 4)         // 28+168+4 = 200

// Apparaat knoppen: 1 rij volledige breedte, verankerd boven nav bar
// 4 knoppen × 55px + 5×4px marge = 240px
#define PICO_DKNOP_W    55
#define PICO_DKNOP_H    34
#define PICO_DKNOP_X(i) (4 + (i) * (PICO_DKNOP_W + 4))  // 4, 63, 122, 181
#define PICO_DKNOP_Y    (NAV_Y - 4 - PICO_DKNOP_H)       // 284-4-34 = 246
#endif

void screen_main_teken();
void screen_main_run(int x, int y, bool aanraking);
void screen_main_update_boot();
void screen_main_update_controls();
void screen_main_lang_indruk(int x, int y);
void boot_teken();
void boot_lichten_teken();
