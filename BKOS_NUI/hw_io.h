#pragma once
#include <FS.h>
#include <SPIFFS.h>

// UART naar ATtiny3217
#define IO_SERIAL       Serial2
#define IO_TX_PIN       17
#define IO_RX_PIN       16
#define IO_BAUD         9600

// IO limieten
#define MAX_MODULES     30
#define MAX_IO_KANALEN  240
#define IO_NAAM_LEN     12

// Module type IDs
#define MODULE_LOGICA8   2
#define MODULE_LOGICA16  3
#define MODULE_HUB8      50
#define MODULE_HUB_AN    51
#define MODULE_HUB_UART  52
#define MODULE_SCHAKEL8  130
#define MODULE_SCHAKEL16 147
#define MODULE_EINDE     255

// Timing
#define IO_INTERVAL      50    // ms tussen IO cycli
#define IO_TIMEOUT       5000  // ms timeout per actie
#define IO_DETECTIE_INT  30000 // ms herdetectie interval

// Kanaal namen conventies
#define NAAM_PREFIX_LICHT   "**L_"    // extern licht
#define NAAM_PREFIX_INT_WIT "**IL_wit"
#define NAAM_PREFIX_INT_ROO "**IL_rood"
#define NAAM_PREFIX_HAVEN   "**haven"
#define NAAM_PREFIX_ZEILEN  "**zeilen"
#define NAAM_PREFIX_MOTOR   "**motor"
#define NAAM_PREFIX_ANKER   "**anker"

extern int   io_kanalen_cnt;
extern byte  io_output[];
extern bool  io_input[];
extern bool  io_gewijzigd[];
extern char  io_namen[][IO_NAAM_LEN];
extern byte  io_aparaten[];
extern int   io_aparaten_cnt;
extern bool  io_actief;
extern bool  io_runned;
extern unsigned long io_gecheckt;

void hw_io_setup();
void hw_io_namen_laden();
void hw_io_namen_opslaan();
