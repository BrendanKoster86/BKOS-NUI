#include "screen_meteo.h"
#include "meteo.h"
#include "nav_bar.h"
#include "ui_draw.h"

int meteo_tab = METEO_TAB_WEER;

// ─── Layout ───────────────────────────────────────────────────────────────
#define TAB_Y       CONTENT_Y
#define TAB_H       38
#define TAB_CNT     3
#define TAB_W       (TFT_W / TAB_CNT)
#define PANEL_Y     (TAB_Y + TAB_H + 2)
#define PANEL_H     (TFT_H - NAV_H - PANEL_Y)

// ─── Weericons (getekend) ─────────────────────────────────────────────────
static void weer_zon(int cx, int cy, int r, uint16_t c) {
    tft.fillCircle(cx, cy, r, c);
    for (int a = 0; a < 360; a += 45) {
        float rad = a * M_PI / 180.0f;
        int x1 = cx + (r + 3) * cos(rad);
        int y1 = cy + (r + 3) * sin(rad);
        int x2 = cx + (r + 8) * cos(rad);
        int y2 = cy + (r + 8) * sin(rad);
        tft.drawLine(x1, y1, x2, y2, c);
    }
}

static void weer_wolk(int cx, int cy, int w, int h, uint16_t c) {
    tft.fillRoundRect(cx - w/2, cy - h/4, w, h/2, h/4, c);
    tft.fillCircle(cx - w/5, cy - h/4 + 1, h/3, c);
    tft.fillCircle(cx + w/6, cy - h/4 - 1, h/4, c);
}

static void weer_regen(int cx, int cy, int w, int h, uint16_t cc, uint16_t cr) {
    weer_wolk(cx, cy - 4, w, h/2, cc);
    for (int i = -1; i <= 1; i++) {
        tft.drawLine(cx + i*8, cy + h/4, cx + i*8 - 3, cy + h/2 + 2, cr);
    }
}

static void weer_sneeuw(int cx, int cy, int w, int h, uint16_t cc, uint16_t cs) {
    weer_wolk(cx, cy - 4, w, h/2, cc);
    for (int i = -1; i <= 1; i++) {
        tft.drawPixel(cx + i*8,     cy + h/3, cs);
        tft.drawPixel(cx + i*8 + 1, cy + h/3, cs);
        tft.fillCircle(cx + i*8, cy + h/2, 2, cs);
    }
}

static void weer_onweer(int cx, int cy, int w, int h, uint16_t cc, uint16_t cl) {
    weer_wolk(cx, cy - 4, w, h/2, cc);
    // bliksem
    tft.drawLine(cx + 2, cy + h/4,     cx - 3, cy + h/4 + 8, cl);
    tft.drawLine(cx - 3, cy + h/4 + 8, cx + 3, cy + h/4 + 8, cl);
    tft.drawLine(cx + 3, cy + h/4 + 8, cx - 2, cy + h/2 + 4, cl);
}

static void weer_icon(int code, int cx, int cy, int maat, bool dag) {
    uint16_t czon  = RGB565(255, 220, 40);
    uint16_t cwolk = RGB565(130, 145, 165);
    uint16_t cregen = RGB565(80, 160, 255);
    uint16_t csneeuw = RGB565(200, 220, 255);
    uint16_t cbliksem = RGB565(255, 230, 50);
    uint16_t cmaan = RGB565(210, 210, 160);

    int r = maat / 2;
    if (code == 0) {
        if (dag) weer_zon(cx, cy, r, czon);
        else { tft.drawCircle(cx, cy, r, cmaan); tft.fillCircle(cx + r/3, cy - r/4, r*3/4, C_BG); }
    } else if (code <= 2) {
        if (dag) { weer_zon(cx - r/2, cy, r*2/3, czon); }
        else { tft.fillCircle(cx - r/2, cy, r*2/3, cmaan); }
        weer_wolk(cx + r/3, cy + r/4, maat*2/3, maat/2, cwolk);
    } else if (code == 3) {
        weer_wolk(cx, cy, maat*3/4, maat/2, cwolk);
    } else if (code <= 48) {
        // Mist
        for (int i = 0; i < 3; i++)
            tft.drawFastHLine(cx - r, cy - 4 + i*8, maat, cwolk);
    } else if (code <= 67) {
        weer_regen(cx, cy, maat*3/4, maat/2, cwolk, cregen);
    } else if (code <= 77) {
        weer_sneeuw(cx, cy, maat*3/4, maat/2, cwolk, csneeuw);
    } else if (code <= 82) {
        weer_regen(cx, cy, maat*3/4, maat/2, cwolk, cregen);
    } else {
        weer_onweer(cx, cy, maat*3/4, maat/2, cwolk, cbliksem);
    }
}

