#include "screen_io.h"
#include "nav_bar.h"

int io_scroll_offset = 0;

static void io_rij_teken(int kanaal, int rij_y) {
    if (kanaal >= io_kanalen_cnt || kanaal >= MAX_IO_KANALEN) {
        tft.fillRect(70, rij_y, TFT_W - 120, IO_RIJ_H - 2, C_BG);
        return;
    }

    int x = 70;
    int h = IO_RIJ_H - 4;
    int w = TFT_W - 140;
    byte staat  = io_licht_staat(kanaal);
    byte output = io_output[kanaal];

    // Achtergrond
    uint16_t bg = (kanaal % 2 == 0) ? C_SURFACE : C_BG;
    tft.fillRoundRect(x, rij_y + 2, w, h, 5, bg);

    // Kanaal nummer
    tft.setTextSize(2);
    tft.setTextColor(C_TEXT_DIM);
    char nr[5];
    snprintf(nr, sizeof(nr), "%3d", kanaal);
    tft.setCursor(x + 6, rij_y + (IO_RIJ_H - 16) / 2);
    tft.print(nr);

    // Naam
    tft.setTextColor(C_TEXT);
    tft.setCursor(x + 50, rij_y + (IO_RIJ_H - 16) / 2);
    tft.print(io_namen[kanaal]);

    // Status indicator
    uint16_t staat_kleur[] = {C_LIGHT_OFF, C_LIGHT_COOLING, C_LIGHT_PENDING, C_GREEN};
    const char* staat_txt[] = {"UIT", "KOELT", "WACHT", "AAN"};
    int ind_x = x + w - 180;
    tft.fillRoundRect(ind_x, rij_y + 6, 70, h - 12, 4, staat_kleur[staat]);
    tft.setTextSize(1);
    tft.setTextColor(staat == LSTATE_ECHT_AAN ? C_TEXT_DARK : C_TEXT_DIM);
    int tw = strlen(staat_txt[staat]) * 6;
    tft.setCursor(ind_x + (70 - tw) / 2, rij_y + (IO_RIJ_H - 8) / 2);
    tft.print(staat_txt[staat]);

    // Schakelaar knop
    int sw_x = x + w - 100;
    bool aan = (output == IO_AAN || output == IO_INV_AAN);
    uint16_t sw_bg = aan ? RGB565(0, 140, 60) : C_SURFACE2;
    tft.fillRoundRect(sw_x, rij_y + 8, 80, h - 16, 6, sw_bg);
    tft.setTextSize(2);
    tft.setTextColor(aan ? C_WHITE : C_TEXT_DIM);
    const char* sw_lbl = aan ? "AAN" : "UIT";
    tw = strlen(sw_lbl) * 12;
    tft.setCursor(sw_x + (80 - tw) / 2, rij_y + (IO_RIJ_H - 16) / 2);
    tft.print(sw_lbl);

    // Input dot
    tft.fillCircle(x + w - 12, rij_y + IO_RIJ_H / 2, 6,
                   io_input[kanaal] ? C_GREEN : C_DARK_GRAY);
}

void screen_io_teken_rijen() {
    int content_y = CONTENT_Y;
    tft.fillRect(0, content_y, TFT_W, CONTENT_H, C_BG);

    // Scrollknoppen links/rechts (pijlen omhoog/omlaag)
    tft.fillRoundRect(10, content_y + 10, IO_SCROLL_BTN_W, CONTENT_H - 20, 8, C_SURFACE);
    tft.setTextSize(3);
    tft.setTextColor(C_TEXT);
    tft.setCursor(20, content_y + CONTENT_H / 2 - 30);
    tft.print("^");
    tft.setCursor(20, content_y + CONTENT_H / 2 + 10);
    tft.print("v");

    tft.fillRoundRect(TFT_W - IO_SCROLL_BTN_W - 10, content_y + 10, IO_SCROLL_BTN_W, CONTENT_H - 20, 8, C_SURFACE);

    // Scroll positie indicator
    int max_offset = max(0, io_kanalen_cnt - IO_RIJEN_PER_PAGINA);
    if (max_offset > 0) {
        int bar_h = CONTENT_H - 20;
        int pos_h = max(20, bar_h * IO_RIJEN_PER_PAGINA / max(io_kanalen_cnt, 1));
        int pos_y = content_y + 10 + (bar_h - pos_h) * io_scroll_offset / max_offset;
        tft.fillRoundRect(TFT_W - IO_SCROLL_BTN_W - 10, pos_y, IO_SCROLL_BTN_W, pos_h, 6, C_SURFACE2);
    }

    // Kanaal rijen
    for (int r = 0; r < IO_RIJEN_PER_PAGINA; r++) {
        int kanaal = io_scroll_offset + r;
        int rij_y  = content_y + r * IO_RIJ_H;
        io_rij_teken(kanaal, rij_y);
    }

    // Header tekst
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_DIM);
    char hdr[40];
    snprintf(hdr, sizeof(hdr), "IO kanalen: %d  (offset %d)", io_kanalen_cnt, io_scroll_offset);
    tft.setCursor(70, content_y + 2);
    tft.print(hdr);
}

