#pragma once
#include "preset_common.hpp"

namespace display_320x480 {
// display ratio is 6:9
// let's take a base grid block size as 10x15 (i.e. grid 32x32), all larger grid sizing must be multpliers to this base size

// Clock
static constexpr clock_time_cfg_t clock_time_cfg {
  { 0, 7, muipp::coordinate_spec_t::grid, muipp::coordinate_spec_t::grid, 32, 32 },      // placement at (0.2) on (8x8) grid
  nullptr, nullptr,
  &FONT_CLOCK_DOTS_H, &FONT_CLOCK_DOTS_S,
  FONT_DEFAULT_COLOR, 0,    // color, bgcolor;
  1, 1,       // font_hours_size, font_seconds_size;
  0, -15,     // offset for seconds print after minite's position
  true        // print_seconds
};

// Date
static constexpr clock_date_cfg_t clock_date_cfg {
  { 0, 10, muipp::coordinate_spec_t::absolute, muipp::coordinate_spec_t::grid, 32, 32 },  // grid placement at 3/8 on Y axis
  FONT_SMALL_U8G2,
  FONT_DEFAULT_COLOR, 0,  //  uint16_t color, bgcolor;
  1,                      // int font_date_size;
  true, false, false      //print_date, dow_short, month_short;
};

static constexpr clock_cfg_t clock_cfg { &clock_time_cfg, &clock_date_cfg };

// text Widget for device state
static constexpr text_wdgt_t device_state_cfg {
  { 0, 10, muipp::coordinate_spec_t::center_offset, muipp::coordinate_spec_t::absolute, 0, 0 },   // centered at the top of the screen
  {
    FONT_VERY_SMALL_U8G2,         // font
    RGB565_WHITE, 0,              // color, bgcolor;
    1,                            // font size multiplicator
    muipp::text_align_t::center, muipp::text_align_t::baseline,   // horizontal / vertical alignment
    true                          // transparent background
  }
};

// Scroller 1 - station title or so
static constexpr scroller_cfg_t scroll_s1_cfg {
  1,                              // Message queue Group
  {32, 32, 0, 0, 32, 2},          // grid x,y; box position on a grid, box size on a grid;
  {
    FONT_SMALL_U8G2,              // font
    RGB565_OLIVE, 0,              // color, bgcolor;
    1,                            // font size
    muipp::text_align_t::left, muipp::text_align_t::baseline, //  muipp::text_align_t halign{muipp::text_align_t::left}, valign{muipp::text_align_t::baseline};
    false                         // transp_bg
  },
  30                              // scroll speed
};

// Scroller 2 - song title or so
static constexpr scroller_cfg_t scroll_s2_cfg {
  2,                              // Message queue Group
  {32, 32, 0, 2, 32, 2},          // grid x,y; box position on a grid, box size on a grid;
  {
    FONT_SMALL_U8G2,              // font
    RGB565_CYAN, 0,               // color, bgcolor;
    1,                            // font size
    muipp::text_align_t::left, muipp::text_align_t::baseline, //  muipp::text_align_t halign{muipp::text_align_t::left}, valign{muipp::text_align_t::baseline};
    false                         // transp_bg
  },
  40                              // scroll speed
};

// Bitrate widget box
static constexpr bitrate_box_cfg_t bitrate_cfg {
  {16, 16, 0, 6, 3, 2},           // grid x,y; box position on a grid (x,y), box size on a grid (w,h)
  10,                             // radius for round-shaped corners
  {
    FONT_SMALL_U8G2,              // font
    RGB565_CYAN, 0,               // color, bgcolor;
    1,                            // font size
    muipp::text_align_t::left, muipp::text_align_t::baseline, //  muipp::text_align_t halign{muipp::text_align_t::left}, valign{muipp::text_align_t::baseline};
    false                         // transp_bg
  },
  T_bitrate_k                     // print format
};

static constexpr spectrum_box_cfg_t spectrum_cfg {
  {16, 16, 0, 10, 16, 6},        // grid x,y; box position on a grid (x,y), box size on a grid (w,h)

};

// a preset with set of widgets
#define  baseline_320x480  25091201UL

static const std::vector<widget_cfgitem_t> cfg1 {
  // Clock
  { yoyo_wdgt_t::clock, T_clock, true, &clock_cfg},
  // text - device state
  //{ yoyo_wdgt_t::textStatic, T_stateHeader, true, &device_state_cfg},
  // Scroller 1 - station
  { yoyo_wdgt_t::textScroller, T_scrollerStation, true, &scroll_s1_cfg},
  // Scroller 2 - track title, etc...
  { yoyo_wdgt_t::textScroller, T_scrollerTitle, true, &scroll_s2_cfg},
  // bitrate
  { yoyo_wdgt_t::bitrate, T_bitrate, true, &bitrate_cfg},
  // spectrum
  { yoyo_wdgt_t::spectrumAnalyzer, T_spectrumAnalyzer, true, &spectrum_cfg }
};

};    // namespace   display_320x480