#include "screen_apps.h"
#include "lua_runtime.h"

// ─── Tabs ────────────────────────────────────────────────────────────────────
#define APPS_TAB_N    3
#define APPS_TAB_H    36
#define APPS_TAB_Y    CONTENT_Y
static const char* apps_tab_labels[APPS_TAB_N] = {"GEÏNSTALLEERD", "WINKEL", "INSTELLINGEN"};

static byte apps_tab = 0;

// ─── Lijst state ──────────────────────────────────────────────────────────────
#define APPS_RIJ_H    64
#define APPS_LIJST_Y  (APPS_TAB_Y + APPS_TAB_H + 2)
#define APPS_RIJEN_N  5    // max zichtbare rijen

static int  apps_scroll     = 0;
static int  apps_winkel_scroll = 0;

// Bevestigings-overlay (verwijderen)
static bool  apps_bevestig_actief = false;
static int   apps_bevestig_idx    = -1;

// Status tekst (download feedback)
static char  apps_status[64] = "";
static bool  apps_bezig      = false;

// ─── Tab tekenen ─────────────────────────────────────────────────────────────
#define APPS_TAB_W  (TFT_W / APPS_TAB_N)

static void _apps_tabs_teken() {
    for (int i = 0; i < APPS_TAB_N; i++) {
        bool act = (apps_tab == (byte)i);
        tft.fillRect(i * APPS_TAB_W, APPS_TAB_Y, APPS_TAB_W, APPS_TAB_H,
                     act ? C_SURFACE2 : C_SURFACE);
        if (act) {
            tft.drawFastHLine(i * APPS_TAB_W + 6, APPS_TAB_Y,     APPS_TAB_W - 12, C_CYAN);
            tft.drawFastHLine(i * APPS_TAB_W + 6, APPS_TAB_Y + 1, APPS_TAB_W - 12, C_CYAN);
        }
        tft.setTextSize(1);
        tft.setTextColor(act ? C_CYAN : C_TEXT_DIM);
        int tw = strlen(apps_tab_labels[i]) * 6;
        tft.setCursor(i * APPS_TAB_W + (APPS_TAB_W - tw) / 2,
                      APPS_TAB_Y + (APPS_TAB_H - 8) / 2);
        tft.print(apps_tab_labels[i]);
    }
    tft.drawFastHLine(0, APPS_TAB_Y + APPS_TAB_H, TFT_W, C_SURFACE2);
}

// ─── App-rij tekenen ─────────────────────────────────────────────────────────
static void _app_rij(int rij_y, AppManifest& m, bool is_winkel, int idx) {
    bool even = (idx % 2 == 0);
    tft.fillRect(0, rij_y, TFT_W, APPS_RIJ_H - 1, even ? C_SURFACE : C_BG);
    tft.drawFastHLine(0, rij_y + APPS_RIJ_H - 1, TFT_W, C_SURFACE2);

    // Naam + auteur
    tft.setTextSize(2);
    tft.setTextColor(C_TEXT);
    tft.setCursor(12, rij_y + 8);
    tft.print(m.naam);

    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(12, rij_y + 30);
    tft.print(m.auteur);
    tft.print("  v");
    tft.print(m.versie);

    // Vervangt indicator
    if (m.vervangt >= 0) {
        static const char* scherm_namen[] = {
            "Paneel","IO","Meteo","Config","WiFi","Info","OTA","IO-cfg","Apps"
        };
        tft.setTextColor(C_AMBER);
        tft.setCursor(12, rij_y + 44);
        tft.print("vervangt: ");
        if (m.vervangt < 9) tft.print(scherm_namen[m.vervangt]);
    }

    if (is_winkel) {
        // Installeer-knop
        bool al_actief = (app_vindt(m.id) >= 0);
        uint16_t kl = al_actief ? C_SURFACE3 : C_CYAN;
        ui_knop(TFT_W - 130, rij_y + 12, 118, 36,
                al_actief ? "GEÏNST." : "INSTALLEER", C_SURFACE2, kl);
    } else {
        // Aan/uit schakelaar
        bool aan = m.actief;
        uint16_t kl_schakel = aan ? C_GREEN : C_SURFACE3;
        tft.fillRoundRect(TFT_W - 120, rij_y + 16, 56, 28, 14, kl_schakel);
        tft.fillCircle(aan ? TFT_W - 76 : TFT_W - 106, rij_y + 30, 11, C_TEXT);

        // Verwijder knop
        ui_knop(TFT_W - 56, rij_y + 16, 44, 28, "X", C_SURFACE2, C_RED_BRIGHT);
    }
}

