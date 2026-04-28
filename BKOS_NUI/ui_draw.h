#pragma once
#include "hw_scherm.h"
#include "ui_colors.h"

// Layout constanten
#define SB_H       42    // Status bar hoogte
#define NAV_H      42    // Nav bar hoogte
#define NAV_Y      (TFT_H - NAV_H)  // Nav bar y positie
#define CONTENT_Y  SB_H
#define CONTENT_H  (TFT_H - SB_H - NAV_H)

// Bootvlak breedte (linker paneel)
#define BOOT_PANEL_W  390
#define CTRL_PANEL_X  400
#define CTRL_PANEL_W  (TFT_W - CTRL_PANEL_X)

// Knop afmetingen
#define KNOP_R  8    // corner radius

void ui_rrect(int x, int y, int w, int h, uint16_t kleur);
void ui_rrect_gevuld(int x, int y, int w, int h, uint16_t kleur);
void ui_rrect_gevuld_rand(int x, int y, int w, int h, uint16_t vul, uint16_t rand, int dikte = 2);
void ui_knop(int x, int y, int w, int h, const char* tekst, uint16_t bg, uint16_t fg, bool actief = false);
void ui_knop_groot(int x, int y, int w, int h, const char* regel1, const char* regel2,
                   uint16_t bg, uint16_t fg, uint16_t accent, bool actief = false);
void ui_licht_cirkel(int cx, int cy, int r, byte staat);
void ui_glow(int cx, int cy, int r, uint16_t kleur, int lagen = 3);
void ui_tekst_midden(int x, int y, int w, const char* tekst, uint16_t kleur, uint8_t grootte = 1);
void ui_tekst_midden_v(int x, int y, int h, const char* tekst, uint16_t kleur, uint8_t grootte = 1);
void ui_scheidingslijn(int x, int y, int len, uint16_t kleur, bool horizontaal = true);
void ui_panel_bg(int x, int y, int w, int h, uint16_t kleur);
void ui_maan_symbool(int cx, int cy, int r, float fase);  // fase 0..1
