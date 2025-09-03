#pragma once
#include "preset_common.hpp"

namespace display_1024x600 {
// display ratio is 16:9
// let's take a base grid block size as 16x9 (i.e. grid close to 64x64), all larger grid sizing must be multpliers to this base size

// Clock
static constexpr clock_time_cfg_t clock_time_cfg {
  { 22, 2, muipp::coordinate_spec_t::grid, muipp::coordinate_spec_t::grid, 32, 32 },      // placement at (22.2) on (32x32) grid
  nullptr, nullptr,
  &FONT_CLOCK_DOTS_H, &FONT_CLOCK_DOTS_S,
  FONT_DEFAULT_COLOR, 0,    // color, bgcolor;
  1, 1,       // font_hours_size, font_seconds_size;
  0, -15,     // offset for seconds print after minite's position
  true        // print_seconds
};

// Date
static constexpr clock_date_cfg_t clock_date_cfg {
  { 22, 4, muipp::coordinate_spec_t::grid, muipp::coordinate_spec_t::grid, 32, 32 },     // placement at (22.3) on (32x32) grid
  FONT_SMALL_U8G2,
  FONT_DEFAULT_COLOR, 0,  //  uint16_t color, bgcolor;
  1,                      // int font_date_size;
  true, false, false      //print_date, dow_short, month_short;
};

static constexpr clock_cfg_t clock_cfg { &clock_time_cfg, &clock_date_cfg };

// text Widget for device state
static constexpr text_wdgt_t device_state_cfg {
  { 0, 14, muipp::coordinate_spec_t::center_offset, muipp::coordinate_spec_t::absolute, 0, 0 },   // centered at the top of the screen
  {
    FONT_SMALL_U8G2,              // font
    RGB565_WHITE, 0,              // color, bgcolor;
    1,                            // font size multiplicator
    muipp::text_align_t::left, muipp::text_align_t::baseline,   // horizontal / vertical alignment
    true                          // transparent background
  }
};

// Scroller 1 - station title or so
static constexpr scroller_cfg_t scroll_s1_cfg {
  {32, 32, 0, 0, 16, 2},           // grid x,y; box position on a grid, box size on a grid;
  {
    FONT_SMALL_U8G2,              // font
    RGB565_OLIVE, 0,              // color, bgcolor;
    2,                            // font size
    muipp::text_align_t::left, muipp::text_align_t::baseline, //  muipp::text_align_t halign{muipp::text_align_t::left}, valign{muipp::text_align_t::baseline};
    false                         // transp_bg
  },
  20                              // scroll speed
};

// Scroller 2 - song title or so
static constexpr scroller_cfg_t scroll_s2_cfg {
  {32, 32, 0, 2, 16, 2},           // grid x,y; box position on a grid, box size on a grid;
  {
    FONT_SMALL_U8G2,              // font
    RGB565_CYAN, 0,               // color, bgcolor;
    2,                            // font size
    muipp::text_align_t::left, muipp::text_align_t::baseline, //  muipp::text_align_t halign{muipp::text_align_t::left}, valign{muipp::text_align_t::baseline};
    false                         // transp_bg
  },
  60                              // scroll speed
};

// Bitrate widget box
static constexpr bitrate_box_cfg_t bitrate_cfg {
  {32, 32, 16, 1, 3, 3},           // grid x,y; box position on a grid (x,y), box size on a grid (w,h)
  10,                             // radius for round-shaped corners
  {
    FONT_SMALL_U8G2,              // font
    RGB565_CYAN, 0,               // color, bgcolor;
    1,                            // font size
    muipp::text_align_t::left, muipp::text_align_t::baseline, //  muipp::text_align_t halign{muipp::text_align_t::left}, valign{muipp::text_align_t::baseline};
    false                         // transp_bg
  },
  T_bitrate_Kbps                  // print format
};


// Spectrum Analyzer
static constexpr spectrum_box_cfg_t spectrum_cfg {
  {32, 32, 0, 16, 16, 16},        // grid x,y; box position on a grid (x,y), box size on a grid (w,h)

};

// a preset with set of widgets

static const std::vector<widget_cfgitem_t> cfg1 {
  // Clock
  { yoyo_wdgt_t::clock, &clock_cfg},
  // text - device state
  { yoyo_wdgt_t::text, &device_state_cfg},
  // Scroller 1 - station
  { yoyo_wdgt_t::scrollerStation, &scroll_s1_cfg},
  // Scroller 2 - track title, etc...
  { yoyo_wdgt_t::scrollerTitle, &scroll_s2_cfg},
  // bitrate
  { yoyo_wdgt_t::bitrate, &bitrate_cfg},
  // spectrum
  { yoyo_wdgt_t::spectrumAnalyzer, &spectrum_cfg }
};

};    // namespace   display_320x480