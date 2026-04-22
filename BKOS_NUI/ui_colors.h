#pragma once

#define RGB565(r,g,b) ((uint16_t)(((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | ((uint16_t)(b & 0xF8) >> 3)))

// Backgrounds
#define C_BG          RGB565(8,  12, 25)   // Achtergrond
#define C_SURFACE     RGB565(18, 28, 52)   // Kaart
#define C_SURFACE2    RGB565(28, 42, 78)   // Verhoogde kaart
#define C_SURFACE3    RGB565(40, 58, 100)  // Knop normaal

// Accenten
#define C_CYAN        RGB565(0,  200, 230)
#define C_BLUE        RGB565(80, 140, 255)
#define C_GREEN       RGB565(0,  220, 100)
#define C_RED_BRIGHT  RGB565(255, 50,  70)
#define C_ORANGE      RGB565(255, 150,  0)
#define C_AMBER       RGB565(255, 180,  0)
#define C_PURPLE      RGB565(160,  80, 255)
#define C_WHITE       RGB565(255, 255, 255)
#define C_GRAY        RGB565(80,  100, 130)
#define C_DARK_GRAY   RGB565(40,   50,  70)
#define C_BLACK       RGB565(0,     0,   0)

// Tekst
#define C_TEXT        RGB565(200, 220, 255)
#define C_TEXT_DIM    RGB565(100, 130, 160)
#define C_TEXT_DARK   RGB565(20,  30,  55)

// Vaarmodi
#define C_HAVEN       RGB565(60,  100, 255)
#define C_ZEILEN      RGB565(0,   200, 170)
#define C_MOTOR       RGB565(255, 120,   0)
#define C_ANKER       RGB565(140, 100,  40)

// Verlichting stadia
#define C_LIGHT_OFF     RGB565(35, 45, 65)    // 0: echt uit
#define C_LIGHT_COOLING RGB565(110, 70,  0)   // 1: uit, voelt nog aan (amber)
#define C_LIGHT_PENDING RGB565(255, 140,  0)  // 2: aan, geen signaal (oranje)
#define C_LIGHT_ON      RGB565(220, 235, 255) // 3: echt aan (wit)
#define C_LIGHT_ON_RED  RGB565(255,  20,  30) // 3: echt aan rood
#define C_LIGHT_ON_GRN  RGB565(0,   255,  60) // 3: echt aan groen

// Status bar
#define C_STATUSBAR   RGB565(12, 18, 38)

// Nav bar
#define C_NAVBAR      RGB565(12, 18, 38)
#define C_NAV_ACTIVE  C_CYAN
#define C_NAV_NORMAL  C_TEXT_DIM
