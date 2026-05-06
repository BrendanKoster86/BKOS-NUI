#pragma once
// Bestandssysteem abstractie: SPIFFS op ESP32, LittleFS op Pico.
// Gebruik altijd "SPIFFS.xxx()" in de code — dit bestand zorgt voor de juiste mapping.
#include "platform.h"
#if PLATFORM_PICO
  #include <LittleFS.h>
  #define SPIFFS LittleFS
#else
  #include <SPIFFS.h>
#endif
