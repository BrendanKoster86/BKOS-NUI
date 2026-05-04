#include "screen_apps.h"
#include "lua_runtime.h"

// ─── Layout (2-panel zij-aan-zij) ─────────────────────────────────────────────
#define APPS_HDR_H    32                           // hoogte deelscherm-header
#define APPS_HDR_Y    CONTENT_Y                    // direct onder statusbalk
#define APPS_LIST_Y   (APPS_HDR_Y + APPS_HDR_H)   // inhoud start hier
#define APPS_LIST_H   (TFT_H - NAV_H - APPS_LIST_Y)
#define APPS_PNL_W    (TFT_W / 2)                  // 400px per deelscherm
#define APPS_RIJ_H    58                            // rijhoogte per app
#define APPS_RIJEN_N  (APPS_LIST_H / APPS_RIJ_H)  // max rijen per deelscherm

// Scherm-toewijzing modus toont de SCHERMEN overlay over het linker deelscherm
static bool apps_toewijzing_modus = false;

// Scroll state
static int  apps_scroll        = 0;
static int  apps_winkel_scroll = 0;

// Bevestigings-overlay (verwijderen)
static bool apps_bevestig_actief = false;
static int  apps_bevestig_idx    = -1;

// Status/download feedback (rechter deelscherm)
static char apps_status[64] = "";
static bool apps_bezig       = false;

// ─── Deelscherm-headers ───────────────────────────────────────────────────────
static void _apps_headers_teken() {
    const char* links_label = apps_toewijzing_modus ? "SCHERMEN" : "GEINSTALLEERD";

    tft.fillRect(0,          APPS_HDR_Y, APPS_PNL_W, APPS_HDR_H, C_SURFACE2);
    tft.setTextSize(1);
    tft.setTextColor(C_CYAN);
    int lw = strlen(links_label) * 6;
    tft.setCursor((APPS_PNL_W - lw) / 2, APPS_HDR_Y + (APPS_HDR_H - 8) / 2);
    tft.print(links_label);

    tft.fillRect(APPS_PNL_W, APPS_HDR_Y, APPS_PNL_W, APPS_HDR_H, C_SURFACE2);
    tft.setTextColor(C_CYAN);
    int rw = strlen("APP STORE") * 6;
    tft.setCursor(APPS_PNL_W + (APPS_PNL_W - rw) / 2, APPS_HDR_Y + (APPS_HDR_H - 8) / 2);
    tft.print("APP STORE");

    // Scheidingslijnen
    tft.drawFastVLine(APPS_PNL_W, APPS_HDR_Y, TFT_H - NAV_H - APPS_HDR_Y, C_SURFACE3);
    tft.drawFastHLine(0, APPS_HDR_Y + APPS_HDR_H, TFT_W, C_SURFACE3);
}

// ─── Linker deelscherm: GEÏNSTALLEERD ─────────────────────────────────────────
static void _apps_rij_links(int y, int app_idx, int visueel_idx) {
    AppManifest& m = apps[app_idx];
    bool even = (visueel_idx % 2 == 0);
    tft.fillRect(0, y, APPS_PNL_W - 1, APPS_RIJ_H - 1, even ? C_SURFACE : C_BG);
    tft.drawFastHLine(0, y + APPS_RIJ_H - 1, APPS_PNL_W - 1, C_SURFACE2);

    tft.setTextSize(2);
    tft.setTextColor(C_TEXT);
    tft.setCursor(8, y + 6);
    tft.print(m.naam);

    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(8, y + 28);
    tft.print(m.auteur);
    tft.print(" v");
    tft.print(m.versie);

    // Schakelaar
    bool aan = m.actief;
    tft.fillRoundRect(APPS_PNL_W - 110, y + 16, 52, 26, 13, aan ? C_GREEN : C_SURFACE3);
    tft.fillCircle(aan ? APPS_PNL_W - 70 : APPS_PNL_W - 100, y + 29, 10, C_TEXT);

    // Verwijder
    ui_knop(APPS_PNL_W - 50, y + 15, 38, 26, "X", C_SURFACE2, C_RED_BRIGHT);
}

