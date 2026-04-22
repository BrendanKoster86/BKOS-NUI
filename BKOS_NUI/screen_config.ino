#include "screen_config.h"
#include "nav_bar.h"

int  cfg_scroll           = 0;
int  cfg_geselecteerd     = -1;
bool cfg_toetsenbord_actief = false;
char cfg_invoer[IO_NAAM_LEN] = "";

// Eenvoudig touchscreen toetsenbord (QWERTY)
static const char* kb_rijen[4] = {"1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM_*"};
#define KB_X    40
#define KB_Y    (CONTENT_Y + 60)
#define KB_W    (TFT_W - 80)
#define KB_TOETS_H  48

static void cfg_rij_teken(int kanaal, int rij_y) {
    bool geselecteerd = (kanaal == cfg_geselecteerd);
    uint16_t bg = geselecteerd ? C_SURFACE2 : (kanaal % 2 == 0 ? C_SURFACE : C_BG);
    tft.fillRoundRect(10, rij_y + 2, TFT_W - 20, CFG_RIJ_H - 4, 6, bg);
    if (geselecteerd)
        tft.drawRoundRect(10, rij_y + 2, TFT_W - 20, CFG_RIJ_H - 4, 6, C_CYAN);

    // Kanaal nummer
    tft.setTextSize(2);
    tft.setTextColor(C_TEXT_DIM);
    char nr[5];
    snprintf(nr, sizeof(nr), "%d", kanaal);
    tft.setCursor(18, rij_y + (CFG_RIJ_H - 16) / 2);
    tft.print(nr);

    // Naam
    tft.setTextColor(geselecteerd ? C_CYAN : C_TEXT);
    tft.setCursor(70, rij_y + (CFG_RIJ_H - 16) / 2);
    if (kanaal < io_kanalen_cnt && kanaal < MAX_IO_KANALEN) {
        tft.print(io_namen[kanaal]);
    } else {
        tft.setTextColor(C_DARK_GRAY);
        tft.print("(geen module)");
    }

    // Speciale naam label
    if (kanaal < io_kanalen_cnt) {
        String naam = String(io_namen[kanaal]);
        uint16_t tag_kleur = C_SURFACE3;
        const char* tag = nullptr;
        if (naam.startsWith("**L_"))     { tag = "LICHT"; tag_kleur = C_AMBER; }
        if (naam.startsWith("**IL_"))    { tag = "INT";   tag_kleur = C_BLUE; }
        if (naam.startsWith("**haven"))  { tag = "HAVEN"; tag_kleur = C_HAVEN; }
        if (naam.startsWith("**zeilen")) { tag = "ZEIL";  tag_kleur = C_ZEILEN; }
        if (naam.startsWith("**motor"))  { tag = "MOTOR"; tag_kleur = C_MOTOR; }
        if (naam.startsWith("**anker"))  { tag = "ANKER"; tag_kleur = C_ANKER; }
        if (tag) {
            int tx = TFT_W - 120;
            tft.fillRoundRect(tx, rij_y + 10, 100, CFG_RIJ_H - 20, 4, tag_kleur);
            tft.setTextSize(1);
            tft.setTextColor(C_TEXT_DARK);
            int tw = strlen(tag) * 6;
            tft.setCursor(tx + (100 - tw) / 2, rij_y + (CFG_RIJ_H - 8) / 2);
            tft.print(tag);
        }
    }
}

void screen_config_rijen_teken() {
    tft.fillRect(0, CONTENT_Y, TFT_W, CONTENT_H - 0, C_BG);

    for (int r = 0; r < CFG_RIJEN_PER_PAG; r++) {
        int kanaal = cfg_scroll + r;
        int rij_y  = CONTENT_Y + r * CFG_RIJ_H;
        cfg_rij_teken(kanaal, rij_y);
    }

    // Scroll indicatie
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    char hdr[50];
    snprintf(hdr, sizeof(hdr), "Kanalen %d-%d van %d  |  Tik om naam te wijzigen",
             cfg_scroll, cfg_scroll + CFG_RIJEN_PER_PAG - 1, io_kanalen_cnt);
    tft.setCursor(10, CONTENT_Y + CFG_RIJEN_PER_PAG * CFG_RIJ_H + 4);
    tft.print(hdr);
}

void screen_config_toetsenbord_teken() {
    tft.fillRect(0, CONTENT_Y, TFT_W, CONTENT_H, C_SURFACE);

    // Huidige invoer
    tft.fillRoundRect(KB_X, KB_Y - 50, KB_W, 40, 6, C_SURFACE2);
    tft.drawRoundRect(KB_X, KB_Y - 50, KB_W, 40, 6, C_CYAN);
    tft.setTextSize(2);
    tft.setTextColor(C_WHITE);
    tft.setCursor(KB_X + 10, KB_Y - 42);
    tft.print(cfg_invoer);
    tft.print("_");

    // Toetsen
    for (int rij = 0; rij < 4; rij++) {
        const char* keys = kb_rijen[rij];
        int cnt = strlen(keys);
        int tw = KB_W / cnt;
        for (int k = 0; k < cnt; k++) {
            int kx = KB_X + k * tw;
            int ky = KB_Y + rij * (KB_TOETS_H + 4);
            tft.fillRoundRect(kx + 2, ky + 2, tw - 4, KB_TOETS_H - 4, 5, C_SURFACE2);
            tft.drawRoundRect(kx + 2, ky + 2, tw - 4, KB_TOETS_H - 4, 5, C_SURFACE3);
            tft.setTextSize(2);
            tft.setTextColor(C_TEXT);
            tft.setCursor(kx + (tw - 12) / 2, ky + (KB_TOETS_H - 16) / 2);
            tft.print(keys[k]);
        }
    }

    // Backspace, spatie, opslaan
    int btn_y = KB_Y + 4 * (KB_TOETS_H + 4) + 8;
    ui_knop(KB_X, btn_y, 120, 44, "< DEL", C_SURFACE2, C_RED_BRIGHT);
    ui_knop(KB_X + 130, btn_y, 200, 44, "SPATIE", C_SURFACE2, C_TEXT);
    ui_knop(KB_X + 340, btn_y, 140, 44, "OPSLAAN", C_GREEN, C_TEXT_DARK);
    ui_knop(KB_X + 490, btn_y, 100, 44, "CANCEL", C_SURFACE2, C_TEXT_DIM);
}

