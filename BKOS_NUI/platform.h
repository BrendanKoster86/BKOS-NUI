#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// platform.h — Eén plek voor alle platform-afhankelijke defines
//
// Ondersteunde targets:
//   PLATFORM_ESP32  — ESP32-S3, 800×480 RGB panel, PSRAM, GT911 touch
//   PLATFORM_PICO   — RP2040/RP2350 (Pico W), 240×320 SPI display, XPT2046
//
// Pinnen zijn standaard-waarden; pas aan in dit bestand voor jouw hardware.
// ─────────────────────────────────────────────────────────────────────────────

// ─── Platform detectie ────────────────────────────────────────────────────────
#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
  #define PLATFORM_PICO  1
  #define PLATFORM_ESP32 0
#else
  #define PLATFORM_PICO  0
  #define PLATFORM_ESP32 1
#endif

// ─── Scherm en pinnen ─────────────────────────────────────────────────────────
#if PLATFORM_PICO
  // ILI9341 SPI display, 240×320 portret
  #define TFT_W     240
  #define TFT_H     320
  #define TFT_BL    22    // GP22 — backlight PWM
  #define TFT_CS    17    // GP17 — SPI chip select
  #define TFT_DC    20    // GP20 — data/command
  #define TFT_RST   21    // GP21 — reset
  #define TFT_SCK   18    // GP18 — SPI0 clock
  #define TFT_MOSI  19    // GP19 — SPI0 data

  // XPT2046 resistieve touch — verwijder de define om touch uit te schakelen
  #define PICO_TOUCH_XPT2046
  #define PICO_TS_CS   16    // GP16 — touch chip select
  #define PICO_TS_IRQ  15    // GP15 — touch interrupt (-1 voor polling)
#else
  // Arduino_ESP32RGBPanel 800×480 liggend
  #define TFT_W    800
  #define TFT_H    480
  #define TFT_BL   2     // GPIO2 — backlight
  // RGB panel pinnen: zie hw_scherm.ino
  // GT911 touch pinnen: zie hw_touch.h
#endif

// ─── Geheugen allocatie ───────────────────────────────────────────────────────
#if PLATFORM_ESP32
  #include <esp_heap_caps.h>
  #define PLATFORM_MALLOC(n)    heap_caps_malloc((n), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
  #define PLATFORM_REALLOC(p,n) heap_caps_realloc((p), (n), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
  #define PLATFORM_FREE(p)      heap_caps_free(p)
#else
  #define PLATFORM_MALLOC(n)    malloc(n)
  #define PLATFORM_REALLOC(p,n) realloc((p), (n))
  #define PLATFORM_FREE(p)      free(p)
#endif

// ─── Vrij geheugen ───────────────────────────────────────────────────────────
#if PLATFORM_PICO
  #define PLATFORM_FREE_HEAP() ((unsigned)rp2040.getFreeHeap())
#else
  #define PLATFORM_FREE_HEAP() ((unsigned)ESP.getFreeHeap())
#endif

// ─── Herstart ─────────────────────────────────────────────────────────────────
#if PLATFORM_PICO
  #define PLATFORM_REBOOT() rp2040.reboot()
#else
  #define PLATFORM_REBOOT() ESP.restart()
#endif

// ─── FreeRTOS taak aanmaken ───────────────────────────────────────────────────
// xTaskCreatePinnedToCore is ESP32-specifiek; Pico gebruikt xTaskCreate
#if PLATFORM_PICO
  #define PLATFORM_TASK_CREATE(fn, naam, stack, param, prio, handle) \
      xTaskCreate((fn), (naam), (stack), (param), (prio), (handle))
#else
  #define PLATFORM_TASK_CREATE(fn, naam, stack, param, prio, handle) \
      xTaskCreatePinnedToCore((fn), (naam), (stack), (param), (prio), (handle), 0)
#endif

// ─── FreeRTOS headers (RP2040 vereist expliciete include; ESP32 heeft dit via Arduino.h) ──
#if PLATFORM_PICO
  #include <FreeRTOS.h>
  #include <task.h>
#endif
