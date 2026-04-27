#include "ui_colors.h"

// ─── Runtime variabelen (worden gevuld door palette_toepassen) ───────────
uint16_t C_BG;
uint16_t C_SURFACE;
uint16_t C_SURFACE2;
uint16_t C_SURFACE3;
uint16_t C_STATUSBAR;
uint16_t C_NAVBAR;
uint16_t C_TEXT;
uint16_t C_TEXT_DIM;
uint16_t C_TEXT_DARK;
uint16_t C_DARK_GRAY;
uint16_t C_CYAN;
uint16_t C_NAV_ACTIVE;
uint16_t C_NAV_NORMAL;

// ─── Paletdefinities ─────────────────────────────────────────────────────
struct Palette {
    uint16_t bg, surface, surface2, surface3;
    uint16_t statusbar;
    uint16_t text, text_dim, text_dark, dark_gray;
    uint16_t accent;
};

static const Palette paletten[PALETTE_CNT] = {
    // 0: MARINE — donker marine blauw (standaard)
    { RGB565(8,12,25),    RGB565(18,28,52),   RGB565(28,42,78),   RGB565(40,58,100),
      RGB565(12,18,38),
      RGB565(200,220,255), RGB565(100,130,160), RGB565(20,30,55), RGB565(40,50,70),
      RGB565(0,200,230) },
    // 1: ROOD — donker rood als overheersende achtergrondkleur
    { RGB565(18,3,3),     RGB565(42,8,8),     RGB565(65,14,14),   RGB565(88,22,22),
      RGB565(12,2,2),
      RGB565(255,205,205), RGB565(158,72,72),  RGB565(25,5,5),    RGB565(55,14,14),
      RGB565(255,55,55) },
    // 2: GOUD — donker goud/amber als overheersende achtergrondkleur
    { RGB565(18,12,2),    RGB565(40,28,5),    RGB565(62,44,8),    RGB565(84,60,12),
      RGB565(12,8,1),
      RGB565(255,242,200), RGB565(158,128,55), RGB565(26,18,2),   RGB565(58,38,8),
      RGB565(255,185,0) },
    // 3: BLAUW — diep kobaltblauw (anders dan marine grijs-blauw)
    { RGB565(2,4,20),     RGB565(5,10,48),    RGB565(8,18,76),    RGB565(12,28,102),
      RGB565(1,3,14),
      RGB565(195,210,255), RGB565(78,105,188), RGB565(4,8,48),    RGB565(16,30,70),
      RGB565(50,120,255) },
    // 4: GROEN — donker bosgroen als overheersende achtergrondkleur
    { RGB565(2,12,2),     RGB565(5,28,5),     RGB565(8,44,8),     RGB565(12,60,12),
      RGB565(1,8,1),
      RGB565(200,255,205), RGB565(68,150,72),  RGB565(3,20,3),    RGB565(12,45,12),
      RGB565(0,215,75) },
    // 5: WIT — licht thema met lichtgrijze achtergrond
    { RGB565(225,230,240), RGB565(205,213,228), RGB565(182,193,212), RGB565(158,171,196),
      RGB565(185,196,218),
      RGB565(18,22,40),  RGB565(78,88,112),   RGB565(205,215,235), RGB565(135,145,170),
      RGB565(0,88,195) },
    // 6: NACHT — minimaal rood licht voor nachtzicht
    { RGB565(0,0,0),      RGB565(14,4,4),     RGB565(26,7,7),     RGB565(38,10,10),
      RGB565(8,2,2),
      RGB565(200,55,55), RGB565(115,28,28),   RGB565(10,2,2),    RGB565(24,7,7),
      RGB565(175,18,18) },
};

uint16_t palette_accent(byte schema) {
    if (schema >= PALETTE_CNT) schema = 0;
    return paletten[schema].accent;
}
uint16_t palette_bg(byte schema) {
    if (schema >= PALETTE_CNT) schema = 0;
    return paletten[schema].bg;
}
uint16_t palette_text(byte schema) {
    if (schema >= PALETTE_CNT) schema = 0;
    return paletten[schema].text;
}

void palette_toepassen(byte schema) {
    if (schema >= PALETTE_CNT) schema = 0;
    const Palette& p = paletten[schema];
    C_BG        = p.bg;
    C_SURFACE   = p.surface;
    C_SURFACE2  = p.surface2;
    C_SURFACE3  = p.surface3;
    C_STATUSBAR = p.statusbar;
    C_NAVBAR    = p.statusbar;
    C_TEXT      = p.text;
    C_TEXT_DIM  = p.text_dim;
    C_TEXT_DARK = p.text_dark;
    C_DARK_GRAY = p.dark_gray;
    C_CYAN      = p.accent;
    C_NAV_ACTIVE = p.accent;
    C_NAV_NORMAL = p.text_dim;
}
