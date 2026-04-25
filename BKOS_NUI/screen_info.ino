#include "screen_info.h"
#include "nav_bar.h"

// ─── Info state ──────────────────────────────────────────────────────
static byte info_tab = 0;  // 0=boot, 1=eigenaar

// Veldnamen en waarden
static const char* boot_labels[6]  = {"Naam", "Type", "Lengte", "Breedte", "Diepgang", "Hoogte"};
static const char* boot_keys[6]    = {"b_naam","b_type","b_len","b_br","b_dg","b_hg"};
static char        boot_vals[6][INFO_VELD_LEN];

static const char* eig_labels[5]   = {"Naam", "Telefoon", "Stad", "Adres", "E-mail"};
static const char* eig_keys[5]     = {"e_naam","e_tel","e_stad","e_adres","e_email"};
static char        eig_vals[5][INFO_VELD_LEN];

static bool info_geladen = false;

// ─── NVS opslaan/laden ───────────────────────────────────────────────
void info_laden() {
    Preferences prefs;
    prefs.begin("bkos_info", true);
    for (int i = 0; i < 6; i++) {
        String v = prefs.getString(boot_keys[i], "");
        strncpy(boot_vals[i], v.c_str(), INFO_VELD_LEN - 1);
        boot_vals[i][INFO_VELD_LEN - 1] = '\0';
    }
    for (int i = 0; i < 5; i++) {
        String v = prefs.getString(eig_keys[i], "");
        strncpy(eig_vals[i], v.c_str(), INFO_VELD_LEN - 1);
        eig_vals[i][INFO_VELD_LEN - 1] = '\0';
    }
    prefs.end();
    info_geladen = true;
}

void info_opslaan() {
    Preferences prefs;
    prefs.begin("bkos_info", false);
    for (int i = 0; i < 6; i++) prefs.putString(boot_keys[i], boot_vals[i]);
    for (int i = 0; i < 5; i++) prefs.putString(eig_keys[i], eig_vals[i]);
    prefs.end();
}

// ─── Toetsenbord ────────────────────────────────────────────────────
static bool   info_kb_actief  = false;
static int    info_kb_idx     = -1;  // veldindex
static bool   info_kb_boot    = true;
static char   info_kb_invoer[INFO_VELD_LEN] = "";
static unsigned long info_kb_sloot = 0;

static const char* info_kb_rijen[4] = {"1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM_."};

#define IKB_X       40
#define IKB_W       (TFT_W - 80)
#define IKB_INV_Y   (CONTENT_Y + 8)
#define IKB_INV_H   44
#define IKB_KEYS_Y  (IKB_INV_Y + IKB_INV_H + 8)
#define IKB_TH      46
#define IKB_BTN_Y   (IKB_KEYS_Y + 4 * (IKB_TH + 4) + 6)
#define IKB_BTN_H   42

