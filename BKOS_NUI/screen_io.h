#pragma once
#include "ui_draw.h"
#include "app_state.h"
#include "io.h"

#define IO_RIJEN_PER_PAGINA  8
#define IO_RIJ_H             44
#define IO_SCROLL_BTN_W      50
#define IO_KANAAL_W         (TFT_W - IO_SCROLL_BTN_W * 2 - 20)

extern int io_scroll_offset;

void screen_io_teken();
void screen_io_run(int x, int y, bool aanraking);
void screen_io_teken_rijen();
