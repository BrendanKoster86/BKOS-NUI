#include "screen_config.h"
#include "nav_bar.h"

// ─── Configuratie state ──────────────────────────────────────────────
int  cfg_scroll             = 0;
int  cfg_geselecteerd       = -1;
bool cfg_toetsenbord_actief = false;
char cfg_invoer[CFG_INVOER_LEN] = "";
static unsigned long cfg_kb_sloot = 0;  // debounce na sluiten toetsenbord

// ─── Toetsenbord layout ──────────────────────────────────────────────
static const char* kb_rijen[4] = {"1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM_*"};
#define KB_X         40
#define KB_W         (TFT_W - 80)
#define KB_INV_Y     (CONTENT_Y + 8)   // invoerveld
#define KB_INV_H     40
#define KB_CHIP_Y    (KB_INV_Y + KB_INV_H + 6)  // chips rij
#define KB_CHIP_H    36
#define KB_KEYS_Y    (KB_CHIP_Y + KB_CHIP_H + 6) // toetsen
#define KB_TOETS_H   46
#define KB_BTN_Y     (KB_KEYS_Y + 4 * (KB_TOETS_H + 4) + 6)
#define KB_BTN_H     42

// Snelle naam chips
static const char* cfg_chips[] = {
    "**L_hek",  "**L_navi",  "**L_3kl",   "**L_anker", "**L_stoom",
    "**IL_wit", "**IL_rood",
    "**haven",  "**zeilen",  "**motor",    "**anker",
    "**USB",    "**230",     "**tv",       "**water",   "**E_dek",
    nullptr
};

// ─── Helderheid balk (boven IO-lijst) ───────────────────────────────
#define HLD_Y   (CONTENT_Y + 2)
#define HLD_H   40
#define HLD_BTN_W  44
#define CFG_IO_Y    (CONTENT_Y + HLD_H + 4)

static void helderheid_balk_teken() {
    tft.fillRect(0, HLD_Y, TFT_W, HLD_H + 2, C_BG);
    tft.fillRoundRect(8, HLD_Y, TFT_W - 16, HLD_H, 6, C_SURFACE);

    // Minus knop
    tft.fillRoundRect(12, HLD_Y + 4, HLD_BTN_W, HLD_H - 8, 5, C_SURFACE2);
    tft.setTextSize(2);
    tft.setTextColor(C_TEXT);
    tft.setCursor(28, HLD_Y + 12);
    tft.print("-");

    // Plus knop
    tft.fillRoundRect(TFT_W - 12 - HLD_BTN_W - 120 - 4, HLD_Y + 4, HLD_BTN_W, HLD_H - 8, 5, C_SURFACE2);
    tft.setTextSize(2);
    tft.setTextColor(C_TEXT);
    tft.setCursor(TFT_W - 12 - HLD_BTN_W - 120 - 4 + 16, HLD_Y + 12);
    tft.print("+");

    // Waarde
    char buf[12];
    snprintf(buf, sizeof(buf), "%d%%", tft_helderheid);
    int bx = 12 + HLD_BTN_W + 6;
    int bw = TFT_W - 16 - 2 * (HLD_BTN_W + 6) - 120 - 4;
    tft.setTextSize(2);
    tft.setTextColor(C_CYAN);
    int tw = strlen(buf) * 12;
    tft.setCursor(bx + (bw - tw) / 2, HLD_Y + 12);
    tft.print(buf);

    // Scherm-timer
    char tbuf[14];
    snprintf(tbuf, sizeof(tbuf), "Dim: %lds", scherm_timer);
    int tx = TFT_W - 12 - 120;
    tft.fillRoundRect(tx, HLD_Y + 4, 116, HLD_H - 8, 5, C_SURFACE2);
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(tx + 8, HLD_Y + 16);
    tft.print(tbuf);

    // Label
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(bx + 4, HLD_Y + 4);
    tft.print("HELDER");
}

