#pragma once
#include <Arduino.h>

#define FOUT_LOG_API "https://api.github.com/repos/brennyc86/BKOS-NUI-logs/issues"

// Token instellen (eenmalig, via CONFIG → TOKEN INSTELLEN):
//   Opgeslagen in Preferences, namespace "foutlog", key "token".
// Raadpleeg de repo README voor het aanmaken van een fine-grained PAT
// met uitsluitend "issues: write" op BKOS-NUI-logs.

enum FoutType {
    FOUT_LUA_RUNTIME = 0,
    FOUT_APP_CRASH,
    FOUT_WIFI,
    FOUT_OTA,
    FOUT_IO,
    FOUT_SPIFFS,
    FOUT_ALGEMEEN
};

extern bool fout_rapportage;   // privacy-schakelaar (opgeslagen in config)

void fout_log_setup();         // laad token uit Preferences bij opstart
void fout_log_stuur(FoutType type, const char* bericht, const char* context = "");
void fout_log_token_zet(const char* token);   // sla PAT op in Preferences
bool fout_log_token_aanwezig();
