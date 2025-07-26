/*************************************************************************************
    1024х600  displays configuration file.
    Copy this file to yoRadio/src/displays/conf/display_1024x600_conf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/

#pragma once
#include "../widgets/widgets.h"
#include "clib/u8g2.h"

#ifndef FONT_DEFAULT_U8G2
#define FONT_DEFAULT_U8G2 u8g2_font_6x12_t_cyrillic  // CourierCyr6pt8b  //Bahamas6pt8b
#endif

#if CLOCKFONT_MONO
  //#include "fonts/DS_DIGI56pt7b_mono.h"        // https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
  #include "../fonts/DirectiveFour56.h"
  #include "../fonts/DirectiveFour30.h"
  #define CLK_FONT1 DirectiveFour56
  #define CLK_FONT2 DirectiveFour30
#else
  #include "fonts/DS_DIGI56pt7b.h"
#endif


// Определение для скрытия VU-метра (если нужно)
// #define HIDE_VU

#define TFT_FRAMEWDT    10
#define MAX_WIDTH       DSP_WIDTH - TFT_FRAMEWDT

#define CHARWIDTH   6
#define CHARHEIGHT  8

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

/* SCROLLS  */                           /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
static const ScrollConfig metaConf       = {{ 0, TFT_FRAMEWDT+30, 4, WA_CENTER}, 140, true, MAX_WIDTH, 5000, 30, 12 }; //5,12
// state/ready titile
static const ScrollConfig title1Conf     = {{ TFT_FRAMEWDT, 130, 3, WA_LEFT }, 240, true, DSP_WIDTH-TFT_FRAMEWDT*2 , 5000, 30, 12 };
static const ScrollConfig title2Conf     = {{ TFT_FRAMEWDT, 180, 3, WA_LEFT }, 240, false, DSP_WIDTH-TFT_FRAMEWDT*2, 5000, 30, 12 };
static const ScrollConfig playlistConf   = {{ TFT_FRAMEWDT, 146, 4, WA_LEFT }, 140, false, MAX_WIDTH, 1000, 30, 12 };
static const ScrollConfig apTitleConf    = {{ TFT_FRAMEWDT, TFT_FRAMEWDT+40, 6, WA_CENTER }, 140, false, MAX_WIDTH, 0, 30, 8 };
static const ScrollConfig apSettConf     = {{ TFT_FRAMEWDT, 600-TFT_FRAMEWDT-30, 3, WA_CENTER }, 140, false, MAX_WIDTH, 0, 30, 8 };
static const ScrollConfig weatherConf    = {{ TFT_FRAMEWDT,197, 2, WA_LEFT }, 240, false, MAX_WIDTH, 1000, 30, 4 }; //10,4

/* BACKGROUNDS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
static const FillConfig   metaBGConf     = {{ 0, 0, 0, WA_LEFT }, DSP_WIDTH, 40, false };
static const FillConfig   metaBGConfInv  = {{ 0, 80, 0, WA_LEFT }, DSP_WIDTH, 5, false };
static const FillConfig   volbarConf     = {{ TFT_FRAMEWDT, DSP_HEIGHT-TFT_FRAMEWDT-30, 0, WA_LEFT }, MAX_WIDTH, 15, true };
static const FillConfig   playlBGConf    = {{ 0, 138, 0, WA_LEFT }, DSP_WIDTH, 36, false };
static const FillConfig   heapbarConf    = {{ TFT_FRAMEWDT, DSP_HEIGHT-TFT_FRAMEWDT-14, 0, WA_LEFT }, MAX_WIDTH, 8, false };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
static const WidgetConfig bootstrConf    = { 0, bootLogoTop+150, 3, WA_CENTER };
static const WidgetConfig bitrateConf    = { 6, 62, 2, WA_RIGHT };
static const WidgetConfig voltxtConf     = { TFT_FRAMEWDT, DSP_HEIGHT-70, 3, WA_CENTER };
static const WidgetConfig iptxtConf      = { TFT_FRAMEWDT+80, DSP_HEIGHT-70, 3, WA_LEFT };
static const WidgetConfig rssiConf       = { TFT_FRAMEWDT, DSP_HEIGHT-70, 3, WA_RIGHT };
static const WidgetConfig cpuConf        = { TFT_FRAMEWDT, DSP_HEIGHT-70, 1, WA_RIGHT };//эксперимент. новый
static const WidgetConfig numConf        = { TFT_FRAMEWDT, 250, 1, WA_CENTER };
static const WidgetConfig apNameConf     = { TFT_FRAMEWDT, 220, 4, WA_CENTER };
static const WidgetConfig apName2Conf    = { TFT_FRAMEWDT, 275, 4, WA_CENTER };
static const WidgetConfig apPassConf     = { TFT_FRAMEWDT, 400, 4, WA_CENTER };
static const WidgetConfig apPass2Conf    = { TFT_FRAMEWDT, 455, 4, WA_CENTER };
static const WidgetConfig clockConf      = { 420, 180, 2, WA_RIGHT };
// cursor offset for clock's seconds
#define CLOCK_SECONDS_X_OFFSET  0
#define CLOCK_SECONDS_Y_OFFSET  -30

static const WidgetConfig vuConf         = { TFT_FRAMEWDT, 268, 1, WA_LEFT };

static const WidgetConfig bootWdtConf    = { 120, bootLogoTop+130, 2, WA_LEFT }; //отступ, вертикальное положение, размер, выравнивание
static const ProgressConfig bootPrgConf  = { 90, 9, 4 }; //скорость, ширина, ширина движущей части
static const BitrateConfig fullbitrateConf = {{DSP_WIDTH-TFT_FRAMEWDT-80, 226, 2, WA_LEFT}, 60 };

/* BANDS  */                             /* { onebandwidth, onebandheight, bandsHspace, bandsVspace, numofbands, fadespeed } */
static const VUBandsConfig bandsConf     = {  MAX_WIDTH, 35, 4, 5, 45, 30};

/* STRINGS  */
static const char         numtxtFmt[]    = "%d";
static const char           rssiFmt[]    = "WiFi %d";
static const char          iptxtFmt[]    = "%s";
static const char         voltxtFmt[]    = "\023\025%d";
static const char        bitrateFmt[]    = "%d kBs";

/* MOVES  */                             /* { left, top, width } */
static const MoveConfig    clockMove     = { 0, 220, MAX_WIDTH /* MAX_WIDTH */ }; // -1 disables move
static const MoveConfig   weatherMove    = { TFT_FRAMEWDT, 197, MAX_WIDTH};
static const MoveConfig   weatherMoveVU  = { TFT_FRAMEWDT, 197, MAX_WIDTH};
