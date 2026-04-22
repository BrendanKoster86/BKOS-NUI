#include "screen_main.h"
#include "nav_bar.h"

// ──────────────────────────────────────────────
//  Boot tekening (bovenaanzicht, in BDX/BDY/BDW/BDH)
// ──────────────────────────────────────────────
void boot_teken() {
    tft.fillRect(BDX, BDY, BDW, BDH, C_BG);

    int cx = BDX + BDW / 2;
    int yBow  = BDY + 30;       // neus
    int yMid  = BDY + BDH / 2 - 20;
    int yMax  = BDY + BDH - 40; // hek
    int wMid  = 100;             // halve breedte op midden

    // Romp vulling (grijs-blauw)
    uint16_t romp_kleur = RGB565(25, 45, 80);
    for (int y = yBow; y < yMax; y++) {
        float t = (float)(y - yBow) / (yMax - yBow);
        float halveBreedte;
        if (t < 0.15f) {
            halveBreedte = wMid * (t / 0.15f) * 0.5f;
        } else if (t < 0.7f) {
            halveBreedte = wMid * (0.5f + (t - 0.15f) / 0.55f * 0.5f);
        } else {
            halveBreedte = wMid * (1.0f - (t - 0.7f) / 0.3f * 0.2f);
        }
        tft.drawFastHLine((int)(cx - halveBreedte), y, (int)(halveBreedte * 2), romp_kleur);
    }

    // Romp rand (wit)
    // Port zijde
    for (int stap = 0; stap < 40; stap++) {
        float t = stap / 39.0f;
        float t2 = (stap + 1) / 39.0f;
        float y1f = yBow + t  * (yMax - yBow);
        float y2f = yBow + t2 * (yMax - yBow);
        float hb1, hb2;
        auto halveBr = [&](float t) -> float {
            if (t < 0.15f) return wMid * (t / 0.15f) * 0.5f;
            if (t < 0.7f)  return wMid * (0.5f + (t - 0.15f) / 0.55f * 0.5f);
            return wMid * (1.0f - (t - 0.7f) / 0.3f * 0.2f);
        };
        hb1 = halveBr(t);
        hb2 = halveBr(t2);
        tft.drawLine(cx - (int)hb1, (int)y1f, cx - (int)hb2, (int)y2f, C_TEXT_DIM);
        tft.drawLine(cx + (int)hb1, (int)y1f, cx + (int)hb2, (int)y2f, C_TEXT_DIM);
    }
    // Heklijn
    int yHek = yMax;
    int hbHek = (int)(wMid * 0.8f);
    tft.drawFastHLine(cx - hbHek, yHek, hbHek * 2, C_TEXT_DIM);

    // Dekkleur vlak (donkerder groen overlay → boot kleur)
    uint16_t dek_kleur = RGB565(15, 30, 55);
    int yDek = yBow + (yMax - yBow) / 6;
    int yDekEnd = yMax - (yMax - yBow) / 8;
    for (int y = yDek + 4; y < yDekEnd - 4; y++) {
        float t = (float)(y - yBow) / (yMax - yBow);
        float hb;
        if (t < 0.15f) hb = wMid * (t / 0.15f) * 0.5f - 8;
        else if (t < 0.7f) hb = wMid * (0.5f + (t - 0.15f) / 0.55f * 0.5f) - 8;
        else hb = wMid * (1.0f - (t - 0.7f) / 0.3f * 0.2f) - 8;
        if (hb > 2) tft.drawFastHLine((int)(cx - hb), y, (int)(hb * 2), dek_kleur);
    }

    // Kajuit (rechthoek midden van de boot)
    int kajX = cx - 40;
    int kajY = yDek + 20;
    int kajW = 80;
    int kajH = (yDekEnd - yDek) / 2;
    uint16_t kajKleur = RGB565(30, 50, 90);
    tft.fillRoundRect(kajX, kajY, kajW, kajH, 5, kajKleur);
    tft.drawRoundRect(kajX, kajY, kajW, kajH, 5, RGB565(60, 90, 150));

    // Cockpit
    int ckX = cx - 28;
    int ckY = kajY + kajH + 10;
    int ckW = 56;
    int ckH = 45;
    tft.fillRoundRect(ckX, ckY, ckW, ckH, 4, RGB565(20, 35, 65));
    tft.drawRoundRect(ckX, ckY, ckW, ckH, 4, RGB565(50, 75, 130));

    // Mast (cirkel)
    tft.fillCircle(cx, BL_MAST_Y, 6, RGB565(180, 190, 210));
    tft.drawCircle(cx, BL_MAST_Y, 6, C_WHITE);

    // Waterlijn effect onderaan
    for (int i = 0; i < 3; i++) {
        tft.drawFastHLine(BDX, BDY + BDH - 15 + i * 4, BDW, RGB565(10, 30, 60 + i*10));
    }

    // Label
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(BDX + 4, BDY + 4);
    tft.print("BOOT AANZICHT");
}

