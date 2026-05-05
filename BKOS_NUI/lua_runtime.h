#pragma once
#include "app_manager.h"
#include "hw_scherm.h"

// Lua 5.4 runtime voor BKOS apps.
// De Lua-bibliotheek (LuaBKOS) wordt tijdens CI gedownload van lua.org.
// Lokaal installeren: zie CLAUDE.md sectie "App Systeem".
//
// Als Lua niet beschikbaar is (#if __has_include mislukt), worden
// alle functies stubbies die niets doen.

#if __has_include("lua.h")
extern "C" {
  #include "lua.h"
  #include "lualib.h"
  #include "lauxlib.h"
}
#define LUA_BESCHIKBAAR 1
#else
#define LUA_BESCHIKBAAR 0
#endif

extern bool  lua_fout_actief;
extern char  lua_fout_tekst[];
#define LUA_FOUT_LEN 128

void lua_setup();
bool lua_app_laden(int app_idx, bool sandbox = false);
void lua_app_teken(int app_idx);
void lua_app_run(int app_idx, int x, int y, bool aanraking);
void lua_app_sluiten();

// Schaalvariabelen (app design → schermcoördinaten)
extern float lua_sx;          // schaal X
extern float lua_sy;          // schaal Y
extern bool  lua_sandbox_modus; // true = standalone app in sandbox (SB_H..NAV_Y)
extern int   lua_y_offset;      // SB_H bij sandbox, anders 0
