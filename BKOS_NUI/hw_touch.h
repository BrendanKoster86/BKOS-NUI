#pragma once
#include <TAMC_GT911.h>

#define TS_SDA  19
#define TS_SCK  20
#define TS_RST  38

TAMC_GT911 ts(TS_SDA, TS_SCK, -1, TS_RST, 490, 480);

bool actieve_touch = false;
int  ts_x = 0;
int  ts_y = 0;

void ts_setup();
bool ts_touched();
int  touch_x();
int  touch_y();
