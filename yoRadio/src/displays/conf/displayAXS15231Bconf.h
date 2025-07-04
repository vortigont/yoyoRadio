/*************************************************************************************
    _AXS15231B 320x480 displays configuration file.
    Copy this file to yoRadio/src/displays/conf/displayAXS15231Bconf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/

#ifndef displayAXS15231Bconf_h
#define displayAXS15231Bconf_h

#include "../widgets/widgets.h"

// Определение для скрытия VU-метра (если нужно)
// #define HIDE_VU

#define DSP_WIDTH       320
#define DSP_HEIGHT      480
#define TFT_FRAMEWDT    8
#define MAX_WIDTH       DSP_WIDTH-TFT_FRAMEWDT

#if BITRATE_FULL
  #define TITLE_FIX 44
#else
  #define TITLE_FIX 0
#endif
#define bootLogoTop     220

#ifndef BATTERY_OFF
  #define BatX      92				// X coordinate for batt. ( X  )
  #define BatY      420		// Y cordinate for batt. ( Y  )
  #define BatFS     3		// FontSize for batt. (   )
  #define ProcX     20				// X coordinate for percent ( X   )
  #define ProcY     425		// Y coordinate for percent ( Y   )
  #define ProcFS    2		// FontSize for percent (    )
  #define VoltX      175				// X coordinate for voltage ( X  )
  #define VoltY      425		// Y coordinate for voltage ( Y  )
  #define VoltFS     2		// FontSize for voltage (   )
#endif

/* SROLLS  */                            /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
const ScrollConfig metaConf       PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 3, WA_CENTER}, 140, true, MAX_WIDTH, 5000, 30, 12 }; //5,12
const ScrollConfig title1Conf     PROGMEM = {{ TFT_FRAMEWDT, 48, 2, WA_CENTER }, 140, true, DSP_WIDTH-TFT_FRAMEWDT*2 , 5000, 30, 12 };
const ScrollConfig title2Conf     PROGMEM = {{ TFT_FRAMEWDT, 69, 2, WA_CENTER }, 140, false, DSP_WIDTH-TFT_FRAMEWDT*2, 5000, 30, 12 };
const ScrollConfig playlistConf   PROGMEM = {{ TFT_FRAMEWDT, 146, 4, WA_LEFT }, 140, false, MAX_WIDTH, 1000, 30, 12 };
const ScrollConfig apTitleConf    PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 4, WA_CENTER }, 140, false, MAX_WIDTH, 0, 30, 8 };
const ScrollConfig apSettConf     PROGMEM = {{ TFT_FRAMEWDT, 320-TFT_FRAMEWDT-16, 2, WA_LEFT }, 140, false, MAX_WIDTH, 0, 30, 8 };
const ScrollConfig weatherConf    PROGMEM = {{ TFT_FRAMEWDT,197, 2, WA_LEFT }, 140, false, MAX_WIDTH, 1000, 30, 4 }; //10,4

/* BACKGROUNDS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
const FillConfig   metaBGConf     PROGMEM = {{ 0, 38, 0, WA_LEFT }, DSP_WIDTH, 2, false };
const FillConfig   metaBGConfInv  PROGMEM = {{ 0, 40, 0, WA_LEFT }, DSP_WIDTH, 2, false };
const FillConfig   volbarConf     PROGMEM = {{ TFT_FRAMEWDT, DSP_HEIGHT-TFT_FRAMEWDT-4, 0, WA_LEFT }, MAX_WIDTH, 8, true };
const FillConfig  playlBGConf     PROGMEM = {{ 0, 138, 0, WA_LEFT }, DSP_WIDTH, 36, false };
const FillConfig  heapbarConf     PROGMEM = {{ TFT_FRAMEWDT, DSP_HEIGHT-4, 0, WA_LEFT }, MAX_WIDTH, 4, false };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
const WidgetConfig bootstrConf    PROGMEM = { 0, bootLogoTop+100, 2, WA_CENTER };
const WidgetConfig bitrateConf    PROGMEM = { 6, 62, 2, WA_RIGHT };
const WidgetConfig voltxtConf     PROGMEM = { TFT_FRAMEWDT, DSP_HEIGHT-24, 1, WA_LEFT };
const WidgetConfig  iptxtConf     PROGMEM = { TFT_FRAMEWDT+80, DSP_HEIGHT-29, 2, WA_LEFT };
const WidgetConfig   rssiConf     PROGMEM = { TFT_FRAMEWDT, DSP_HEIGHT-24, 1, WA_RIGHT };
const WidgetConfig   cpuConf      PROGMEM = { TFT_FRAMEWDT, DSP_HEIGHT-50, 1, WA_RIGHT };//эксперимент. новый
const WidgetConfig numConf        PROGMEM = { TFT_FRAMEWDT, 250, 1, WA_CENTER };
const WidgetConfig apNameConf     PROGMEM = { TFT_FRAMEWDT, 88, 3, WA_CENTER };
const WidgetConfig apName2Conf    PROGMEM = { TFT_FRAMEWDT, 120, 3, WA_CENTER };
const WidgetConfig apPassConf     PROGMEM = { TFT_FRAMEWDT, 173, 3, WA_CENTER };
const WidgetConfig apPass2Conf    PROGMEM = { TFT_FRAMEWDT, 205, 3, WA_CENTER };
const WidgetConfig  clockConf     PROGMEM = { 0, 98, 52, WA_CENTER };  /* 52 is a fixed font size. do not change */
const WidgetConfig vuConf         PROGMEM = { TFT_FRAMEWDT, 268, 1, WA_LEFT };

const WidgetConfig bootWdtConf    PROGMEM = { 120, bootLogoTop+130, 2, WA_LEFT }; //отступ, вертикальное положение, размер, выравнивание
const ProgressConfig bootPrgConf  PROGMEM = { 90, 9, 4 }; //скорость, ширина, ширина движущей части
const BitrateConfig fullbitrateConf PROGMEM = {{DSP_WIDTH-TFT_FRAMEWDT-80, 226, 2, WA_LEFT}, 60 };

/* BANDS  */                             /* { onebandwidth, onebandheight, bandsHspace, bandsVspace, numofbands, fadespeed } */
const VUBandsConfig bandsConf     PROGMEM = {  MAX_WIDTH, 35, 4, 5, 45, 30};

/* STRINGS  */
const char         numtxtFmt[]    PROGMEM = "%d";
//const char           rssiFmt[]    PROGMEM = "WiFi %d";
const char           rssiFmt[]    PROGMEM = "WiFi %d";

const char          iptxtFmt[]    PROGMEM = "%s";
const char         voltxtFmt[]    PROGMEM = "\023\025%d";
//const char         voltxtFmt[]    PROGMEM = "VOL %d";

const char        bitrateFmt[]    PROGMEM = "%d kBs";

/* MOVES  */                             /* { left, top, width } */
const MoveConfig    clockMove     PROGMEM = { 0, 98, MAX_WIDTH /* MAX_WIDTH */ }; // -1 disables move
const MoveConfig   weatherMove    PROGMEM = { TFT_FRAMEWDT, 197, MAX_WIDTH};
const MoveConfig   weatherMoveVU  PROGMEM = { TFT_FRAMEWDT, 197, MAX_WIDTH};

#endif