// ─── Windpijl (kompas) ────────────────────────────────────────────────────
static void wind_kompas(int cx, int cy, int r, int graden, float ms) {
    tft.drawCircle(cx, cy, r, C_SURFACE3);
    tft.drawCircle(cx, cy, r + 1, C_SURFACE2);
    // Richtingspijl
    float rad = (graden - 90) * M_PI / 180.0f;
    int ax = cx + (r - 4) * cos(rad);
    int ay = cy + (r - 4) * sin(rad);
    int bx = cx - 8 * cos(rad);
    int by = cy - 8 * sin(rad);
    tft.drawLine(bx, by, ax, ay, C_CYAN);
    tft.fillCircle(ax, ay, 3, C_CYAN);
    // Beaufort in midden
    tft.setTextSize(2);
    tft.setTextColor(C_TEXT);
    char buf[4]; snprintf(buf, 4, "B%d", meteo_beaufort(ms));
    int tw = strlen(buf) * 12;
    tft.setCursor(cx - tw/2, cy - 8);
    tft.print(buf);
}

// ─── Tabs tekenen ─────────────────────────────────────────────────────────
static void meteo_tabs_teken() {
    const char* tabs[TAB_CNT] = { "WEER", "GETIJ", "LOCATIE" };
    tft.fillRect(0, TAB_Y, TFT_W, TAB_H + 2, C_BG);
    for (int i = 0; i < TAB_CNT; i++) {
        int x = i * TAB_W;
        bool actief = (i == meteo_tab);
        uint16_t bg = actief ? C_SURFACE2 : C_SURFACE;
        uint16_t fg = actief ? C_CYAN    : C_TEXT_DIM;
        tft.fillRect(x, TAB_Y, TAB_W, TAB_H, bg);
        if (actief) tft.drawFastHLine(x, TAB_Y + TAB_H - 2, TAB_W, C_CYAN);
        tft.drawFastVLine(x + TAB_W - 1, TAB_Y, TAB_H, C_SURFACE3);
        ui_tekst_midden(x, TAB_Y + 6, TAB_W, tabs[i], fg, 1);
    }
    tft.drawFastHLine(0, TAB_Y + TAB_H, TFT_W, C_SURFACE3);
}

// ─── Status bar titel ─────────────────────────────────────────────────────
static void meteo_sb_teken() {
    tft.fillRect(0, 0, TFT_W, SB_H, C_STATUSBAR);
    tft.drawFastHLine(0, SB_H - 1, TFT_W, C_SURFACE2);
    tft.setTextSize(2);
    tft.setTextColor(C_CYAN);
    tft.setCursor(10, (SB_H - 16) / 2);
    tft.print("METEO");
    // Locatie + tijdstip laatste update
    if (meteo_geladen) {
        tft.setTextSize(1);
        tft.setTextColor(C_TEXT_DIM);
        tft.setCursor(130, (SB_H - 8) / 2);
        tft.print(meteo_stad);
        tft.print("  \x7e ");
        tft.print(getij_stations[meteo_station_idx].naam);
    } else {
        tft.setTextSize(1);
        tft.setTextColor(C_TEXT_DIM);
        tft.setCursor(130, (SB_H - 8) / 2);
        tft.print(wifi_verbonden ? "Ophalen..." : "Geen WiFi");
    }
}

