#pragma once
#include "platform.h"

#if PLATFORM_ESP32 && !PLATFORM_WROOM
  #include <TAMC_GT911.h>
  #define TS_SDA  19
  #define TS_SCK  20
  #define TS_RST  38
  extern TAMC_GT911 ts;
#elif defined(PICO_TOUCH_XPT2046) || defined(WROOM_TOUCH_XPT2046)
  #include <XPT2046_Touchscreen.h>
  extern XPT2046_Touchscreen ts;
#endif

extern bool actieve_touch;
extern int  ts_x;
extern int  ts_y;

void ts_setup();
bool ts_touched();
int  touch_x();
int  touch_y();