// Geeft aantal zichtbare rijen (laat ruimte voor SCHERMEN-knop onderaan)
static int _apps_rijen_zichtbaar() {
    return min(APPS_RIJEN_N, (APPS_LIST_H - 36) / APPS_RIJ_H);
}

static void _apps_geinstalleerd_teken() {
    tft.fillRect(0, APPS_LIST_Y, APPS_PNL_W - 1, APPS_LIST_H, C_BG);

    if (apps_cnt == 0) {
        int mid = APPS_LIST_Y + APPS_LIST_H / 2;
        ui_tekst_midden(0, mid - 14, APPS_PNL_W, "Geen apps", C_TEXT_DIM, 1);
        ui_tekst_midden(0, mid + 2,  APPS_PNL_W, "Gebruik APP STORE", C_TEXT_DIM, 1);
    } else {
        int max_scroll = max(0, apps_cnt - _apps_rijen_zichtbaar());
        if (apps_scroll > max_scroll) apps_scroll = max_scroll;

        int rijen = _apps_rijen_zichtbaar();
        for (int i = 0; i < rijen; i++) {
            int idx = apps_scroll + i;
            if (idx >= apps_cnt) break;
            _apps_rij_links(APPS_LIST_Y + i * APPS_RIJ_H, idx, i);
        }
    }

    // SCHERMEN BEHEREN knop (altijd onderaan links)
    int ky = APPS_LIST_Y + APPS_LIST_H - 34;
    tft.fillRect(0, ky - 2, APPS_PNL_W - 1, 2, C_SURFACE2);
    ui_knop(8, ky, APPS_PNL_W - 16, 28, "SCHERMEN BEHEREN", C_SURFACE2, C_TEXT_DIM);
}

// ─── Scherm-toewijzing overlay (linker deelscherm) ────────────────────────────
#define INS_RIJ_H    40
static const char* ins_scherm_namen[] = {"Paneel","IO-lijst","Meteo","Configuratie","Info"};
static const int   ins_scherm_ids[]   = {SCREEN_MAIN, SCREEN_IO, SCREEN_METEO, SCREEN_CONFIG, SCREEN_INFO};
#define INS_SCHERM_N  5

static void _apps_schermen_teken() {
    tft.fillRect(0, APPS_LIST_Y, APPS_PNL_W - 1, APPS_LIST_H, C_BG);

    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(8, APPS_LIST_Y + 6);
    tft.print("Koppel app aan scherm:");

    int y = APPS_LIST_Y + 20;
    for (int s = 0; s < INS_SCHERM_N; s++) {
        if (y + INS_RIJ_H > APPS_LIST_Y + APPS_LIST_H - 36) break;
        bool even = (s % 2 == 0);
        tft.fillRect(0, y, APPS_PNL_W - 1, INS_RIJ_H - 1, even ? C_SURFACE : C_BG);
        tft.drawFastHLine(0, y + INS_RIJ_H - 1, APPS_PNL_W - 1, C_SURFACE2);

        tft.setTextSize(1);
        tft.setTextColor(C_TEXT);
        tft.setCursor(8, y + (INS_RIJ_H - 8) / 2);
        tft.print(ins_scherm_namen[s]);

        int app_idx = app_voor_scherm(ins_scherm_ids[s]);
        if (app_idx >= 0) {
            tft.setTextColor(C_CYAN);
            tft.setCursor(APPS_PNL_W / 2, y + (INS_RIJ_H - 8) / 2);
            tft.print(apps[app_idx].naam);
            ui_knop(APPS_PNL_W - 90, y + 7, 78, 24, "HERSTEL", C_SURFACE2, C_AMBER);
        } else {
            tft.setTextColor(C_TEXT_DIM);
            tft.setCursor(APPS_PNL_W / 2, y + (INS_RIJ_H - 8) / 2);
            tft.print("ingebouwd");
        }
        y += INS_RIJ_H;
    }

    // TERUG knop
    int ky = APPS_LIST_Y + APPS_LIST_H - 34;
    tft.fillRect(0, ky - 2, APPS_PNL_W - 1, 2, C_SURFACE2);
    ui_knop(8, ky, APPS_PNL_W - 16, 28, "TERUG", C_SURFACE2, C_CYAN);
}