// ─── IO rij ─────────────────────────────────────────────────────────
static void cfg_rij_teken(int kanaal, int rij_y) {
    bool geselecteerd = (kanaal == cfg_geselecteerd);
    uint16_t bg = geselecteerd ? C_SURFACE2 : (kanaal % 2 == 0 ? C_SURFACE : C_BG);
    tft.fillRoundRect(10, rij_y + 2, TFT_W - 20, CFG_RIJ_H - 4, 6, bg);
    if (geselecteerd)
        tft.drawRoundRect(10, rij_y + 2, TFT_W - 20, CFG_RIJ_H - 4, 6, C_CYAN);

    tft.setTextSize(2);
    tft.setTextColor(C_TEXT_DIM);
    char nr[5]; snprintf(nr, sizeof(nr), "%d", kanaal);
    tft.setCursor(18, rij_y + (CFG_RIJ_H - 16) / 2);
    tft.print(nr);

    tft.setTextColor(geselecteerd ? C_CYAN : C_TEXT);
    tft.setCursor(70, rij_y + (CFG_RIJ_H - 16) / 2);
    if (kanaal < io_kanalen_cnt && kanaal < MAX_IO_KANALEN) {
        tft.print(io_namen[kanaal]);
    } else {
        tft.setTextColor(C_DARK_GRAY);
        tft.print("(geen module)");
    }

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
        if (naam.startsWith("**USB") || naam.startsWith("**230") ||
            naam.startsWith("**tv")  || naam.startsWith("**water") ||
            naam.startsWith("**E_"))    { tag = "APP";   tag_kleur = C_CYAN; }
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
    tft.fillRect(0, CFG_IO_Y, TFT_W, CONTENT_H - (CFG_IO_Y - CONTENT_Y), C_BG);

    int rijen_h = (int)(TFT_H - NAV_H - CFG_IO_Y);
    int rijen_n = rijen_h / CFG_RIJ_H;

    for (int r = 0; r < rijen_n; r++) {
        int kanaal = cfg_scroll + r;
        int rij_y  = CFG_IO_Y + r * CFG_RIJ_H;
        cfg_rij_teken(kanaal, rij_y);
    }

    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    char hdr[60];
    snprintf(hdr, sizeof(hdr), "Kanalen %d-%d  |  Tap om naam te wijzigen  |  Tik links/rechts onder rijen om te scrollen",
             cfg_scroll, cfg_scroll + rijen_n - 1);
    int hint_y = CFG_IO_Y + rijen_n * CFG_RIJ_H + 2;
    if (hint_y < (int)(TFT_H - NAV_H - 8)) {
        tft.fillRect(0, hint_y, TFT_W, 10, C_BG);
        tft.setCursor(10, hint_y);
        tft.print(hdr);
    }
}

// ─── Toetsenbord ────────────────────────────────────────────────────
static void cfg_chips_teken() {
    tft.fillRect(KB_X, KB_CHIP_Y, KB_W, KB_CHIP_H, C_SURFACE);
    int chip_w = 80;
    int chip_gap = 4;
    int chips_per_rij = KB_W / (chip_w + chip_gap);
    int x = KB_X;
    for (int i = 0; cfg_chips[i] && i < chips_per_rij; i++) {
        uint16_t bg = C_SURFACE2;
        const char* c = cfg_chips[i];
        if (strncmp(c, "**L_", 4) == 0 || strncmp(c, "**IL_", 5) == 0) bg = RGB565(40, 30, 0);
        else if (strncmp(c, "**haven", 7) == 0 || strncmp(c, "**zeilen", 8) == 0 ||
                 strncmp(c, "**motor", 7) == 0 || strncmp(c, "**anker", 7) == 0) bg = RGB565(0, 25, 50);
        tft.fillRoundRect(x, KB_CHIP_Y + 2, chip_w, KB_CHIP_H - 4, 4, bg);
        tft.setTextSize(1);
        tft.setTextColor(C_TEXT_DIM);
        // Toon verkorte naam (zonder **)
        const char* lbl = (strncmp(c, "**", 2) == 0) ? c + 2 : c;
        int tw = strlen(lbl) * 6;
        tft.setCursor(x + (chip_w - tw) / 2, KB_CHIP_Y + (KB_CHIP_H - 8) / 2);
        tft.print(lbl);
        x += chip_w + chip_gap;
        if (x + chip_w > KB_X + KB_W) break;
    }
    // Tweede rij chips
    x = KB_X;
    int rij2_start = chips_per_rij;
    // (tweede rij chips worden niet getekend als er geen ruimte is)
}

