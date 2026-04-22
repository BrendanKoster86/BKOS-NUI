#include "hw_io.h"

int   io_kanalen_cnt   = 0;
byte  io_output[MAX_IO_KANALEN];
bool  io_input[MAX_IO_KANALEN];
bool  io_gewijzigd[MAX_IO_KANALEN];
char  io_namen[MAX_IO_KANALEN][IO_NAAM_LEN];
byte  io_aparaten[MAX_MODULES];
int   io_aparaten_cnt  = 0;
bool  io_actief        = false;
bool  io_runned        = false;
unsigned long io_gecheckt = 0;

static Preferences prefs_io;

void hw_io_setup() {
    IO_SERIAL.begin(IO_BAUD, SERIAL_8N1, IO_RX_PIN, IO_TX_PIN);
    memset(io_output, 0, sizeof(io_output));
    memset(io_input,  0, sizeof(io_input));
    memset(io_gewijzigd, 0, sizeof(io_gewijzigd));
    hw_io_namen_laden();
}

void hw_io_namen_laden() {
    prefs_io.begin("io_namen", true);
    for (int i = 0; i < MAX_IO_KANALEN; i++) {
        char sleutel[8];
        snprintf(sleutel, sizeof(sleutel), "n%d", i);
        String naam = prefs_io.getString(sleutel, "");
        if (naam.length() == 0) {
            snprintf(io_namen[i], IO_NAAM_LEN, "IO %d", i);
        } else {
            strncpy(io_namen[i], naam.c_str(), IO_NAAM_LEN - 1);
            io_namen[i][IO_NAAM_LEN - 1] = '\0';
        }
    }
    prefs_io.end();
}

void hw_io_namen_opslaan() {
    prefs_io.begin("io_namen", false);
    for (int i = 0; i < MAX_IO_KANALEN; i++) {
        char sleutel[8];
        snprintf(sleutel, sizeof(sleutel), "n%d", i);
        prefs_io.putString(sleutel, io_namen[i]);
    }
    prefs_io.end();
}
