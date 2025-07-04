#pragma once
#include "dspcore.h"
#include <Arduino_GFX_Library.h>
#define GFX_DEV_DEVICE JC3248W535       // this module uses AXS15231B display 
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

#if __has_include("conf/displayAXS15231Bconf_custom.h")
  #include "conf/displayAXS15231Bconf_custom.h"
#else
  #include "conf/displayAXS15231Bconf.h"
#endif

#define BOOT_PRG_COLOR    0xE68B
#define BOOT_TXT_COLOR    0xFFFF
#define PINK              0xF97F

class DspCore: public DspCoreBase, public  Arduino_AXS15231B {
public:
  DspCore(Arduino_DataBus *b);

  void initDisplay() override;
  void clearDsp(bool black=false) override;

  void drawLogo(uint16_t top) override;
  void printClock(uint16_t top, uint16_t rightspace, uint16_t timeheight, bool redraw) override;
  void clearClock() override;
  void drawPlaylist(uint16_t currentItem) override;
  void printPLitem(uint8_t pos, const char* item, ScrollWidget& current) override;

  static uint16_t DspCore::textWidthGFX(const char *txt, uint8_t textsize);

private:
  uint8_t _charWidth(unsigned char c) override;

};
  

extern DspCore* dsp;


//Arduino_Canvas *gfx = new Arduino_Canvas(320 /* width */, 480 /* height */, g, 0 /* output_x */, 0 /* output_y */, 0 /* rotation */);

bool create_display_dev();