// ─── Rechter deelscherm: APP STORE ───────────────────────────────────────────
static void _apps_rij_rechts(int y, int winkel_idx, int visueel_idx) {
    AppManifest& m = winkel[winkel_idx];
    bool even = (visueel_idx % 2 == 0);
    tft.fillRect(APPS_PNL_W + 1, y, APPS_PNL_W - 1, APPS_RIJ_H - 1, even ? C_SURFACE : C_BG);
    tft.drawFastHLine(APPS_PNL_W + 1, y + APPS_RIJ_H - 1, APPS_PNL_W - 1, C_SURFACE2);

    tft.setTextSize(2);
    tft.setTextColor(C_TEXT);
    tft.setCursor(APPS_PNL_W + 8, y + 6);
    tft.print(m.naam);

    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(APPS_PNL_W + 8, y + 28);
    tft.print(m.auteur);
    tft.print(" v");
    tft.print(m.versie);

    bool al_actief = (app_vindt(m.id) >= 0);
    ui_knop(TFT_W - 112, y + 15, 100, 26,
            al_actief ? "GEINSTALL." : "INSTALLEER",
            C_SURFACE2, al_actief ? C_SURFACE3 : C_CYAN);
}

static void _apps_winkel_teken() {
    tft.fillRect(APPS_PNL_W + 1, APPS_LIST_Y, APPS_PNL_W - 1, APPS_LIST_H, C_BG);

    if (apps_bezig) {
        int mid = APPS_LIST_Y + APPS_LIST_H / 2;
        ui_tekst_midden(APPS_PNL_W, mid - 8, APPS_PNL_W, "Laden...", C_CYAN, 1);
        if (apps_status[0])
            ui_tekst_midden(APPS_PNL_W, mid + 8, APPS_PNL_W, apps_status, C_TEXT_DIM, 1);
        return;
    }

    if (!winkel_geladen) {
        int mid = APPS_LIST_Y + APPS_LIST_H / 2;
        ui_tekst_midden(APPS_PNL_W, mid - 28, APPS_PNL_W, "App store niet geladen", C_TEXT_DIM, 1);
        ui_knop(APPS_PNL_W + 60, mid - 6, 280, 36, "LADEN", C_SURFACE2, C_CYAN);
        return;
    }

    if (winkel_cnt == 0) {
        int mid = APPS_LIST_Y + APPS_LIST_H / 2;
        ui_tekst_midden(APPS_PNL_W, mid - 8, APPS_PNL_W, "Geen apps beschikbaar", C_TEXT_DIM, 1);
        return;
    }

    int max_scroll = max(0, winkel_cnt - APPS_RIJEN_N);
    if (apps_winkel_scroll > max_scroll) apps_winkel_scroll = max_scroll;

    for (int i = 0; i < APPS_RIJEN_N; i++) {
        int idx = apps_winkel_scroll + i;
        if (idx >= winkel_cnt) break;
        _apps_rij_rechts(APPS_LIST_Y + i * APPS_RIJ_H, idx, i);
    }

    // Status feedback
    if (apps_status[0]) {
        tft.setTextSize(1);
        tft.setTextColor(C_AMBER);
        tft.setCursor(APPS_PNL_W + 8, APPS_LIST_Y + APPS_LIST_H - 16);
        tft.print(apps_status);
    }

    // VERNIEUWEN knop (rechtsonder)
    int ky = APPS_LIST_Y + APPS_LIST_H - 34;
    tft.fillRect(APPS_PNL_W + 1, ky - 2, APPS_PNL_W - 1, 2, C_SURFACE2);
    ui_knop(TFT_W - 110, ky, 98, 28, "VERNIEUWEN", C_SURFACE2, C_TEXT_DIM);
}

// ─── Bevestigings overlay ─────────────────────────────────────────────────────
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
        tft.setCursor(130, 186);
        tft.print(apps[apps_bevestig_idx].naam);
    }
    ui_knop(140, 260, 200, 50, "VERWIJDER", C_RED_BRIGHT, C_TEXT);
    ui_knop(460, 260, 200, 50, "ANNULEER",  C_SURFACE2,   C_TEXT);
}

