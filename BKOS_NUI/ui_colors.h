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

// Verlichting stadia — gloeilamp-metafoor, geen legenda nodig
#define C_LIGHT_OFF     RGB565(18,  22,  38)  // 0: echt uit — bijna zwart
#define C_LIGHT_COOLING RGB565(160,  80,   0) // 1: koelt af — warm oranje/amber
#define C_LIGHT_PENDING RGB565(180, 160,  50) // 2: aan/geen signaal — gedimde gele tint
#define C_LIGHT_ON      RGB565(255, 250, 180) // 3: echt aan — gloeiend warm geel-wit
#define C_LIGHT_ON_RED  RGB565(255,  30,  50) // BB navigatielicht rood
#define C_LIGHT_ON_GRN  RGB565( 20, 255,  80) // SB navigatielicht groen

// Status bar
#define C_STATUSBAR   RGB565(12, 18, 38)

// Nav bar
#define C_NAVBAR      RGB565(12, 18, 38)
#define C_NAV_ACTIVE  C_CYAN
#define C_NAV_NORMAL  C_TEXT_DIM
