/*************************************************************************************
    _AXS15231B 320x480 displays configuration file.
    Copy this file to yoRadio/src/displays/conf/displayAXS15231Bconf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/

// this settings are for JC3248W535 module

#pragma once
#include "320x480_agfx_defines.h"


// Определение для скрытия VU-метра (если нужно)
// #define HIDE_VU

#define TFT_FRAMEWDT    8
#define MAX_WIDTH       DSP_WIDTH-TFT_FRAMEWDT

#define CHARWIDTH   6
#define CHARHEIGHT  12

#if BITRATE_FULL
  #define TITLE_FIX 44
#else
  #define TITLE_FIX 0
#endif
#define bootLogoTop     220

#ifdef BATTERY_ON
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

#ifdef NOT_NEEDED
/* SCROLLS  */                           /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
static const ScrollConfig metaConf       = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 3, WA_CENTER}, 140, true, MAX_WIDTH, 5000, 30, 12 }; //5,12
// state/ready titile
static const ScrollConfig title1Conf     = {{ TFT_FRAMEWDT, 48, 1, WA_CENTER }, 140, true, DSP_WIDTH-TFT_FRAMEWDT*2 , 5000, 30, 12 };
static const ScrollConfig title2Conf     = {{ TFT_FRAMEWDT, 269, 1, WA_CENTER }, 140, false, DSP_WIDTH-TFT_FRAMEWDT*2, 5000, 30, 12 };
static const ScrollConfig playlistConf   = {{ TFT_FRAMEWDT, 146, 4, WA_LEFT }, 140, false, MAX_WIDTH, 1000, 30, 12 };
static const ScrollConfig apTitleConf    = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 4, WA_CENTER }, 140, false, MAX_WIDTH, 0, 30, 8 };
static const ScrollConfig apSettConf     = {{ TFT_FRAMEWDT, 320-TFT_FRAMEWDT-16, 2, WA_LEFT }, 140, false, MAX_WIDTH, 0, 30, 8 };
static const ScrollConfig weatherConf    = {{ TFT_FRAMEWDT,197, 2, WA_LEFT }, 140, false, MAX_WIDTH, 1000, 30, 4 }; //10,4

/* BACKGROUNDS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
static const FillConfig   metaBGConf     = {{ 0, 38, 0, WA_LEFT }, DSP_WIDTH, 2, false };
static const FillConfig   metaBGConfInv  = {{ 0, 40, 0, WA_LEFT }, DSP_WIDTH, 2, false };
static const FillConfig   volbarConf     = {{ TFT_FRAMEWDT, DSP_HEIGHT-TFT_FRAMEWDT-4, 0, WA_LEFT }, MAX_WIDTH, 8, true };
static const FillConfig   playlBGConf    = {{ 0, 138, 0, WA_LEFT }, DSP_WIDTH, 36, false };
static const FillConfig   heapbarConf    = {{ TFT_FRAMEWDT, DSP_HEIGHT-4, 0, WA_LEFT }, MAX_WIDTH, 4, false };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
static const WidgetConfig bootstrConf    = { 0, bootLogoTop+100, 2, WA_CENTER };
static const WidgetConfig bitrateConf    = { 6, 62, 2, WA_RIGHT };
static const WidgetConfig voltxtConf     = { TFT_FRAMEWDT, DSP_HEIGHT-24, 1, WA_LEFT };
static const WidgetConfig iptxtConf      = { TFT_FRAMEWDT+80, DSP_HEIGHT-29, 2, WA_LEFT };
static const WidgetConfig rssiConf       = { TFT_FRAMEWDT, DSP_HEIGHT-24, 1, WA_RIGHT };
static const WidgetConfig cpuConf        = { TFT_FRAMEWDT, DSP_HEIGHT-50, 1, WA_RIGHT };//эксперимент. новый
static const WidgetConfig numConf        = { TFT_FRAMEWDT, 250, 1, WA_CENTER };
static const WidgetConfig apNameConf     = { TFT_FRAMEWDT, 88, 3, WA_CENTER };
static const WidgetConfig apName2Conf    = { TFT_FRAMEWDT, 120, 3, WA_CENTER };
static const WidgetConfig apPassConf     = { TFT_FRAMEWDT, 173, 3, WA_CENTER };
static const WidgetConfig apPass2Conf    = { TFT_FRAMEWDT, 205, 3, WA_CENTER };


static const WidgetConfig vuConf         = { TFT_FRAMEWDT, 268, 1, WA_LEFT };

static const WidgetConfig bootWdtConf    = { 120, bootLogoTop+130, 2, WA_LEFT }; //отступ, вертикальное положение, размер, выравнивание
static const ProgressConfig bootPrgConf  = { 90, 9, 4 }; //скорость, ширина, ширина движущей части
static const BitrateConfig fullbitrateConf = {{DSP_WIDTH-TFT_FRAMEWDT-80, 226, 2, WA_LEFT}, 60 };

/* BANDS  */                             /* { onebandwidth, onebandheight, bandsHspace, bandsVspace, numofbands, fadespeed } */
static const VUBandsConfig bandsConf     = {  MAX_WIDTH, 35, 4, 5, 45, 30};

/* MOVES  */                             /* { left, top, width } */
static const MoveConfig    clockMove     = { 0, 98, MAX_WIDTH /* MAX_WIDTH */ }; // -1 disables move
static const MoveConfig   weatherMove    = { TFT_FRAMEWDT, 197, MAX_WIDTH};
static const MoveConfig   weatherMoveVU  = { TFT_FRAMEWDT, 197, MAX_WIDTH};
#endif  // NOT_NEEDED