void screen_io_teken() {
    tft.fillScreen(C_BG);

    // Status bar
    tft.fillRect(0, 0, TFT_W, SB_H, C_STATUSBAR);
    tft.drawFastHLine(0, SB_H - 1, TFT_W, C_SURFACE2);
    tft.setTextSize(2);
    tft.setTextColor(C_CYAN);
    tft.setCursor(10, (SB_H - 16) / 2);
    tft.print("IO OVERZICHT");

    screen_io_teken_rijen();
    nav_bar_teken();
}

void screen_io_run(int x, int y, bool aanraking) {
    // Periodieke update
    if (!aanraking) {
        if (io_runned) {
            screen_io_teken_rijen();
            io_runned = false;
        }
        return;
    }

    int nav = nav_bar_klik(x, y);
    if (nav >= 0 && nav != actief_scherm) {
        actief_scherm = nav;
        scherm_bouwen = true;
        return;
    }

    // Scroll omhoog
    if (x < 70 && y > CONTENT_Y && y < TFT_H - NAV_H) {
        if (y < CONTENT_Y + CONTENT_H / 2) {
            io_scroll_offset = max(0, io_scroll_offset - IO_RIJEN_PER_PAGINA);
        } else {
            io_scroll_offset = min(max(0, io_kanalen_cnt - IO_RIJEN_PER_PAGINA),
                                   io_scroll_offset + IO_RIJEN_PER_PAGINA);
        }
        screen_io_teken_rijen();
        return;
    }
    // Scroll omlaag (rechterknop)
    if (x > TFT_W - 70 && y > CONTENT_Y && y < TFT_H - NAV_H) {
        if (y < CONTENT_Y + CONTENT_H / 2) {
            io_scroll_offset = max(0, io_scroll_offset - 1);
        } else {
            io_scroll_offset = min(max(0, io_kanalen_cnt - IO_RIJEN_PER_PAGINA),
                                   io_scroll_offset + 1);
        }
        screen_io_teken_rijen();
        return;
    }

    // Schakelaar klik
    if (y > CONTENT_Y && y < TFT_H - NAV_H) {
        int rij  = (y - CONTENT_Y) / IO_RIJ_H;
        int kanaal = io_scroll_offset + rij;
        if (kanaal < io_kanalen_cnt && kanaal < MAX_IO_KANALEN) {
            // Check of de schakelaar geraakt is
            int rij_y = CONTENT_Y + rij * IO_RIJ_H;
            int rx = TFT_W - 140 + 70 - 100;   // sw_x berekening
            int sw_x = 70 + (TFT_W - 140) - 100;
            if (x >= sw_x && x <= sw_x + 80) {
                // Toggle output
                if (io_output[kanaal] == IO_UIT || io_output[kanaal] == IO_INV_UIT) {
                    io_output[kanaal] = (io_output[kanaal] == IO_UIT) ? IO_AAN : IO_INV_AAN;
                } else {
                    io_output[kanaal] = (io_output[kanaal] == IO_AAN) ? IO_UIT : IO_INV_UIT;
                }
                io_rij_teken(kanaal, rij_y);
            }
        }
    }
}
