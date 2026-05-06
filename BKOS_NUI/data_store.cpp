#include "data_store.h"
#include "platform_fs.h"
#include <ArduinoJson.h>

struct DataEntry {
    char   sleutel[DATA_SLEUTEL_LEN];
    char   waarde[DATA_WAARDE_LEN];
    time_t tijdstip;
};

static DataEntry entries[DATA_MAX_ENTRIES];
static int       entries_cnt = 0;
static bool      data_vuil   = false;

static int _zoek(const char* sleutel) {
    for (int i = 0; i < entries_cnt; i++)
        if (strncmp(entries[i].sleutel, sleutel, DATA_SLEUTEL_LEN) == 0) return i;
    return -1;
}

void data_setup() {
    entries_cnt = 0;
    if (!SPIFFS.exists(DATA_BESTAND)) return;

    File f = SPIFFS.open(DATA_BESTAND, "r");
    if (!f) return;

    JsonDocument doc;
    if (deserializeJson(doc, f) != DeserializationError::Ok) { f.close(); return; }
    f.close();

    JsonArray arr = doc["e"].as<JsonArray>();
    for (JsonObject obj : arr) {
        if (entries_cnt >= DATA_MAX_ENTRIES) break;
        const char* k = obj["k"];
        const char* v = obj["v"];
        long        t = obj["t"] | 0L;
        if (!k || !v) continue;
        DataEntry& e = entries[entries_cnt++];
        strncpy(e.sleutel, k, DATA_SLEUTEL_LEN - 1);
        e.sleutel[DATA_SLEUTEL_LEN - 1] = '\0';
        strncpy(e.waarde,  v, DATA_WAARDE_LEN  - 1);
        e.waarde[DATA_WAARDE_LEN  - 1] = '\0';
        e.tijdstip = (time_t)t;
    }
}

void data_opslaan() {
    if (!data_vuil) return;
    data_vuil = false;

    JsonDocument doc;
    JsonArray arr = doc["e"].to<JsonArray>();
    for (int i = 0; i < entries_cnt; i++) {
        JsonObject obj = arr.add<JsonObject>();
        obj["k"] = entries[i].sleutel;
        obj["v"] = entries[i].waarde;
        obj["t"] = (long)entries[i].tijdstip;
    }

    File f = SPIFFS.open(DATA_BESTAND, "w");
    if (!f) return;
    serializeJson(doc, f);
    f.close();
}

void data_schrijf(const char* sleutel, const char* waarde) {
    int idx = _zoek(sleutel);
    if (idx < 0) {
        if (entries_cnt >= DATA_MAX_ENTRIES) return;
        idx = entries_cnt++;
        strncpy(entries[idx].sleutel, sleutel, DATA_SLEUTEL_LEN - 1);
        entries[idx].sleutel[DATA_SLEUTEL_LEN - 1] = '\0';
    }
    strncpy(entries[idx].waarde, waarde, DATA_WAARDE_LEN - 1);
    entries[idx].waarde[DATA_WAARDE_LEN - 1] = '\0';
    entries[idx].tijdstip = time(nullptr);
    data_vuil = true;
}

void data_schrijf_f(const char* sleutel, float waarde, int decimalen) {
    char buf[DATA_WAARDE_LEN];
    dtostrf(waarde, 1, decimalen, buf);
    data_schrijf(sleutel, buf);
}

void data_schrijf_i(const char* sleutel, int32_t waarde) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", (long)waarde);
    data_schrijf(sleutel, buf);
}

bool data_lees(const char* sleutel, char* buf, int len) {
    int idx = _zoek(sleutel);
    if (idx < 0) return false;
    strncpy(buf, entries[idx].waarde, len - 1);
    buf[len - 1] = '\0';
    return true;
}

float data_lees_f(const char* sleutel, float standaard) {
    char buf[DATA_WAARDE_LEN];
    if (!data_lees(sleutel, buf, sizeof(buf))) return standaard;
    return atof(buf);
}

int32_t data_lees_i(const char* sleutel, int32_t standaard) {
    char buf[DATA_WAARDE_LEN];
    if (!data_lees(sleutel, buf, sizeof(buf))) return standaard;
    return (int32_t)atol(buf);
}

bool data_bestaat(const char* sleutel) {
    return _zoek(sleutel) >= 0;
}

long data_leeftijd(const char* sleutel) {
    int idx = _zoek(sleutel);
    if (idx < 0) return -1;
    time_t nu = time(nullptr);
    if (nu < 1000000L || entries[idx].tijdstip == 0) return 0;
    return (long)(nu - entries[idx].tijdstip);
}

void data_verwijder(const char* sleutel) {
    int idx = _zoek(sleutel);
    if (idx < 0) return;
    entries[idx] = entries[--entries_cnt];
    data_vuil = true;
}

void data_ruim_op(long max_leeftijd_s) {
    time_t nu = time(nullptr);
    if (nu < 1000000L) return;
    for (int i = entries_cnt - 1; i >= 0; i--) {
        if (entries[i].tijdstip == 0) continue;
        if ((long)(nu - entries[i].tijdstip) > max_leeftijd_s) {
            entries[i] = entries[--entries_cnt];
            data_vuil = true;
        }
    }
}

int data_aantal() { return entries_cnt; }

bool data_sleutel_bij_idx(int idx, char* buf, int len) {
    if (idx < 0 || idx >= entries_cnt) return false;
    strncpy(buf, entries[idx].sleutel, len - 1);
    buf[len - 1] = '\0';
    return true;
}