void screen_config_toetsenbord_teken() {
    tft.fillRect(0, CONTENT_Y, TFT_W, CONTENT_H, C_SURFACE);

    // Invoerveld
    tft.fillRoundRect(KB_X, KB_INV_Y, KB_W, KB_INV_H, 6, C_SURFACE2);
    tft.drawRoundRect(KB_X, KB_INV_Y, KB_W, KB_INV_H, 6, C_CYAN);
    tft.setTextSize(2);
    tft.setTextColor(C_WHITE);
    tft.setCursor(KB_X + 10, KB_INV_Y + (KB_INV_H - 16) / 2);
    tft.print(cfg_invoer);
    tft.print("_");

    // Snelle naam chips
    cfg_chips_teken();

    // Toetsen
    for (int rij = 0; rij < 4; rij++) {
        const char* keys = kb_rijen[rij];
        int cnt = strlen(keys);
        int tw = KB_W / cnt;
        for (int k = 0; k < cnt; k++) {
            int kx = KB_X + k * tw;
            int ky = KB_KEYS_Y + rij * (KB_TOETS_H + 4);
            tft.fillRoundRect(kx + 2, ky + 2, tw - 4, KB_TOETS_H - 4, 5, C_SURFACE2);
            tft.drawRoundRect(kx + 2, ky + 2, tw - 4, KB_TOETS_H - 4, 5, C_SURFACE3);
            tft.setTextSize(2);
            tft.setTextColor(C_TEXT);
            tft.setCursor(kx + (tw - 12) / 2, ky + (KB_TOETS_H - 16) / 2);
            tft.print(keys[k]);
        }
    }

    // Knoppen onderaan
    tft.setTextSize(2);
    ui_knop(KB_X,           KB_BTN_Y, 110, KB_BTN_H, "< DEL",   C_SURFACE2, C_RED_BRIGHT);
    ui_knop(KB_X + 118,     KB_BTN_Y, 160, KB_BTN_H, "SPATIE",  C_SURFACE2, C_TEXT);
    ui_knop(KB_X + 286,     KB_BTN_Y, 160, KB_BTN_H, "OPSLAAN", C_GREEN,    C_TEXT_DARK);
    ui_knop(KB_X + 454,     KB_BTN_Y, 120, KB_BTN_H, "CANCEL",  C_SURFACE2, C_TEXT_DIM);
}

static bool cfg_chip_klik(int x, int y) {
    if (y < KB_CHIP_Y || y >= KB_CHIP_Y + KB_CHIP_H) return false;
    int chip_w = 80;
    int chip_gap = 4;
    int idx = (x - KB_X) / (chip_w + chip_gap);
    if (idx < 0) return false;
    int count = 0;
    for (int i = 0; cfg_chips[i]; i++) count++;
    if (idx >= count) return false;
    int cx = KB_X + idx * (chip_w + chip_gap);
    if (x < cx || x >= cx + chip_w) return false;
    strncpy(cfg_invoer, cfg_chips[idx], CFG_INVOER_LEN - 1);
    cfg_invoer[CFG_INVOER_LEN - 1] = '\0';
    screen_config_toetsenbord_teken();
    return true;
}

bool screen_config_toetsenbord_run(int x, int y) {
    // Chips
    if (cfg_chip_klik(x, y)) return false;

    // Toetsenbord rijen
    for (int rij = 0; rij < 4; rij++) {
        const char* keys = kb_rijen[rij];
        int cnt = strlen(keys);
        int tw = KB_W / cnt;
        int ky = KB_KEYS_Y + rij * (KB_TOETS_H + 4);
        if (y >= ky && y < ky + KB_TOETS_H) {
            int k = (x - KB_X) / tw;
            if (k >= 0 && k < cnt) {
                int len = strlen(cfg_invoer);
                if (len < CFG_INVOER_LEN - 1) {
                    cfg_invoer[len] = keys[k];
                    cfg_invoer[len + 1] = '\0';
                }
                screen_config_toetsenbord_teken();
                return false;
            }
        }
    }

    // Knoppen onderaan
    if (y >= KB_BTN_Y && y < KB_BTN_Y + KB_BTN_H) {
        if (x >= KB_X && x < KB_X + 110) {
            int len = strlen(cfg_invoer);
            if (len > 0) cfg_invoer[len - 1] = '\0';
            screen_config_toetsenbord_teken();
        } else if (x >= KB_X + 118 && x < KB_X + 278) {
            int len = strlen(cfg_invoer);
            if (len < CFG_INVOER_LEN - 1) { cfg_invoer[len] = ' '; cfg_invoer[len+1] = '\0'; }
            screen_config_toetsenbord_teken();
        } else if (x >= KB_X + 286 && x < KB_X + 446) {
            if (cfg_geselecteerd >= 0 && cfg_geselecteerd < MAX_IO_KANALEN) {
                strncpy(io_namen[cfg_geselecteerd], cfg_invoer, IO_NAAM_LEN - 1);
                io_namen[cfg_geselecteerd][IO_NAAM_LEN - 1] = '\0';
                hw_io_namen_opslaan();
            }
            cfg_toetsenbord_actief = false;
            return true;
        } else if (x >= KB_X + 454) {
            cfg_toetsenbord_actief = false;
            return true;
        }
    }
    return false;
}

// ─── Standaard IO preset ─────────────────────────────────────────────
static void cfg_standaard_laden() {
    static const char* standaard[] = {
        "**haven", "**zeilen", "**motor", "**anker",
        "**L_stoom", "**L_3kl", "**L_hek", "**L_anker",
        "**IL_wit", "**IL_rood",
        "**USB", "**230", "**tv", "**water", "**E_dek",
        nullptr
    };
    for (int i = 0; standaard[i] && i < MAX_IO_KANALEN; i++) {
        strncpy(io_namen[i], standaard[i], IO_NAAM_LEN - 1);
        io_namen[i][IO_NAAM_LEN - 1] = '\0';
    }
    hw_io_namen_opslaan();
}

