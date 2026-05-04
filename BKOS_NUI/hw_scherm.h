#pragma once
#include <Arduino_GFX_Library.h>
#include "ui_colors.h"

#define TFT_BL         2
#define TFT_W          800
#define TFT_H          480
#define TFT_MIN_HELDER 3   // GT911 slaap-drempel: nooit volledig uit

extern Arduino_ESP32RGBPanel *rgbpanel;
extern Arduino_RGB_Display    tft;

extern int           tft_helderheid;
extern long          scherm_timer;
extern bool          tft_actief;
extern long          scherm_touched;
extern bool          scherm_net_gewekt;
extern bool          tft_bijna_uit;
extern unsigned long tft_dim_ms;

void tft_setup();
void tft_loop();
void tft_helderheid_zet(int pct);
void tft_schermvullen(uint16_t kleur);

extern byte bkos_logo_200_75[];
void tft_logo(int32_t x, int32_t y, int schaal, uint16_t kleur);