// ─── WEER TAB ─────────────────────────────────────────────────────────────
static void meteo_weer_teken() {
    tft.fillRect(0, PANEL_Y, TFT_W, PANEL_H, C_BG);

    if (!meteo_geladen) {
        ui_tekst_midden(0, PANEL_Y + PANEL_H/2 - 8, TFT_W, "Geen weerdata beschikbaar", C_TEXT_DIM, 1);
        return;
    }

    // ── Linker blok: actueel weer ────────────────────────────────────────
    int lx = 10, ly = PANEL_Y + 8;
    int lw = 310, lh = 160;
    tft.fillRoundRect(lx, ly, lw, lh, 6, C_SURFACE);

    // Weericon (groot)
    weer_icon(meteo_weer_code, lx + 50, ly + lh/2, 48, meteo_is_dag);

    // Temperatuur
    tft.setTextSize(3);
    tft.setTextColor(C_TEXT);
    char tbuf[10];
    snprintf(tbuf, 10, "%.1f\xF7", meteo_temp);  // ° via 0xF7 in GFX font
    tft.setCursor(lx + 100, ly + 20);
    tft.print(tbuf);

    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(lx + 100, ly + 60);
    tft.print("max vandaag: ");
    tft.setTextColor(C_TEXT);
    char mxbuf[10]; snprintf(mxbuf, 10, "%.1f\xF7", meteo_temp_max);
    tft.print(mxbuf);

    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(lx + 100, ly + 78);
    tft.print(meteo_weer_omschrijving(meteo_weer_code));

    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(lx + 100, ly + 96);
    tft.print("Wind: ");
    tft.setTextColor(C_TEXT);
    char wbuf[24];
    snprintf(wbuf, 24, "%s  B%d  (%.1fm/s)", meteo_wind_richting(meteo_wind_dir), meteo_beaufort(meteo_wind_ms), meteo_wind_ms);
    tft.print(wbuf);

    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(lx + 100, ly + 114);
    tft.print("Stoten: B");
    tft.setTextColor(meteo_beaufort(meteo_wind_max) >= 6 ? C_ORANGE : C_TEXT);
    char gbuf[6]; snprintf(gbuf, 6, "%d", meteo_beaufort(meteo_wind_max));
    tft.print(gbuf);
    tft.setTextColor(C_TEXT_DIM);
    snprintf(gbuf, 6, " (%.1f)", meteo_wind_max);
    tft.print(gbuf);

    // Windkompas
    wind_kompas(lx + 260, ly + lh/2, 32, meteo_wind_dir, meteo_wind_ms);

    // ── Midden blok: 4-daagse vooruitzichten ────────────────────────────
    int dx = lx, dy = ly + lh + 8;
    int dw = (TFT_W - 20) / 4 - 4;
    int dh = PANEL_H - lh - 20;
    if (dh < 80) dh = 80;

    for (int i = 0; i < 4; i++) {
        int bx = dx + i * (dw + 4);
        bool vndg = (i == 0);
        uint16_t dbg = vndg ? C_SURFACE2 : C_SURFACE;
        tft.fillRoundRect(bx, dy, dw, dh, 5, dbg);
        // Dagnaam
        tft.setTextSize(1);
        tft.setTextColor(vndg ? C_CYAN : C_TEXT_DIM);
        ui_tekst_midden(bx, dy + 5, dw, meteo_dag_naam[i], vndg ? C_CYAN : C_TEXT_DIM, 1);
        // Weericon (klein)
        weer_icon(meteo_dag_code[i], bx + dw/2, dy + 36, 28, true);
        // Temperatuur
        char dmbuf[12];
        snprintf(dmbuf, 12, "%.0f/%.0f\xF7", meteo_dag_temp_max[i], meteo_dag_temp_min[i]);
        tft.setTextSize(1);
        tft.setTextColor(C_TEXT);
        ui_tekst_midden(bx, dy + 56, dw, dmbuf, C_TEXT, 1);
        // Wind
        char dwbuf[8];
        snprintf(dwbuf, 8, "%s B%d", meteo_wind_richting(meteo_dag_wind_dir[i]), meteo_beaufort(meteo_dag_wind[i]));
        tft.setTextColor(C_TEXT_DIM);
        ui_tekst_midden(bx, dy + 70, dw, dwbuf, C_TEXT_DIM, 1);
    }

    // ── Rechter blok: compact getij ──────────────────────────────────────
    int rx = lx + lw + 8, ry = ly;
    int rw = TFT_W - rx - 10, rh = lh;
    tft.fillRoundRect(rx, ry, rw, rh, 6, C_SURFACE);
    tft.setTextSize(1);
    tft.setTextColor(C_CYAN);
    ui_tekst_midden(rx, ry + 5, rw, "GETIJ", C_CYAN, 1);
    tft.setTextColor(C_TEXT_DIM);
    ui_tekst_midden(rx, ry + 18, rw, getij_stations[meteo_station_idx].naam, C_TEXT_DIM, 1);

    int cnt = min(getij_ext_cnt, 4);
    for (int i = 0; i < cnt; i++) {
        const GetijExtreme& e = getij_ext[i];
        struct tm* lt = localtime(&e.tijd);
        char tijdbuf[8]; snprintf(tijdbuf, 8, "%02d:%02d", lt->tm_hour, lt->tm_min);
        char hbuf[12];
        float lat_af = e.hoogte - getij_stations[meteo_station_idx].LAT_nap;
        snprintf(hbuf, 12, "%.2fm+%.1f", e.hoogte, lat_af);
        int ey = ry + 36 + i * 28;
        uint16_t ec = e.hoog_water ? C_BLUE : C_TEXT_DIM;
        uint16_t es = e.hoog_water ? RGB565(0,60,140) : C_SURFACE2;
        tft.fillRoundRect(rx + 4, ey, rw - 8, 24, 4, es);
        tft.setTextColor(ec);
        tft.setCursor(rx + 8, ey + 5);
        tft.print(e.hoog_water ? "HW " : "LW ");
        tft.print(tijdbuf);
        tft.setTextColor(C_TEXT);
        tft.setCursor(rx + rw/2, ey + 5);
        tft.print(hbuf);
    }
    if (getij_ext_cnt == 0) {
        ui_tekst_midden(rx, ry + lh/2, rw, "Geen data", C_TEXT_DIM, 1);
    }
}

