#pragma once
#include "320x480_agfx_defines.h"

// this file contains static structs for widget's configs related to display layout

// Clock
static const clock_time_cfg_t clock_cfg{
  0, 100,   // clock print position
  &FONT_CLOCK_H, &FONT_CLOCK_S,
  FONT_DEFAULT_COLOR, 0,    // color, bgcolor;
  1, 1,       // font_hours_size, font_seconds_size;
  0, -15,     // offset for seconds print after minite's position
  true, true  // print_seconds, print_date;
};

// Date
static const clock_date_cfg_t date_cfg{
  0, 150,                 // date print position
  FONT_DATE,
  FONT_DEFAULT_COLOR, 0,  //  uint16_t color, bgcolor;
  2,                      // int font_date_size;
  true, false, false      //print_date, dow_short, month_short;
};

/* STRINGS  */
static const char         numtxtFmt[]    = "%d";
static const char           rssiFmt[]    = "WiFi %d";
static const char          iptxtFmt[]    = "%s";
static const char         voltxtFmt[]    = "\023\025%d";
static const char        bitrateFmt[]    = "%d kBs";