static void info_kb_teken() {
    tft.fillRect(0, CONTENT_Y, TFT_W, CONTENT_H, C_SURFACE);

    // Huidige invoer
    tft.fillRoundRect(IKB_X, IKB_INV_Y, IKB_W, IKB_INV_H, 6, C_SURFACE2);
    tft.drawRoundRect(IKB_X, IKB_INV_Y, IKB_W, IKB_INV_H, 6, C_CYAN);
    tft.setTextSize(2);
    tft.setTextColor(C_WHITE);
    tft.setCursor(IKB_X + 12, IKB_INV_Y + (IKB_INV_H - 16) / 2);

    // Toon veldnaam
    const char* lbl = info_kb_boot ? boot_labels[info_kb_idx] : eig_labels[info_kb_idx];
    tft.setTextColor(C_TEXT_DIM);
    tft.print(lbl);
    tft.print(": ");
    tft.setTextColor(C_WHITE);
    tft.print(info_kb_invoer);
    tft.print("_");

    // Toetsen
    for (int rij = 0; rij < 4; rij++) {
        const char* keys = info_kb_rijen[rij];
        int cnt = strlen(keys);
        int tw = IKB_W / cnt;
        for (int k = 0; k < cnt; k++) {
            int kx = IKB_X + k * tw;
            int ky = IKB_KEYS_Y + rij * (IKB_TH + 4);
            tft.fillRoundRect(kx + 2, ky + 2, tw - 4, IKB_TH - 4, 5, C_SURFACE2);
            tft.drawRoundRect(kx + 2, ky + 2, tw - 4, IKB_TH - 4, 5, C_SURFACE3);
            tft.setTextSize(2);
            tft.setTextColor(C_TEXT);
            tft.setCursor(kx + (tw - 12) / 2, ky + (IKB_TH - 16) / 2);
            tft.print(keys[k]);
        }
    }

    ui_knop(IKB_X,           IKB_BTN_Y, 110, IKB_BTN_H, "< DEL",   C_SURFACE2, C_RED_BRIGHT);
    ui_knop(IKB_X + 118,     IKB_BTN_Y, 160, IKB_BTN_H, "SPATIE",  C_SURFACE2, C_TEXT);
    ui_knop(IKB_X + 286,     IKB_BTN_Y, 160, IKB_BTN_H, "OPSLAAN", C_GREEN,    C_TEXT_DARK);
    ui_knop(IKB_X + 454,     IKB_BTN_Y, 120, IKB_BTN_H, "CANCEL",  C_SURFACE2, C_TEXT_DIM);
}

static bool info_kb_run(int x, int y) {
    for (int rij = 0; rij < 4; rij++) {
        const char* keys = info_kb_rijen[rij];
        int cnt = strlen(keys);
        int tw = IKB_W / cnt;
        int ky = IKB_KEYS_Y + rij * (IKB_TH + 4);
        if (y >= ky && y < ky + IKB_TH) {
            int k = (x - IKB_X) / tw;
            if (k >= 0 && k < cnt) {
                int len = strlen(info_kb_invoer);
                if (len < INFO_VELD_LEN - 1) {
                    info_kb_invoer[len] = keys[k];
                    info_kb_invoer[len + 1] = '\0';
                }
                info_kb_teken();
                return false;
            }
        }
    }

    if (y >= IKB_BTN_Y && y < IKB_BTN_Y + IKB_BTN_H) {
        if (x >= IKB_X && x < IKB_X + 110) {
            int len = strlen(info_kb_invoer);
            if (len > 0) info_kb_invoer[len - 1] = '\0';
            info_kb_teken();
        } else if (x >= IKB_X + 118 && x < IKB_X + 278) {
            int len = strlen(info_kb_invoer);
            if (len < INFO_VELD_LEN - 1) { info_kb_invoer[len] = ' '; info_kb_invoer[len+1] = '\0'; }
            info_kb_teken();
        } else if (x >= IKB_X + 286 && x < IKB_X + 446) {
            // Opslaan
            if (info_kb_boot) {
                strncpy(boot_vals[info_kb_idx], info_kb_invoer, INFO_VELD_LEN - 1);
            } else {
                strncpy(eig_vals[info_kb_idx], info_kb_invoer, INFO_VELD_LEN - 1);
            }
            info_opslaan();
            info_kb_actief = false;
            return true;
        } else if (x >= IKB_X + 454) {
            info_kb_actief = false;
            return true;
        }
    }
    return false;
}

// ─── Tab balk ────────────────────────────────────────────────────────
#define TAB_Y     CONTENT_Y
#define TAB_H     36
#define TAB_W     (TFT_W / 2)

static void info_tabs_teken() {
    for (int i = 0; i < 2; i++) {
        bool actief = (info_tab == (byte)i);
        uint16_t bg = actief ? C_SURFACE2 : C_SURFACE;
        tft.fillRect(i * TAB_W, TAB_Y, TAB_W, TAB_H, bg);
        if (actief) {
            tft.drawFastHLine(i * TAB_W + 10, TAB_Y, TAB_W - 20, C_CYAN);
            tft.drawFastHLine(i * TAB_W + 10, TAB_Y + 1, TAB_W - 20, C_CYAN);
        }
        const char* lbl = (i == 0) ? "BOOT" : "EIGENAAR";
        tft.setTextSize(2);
        tft.setTextColor(actief ? C_CYAN : C_TEXT_DIM);
        int tw = strlen(lbl) * 12;
        tft.setCursor(i * TAB_W + (TAB_W - tw) / 2, TAB_Y + (TAB_H - 16) / 2);
        tft.print(lbl);
    }
    tft.drawFastHLine(0, TAB_Y + TAB_H, TFT_W, C_SURFACE2);
}