// ─── GETIJ TAB ────────────────────────────────────────────────────────────
static void _getij_grafiek(int gx, int gy, int gw, int gh) {
    tft.fillRect(gx, gy, gw, gh, C_SURFACE);
    tft.drawRect(gx, gy, gw, gh, C_SURFACE3);
    if (getij_ext_cnt < 2) return;

    // Bepaal min/max hoogte voor schaling
    float hmin = 99.0f, hmax = -99.0f;
    for (int i = 0; i < getij_ext_cnt; i++) {
        if (getij_ext[i].hoogte < hmin) hmin = getij_ext[i].hoogte;
        if (getij_ext[i].hoogte > hmax) hmax = getij_ext[i].hoogte;
    }
    if (hmax - hmin < 0.1f) hmax = hmin + 0.1f;

    // LAT-nap referentielijn
    const GetijStation& s = getij_stations[meteo_station_idx];
    float lat_y_frac = 1.0f - (s.LAT_nap - hmin) / (hmax - hmin);
    // lat_y_frac kan < 0 zijn als LAT_nap onder hmin
    if (lat_y_frac >= 0.0f && lat_y_frac <= 1.0f) {
        int yl = gy + (int)(lat_y_frac * gh);
        tft.drawFastHLine(gx, yl, gw, C_TEXT_DIM);
        tft.setTextSize(1);
        tft.setTextColor(C_TEXT_DIM);
        tft.setCursor(gx + 2, yl + 2);
        tft.print("LAT");
    }

    // Tijdrange
    time_t t0 = getij_ext[0].tijd;
    time_t t1 = getij_ext[getij_ext_cnt-1].tijd;
    if (t1 <= t0) return;
    float tspan = (float)(t1 - t0);

    // Teken sinusoïde via extremen
    int prev_px = -1, prev_py = -1;
    for (int px = 0; px < gw; px++) {
        time_t t = t0 + (time_t)((float)px / gw * tspan);
        // Interpoleer tussen dichtstbijzijnde extremen
        float h = getij_ext[0].hoogte;
        for (int i = 0; i < getij_ext_cnt - 1; i++) {
            if (t >= getij_ext[i].tijd && t <= getij_ext[i+1].tijd) {
                float frac = (float)(t - getij_ext[i].tijd) /
                             (float)(getij_ext[i+1].tijd - getij_ext[i].tijd);
                // Cosinus interpolatie
                float c = (1.0f - cosf(frac * M_PI)) / 2.0f;
                h = getij_ext[i].hoogte + c * (getij_ext[i+1].hoogte - getij_ext[i].hoogte);
                break;
            }
        }
        float norm = 1.0f - (h - hmin) / (hmax - hmin);
        int py = gy + (int)(norm * (gh - 4)) + 2;
        if (prev_px >= 0) {
            tft.drawLine(gx + prev_px, prev_py, gx + px, py, C_CYAN);
        }
        prev_px = px; prev_py = py;
    }

    // Nu-lijn
    time_t nu = time(nullptr);
    if (nu >= t0 && nu <= t1) {
        int nx = gx + (int)((float)(nu - t0) / tspan * gw);
        tft.drawFastVLine(nx, gy, gh, C_RED_BRIGHT);
    }

    // Tijdlabels bij extremen
    for (int i = 0; i < getij_ext_cnt; i++) {
        int px = gx + (int)((float)(getij_ext[i].tijd - t0) / tspan * gw);
        float norm = 1.0f - (getij_ext[i].hoogte - hmin) / (hmax - hmin);
        int py = gy + (int)(norm * (gh - 4)) + 2;
        uint16_t c = getij_ext[i].hoog_water ? C_BLUE : C_TEXT_DIM;
        tft.fillCircle(px, py, 3, c);
        struct tm* lt = localtime(&getij_ext[i].tijd);
        char tbuf[6]; snprintf(tbuf, 6, "%02d:%02d", lt->tm_hour, lt->tm_min);
        tft.setTextSize(1);
        tft.setTextColor(c);
        tft.setCursor(max(gx + 1, px - 10), py + (getij_ext[i].hoog_water ? -12 : 4));
        tft.print(tbuf);
    }
}