// ──────────────────────────────────────────────
//  Licht indicatoren op de boot
// ──────────────────────────────────────────────
void boot_lichten_teken() {
    // Zoek IO kanalen op naam
    int anker_k = -1, mast_k = -1, bb_k = -1, sb_k = -1, hek_k = -1, stoom_k = -1;

    for (int i = 0; i < io_kanalen_cnt && i < MAX_IO_KANALEN; i++) {
        if      (io_naam_is(i, "**L_anker")) anker_k = i;
        else if (io_naam_is(i, "**L_3kl"))   { bb_k = i; sb_k = i; }  // gecombineerd
        else if (io_naam_is(i, "**L_navi"))  { bb_k = i; sb_k = i; }
        else if (io_naam_is(i, "**L_hek"))   hek_k = i;
        else if (io_naam_is(i, "**L_stoom")) stoom_k = i;
    }

    // Ankerlicht (wit, voor op de boot)
    byte staat;
    staat = (anker_k >= 0) ? io_licht_staat(anker_k) : LSTATE_ECHT_UIT;
    ui_licht_cirkel(BL_ANKER_X, BL_ANKER_Y, LICHT_R, staat);
    if (staat == LSTATE_ECHT_AAN) ui_glow(BL_ANKER_X, BL_ANKER_Y, LICHT_R, C_LIGHT_ON, 4);

    // Toplicht / masttop (wit)
    staat = (mast_k >= 0) ? io_licht_staat(mast_k) : LSTATE_ECHT_UIT;
    ui_licht_cirkel(BL_MAST_X, BL_MAST_Y, LICHT_R, staat);

    // Stoomlicht (wit, iets kleiner)
    staat = (stoom_k >= 0) ? io_licht_staat(stoom_k) : LSTATE_ECHT_UIT;
    tft.fillCircle(BL_STOOM_X, BL_STOOM_Y, 10, (staat == LSTATE_ECHT_AAN) ? C_LIGHT_ON : C_LIGHT_OFF);
    if (staat == LSTATE_ECHT_AAN) ui_glow(BL_STOOM_X, BL_STOOM_Y, 10, C_LIGHT_ON, 3);

    // Bakboord (rood)
    staat = (bb_k >= 0) ? io_licht_staat(bb_k) : LSTATE_ECHT_UIT;
    uint16_t bb_on = C_LIGHT_ON_RED;
    if (staat == LSTATE_ECHT_AAN) {
        tft.fillCircle(BL_BB_X, BL_BB_Y, LICHT_R, bb_on);
        ui_glow(BL_BB_X, BL_BB_Y, LICHT_R, bb_on, 4);
    } else if (staat == LSTATE_KOELT_AF) {
        tft.fillCircle(BL_BB_X, BL_BB_Y, LICHT_R, C_LIGHT_COOLING);
        tft.drawCircle(BL_BB_X, BL_BB_Y, LICHT_R, C_RED_BRIGHT);
    } else if (staat == LSTATE_GEEN_SIGNAAL) {
        tft.fillCircle(BL_BB_X, BL_BB_Y, LICHT_R, C_LIGHT_PENDING);
    } else {
        tft.fillCircle(BL_BB_X, BL_BB_Y, LICHT_R, C_LIGHT_OFF);
    }

    // Stuurboord (groen)
    staat = (sb_k >= 0) ? io_licht_staat(sb_k) : LSTATE_ECHT_UIT;
    uint16_t sb_on = C_LIGHT_ON_GRN;
    if (staat == LSTATE_ECHT_AAN) {
        tft.fillCircle(BL_SB_X, BL_SB_Y, LICHT_R, sb_on);
        ui_glow(BL_SB_X, BL_SB_Y, LICHT_R, sb_on, 4);
    } else if (staat == LSTATE_KOELT_AF) {
        tft.fillCircle(BL_SB_X, BL_SB_Y, LICHT_R, C_LIGHT_COOLING);
    } else if (staat == LSTATE_GEEN_SIGNAAL) {
        tft.fillCircle(BL_SB_X, BL_SB_Y, LICHT_R, C_LIGHT_PENDING);
    } else {
        tft.fillCircle(BL_SB_X, BL_SB_Y, LICHT_R, C_LIGHT_OFF);
    }

    // Heklicht (wit, achter)
    staat = (hek_k >= 0) ? io_licht_staat(hek_k) : LSTATE_ECHT_UIT;
    ui_licht_cirkel(BL_HEK_X, BL_HEK_Y, LICHT_R, staat);
    if (staat == LSTATE_ECHT_AAN) ui_glow(BL_HEK_X, BL_HEK_Y, LICHT_R, C_LIGHT_ON, 4);

    // Legenda
    int ly = BDY + BDH - 22;
    tft.setTextSize(1);
    tft.fillCircle(BDX + 14, ly, 5, C_LIGHT_OFF);      tft.setTextColor(C_TEXT_DIM); tft.setCursor(BDX + 22, ly - 4); tft.print("Uit");
    tft.fillCircle(BDX + 60, ly, 5, C_LIGHT_COOLING);  tft.setCursor(BDX + 68, ly - 4); tft.print("Koelt");
    tft.fillCircle(BDX + 110, ly, 5, C_LIGHT_PENDING); tft.setCursor(BDX + 118, ly - 4); tft.print("Wacht");
    tft.fillCircle(BDX + 165, ly, 5, C_LIGHT_ON);      tft.setTextColor(C_WHITE); tft.setCursor(BDX + 173, ly - 4); tft.print("Aan");
}

