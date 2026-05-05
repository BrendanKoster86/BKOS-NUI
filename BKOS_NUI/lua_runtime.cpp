#include "lua_runtime.h"
#include "ui_draw.h"
#include "data_store.h"
#include "hw_io.h"
#include "io.h"
#include "ui_colors.h"
#include "ota.h"
#include <SPIFFS.h>

bool  lua_fout_actief   = false;
char  lua_fout_tekst[LUA_FOUT_LEN] = "";
float lua_sx            = 1.0f;
float lua_sy            = 1.0f;
bool  lua_sandbox_modus = false;
int   lua_y_offset      = 0;

static int lua_app_huidig = -1;

// ─────────────────────────────────────────────────────────────────────────────
#if LUA_BESCHIKBAAR

static lua_State* L = nullptr;

// PSRAM-bewuste allocator: gebruik eerst SPI RAM, val terug op intern RAM
static void* lua_bkos_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
    (void)ud; (void)osize;
    if (nsize == 0) { heap_caps_free(ptr); return nullptr; }
    if (ptr == nullptr)
        return heap_caps_malloc(nsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    return heap_caps_realloc(ptr, nsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

// ─── bkos.scherm ─────────────────────────────────────────────────────────────
static int l_vul(lua_State* ls) {
    int x = (int)(luaL_checkinteger(ls, 1) * lua_sx);
    int y = (int)(luaL_checkinteger(ls, 2) * lua_sy) + lua_y_offset;
    int b = (int)(luaL_checkinteger(ls, 3) * lua_sx);
    int h = (int)(luaL_checkinteger(ls, 4) * lua_sy);
    uint16_t kl = (uint16_t)luaL_checkinteger(ls, 5);
    tft.fillRect(x, y, b, h, kl);
    return 0;
}

static int l_lijn(lua_State* ls) {
    int x1 = (int)(luaL_checkinteger(ls, 1) * lua_sx);
    int y1 = (int)(luaL_checkinteger(ls, 2) * lua_sy) + lua_y_offset;
    int x2 = (int)(luaL_checkinteger(ls, 3) * lua_sx);
    int y2 = (int)(luaL_checkinteger(ls, 4) * lua_sy) + lua_y_offset;
    uint16_t kl = (uint16_t)luaL_checkinteger(ls, 5);
    tft.drawLine(x1, y1, x2, y2, kl);
    return 0;
}

static int l_tekst(lua_State* ls) {
    int x    = (int)(luaL_checkinteger(ls, 1) * lua_sx);
    int y    = (int)(luaL_checkinteger(ls, 2) * lua_sy) + lua_y_offset;
    const char* t = luaL_checkstring(ls, 3);
    int sz   = (int)luaL_checkinteger(ls, 4);
    uint16_t kl = (uint16_t)luaL_checkinteger(ls, 5);
    tft.setTextSize(sz);
    tft.setTextColor(kl);
    tft.setCursor(x, y);
    tft.print(t);
    return 0;
}

static int l_cirkel(lua_State* ls) {
    int cx = (int)(luaL_checkinteger(ls, 1) * lua_sx);
    int cy = (int)(luaL_checkinteger(ls, 2) * lua_sy) + lua_y_offset;
    int r  = (int)(luaL_checkinteger(ls, 3) * ((lua_sx + lua_sy) / 2.0f));
    uint16_t kl = (uint16_t)luaL_checkinteger(ls, 4);
    bool gevuld = lua_toboolean(ls, 5);
    if (gevuld) tft.fillCircle(cx, cy, r, kl);
    else        tft.drawCircle(cx, cy, r, kl);
    return 0;
}

static int l_rgb(lua_State* ls) {
    int r = (int)luaL_checkinteger(ls, 1) & 0xFF;
    int g = (int)luaL_checkinteger(ls, 2) & 0xFF;
    int b = (int)luaL_checkinteger(ls, 3) & 0xFF;
    lua_pushinteger(ls, ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    return 1;
}

// ─── Poortnummer resolver ─────────────────────────────────────────────────────
// Accepteert: int, "A1"-"Z8" (poortgroep-notatie), of kanaalnaam
static int _poort_resolve(lua_State* ls, int arg) {
    if (lua_isinteger(ls, arg)) return (int)lua_tointeger(ls, arg);
    if (lua_isnumber(ls, arg))  return (int)(int)lua_tonumber(ls, arg);
    if (lua_isstring(ls, arg)) {
        const char* s = lua_tostring(ls, arg);
        int len = (int)strlen(s);
        // "A1".."Z8": één hoofdletter + cijfer 1-8 → groep*8 + (cijfer-1)
        if (len == 2 && s[0] >= 'A' && s[0] <= 'Z' && s[1] >= '1' && s[1] <= '8')
            return (s[0] - 'A') * 8 + (s[1] - '1');
        // Kanaalnaam opzoeken
        int n = io_zichtbaar();
        for (int i = 0; i < n; i++)
            if (strncmp(io_namen[i], s, IO_NAAM_LEN - 1) == 0) return i;
    }
    return -1;
}

// ─── Arduino-stijl functies (digitalRead / digitalWrite / drawCircle / fillCircle)
static int l_digitalRead(lua_State* ls) {
    int p = _poort_resolve(ls, 1);
    if (p < 0 || p >= io_kanalen_cnt) { lua_pushnil(ls); return 1; }
    lua_pushboolean(ls, io_input[p] ? 1 : 0);
    return 1;
}

static int l_digitalWrite(lua_State* ls) {
    int p     = _poort_resolve(ls, 1);
    int staat = (int)luaL_checkinteger(ls, 2);
    if (p >= 0 && p < io_kanalen_cnt) {
        io_output[p]    = (byte)staat;
        io_gewijzigd[p] = true;
    }
    return 0;
}

static int l_drawCircle(lua_State* ls) {
    int cx = (int)(luaL_checkinteger(ls, 1) * lua_sx);
    int cy = (int)(luaL_checkinteger(ls, 2) * lua_sy) + lua_y_offset;
    int r  = (int)(luaL_checkinteger(ls, 3) * ((lua_sx + lua_sy) * 0.5f));
    uint16_t kl = (uint16_t)luaL_checkinteger(ls, 4);
    tft.drawCircle(cx, cy, r, kl);
    return 0;
}

static int l_fillCircle(lua_State* ls) {
    int cx = (int)(luaL_checkinteger(ls, 1) * lua_sx);
    int cy = (int)(luaL_checkinteger(ls, 2) * lua_sy) + lua_y_offset;
    int r  = (int)(luaL_checkinteger(ls, 3) * ((lua_sx + lua_sy) * 0.5f));
    uint16_t kl = (uint16_t)luaL_checkinteger(ls, 4);
    tft.fillCircle(cx, cy, r, kl);
    return 0;
}

// ─── bkos.io ─────────────────────────────────────────────────────────────────
static int l_io_lees(lua_State* ls) {
    int nr = (int)luaL_checkinteger(ls, 1);
    if (nr < 0 || nr >= io_kanalen_cnt) { lua_pushboolean(ls, 0); return 1; }
    lua_pushboolean(ls, io_input[nr] ? 1 : 0);
    return 1;
}

static int l_io_zet(lua_State* ls) {
    int nr    = (int)luaL_checkinteger(ls, 1);
    int staat = (int)luaL_checkinteger(ls, 2);
    if (nr >= 0 && nr < io_kanalen_cnt) {
        io_output[nr]    = (byte)staat;
        io_gewijzigd[nr] = true;
    }
    return 0;
}

static int l_io_wissel(lua_State* ls) {
    int nr = (int)luaL_checkinteger(ls, 1);
    if (nr >= 0 && nr < io_kanalen_cnt) {
        io_output[nr]    = (io_output[nr] == IO_AAN) ? IO_UIT : IO_AAN;
        io_gewijzigd[nr] = true;
    }
    return 0;
}

static int l_io_lees_naam(lua_State* ls) {
    const char* naam = luaL_checkstring(ls, 1);
    int n = io_zichtbaar();
    for (int i = 0; i < n; i++) {
        if (io_naam_is(i, naam)) {
            lua_pushboolean(ls, io_input[i] ? 1 : 0);
            return 1;
        }
    }
    lua_pushnil(ls);
    return 1;
}

static int l_io_zet_naam(lua_State* ls) {
    const char* naam  = luaL_checkstring(ls, 1);
    int         staat = (int)luaL_checkinteger(ls, 2);
    int n = io_zichtbaar();
    for (int i = 0; i < n; i++) {
        if (io_naam_is(i, naam)) {
            io_output[i]    = (byte)staat;
            io_gewijzigd[i] = true;
        }
    }
    return 0;
}

static int l_io_wissel_naam(lua_State* ls) {
    const char* naam = luaL_checkstring(ls, 1);
    io_apparaat_toggle(naam);
    return 0;
}

static int l_io_naam(lua_State* ls) {
    int nr = (int)luaL_checkinteger(ls, 1);
    if (nr >= 0 && nr < io_kanalen_cnt)
        lua_pushstring(ls, io_namen[nr]);
    else
        lua_pushnil(ls);
    return 1;
}

static int l_io_cnt(lua_State* ls) {
    lua_pushinteger(ls, io_zichtbaar());
    return 1;
}

// ─── bkos.data ───────────────────────────────────────────────────────────────
static int l_data_lees(lua_State* ls) {
    const char* k = luaL_checkstring(ls, 1);
    char buf[DATA_WAARDE_LEN];
    if (data_lees(k, buf, sizeof(buf)))
        lua_pushstring(ls, buf);
    else
        lua_pushnil(ls);
    return 1;
}

static int l_data_lees_f(lua_State* ls) {
    const char* k = luaL_checkstring(ls, 1);
    float std = (float)luaL_optnumber(ls, 2, 0.0);
    lua_pushnumber(ls, (lua_Number)data_lees_f(k, std));
    return 1;
}

static int l_data_schrijf(lua_State* ls) {
    const char* k = luaL_checkstring(ls, 1);
    const char* v = luaL_checkstring(ls, 2);
    data_schrijf(k, v);
    return 0;
}

static int l_data_schrijf_f(lua_State* ls) {
    const char* k = luaL_checkstring(ls, 1);
    float v = (float)luaL_checknumber(ls, 2);
    data_schrijf_f(k, v);
    return 0;
}

static int l_data_leeftijd(lua_State* ls) {
    const char* k = luaL_checkstring(ls, 1);
    lua_pushinteger(ls, (lua_Integer)data_leeftijd(k));
    return 1;
}

// ─── bkos.sys ────────────────────────────────────────────────────────────────
static int l_versie(lua_State* ls) {
    lua_pushstring(ls, BKOS_NUI_VERSIE);
    return 1;
}

static int l_millis(lua_State* ls) {
    lua_pushinteger(ls, (lua_Integer)millis());
    return 1;
}

static int l_log(lua_State* ls) {
#ifdef DEBUG
    const char* s = luaL_checkstring(ls, 1);
    Serial.println(s);
#endif
    return 0;
}

// ─── bkos tabel opbouwen ─────────────────────────────────────────────────────
static void lua_registreer_api(lua_State* ls) {
    lua_newtable(ls);  // bkos

    // Schermdimensies
    lua_pushinteger(ls, (lua_Integer)TFT_W);  lua_setfield(ls, -2, "W");
    lua_pushinteger(ls, (lua_Integer)TFT_H);  lua_setfield(ls, -2, "H");

    // IO constanten
    lua_pushinteger(ls, IO_UIT); lua_setfield(ls, -2, "IO_UIT");
    lua_pushinteger(ls, IO_AAN); lua_setfield(ls, -2, "IO_AAN");

    // Arduino-stijl aliassen en constanten
    lua_pushinteger(ls, 1);                      lua_setfield(ls, -2, "HIGH");
    lua_pushinteger(ls, 0);                      lua_setfield(ls, -2, "LOW");
    lua_pushcfunction(ls, l_digitalRead);        lua_setfield(ls, -2, "digitalRead");
    lua_pushcfunction(ls, l_digitalWrite);       lua_setfield(ls, -2, "digitalWrite");
    lua_pushcfunction(ls, l_drawCircle);         lua_setfield(ls, -2, "drawCircle");
    lua_pushcfunction(ls, l_fillCircle);         lua_setfield(ls, -2, "fillCircle");

    // Schermdrawing functies
    lua_pushcfunction(ls, l_vul);     lua_setfield(ls, -2, "vul");
    lua_pushcfunction(ls, l_lijn);    lua_setfield(ls, -2, "lijn");
    lua_pushcfunction(ls, l_tekst);   lua_setfield(ls, -2, "tekst");
    lua_pushcfunction(ls, l_cirkel);  lua_setfield(ls, -2, "cirkel");
    lua_pushcfunction(ls, l_rgb);     lua_setfield(ls, -2, "rgb");

    // kleur-tabel (huidige palette waarden)
    lua_newtable(ls);
    lua_pushinteger(ls, (lua_Integer)C_BG);       lua_setfield(ls, -2, "bg");
    lua_pushinteger(ls, (lua_Integer)C_SURFACE);  lua_setfield(ls, -2, "surface");
    lua_pushinteger(ls, (lua_Integer)C_TEXT);     lua_setfield(ls, -2, "tekst");
    lua_pushinteger(ls, (lua_Integer)C_TEXT_DIM); lua_setfield(ls, -2, "tekst_dim");
    lua_pushinteger(ls, (lua_Integer)C_CYAN);     lua_setfield(ls, -2, "cyaan");
    lua_pushinteger(ls, (lua_Integer)C_GREEN);    lua_setfield(ls, -2, "groen");
    lua_pushinteger(ls, (lua_Integer)C_AMBER);    lua_setfield(ls, -2, "amber");
    lua_pushinteger(ls, (lua_Integer)C_RED_BRIGHT); lua_setfield(ls, -2, "rood");
    lua_setfield(ls, -2, "kleur");

    // io-tabel
    lua_newtable(ls);
    lua_pushcfunction(ls, l_io_lees);       lua_setfield(ls, -2, "lees");
    lua_pushcfunction(ls, l_io_zet);        lua_setfield(ls, -2, "zet");
    lua_pushcfunction(ls, l_io_wissel);     lua_setfield(ls, -2, "wissel");
    lua_pushcfunction(ls, l_io_lees_naam);  lua_setfield(ls, -2, "lees_naam");
    lua_pushcfunction(ls, l_io_zet_naam);   lua_setfield(ls, -2, "zet_naam");
    lua_pushcfunction(ls, l_io_wissel_naam);lua_setfield(ls, -2, "wissel_naam");
    lua_pushcfunction(ls, l_io_naam);       lua_setfield(ls, -2, "naam");
    lua_pushcfunction(ls, l_io_cnt);        lua_setfield(ls, -2, "kanalen");
    lua_setfield(ls, -2, "io");

    // data-tabel
    lua_newtable(ls);
    lua_pushcfunction(ls, l_data_lees);     lua_setfield(ls, -2, "lees");
    lua_pushcfunction(ls, l_data_lees_f);   lua_setfield(ls, -2, "lees_f");
    lua_pushcfunction(ls, l_data_schrijf);  lua_setfield(ls, -2, "schrijf");
    lua_pushcfunction(ls, l_data_schrijf_f);lua_setfield(ls, -2, "schrijf_f");
    lua_pushcfunction(ls, l_data_leeftijd); lua_setfield(ls, -2, "leeftijd");
    lua_setfield(ls, -2, "data");

    // sys-tabel
    lua_newtable(ls);
    lua_pushcfunction(ls, l_versie); lua_setfield(ls, -2, "versie");
    lua_pushcfunction(ls, l_millis); lua_setfield(ls, -2, "millis");
    lua_pushcfunction(ls, l_log);    lua_setfield(ls, -2, "log");
    lua_setfield(ls, -2, "sys");

    lua_setglobal(ls, "bkos");
}

// ─── Callback aanroepen ──────────────────────────────────────────────────────
static void _callback(const char* naam, int argc, ...) {
    lua_getglobal(L, "bkos");
    if (!lua_istable(L, -1)) { lua_pop(L, 1); return; }
    lua_getfield(L, -1, naam);
    lua_remove(L, -2);
    if (!lua_isfunction(L, -1)) { lua_pop(L, 1); return; }

    va_list args;
    va_start(args, argc);
    for (int i = 0; i < argc; i++)
        lua_pushinteger(L, (lua_Integer)va_arg(args, int));
    va_end(args);

    if (lua_pcall(L, argc, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        strncpy(lua_fout_tekst, err ? err : "onbekende fout", LUA_FOUT_LEN - 1);
        lua_fout_actief = true;
        lua_pop(L, 1);
    }
}

// ─── Publieke API ─────────────────────────────────────────────────────────────
void lua_setup() {
    if (L) { lua_close(L); L = nullptr; }

    // Alleen initialiseren als er actieve apps zijn
    bool heeft_apps = false;
    for (int i = 0; i < apps_cnt; i++)
        if (apps[i].actief) { heeft_apps = true; break; }
    if (!heeft_apps) return;

    L = lua_newstate(lua_bkos_alloc, nullptr);
    if (!L) return;
    luaL_openlibs(L);  // linit_bkos.c registreert alleen base/math/string/table/coroutine
    lua_registreer_api(L);
}

bool lua_app_laden(int app_idx, bool sandbox) {
    if (!L || app_idx < 0 || app_idx >= apps_cnt) return false;
    lua_fout_actief   = false;
    lua_app_huidig    = app_idx;
    lua_sandbox_modus = sandbox;
    lua_y_offset      = sandbox ? SB_H : 0;

    AppManifest& app = apps[app_idx];
    lua_sx = (float)TFT_W  / max(1, app.scherm_b);
    lua_sy = sandbox
             ? (float)CONTENT_H / max(1, app.scherm_h)
             : (float)TFT_H     / max(1, app.scherm_h);

    // Laad het main.lua bestand vanuit SPIFFS
    String pad = app_pad(app.id);   // geeft /app_<id>_main.lua
    File f = SPIFFS.open(pad, "r");
    if (!f) {
        snprintf(lua_fout_tekst, LUA_FOUT_LEN, "Bestand niet gevonden:\n%s", pad.c_str());
        lua_fout_actief = true;
        return false;
    }

    String src = f.readString();
    f.close();

    if (luaL_dostring(L, src.c_str()) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        strncpy(lua_fout_tekst, err ? err : "syntax fout", LUA_FOUT_LEN - 1);
        lua_fout_actief = true;
        lua_pop(L, 1);
        return false;
    }
    return true;
}

void lua_app_teken(int app_idx) {
    if (lua_fout_actief) {
        int ey = lua_y_offset;
        int eh = lua_sandbox_modus ? CONTENT_H : TFT_H;
        tft.fillRect(0, ey, TFT_W, eh, C_BG);
        tft.setTextSize(1);
        tft.setTextColor(C_RED_BRIGHT);
        tft.setCursor(10, ey + 10);
        tft.print("Lua fout: ");
        tft.println(lua_fout_tekst);
        return;
    }
    if (!L) return;
    _callback("teken", 0);
}

void lua_app_run(int app_idx, int x, int y, bool aanraking) {
    if (lua_fout_actief || !L) return;
    if (aanraking) {
        // Schaal terug naar app-ontwerpruimte (y_offset aftrekken vóór schalen)
        int app_x = (lua_sx > 0.01f) ? (int)(x / lua_sx) : x;
        int app_y = (lua_sy > 0.01f) ? (int)((y - lua_y_offset) / lua_sy) : y;
        _callback("aanraking", 2, app_x, app_y);
    } else {
        _callback("update", 0);
    }
}

void lua_app_sluiten() {
    lua_app_huidig = -1;
    lua_sx = lua_sy = 1.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
#else   // LUA_BESCHIKBAAR == 0: stubs zodat de rest van de code compileert

void lua_setup()                                          { }
bool lua_app_laden(int)                                   { return false; }
void lua_app_teken(int)                                   {
    tft.fillScreen(C_BG);
    tft.setTextSize(2);
    tft.setTextColor(C_AMBER);
    tft.setCursor(20, TFT_H / 2 - 20);
    tft.println("Lua runtime niet");
    tft.setCursor(20, TFT_H / 2 + 4);
    tft.println("geïnstalleerd");
}
void lua_app_run(int, int, int, bool)                     { }
void lua_app_sluiten()                                    { }

#endif  // LUA_BESCHIKBAAR
