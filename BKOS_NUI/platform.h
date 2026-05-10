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
  #define TFT_BL    10    // GP10 — backlight PWM
  #define TFT_CS    17    // GP17 — SPI chip select
  #define TFT_DC    15    // GP15 — data/command
  #define TFT_RST   14    // GP14 — reset
  #define TFT_SCK   18    // GP18 — SPI0 clock
  #define TFT_MOSI  19    // GP19 — SPI0 MOSI
  #define TFT_MISO  16    // GP16 — SPI0 MISO (nodig voor XPT2046 touch)

  // XPT2046 resistieve touch (gedeelde SPI bus met display)
  #define PICO_TOUCH_XPT2046
  #define PICO_TS_CS   13    // GP13 — touch chip select
  #define PICO_TS_IRQ  11    // GP11 — touch interrupt

  // SD kaart (gedeelde SPI bus: SCK=18, MISO=16, MOSI=19)
  #define PICO_SD_CS   12    // GP12 — SD chip select

  // IO shift register bus (BKOS4 protocol, direct GPIO)
  #define HC_IN    0    // GP0  — seriële data ingang (HC165 → Pico)
  #define HC_SCK   1    // GP1  — seriële klok
  #define HC_PCK   2    // GP2  — parallelle klok (load)
  #define HC_UIT   3    // GP3  — seriële data uitgang (Pico → HC595)
  #define HC_ID    4    // GP4  — module ID data ingang
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
// ESP32: xTaskCreatePinnedToCore op core 0
// Pico:  geen echte FreeRTOS — taken worden niet gestart (no-op)
#if PLATFORM_PICO
  #define PLATFORM_TASK_CREATE(fn, naam, stack, param, prio, handle) ((void)0)
#else
  #define PLATFORM_TASK_CREATE(fn, naam, stack, param, prio, handle) \
      xTaskCreatePinnedToCore((fn), (naam), (stack), (param), (prio), (handle), 0)
#endif

// ─── FreeRTOS type/functie stubs voor RP2040 ─────────────────────────────────
// De earlephilhower RP2040 core heeft geen standaard FreeRTOS bibliotheek
// tenzij je de RTOS-build activeert. Stubs vervangen de RTOS-calls.
#if PLATFORM_PICO
  typedef void*  TaskHandle_t;
  typedef long   BaseType_t;
  #define pdTRUE             ((BaseType_t)1)
  #define portTICK_PERIOD_MS 1
  static inline void vTaskDelay(unsigned long ms) { delay(ms); }
  static inline void vTaskDelete(TaskHandle_t)    { }
#endif