// ─── GEÏNSTALLEERD tab ───────────────────────────────────────────────────────
static void _apps_geinstalleerd_teken() {
    tft.fillRect(0, APPS_LIJST_Y, TFT_W, TFT_H - NAV_H - APPS_LIJST_Y, C_BG);

    if (apps_cnt == 0) {
        tft.setTextSize(2);
        tft.setTextColor(C_TEXT_DIM);
        ui_tekst_midden(0, TFT_H / 2 - 16, TFT_W, "Geen apps geïnstalleerd", C_TEXT_DIM, 2);
        tft.setTextSize(1);
        ui_tekst_midden(0, TFT_H / 2 + 16, TFT_W, "Ga naar WINKEL om apps te downloaden", C_TEXT_DIM, 1);
        return;
    }

    int max_scroll = max(0, apps_cnt - APPS_RIJEN_N);
    if (apps_scroll > max_scroll) apps_scroll = max_scroll;

    for (int i = 0; i < APPS_RIJEN_N; i++) {
        int idx = apps_scroll + i;
        if (idx >= apps_cnt) break;
        _app_rij(APPS_LIJST_Y + i * APPS_RIJ_H, apps[idx], false, i);
    }

    // Scroll indicator
    if (apps_cnt > APPS_RIJEN_N) {
        int sh = (TFT_H - NAV_H - APPS_LIJST_Y);
        int bh = max(30, sh * APPS_RIJEN_N / apps_cnt);
        int by = APPS_LIJST_Y + (sh - bh) * apps_scroll / max_scroll;
        tft.fillRect(TFT_W - 4, by, 4, bh, C_SURFACE3);
    }
}

// ─── WINKEL tab ──────────────────────────────────────────────────────────────
static void _apps_winkel_teken() {
    tft.fillRect(0, APPS_LIJST_Y, TFT_W, TFT_H - NAV_H - APPS_LIJST_Y, C_BG);

    if (apps_bezig) {
        ui_tekst_midden(0, TFT_H / 2 - 8, TFT_W, apps_bezig ? "Laden..." : apps_status,
                        C_CYAN, 2);
        return;
    }

    if (!winkel_geladen) {
        // Laad-knop tonen
        ui_tekst_midden(0, TFT_H / 2 - 30, TFT_W, "App store nog niet geladen", C_TEXT_DIM, 1);
        ui_knop(TFT_W / 2 - 80, TFT_H / 2, 160, 40, "LADEN", C_SURFACE2, C_CYAN);
        return;
    }

    if (winkel_cnt == 0) {
        ui_tekst_midden(0, TFT_H / 2 - 8, TFT_W, "Geen apps beschikbaar", C_TEXT_DIM, 2);
        return;
    }

    int max_scroll = max(0, winkel_cnt - APPS_RIJEN_N);
    if (apps_winkel_scroll > max_scroll) apps_winkel_scroll = max_scroll;

    for (int i = 0; i < APPS_RIJEN_N; i++) {
        int idx = apps_winkel_scroll + i;
        if (idx >= winkel_cnt) break;
        _app_rij(APPS_LIJST_Y + i * APPS_RIJ_H, winkel[idx], true, i);
    }
}

// ─── INSTELLINGEN tab (scherm-overschrijvingen) ───────────────────────────────
#define INS_RIJ_H  44
static const char* ins_scherm_namen[] = {
    "Paneel", "IO-lijst", "Meteo / Getij", "Configuratie", "Info"
};
static const int ins_scherm_ids[] = {
    SCREEN_MAIN, SCREEN_IO, SCREEN_METEO, SCREEN_CONFIG, SCREEN_INFO
};
#define INS_SCHERM_N  5