// ──────────────────────────────────────────────
//  Interieur licht status indicator
// ──────────────────────────────────────────────
void interieur_status_teken() {
    int x = CTRL_PANEL_X + 10;
    int y = INT_STATUS_Y;
    int w = CTRL_PANEL_W - 20;
    int h = 64;

    tft.fillRoundRect(x, y, w, h, 8, C_SURFACE);
    tft.drawRoundRect(x, y, w, h, 8, C_SURFACE2);

    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(x + 8, y + 6);
    tft.print("INTERIEUR");

    // Bepaal interieur status
    bool wit_aan = false, rood_aan = false;
    for (int i = 0; i < io_kanalen_cnt && i < MAX_IO_KANALEN; i++) {
        if (io_naam_is(i, "**IL_wit")  && io_output[i] == IO_AAN) wit_aan  = true;
        if (io_naam_is(i, "**IL_rood") && io_output[i] == IO_AAN) rood_aan = true;
    }

    // Wit indicator
    uint16_t wit_kleur = wit_aan ? C_WHITE : C_DARK_GRAY;
    tft.fillCircle(x + 30, y + h/2 + 4, 14, wit_kleur);
    if (wit_aan) ui_glow(x + 30, y + h/2 + 4, 14, C_WHITE, 3);
    tft.setTextSize(1);
    tft.setTextColor(wit_aan ? C_TEXT_DARK : C_TEXT_DIM);
    tft.setCursor(x + 23, y + h/2 + 1);
    tft.print("W");

    // Rood indicator
    uint16_t rood_kleur = rood_aan ? C_LIGHT_ON_RED : C_DARK_GRAY;
    tft.fillCircle(x + 70, y + h/2 + 4, 14, rood_kleur);
    if (rood_aan) ui_glow(x + 70, y + h/2 + 4, 14, C_LIGHT_ON_RED, 3);
    tft.setTextColor(rood_aan ? C_TEXT_DARK : C_TEXT_DIM);
    tft.setCursor(x + 65, y + h/2 + 1);
    tft.print("R");

    // Status tekst
    tft.setTextSize(2);
    const char* status_txt;
    uint16_t status_kleur;
    if (wit_aan)  { status_txt = "WIT AAN";  status_kleur = C_WHITE; }
    else if (rood_aan) { status_txt = "ROOD AAN"; status_kleur = C_LIGHT_ON_RED; }
    else          { status_txt = "UIT";       status_kleur = C_TEXT_DIM; }
    tft.setTextColor(status_kleur);
    tft.setCursor(x + 105, y + h/2 - 2);
    tft.print(status_txt);
}