// ─── Velden tekenen ──────────────────────────────────────────────────
#define VELD_START_Y  (TAB_Y + TAB_H + 4)
#define VELD_H        50
#define VELD_LABEL_W  120

static void info_veld_teken(int idx, int y, const char* label, const char* waarde) {
    bool even = (idx % 2 == 0);
    tft.fillRect(10, y, TFT_W - 20, VELD_H - 2, even ? C_SURFACE : C_BG);

    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(18, y + (VELD_H - 8) / 2);
    tft.print(label);

    tft.setTextSize(2);
    tft.setTextColor(strlen(waarde) > 0 ? C_TEXT : C_DARK_GRAY);
    tft.setCursor(VELD_LABEL_W + 18, y + (VELD_H - 16) / 2);
    tft.print(strlen(waarde) > 0 ? waarde : "(niet ingevuld)");

    // Bewerk icoon
    tft.setTextColor(C_SURFACE3);
    tft.setCursor(TFT_W - 30, y + (VELD_H - 8) / 2);
    tft.print(">");
}

static void info_velden_teken() {
    int fy = VELD_START_Y;
    tft.fillRect(0, VELD_START_Y, TFT_W, TFT_H - NAV_H - VELD_START_Y, C_BG);

    if (info_tab == 0) {
        for (int i = 0; i < 6; i++) {
            info_veld_teken(i, fy, boot_labels[i], boot_vals[i]);
            fy += VELD_H;
        }
    } else {
        for (int i = 0; i < 5; i++) {
            info_veld_teken(i, fy, eig_labels[i], eig_vals[i]);
            fy += VELD_H;
        }
    }
}

// ─── Hoofdfuncties ───────────────────────────────────────────────────
void screen_info_teken() {
    if (!info_geladen) info_laden();
    tft.fillScreen(C_BG);
    tft.fillRect(0, 0, TFT_W, SB_H, C_STATUSBAR);
    tft.drawFastHLine(0, SB_H - 1, TFT_W, C_SURFACE2);
    tft.setTextSize(2);
    tft.setTextColor(C_CYAN);
    tft.setCursor(10, (SB_H - 16) / 2);
    tft.print("BOOT & EIGENAAR");
    info_tabs_teken();
    info_velden_teken();
    nav_bar_teken();
}

void screen_info_run(int x, int y, bool aanraking) {
    if (!aanraking) return;
    if (millis() - info_kb_sloot < 400) return;

    if (info_kb_actief) {
        bool klaar = info_kb_run(x, y);
        if (klaar) {
            info_kb_actief = false;
            info_kb_sloot = millis();
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

    // Tab wisselen
    if (y >= TAB_Y && y < TAB_Y + TAB_H) {
        byte nieuwe_tab = (x < TFT_W / 2) ? 0 : 1;
        if (nieuwe_tab != info_tab) {
            info_tab = nieuwe_tab;
            info_tabs_teken();
            info_velden_teken();
        }
        return;
    }

    // Veld klikken
    if (y >= VELD_START_Y) {
        int veld_idx = (y - VELD_START_Y) / VELD_H;
        int n_velden = (info_tab == 0) ? 6 : 5;
        if (veld_idx >= 0 && veld_idx < n_velden) {
            info_kb_idx   = veld_idx;
            info_kb_boot  = (info_tab == 0);
            const char* huidige = info_kb_boot ? boot_vals[veld_idx] : eig_vals[veld_idx];
            strncpy(info_kb_invoer, huidige, INFO_VELD_LEN - 1);
            info_kb_invoer[INFO_VELD_LEN - 1] = '\0';
            info_kb_actief = true;
            info_kb_teken();
        }
    }
}
