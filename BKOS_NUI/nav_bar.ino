#include "nav_bar.h"

void nav_bar_teken() {
    int y = NAV_Y;
    tft.fillRect(0, y, TFT_W, NAV_H, C_NAVBAR);
    tft.drawFastHLine(0, y, TFT_W, C_SURFACE2);

    int bw = TFT_W / NAV_ITEMS;
    for (int i = 0; i < NAV_ITEMS; i++) {
        int x = i * bw;
        bool actief = (actief_scherm == i);

        if (actief) {
            tft.fillRect(x + 2, y + 2, bw - 4, NAV_H - 4, C_SURFACE2);
            tft.drawFastHLine(x + 8, y, bw - 16, C_CYAN);
            tft.drawFastHLine(x + 8, y + 1, bw - 16, C_CYAN);
        }

        tft.setTextSize(2);
        tft.setTextColor(actief ? C_CYAN : C_TEXT_DIM);
        int tw = strlen(nav_labels[i]) * 12;
        tft.setCursor(x + (bw - tw) / 2, y + (NAV_H - 16) / 2);
        tft.print(nav_labels[i]);

        if (i < NAV_ITEMS - 1)
            tft.drawFastVLine(x + bw - 1, y + 6, NAV_H - 12, C_SURFACE2);
    }
}

int nav_bar_klik(int x, int y) {
    if (y < NAV_Y || y >= TFT_H) return -1;
    int bw = TFT_W / NAV_ITEMS;
    int idx = x / bw;
    if (idx >= 0 && idx < NAV_ITEMS) return idx;
    return -1;
}