// ─── Scherm tekenen ──────────────────────────────────────────────────
void screen_config_teken() {
    tft.fillScreen(C_BG);
    tft.fillRect(0, 0, TFT_W, SB_H, C_STATUSBAR);
    tft.drawFastHLine(0, SB_H - 1, TFT_W, C_SURFACE2);
    tft.setTextSize(2);
    tft.setTextColor(C_CYAN);
    tft.setCursor(10, (SB_H - 16) / 2);
    tft.print("CONFIG");
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(100, (SB_H - 8) / 2);
    tft.print("'**L_'=licht  '**IL_'=int.licht  '**haven/zeilen/motor/anker'=modus");

    helderheid_balk_teken();

    // STANDAARD knop (rechts van helderheid balk)
    ui_knop(TFT_W - 138, HLD_Y + 4, 130, HLD_H - 8, "STANDAARD", C_SURFACE2, C_AMBER);

    screen_config_rijen_teken();
    nav_bar_teken();
}

void screen_config_run(int x, int y, bool aanraking) {
    if (!aanraking) return;
    if (millis() - cfg_kb_sloot < 400) return;  // debounce na sluiten toetsenbord

    if (cfg_toetsenbord_actief) {
        bool klaar = screen_config_toetsenbord_run(x, y);
        if (klaar) {
            cfg_toetsenbord_actief = false;
            cfg_kb_sloot = millis();
            scherm_bouwen = true;
        }
        return;
    }

    // Nav bar
    int nav = nav_bar_klik(x, y);
    if (nav >= 0 && nav != actief_scherm) {
        actief_scherm = nav;
        scherm_bouwen = true;
        return;
    }

    // Helderheid balk
    if (y >= HLD_Y && y < HLD_Y + HLD_H) {
        // Minus knop
        if (x >= 12 && x < 12 + HLD_BTN_W) {
            tft_helderheid = max(5, tft_helderheid - 5);
            tft_helderheid_zet(tft_helderheid);
            state_save();
            helderheid_balk_teken();
            return;
        }
        // Plus knop
        int plus_x = TFT_W - 12 - HLD_BTN_W - 120 - 4;
        if (x >= plus_x && x < plus_x + HLD_BTN_W) {
            tft_helderheid = min(100, tft_helderheid + 5);
            tft_helderheid_zet(tft_helderheid);
            state_save();
            helderheid_balk_teken();
            return;
        }
        // Scherm-timer knop — cycleert 15/30/60/120/0 (nooit)
        if (x >= TFT_W - 12 - 120) {
            long staps[] = {15, 30, 60, 120, 0};
            int huidige = 0;
            for (int i = 0; i < 5; i++) if (scherm_timer == staps[i]) { huidige = i; break; }
            scherm_timer = staps[(huidige + 1) % 5];
            state_save();
            helderheid_balk_teken();
            return;
        }
    }

    // STANDAARD knop
    if (x >= TFT_W - 138 && x < TFT_W - 8 &&
        y >= HLD_Y + 4 && y < HLD_Y + HLD_H - 4) {
        cfg_standaard_laden();
        screen_config_rijen_teken();
        return;
    }

    // IO rijen
    int rijen_h = (int)(TFT_H - NAV_H - CFG_IO_Y);
    int rijen_n = rijen_h / CFG_RIJ_H;
    if (y >= CFG_IO_Y && y < CFG_IO_Y + rijen_n * CFG_RIJ_H) {
        int rij    = (y - CFG_IO_Y) / CFG_RIJ_H;
        int kanaal = cfg_scroll + rij;
        if (kanaal < MAX_IO_KANALEN) {
            cfg_geselecteerd = kanaal;
            strncpy(cfg_invoer, io_namen[kanaal], CFG_INVOER_LEN);
            cfg_invoer[CFG_INVOER_LEN - 1] = '\0';
            cfg_toetsenbord_actief = true;
            screen_config_toetsenbord_teken();
        }
        return;
    }

    // Scroll (tik buiten rijen maar binnen content)
    if (y >= CFG_IO_Y + rijen_n * CFG_RIJ_H && y < (int)(TFT_H - NAV_H)) {
        if (x < TFT_W / 2) {
            cfg_scroll = max(0, cfg_scroll - rijen_n);
        } else {
            cfg_scroll = min(max(0, io_kanalen_cnt - rijen_n), cfg_scroll + rijen_n);
        }
        screen_config_rijen_teken();
    }
}