static void _apps_instellingen_teken() {
    tft.fillRect(0, APPS_LIJST_Y, TFT_W, TFT_H - NAV_H - APPS_LIJST_Y, C_BG);

    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(12, APPS_LIJST_Y + 6);
    tft.print("Kies welke app een ingebouwd scherm vervangt:");

    int y = APPS_LIJST_Y + 22;
    for (int s = 0; s < INS_SCHERM_N; s++) {
        bool even = (s % 2 == 0);
        tft.fillRect(0, y, TFT_W, INS_RIJ_H - 1, even ? C_SURFACE : C_BG);
        tft.drawFastHLine(0, y + INS_RIJ_H - 1, TFT_W, C_SURFACE2);

        tft.setTextSize(2);
        tft.setTextColor(C_TEXT);
        tft.setCursor(12, y + (INS_RIJ_H - 16) / 2);
        tft.print(ins_scherm_namen[s]);

        int app_idx = app_voor_scherm(ins_scherm_ids[s]);
        tft.setTextSize(1);
        if (app_idx >= 0) {
            tft.setTextColor(C_CYAN);
            tft.setCursor(TFT_W / 2, y + (INS_RIJ_H - 8) / 2);
            tft.print(apps[app_idx].naam);
            ui_knop(TFT_W - 100, y + 8, 88, 28, "HERSTEL", C_SURFACE2, C_AMBER);
        } else {
            tft.setTextColor(C_TEXT_DIM);
            tft.setCursor(TFT_W / 2, y + (INS_RIJ_H - 8) / 2);
            tft.print("ingebouwd");
        }
        y += INS_RIJ_H;
    }

    // Status tekst
    if (apps_status[0]) {
        tft.setTextSize(1);
        tft.setTextColor(C_AMBER);
        tft.setCursor(12, y + 8);
        tft.print(apps_status);
    }
}

// ─── Bevestigings overlay ────────────────────────────────────────────────────
static void _apps_bevestig_teken() {
    tft.fillRect(100, 140, 600, 200, C_SURFACE);
    tft.drawRect(100, 140, 600, 200, C_SURFACE3);
    tft.setTextSize(2);
    tft.setTextColor(C_TEXT);
    tft.setCursor(130, 160);
    tft.print("App verwijderen?");
    if (apps_bevestig_idx >= 0 && apps_bevestig_idx < apps_cnt) {
        tft.setTextSize(1);
        tft.setTextColor(C_TEXT_DIM);
        tft.setCursor(130, 185);
        tft.print(apps[apps_bevestig_idx].naam);
    }
    ui_knop(140, 260, 200, 50, "VERWIJDER", C_RED_BRIGHT, C_TEXT);
    ui_knop(460, 260, 200, 50, "ANNULEER",  C_SURFACE2,   C_TEXT);
}

// ─── Hoofdfuncties ───────────────────────────────────────────────────────────
void screen_apps_teken() {
    tft.fillScreen(C_BG);
    sb_scherm_teken("APPS", C_CYAN);
    _apps_tabs_teken();

    switch (apps_tab) {
        case 0: _apps_geinstalleerd_teken(); break;
        case 1: _apps_winkel_teken();        break;
        case 2: _apps_instellingen_teken();  break;
    }

    if (apps_bevestig_actief) _apps_bevestig_teken();
    nav_bar_teken();
}

