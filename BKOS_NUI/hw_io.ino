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

#define IO_NAMEN_BESTAND "/io_namen.csv"

void hw_io_setup() {
    IO_SERIAL.begin(IO_BAUD, SERIAL_8N1, IO_RX_PIN, IO_TX_PIN);
    memset(io_output,    0, sizeof(io_output));
    memset(io_input,     0, sizeof(io_input));
    memset(io_gewijzigd, 0, sizeof(io_gewijzigd));
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount mislukt");
    }
    hw_io_namen_laden();
}

void hw_io_namen_laden() {
    for (int i = 0; i < MAX_IO_KANALEN; i++) {
        snprintf(io_namen[i], IO_NAAM_LEN, "IO %d", i);
    }
    if (!SPIFFS.exists(IO_NAMEN_BESTAND)) return;
    File f = SPIFFS.open(IO_NAMEN_BESTAND, "r");
    if (!f) return;
    while (f.available()) {
        String lijn = f.readStringUntil('\n');
        lijn.trim();
        if (lijn.length() == 0) continue;
        int sep = lijn.indexOf(':');
        if (sep < 1) continue;
        int idx = lijn.substring(0, sep).toInt();
        String naam = lijn.substring(sep + 1);
        if (idx >= 0 && idx < MAX_IO_KANALEN && naam.length() > 0) {
            strncpy(io_namen[idx], naam.c_str(), IO_NAAM_LEN - 1);
            io_namen[idx][IO_NAAM_LEN - 1] = '\0';
        }
    }
    f.close();
}

void hw_io_namen_opslaan() {
    File f = SPIFFS.open(IO_NAMEN_BESTAND, "w");
    if (!f) {
        Serial.println("SPIFFS schrijven mislukt");
        return;
    }
    for (int i = 0; i < MAX_IO_KANALEN; i++) {
        char standaard[IO_NAAM_LEN];
        snprintf(standaard, IO_NAAM_LEN, "IO %d", i);
        if (strcmp(io_namen[i], standaard) != 0) {
            f.printf("%d:%s\n", i, io_namen[i]);
        }
    }
    f.close();
}
