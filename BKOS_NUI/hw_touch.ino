#include "hw_touch.h"
#include "hw_scherm.h"

// Definities (extern gedeclareerd in hw_touch.h)
bool actieve_touch = false;
int  ts_x = 0;
int  ts_y = 0;

#if PLATFORM_ESP32
  TAMC_GT911 ts(TS_SDA, TS_SCK, -1, TS_RST, 490, 480);
#elif defined(PICO_TOUCH_XPT2046)
  XPT2046_Touchscreen ts(PICO_TS_CS, PICO_TS_IRQ);
#endif

void ts_setup() {
#if PLATFORM_ESP32
    ts.begin();
    ts.setRotation(0);
#elif defined(PICO_TOUCH_XPT2046)
    SPI.begin();
    ts.begin();
#endif
}

bool ts_touched() {
#if PLATFORM_ESP32
    ts.read();
    if (ts.isTouched) {
        scherm_touched = millis();
        actieve_touch  = true;
        ts_x = touch_x();
        ts_y = touch_y();
        return true;
    }
    actieve_touch = false;
    return false;

#elif defined(PICO_TOUCH_XPT2046)
    // tirqTouched() werkt alleen als IRQ pin aangesloten is;
    // bij PICO_TS_IRQ == -1 altijd ts.touched() pollen
    bool aangeraakt = ts.tirqTouched() && ts.touched();
    if (aangeraakt) {
        scherm_touched = millis();
        actieve_touch  = true;
        ts_x = touch_x();
        ts_y = touch_y();
        return true;
    }
    actieve_touch = false;
    return false;

#else
    // Geen touch hardware geconfigureerd
    actieve_touch = false;
    return false;
#endif
}

int touch_x() {
#if PLATFORM_ESP32
    // Liggend: raw Y → display X
    return map(ts.points[0].y, 5, 800, 0, TFT_W);
#elif defined(PICO_TOUCH_XPT2046)
    // Portret ILI9341: touch-paneel 90° gedraaid → p.y → display X
    TS_Point p = ts.getPoint();
    return map(p.y, 200, 3700, 0, TFT_W);
#else
    return 0;
#endif
}

int touch_y() {
#if PLATFORM_ESP32
    // Liggend: raw X omgekeerd → display Y
    return map(ts.points[0].x, 490, 5, 0, TFT_H);
#elif defined(PICO_TOUCH_XPT2046)
    // Portret ILI9341: touch-paneel 90° gedraaid → p.x → display Y
    TS_Point p = ts.getPoint();
    return map(p.x, 200, 3700, 0, TFT_H);
#else
    return 0;
#endif
}