void screen_apps_run(int x, int y, bool aanraking) {
    if (!aanraking) return;

    // Bevestig overlay
    if (apps_bevestig_actief) {
        if (y >= 260 && y <= 310) {
            if (x >= 140 && x <= 340) {
                app_verwijder(apps_bevestig_idx);
                apps_bevestig_actief = false;
                apps_bevestig_idx    = -1;
                scherm_bouwen = true;
            } else if (x >= 460 && x <= 660) {
                apps_bevestig_actief = false;
                scherm_bouwen = true;
            }
        }
        return;
    }

    // Nav bar
    int nav = nav_bar_klik(x, y);
    if (nav >= 0 && nav != actief_scherm) {
        actief_scherm = nav; scherm_bouwen = true; return;
    }

    // Tab balk
    if (y >= APPS_TAB_Y && y < APPS_TAB_Y + APPS_TAB_H) {
        byte t = (byte)(x / APPS_TAB_W);
        if (t >= APPS_TAB_N) t = APPS_TAB_N - 1;
        if (t != apps_tab) {
            apps_tab = t;
            apps_scroll = apps_winkel_scroll = 0;
            apps_status[0] = '\0';
            scherm_bouwen = true;
        }
        return;
    }

    if (y < APPS_LIJST_Y) return;

    // ── Tab 0: GEÏNSTALLEERD ────────────────────────────────────────────────
    if (apps_tab == 0) {
        int rij = (y - APPS_LIJST_Y) / APPS_RIJ_H;
        int idx = apps_scroll + rij;
        if (idx < 0 || idx >= apps_cnt) return;

        int rij_y = APPS_LIJST_Y + rij * APPS_RIJ_H;

        // Verwijder-knop (X, rechtsboven)
        if (x >= TFT_W - 56 && x <= TFT_W - 12 && y >= rij_y + 16 && y <= rij_y + 44) {
            apps_bevestig_idx    = idx;
            apps_bevestig_actief = true;
            scherm_bouwen = true;
            return;
        }

        // Aan/uit schakelaar
        if (x >= TFT_W - 120 && x <= TFT_W - 64 && y >= rij_y + 16 && y <= rij_y + 44) {
            app_zet_actief(idx, !apps[idx].actief);
            // Herlaad Lua als state wisselt
            lua_app_sluiten();
            lua_setup();
            _apps_geinstalleerd_teken();
        }
        return;
    }

    // ── Tab 1: WINKEL ────────────────────────────────────────────────────────
    if (apps_tab == 1) {
        if (!winkel_geladen) {
            // Laad-knop aangeraakt
            if (y >= TFT_H / 2 && y <= TFT_H / 2 + 40 &&
                x >= TFT_W / 2 - 80 && x <= TFT_W / 2 + 80) {
                apps_bezig = true;
                scherm_bouwen = true;
                // Laden in main loop (WiFi taak) — hier triggeren we het
                strncpy(apps_status, "Laden...", sizeof(apps_status) - 1);
                app_winkel_laden();  // blokkerend, maar kort
                apps_bezig = false;
                scherm_bouwen = true;
            }
            return;
        }

        int rij = (y - APPS_LIJST_Y) / APPS_RIJ_H;
        int idx = apps_winkel_scroll + rij;
        if (idx < 0 || idx >= winkel_cnt) return;

        // Installeer-knop
        if (x >= TFT_W - 130 && x <= TFT_W - 12) {
            if (app_vindt(winkel[idx].id) >= 0) return; // al geïnstalleerd
            apps_bezig = true;
            snprintf(apps_status, sizeof(apps_status), "Installeren: %s...", winkel[idx].naam);
            scherm_bouwen = true;

            bool ok = app_installeer_uit_winkel(idx);
            apps_bezig = false;
            strncpy(apps_status,
                    ok ? "Installatie geslaagd!" : "Installatie mislukt.",
                    sizeof(apps_status) - 1);
            if (ok) { app_manifesten_laden(); lua_setup(); }
            scherm_bouwen = true;
        }
        return;
    }

    // ── Tab 2: INSTELLINGEN ──────────────────────────────────────────────────
    if (apps_tab == 2) {
        int y0 = APPS_LIJST_Y + 22;
        for (int s = 0; s < INS_SCHERM_N; s++) {
            if (y >= y0 && y < y0 + INS_RIJ_H) {
                // HERSTEL knop aangeraakt?
                if (x >= TFT_W - 100) {
                    int app_idx = app_voor_scherm(ins_scherm_ids[s]);
                    if (app_idx >= 0) {
                        apps[app_idx].vervangt = APP_VERVANGT_GEEN;
                        app_manifest_opslaan(app_idx);
                        snprintf(apps_status, sizeof(apps_status),
                                 "%s hersteld naar ingebouwd", ins_scherm_namen[s]);
                        scherm_bouwen = true;
                    }
                }
                return;
            }
            y0 += INS_RIJ_H;
        }
    }
}