static void meteo_getij_teken() {
    tft.fillRect(0, PANEL_Y, TFT_W, PANEL_H, C_BG);

    // Station + locatieinfo
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(10, PANEL_Y + 6);
    tft.print("Station: ");
    tft.setTextColor(C_CYAN);
    tft.print(getij_stations[meteo_station_idx].naam);
    tft.setTextColor(C_TEXT_DIM);
    tft.print("   LAT = ");
    tft.setTextColor(C_TEXT);
    char labuf[12];
    snprintf(labuf, 12, "%.2fm NAP", getij_stations[meteo_station_idx].LAT_nap);
    tft.print(labuf);

    // Getijgrafiek
    int gx = 10, gy = PANEL_Y + 22;
    int gw = TFT_W - 20, gh = 140;
    _getij_grafiek(gx, gy, gw, gh);

    // Tabel met alle extremen
    int ty = gy + gh + 8;
    int kolomw = (TFT_W - 20) / 4;
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(gx, ty);
    tft.print("Tijd    Soort  Hoogte(NAP) +LAT");

    int cnt = min(getij_ext_cnt, (int)((PANEL_H - gh - 36) / 24));
    for (int i = 0; i < cnt; i++) {
        const GetijExtreme& e = getij_ext[i];
        int ey = ty + 14 + i * 24;
        bool hw = e.hoog_water;
        uint16_t bg = hw ? RGB565(0,50,120) : RGB565(20,30,55);
        tft.fillRoundRect(gx, ey, TFT_W - 20, 20, 4, bg);

        struct tm* lt = localtime(&e.tijd);
        char tbuf[24];
        snprintf(tbuf, 24, "%s %02d:%02d", lt->tm_wday == 0 ? "Zo" :
            lt->tm_wday == 1 ? "Ma" : lt->tm_wday == 2 ? "Di" :
            lt->tm_wday == 3 ? "Wo" : lt->tm_wday == 4 ? "Do" :
            lt->tm_wday == 5 ? "Vr" : "Za", lt->tm_hour, lt->tm_min);

        uint16_t fc = hw ? C_BLUE : C_TEXT;
        tft.setTextColor(fc);
        tft.setCursor(gx + 6, ey + 5);
        tft.print(tbuf);
        tft.print("  ");
        tft.print(hw ? "HW" : "LW");

        char hbuf[20];
        float lat_af = e.hoogte - getij_stations[meteo_station_idx].LAT_nap;
        snprintf(hbuf, 20, "  %.2f m NAP  +%.2f m", e.hoogte, lat_af);
        tft.setTextColor(C_TEXT);
        tft.print(hbuf);
    }
}

// ─── LOCATIE TAB ──────────────────────────────────────────────────────────
static int locatie_sel = -1;  // welke stationsknop hover/select