// ──────────────────────────────────────────────
//  Vaarmodus knoppen
// ──────────────────────────────────────────────
static void modus_knoppen_teken() {
    // Titel
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(CTRL_PANEL_X + 10, CONTENT_Y + 4);
    tft.print("VAARMODUS");

    struct { const char* naam; const char* sub; uint16_t kleur; byte modus; int x; int y; } modi[4] = {
        {"HAVEN",  "aangemeerd",  C_HAVEN,  MODE_HAVEN,  MKNOP_X1, MKNOP_Y1},
        {"ZEILEN", "zeilvaren",   C_ZEILEN, MODE_ZEILEN, MKNOP_X2, MKNOP_Y1},
        {"MOTOR",  "motoren",     C_MOTOR,  MODE_MOTOR,  MKNOP_X1, MKNOP_Y2},
        {"ANKER",  "voor anker",  C_ANKER,  MODE_ANKER,  MKNOP_X2, MKNOP_Y2},
    };

    for (int i = 0; i < 4; i++) {
        bool actief = (vaar_modus == modi[i].modus);
        uint16_t bg = actief ? C_SURFACE2 : C_SURFACE;
        ui_knop_groot(modi[i].x, modi[i].y, MKNOP_W, MKNOP_H,
                      modi[i].naam, modi[i].sub,
                      bg, modi[i].kleur, modi[i].kleur, actief);
    }
}

// ──────────────────────────────────────────────
//  Verlichting knoppen
// ──────────────────────────────────────────────
static void licht_knoppen_teken() {
    int ty = LKNOP_Y - 16;
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(CTRL_PANEL_X + 10, ty);
    tft.print("VERLICHTING");

    struct { const char* label; byte inst; int x; } knoppen[3] = {
        {"UIT",  LICHT_UIT,  LKNOP_X1},
        {"AAN",  LICHT_AAN,  LKNOP_X2},
        {"AUTO", LICHT_AUTO, LKNOP_X3},
    };

    for (int i = 0; i < 3; i++) {
        bool actief = (licht_instelling == knoppen[i].inst);
        uint16_t accent = (i == 0) ? C_GRAY :
                          (i == 1) ? C_GREEN : C_CYAN;
        uint16_t bg = actief ? C_SURFACE2 : C_SURFACE;
        tft.fillRoundRect(knoppen[i].x, LKNOP_Y, LKNOP_W, LKNOP_H, KNOP_R, bg);
        if (actief) {
            tft.drawRoundRect(knoppen[i].x, LKNOP_Y, LKNOP_W, LKNOP_H, KNOP_R, accent);
            tft.drawRoundRect(knoppen[i].x+1, LKNOP_Y+1, LKNOP_W-2, LKNOP_H-2, KNOP_R-1, accent);
            tft.fillRoundRect(knoppen[i].x, LKNOP_Y, 5, LKNOP_H, 3, accent);
        } else {
            tft.drawRoundRect(knoppen[i].x, LKNOP_Y, LKNOP_W, LKNOP_H, KNOP_R, C_SURFACE2);
        }
        tft.setTextSize(2);
        tft.setTextColor(actief ? accent : C_TEXT_DIM);
        int tw = strlen(knoppen[i].label) * 12;
        tft.setCursor(knoppen[i].x + (LKNOP_W - tw) / 2, LKNOP_Y + (LKNOP_H - 16) / 2);
        tft.print(knoppen[i].label);
    }

    // AUTO sub-tekst
    if (licht_instelling == LICHT_AUTO) {
        tft.setTextSize(1);
        tft.setTextColor(C_TEXT_DIM);
        int sub_y = LKNOP_Y + LKNOP_H + 4;
        const char* uitleg = (vaar_modus == MODE_HAVEN || vaar_modus == MODE_ANKER) ?
                             "Auto: ext. lichten UIT" : "Auto: ext. lichten AAN";
        tft.setCursor(CTRL_PANEL_X + 10, sub_y);
        tft.fillRect(CTRL_PANEL_X + 10, sub_y, CTRL_PANEL_W - 20, 10, C_BG);
        tft.print(uitleg);
    }
}