bool screen_config_toetsenbord_run(int x, int y) {
    int btn_y = KB_Y + 4 * (KB_TOETS_H + 4) + 8;

    // Toetsenbord rijen
    for (int rij = 0; rij < 4; rij++) {
        const char* keys = kb_rijen[rij];
        int cnt = strlen(keys);
        int tw = KB_W / cnt;
        int ky = KB_Y + rij * (KB_TOETS_H + 4);
        if (y >= ky && y < ky + KB_TOETS_H) {
            int k = (x - KB_X) / tw;
            if (k >= 0 && k < cnt) {
                int len = strlen(cfg_invoer);
                if (len < IO_NAAM_LEN - 1) {
                    cfg_invoer[len] = keys[k];
                    cfg_invoer[len + 1] = '\0';
                }
                screen_config_toetsenbord_teken();
                return false;
            }
        }
    }

    // Knoppen onderaan
    if (y >= btn_y && y < btn_y + 44) {
        if (x >= KB_X && x < KB_X + 120) {
            // Delete
            int len = strlen(cfg_invoer);
            if (len > 0) cfg_invoer[len - 1] = '\0';
            screen_config_toetsenbord_teken();
        } else if (x >= KB_X + 130 && x < KB_X + 330) {
            // Spatie
            int len = strlen(cfg_invoer);
            if (len < IO_NAAM_LEN - 1) { cfg_invoer[len] = ' '; cfg_invoer[len+1] = '\0'; }
            screen_config_toetsenbord_teken();
        } else if (x >= KB_X + 340 && x < KB_X + 480) {
            // Opslaan
            if (cfg_geselecteerd >= 0 && cfg_geselecteerd < MAX_IO_KANALEN) {
                strncpy(io_namen[cfg_geselecteerd], cfg_invoer, IO_NAAM_LEN - 1);
                io_namen[cfg_geselecteerd][IO_NAAM_LEN - 1] = '\0';
                hw_io_namen_opslaan();
            }
            cfg_toetsenbord_actief = false;
            return true;
        } else if (x >= KB_X + 490) {
            // Annuleer
            cfg_toetsenbord_actief = false;
            return true;
        }
    }
    return false;
}

void screen_config_teken() {
    tft.fillScreen(C_BG);
    tft.fillRect(0, 0, TFT_W, SB_H, C_STATUSBAR);
    tft.drawFastHLine(0, SB_H - 1, TFT_W, C_SURFACE2);
    tft.setTextSize(2);
    tft.setTextColor(C_CYAN);
    tft.setCursor(10, (SB_H - 16) / 2);
    tft.print("CONFIGURATIE");
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(200, (SB_H - 8) / 2);
    tft.print("Gebruik '**L_' '**IL_' '**haven' etc. voor speciale functies");

    screen_config_rijen_teken();
    nav_bar_teken();
}

void screen_config_run(int x, int y, bool aanraking) {
    if (!aanraking) return;

    if (cfg_toetsenbord_actief) {
        bool klaar = screen_config_toetsenbord_run(x, y);
        if (klaar) {
            cfg_toetsenbord_actief = false;
            scherm_bouwen = true;
        }
        return;
    }

    int nav = nav_bar_klik(x, y);
    if (nav >= 0 && nav != actief_scherm) {
        actief_scherm = nav;
        scherm_bouwen = true;
        return;
    }

    if (y > CONTENT_Y && y < CONTENT_Y + CFG_RIJEN_PER_PAG * CFG_RIJ_H) {
        int rij    = (y - CONTENT_Y) / CFG_RIJ_H;
        int kanaal = cfg_scroll + rij;
        if (kanaal < MAX_IO_KANALEN) {
            cfg_geselecteerd = kanaal;
            strncpy(cfg_invoer, io_namen[kanaal], IO_NAAM_LEN);
            cfg_invoer[IO_NAAM_LEN - 1] = '\0';
            cfg_toetsenbord_actief = true;
            screen_config_toetsenbord_teken();
        }
    }

    // Scroll (pijltje via swipe of tap buiten rijen)
    if (y > CONTENT_Y + CFG_RIJEN_PER_PAG * CFG_RIJ_H) {
        if (x < TFT_W / 2) {
            cfg_scroll = max(0, cfg_scroll - CFG_RIJEN_PER_PAG);
        } else {
            cfg_scroll = min(max(0, io_kanalen_cnt - CFG_RIJEN_PER_PAG), cfg_scroll + CFG_RIJEN_PER_PAG);
        }
        screen_config_rijen_teken();
    }
}
