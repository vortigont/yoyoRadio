#pragma once
#include "../core/options.h"
#include "tft_espi_adafruit.hpp"

#if CLOCKFONT_MONO
  #include "fonts/DS_DIGI56pt7b_mono.h"        // https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
#else
  #include "fonts/DS_DIGI56pt7b.h"
#endif
#include "tools/l10n.h"

#define CHARWIDTH   6
#define CHARHEIGHT  8

typedef GFXcanvas16 Canvas;
#include "widgets/widgets.h"
#include "widgets/pages.h"

#if __has_include("conf/displayILI9488conf_custom.h")
  #include "conf/displayILI9488conf_custom.h"
#else
  #include "conf/displayILI9488conf.h"
#endif

#define BOOT_PRG_COLOR    0xE68B
#define BOOT_TXT_COLOR    0xFFFF
#define PINK              0xF97F

/*
#define ILI9488_SLPIN     0x10
#define ILI9488_SLPOUT    0x11
#define ILI9488_DISPOFF   0x28
#define ILI9488_DISPON    0x29
*/
class DspCore: public TFT_eSPI_AdafruitGFX_Wrapper {
#include "tools/commongfx.h"    
};

extern DspCore dsp;
