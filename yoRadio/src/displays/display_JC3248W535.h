#pragma once
//#include "../core/options.h"
#include <Arduino_GFX_Library.h>
#define GFX_DEV_DEVICE JC3248W535
#define GFX_BL 1
#define CANVAS


#if CLOCKFONT_MONO
  #include "fonts/DS_DIGI56pt7b_mono.h"        // https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
#else
  #include "fonts/DS_DIGI56pt7b.h"
#endif
#include "tools/l10n.h"

#define CHARWIDTH   6
#define CHARHEIGHT  8

typedef Arduino_Canvas Canvas;
#include "widgets/widgets.h"
#include "widgets/pages.h"

// reuse ILI9488 for it has same dimensions
#if __has_include("conf/display_JC3248W535_conf_custom.h")
  #include "conf/display_JC3248W535_conf_custom.h"
#else
  #include "conf/displayILI9488conf.h"
#endif

#define BOOT_PRG_COLOR    0xE68B
#define BOOT_TXT_COLOR    0xFFFF
#define PINK              0xF97F

class DspCore: public  Arduino_AXS15231B {
  public:
  DspCore(Arduino_DataBus *b);
  #include "tools/commongfx.h"
};
  

extern Arduino_DataBus *bus;
extern DspCore* dsp;


//Arduino_Canvas *gfx = new Arduino_Canvas(320 /* width */, 480 /* height */, g, 0 /* output_x */, 0 /* output_y */, 0 /* rotation */);

bool create_display_dev();
