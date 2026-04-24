#pragma once
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

#define WIFI_CONFIG_FILE "/bkos_nui.json"

extern bool wifi_verbonden;
extern bool wifi_aangesloten;

void wifi_setup();
void wifi_loop();
bool wifi_check();
void wifi_reset();
