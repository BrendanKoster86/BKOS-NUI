#pragma once
#include <Arduino.h>
#include <time.h>

// Gestructureerde persistente data-opslag voor BKOS en apps.
// Sleutels zijn genaamd met namespace-prefix, bijv. "meteo.temp", "getij.station".
// Elke waarde heeft een tijdstempel zodat apps verouderde data kunnen detecteren.

#define DATA_MAX_ENTRIES     64
#define DATA_SLEUTEL_LEN     32
#define DATA_WAARDE_LEN      64
#define DATA_BESTAND         "/bkos_data.json"

void    data_setup();
void    data_opslaan();

void    data_schrijf(const char* sleutel, const char* waarde);
void    data_schrijf_f(const char* sleutel, float waarde, int decimalen = 2);
void    data_schrijf_i(const char* sleutel, int32_t waarde);

bool    data_lees(const char* sleutel, char* buf, int len);
float   data_lees_f(const char* sleutel, float standaard = 0.0f);
int32_t data_lees_i(const char* sleutel, int32_t standaard = 0);

bool    data_bestaat(const char* sleutel);
long    data_leeftijd(const char* sleutel);   // seconden geleden; -1 = niet gevonden
void    data_verwijder(const char* sleutel);
void    data_ruim_op(long max_leeftijd_s);    // verwijder entries ouder dan X seconden

int     data_aantal();
bool    data_sleutel_bij_idx(int idx, char* buf, int len);