// ─── Hoofdfuncties ────────────────────────────────────────────────────────────
void screen_apps_teken() {
    tft.fillScreen(C_BG);
    sb_scherm_teken("APPS", C_CYAN);
    _apps_headers_teken();

    if (apps_toewijzing_modus)
        _apps_schermen_teken();
    else
        _apps_geinstalleerd_teken();

    _apps_winkel_teken();

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

    // Header of statusbalk → negeer
    if (y < APPS_LIST_Y) return;

    // ── Linker deelscherm ──────────────────────────────────────────────────────
    if (x < APPS_PNL_W) {
        int ky = APPS_LIST_Y + APPS_LIST_H - 34;

        if (apps_toewijzing_modus) {
            // TERUG knop
            if (y >= ky && y <= ky + 28) {
                apps_toewijzing_modus = false;
                scherm_bouwen = true;
                return;
            }
            // Scherm-toewijzingsrijen
            int y0 = APPS_LIST_Y + 20;
            for (int s = 0; s < INS_SCHERM_N; s++) {
                if (y >= y0 && y < y0 + INS_RIJ_H) {
                    if (x >= APPS_PNL_W - 90) {
                        int app_idx = app_voor_scherm(ins_scherm_ids[s]);
                        if (app_idx >= 0) {
                            apps[app_idx].vervangt = APP_VERVANGT_GEEN;
                            app_manifest_opslaan(app_idx);
                            scherm_bouwen = true;
                        }
                    }
                    return;
                }
                y0 += INS_RIJ_H;
            }
        } else {
            // SCHERMEN BEHEREN knop
            if (y >= ky && y <= ky + 28) {
                apps_toewijzing_modus = true;
                scherm_bouwen = true;
                return;
            }
            // App-rij aangeraakt
            int rijen = _apps_rijen_zichtbaar();
            int rij   = (y - APPS_LIST_Y) / APPS_RIJ_H;
            if (rij >= rijen) return;
            int idx = apps_scroll + rij;
            if (idx < 0 || idx >= apps_cnt) return;
            int rij_y = APPS_LIST_Y + rij * APPS_RIJ_H;

            // X verwijder
            if (x >= APPS_PNL_W - 50 && x <= APPS_PNL_W - 12 &&
                y >= rij_y + 15 && y <= rij_y + 41) {
                apps_bevestig_idx    = idx;
                apps_bevestig_actief = true;
                scherm_bouwen = true;
                return;
            }
            // Schakelaar
            if (x >= APPS_PNL_W - 110 && x <= APPS_PNL_W - 58 &&
                y >= rij_y + 16 && y <= rij_y + 42) {
                app_zet_actief(idx, !apps[idx].actief);
                lua_app_sluiten();
                lua_setup();
                scherm_bouwen = true;
            }
        }
        return;
    }

    // ── Rechter deelscherm ─────────────────────────────────────────────────────
    if (!winkel_geladen) {
        int mid = APPS_LIST_Y + APPS_LIST_H / 2;
        if (y >= mid - 6 && y <= mid + 30 &&
            x >= APPS_PNL_W + 60 && x <= APPS_PNL_W + 340) {
            apps_bezig = true;
            scherm_bouwen = true;
            app_winkel_laden();
            apps_bezig = false;
            scherm_bouwen = true;
        }
        return;
    }

    int ky = APPS_LIST_Y + APPS_LIST_H - 34;

    // VERNIEUWEN knop
    if (y >= ky && y <= ky + 28 && x >= TFT_W - 110) {
        apps_status[0]     = '\0';
        winkel_geladen     = false;
        apps_winkel_scroll = 0;
        scherm_bouwen      = true;
        return;
    }

    // Winkel-rij aangeraakt
    int rij = (y - APPS_LIST_Y) / APPS_RIJ_H;
    int idx = apps_winkel_scroll + rij;
    if (idx < 0 || idx >= winkel_cnt) return;

    // INSTALLEER knop
    if (x >= TFT_W - 112) {
        if (app_vindt(winkel[idx].id) >= 0) return;
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
}
