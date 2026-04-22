#include "hw_touch.h"
#include "hw_scherm.h"

void ts_setup() {
    ts.begin();
    ts.setRotation(0);
}

bool ts_touched() {
    ts.begin();
    delay(8);
    if (ts.isTouched) {
        scherm_touched = millis();
        actieve_touch  = true;
        ts_x = touch_x();
        ts_y = touch_y();
        return true;
    }
    actieve_touch = false;
    return false;
}

// Landscape: raw Y → display X (0-800), raw X inverted → display Y (0-480)
int touch_x() {
    ts.read();
    return map(ts.points[0].y, 5, 800, 0, TFT_W);
}

int touch_y() {
    ts.read();
    return map(ts.points[0].x, 490, 5, 0, TFT_H);
}