static void meteo_locatie_teken() {
    tft.fillRect(0, PANEL_Y, TFT_W, PANEL_H, C_BG);

    // Huidige locatie info
    tft.fillRoundRect(10, PANEL_Y + 6, 380, 50, 6, C_SURFACE);
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(20, PANEL_Y + 14);
    tft.print("Locatie (IP): ");
    tft.setTextColor(C_TEXT);
    tft.print(meteo_stad);
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(20, PANEL_Y + 30);
    char lbuf[40];
    snprintf(lbuf, 40, "%.4f N  %.4f E", meteo_lat, meteo_lon);
    tft.print(lbuf);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(20, PANEL_Y + 46);
    tft.print("Station: ");
    tft.setTextColor(C_CYAN);
    tft.print(getij_stations[meteo_station_idx].naam);

    // Vernieuwen knop
    ui_knop(400, PANEL_Y + 10, 200, 40, "Locatie vernieuwen", C_SURFACE2, C_CYAN, false);

    // Stationsknoppen
    int sx = 10, sy = PANEL_Y + 66;
    int sw = (TFT_W - 20 - 12) / 4;
    int sh = 52;
    int rijen = (GETIJ_STATIONS + 3) / 4;
    for (int i = 0; i < GETIJ_STATIONS; i++) {
        int col = i % 4, rij = i / 4;
        int bx = sx + col * (sw + 4);
        int by = sy + rij * (sh + 4);
        bool actief = (i == meteo_station_idx);
        uint16_t bg = actief ? C_SURFACE3 : C_SURFACE;
        uint16_t fg = actief ? C_CYAN : C_TEXT;
        tft.fillRoundRect(bx, by, sw, sh, 5, bg);
        if (actief) tft.drawRoundRect(bx, by, sw, sh, 5, C_CYAN);
        tft.setTextSize(1);
        tft.setTextColor(fg);
        ui_tekst_midden(bx, by + 8, sw, getij_stations[i].naam, fg, 1);
        char coordbuf[20];
        snprintf(coordbuf, 20, "%.2fN %.2fE", getij_stations[i].lat, getij_stations[i].lon);
        ui_tekst_midden(bx, by + 22, sw, coordbuf, C_TEXT_DIM, 1);
        char hwfcbuf[14];
        snprintf(hwfcbuf, 14, "HWF&C: %.2fh", getij_stations[i].hwfc);
        ui_tekst_midden(bx, by + 36, sw, hwfcbuf, C_TEXT_DIM, 1);
    }

    // Uitleg onderaan
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    tft.setCursor(10, TFT_H - NAV_H - 16);
    tft.print("Selecteer een station voor getij & weerlocatie. Locatie vernieuwen haalt positie opnieuw op via WiFi.");
}

// ─── Hoofdfuncties ────────────────────────────────────────────────────────
void screen_meteo_teken() {
    tft.fillScreen(C_BG);
    meteo_sb_teken();
    meteo_tabs_teken();
    nav_bar_teken();

    switch (meteo_tab) {
        case METEO_TAB_WEER:    meteo_weer_teken();    break;
        case METEO_TAB_GETIJ:   meteo_getij_teken();   break;
        case METEO_TAB_LOCATIE: meteo_locatie_teken(); break;
    }
}

void screen_meteo_run(int x, int y, bool aanraking) {
    if (!aanraking) return;

    // Tab klik?
    if (y >= TAB_Y && y < TAB_Y + TAB_H) {
        int tab = x / TAB_W;
        if (tab >= 0 && tab < TAB_CNT && tab != meteo_tab) {
            meteo_tab = tab;
            meteo_tabs_teken();
            tft.fillRect(0, PANEL_Y, TFT_W, PANEL_H, C_BG);
            switch (meteo_tab) {
                case METEO_TAB_WEER:    meteo_weer_teken();    break;
                case METEO_TAB_GETIJ:   meteo_getij_teken();   break;
                case METEO_TAB_LOCATIE: meteo_locatie_teken(); break;
            }
        }
        return;
    }

    // Locatie tab interactie
    if (meteo_tab == METEO_TAB_LOCATIE) {
        // Vernieuwen knop (400, PANEL_Y + 10, 200, 40)
        if (x >= 400 && x <= 600 && y >= PANEL_Y + 10 && y <= PANEL_Y + 50) {
            meteo_locatie_ophalen();
            meteo_getij_berekenen();
            meteo_locatie_teken();
            return;
        }
        // Stationsknop
        int sx = 10, sy = PANEL_Y + 66;
        int sw = (TFT_W - 20 - 12) / 4;
        int sh = 52;
        for (int i = 0; i < GETIJ_STATIONS; i++) {
            int col = i % 4, rij = i / 4;
            int bx = sx + col * (sw + 4);
            int by = sy + rij * (sh + 4);
            if (x >= bx && x <= bx + sw && y >= by && y <= by + sh) {
                meteo_station_idx = i;
                meteo_inst_opslaan();
                meteo_getij_berekenen();
                meteo_locatie_teken();
                return;
            }
        }
    }
}