// ──────────────────────────────────────────────
//  Status bar bovenaan
// ──────────────────────────────────────────────
static void status_bar_teken() {
    tft.fillRect(0, 0, TFT_W, SB_H, C_STATUSBAR);
    tft.drawFastHLine(0, SB_H - 1, TFT_W, C_SURFACE2);

    // WiFi indicator
    tft.setTextSize(2);
    tft.setTextColor(wifi_verbonden ? C_GREEN : C_RED_BRIGHT);
    tft.setCursor(8, (SB_H - 16) / 2);
    tft.print(wifi_verbonden ? "WiFi" : "!WiFi");

    // Versie
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    int tw = strlen(BKOS_NUI_VERSIE) * 6;
    tft.setCursor(TFT_W / 2 - tw / 2, (SB_H - 8) / 2);
    tft.print(BKOS_NUI_VERSIE);

    // Klok
    tft.setTextSize(2);
    tft.setTextColor(C_TEXT);
    tw = klok_tijd.length() * 12;
    tft.setCursor(TFT_W - tw - 10, (SB_H - 16) / 2);
    tft.print(klok_tijd);
}

// ──────────────────────────────────────────────
//  Scheidingslijn boot / controls
// ──────────────────────────────────────────────
static void scheidingslijn_teken() {
    for (int i = 0; i < 3; i++)
        tft.drawFastVLine(CTRL_PANEL_X - 2 + i, CONTENT_Y, CONTENT_H, C_SURFACE2);
}

// ──────────────────────────────────────────────
//  Hoofdfuncties
// ──────────────────────────────────────────────
void screen_main_teken() {
    tft.fillScreen(C_BG);
    status_bar_teken();
    scheidingslijn_teken();
    boot_teken();
    boot_lichten_teken();
    modus_knoppen_teken();
    licht_knoppen_teken();
    interieur_status_teken();
    nav_bar_teken();
}

void screen_main_update_boot() {
    boot_lichten_teken();
}

void screen_main_update_controls() {
    // Wis alleen het rechter paneel (sneller dan volledig hertekenen)
    tft.fillRect(CTRL_PANEL_X, CONTENT_Y, CTRL_PANEL_W, CONTENT_H, C_BG);
    modus_knoppen_teken();
    licht_knoppen_teken();
    interieur_status_teken();
}

void screen_main_run(int x, int y, bool aanraking) {
    if (!aanraking) {
        // Periodieke update van lichtindicatoren
        if (io_runned) {
            boot_lichten_teken();
            interieur_status_teken();
            io_runned = false;
        }
        return;
    }

    // Nav bar?
    int nav = nav_bar_klik(x, y);
    if (nav >= 0 && nav != actief_scherm) {
        actief_scherm = nav;
        scherm_bouwen = true;
        return;
    }

    bool gewijzigd = false;

    // Vaarmodus knoppen
    struct { int x; int y; byte modus; } modi[4] = {
        {MKNOP_X1, MKNOP_Y1, MODE_HAVEN},
        {MKNOP_X2, MKNOP_Y1, MODE_ZEILEN},
        {MKNOP_X1, MKNOP_Y2, MODE_MOTOR},
        {MKNOP_X2, MKNOP_Y2, MODE_ANKER},
    };
    for (int i = 0; i < 4; i++) {
        if (x >= modi[i].x && x < modi[i].x + MKNOP_W &&
            y >= modi[i].y && y < modi[i].y + MKNOP_H) {
            if (vaar_modus != modi[i].modus) {
                vaar_modus = modi[i].modus;
                io_verlichting_update();
                gewijzigd = true;
            }
        }
    }

    // Licht knoppen
    struct { int x; byte inst; } lknoppen[3] = {
        {LKNOP_X1, LICHT_UIT},
        {LKNOP_X2, LICHT_AAN},
        {LKNOP_X3, LICHT_AUTO},
    };
    for (int i = 0; i < 3; i++) {
        if (x >= lknoppen[i].x && x < lknoppen[i].x + LKNOP_W &&
            y >= LKNOP_Y        && y < LKNOP_Y + LKNOP_H) {
            if (licht_instelling != lknoppen[i].inst) {
                licht_instelling = lknoppen[i].inst;
                io_verlichting_update();
                gewijzigd = true;
            }
        }
    }

    if (gewijzigd) {
        state_save();
        screen_main_update_controls();
        boot_lichten_teken();
    }
}
