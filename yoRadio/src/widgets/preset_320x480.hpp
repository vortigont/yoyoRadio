#pragma once
#include "preset_common.hpp"

namespace display_320x480 {

// Clock
static constexpr clock_time_cfg_t clock_cfg {
  { 0, 4, muipp::coordinate_spec_t::grid, muipp::coordinate_spec_t::grid, 1, 24 },      // grid placement at 2/8 on Y axis
  nullptr, nullptr,
  &FONT_CLOCK_DOTS_H, &FONT_CLOCK_DOTS_S,
  FONT_DEFAULT_COLOR, 0,    // color, bgcolor;
  1, 1,       // font_hours_size, font_seconds_size;
  0, -15,     // offset for seconds print after minite's position
  true        // print_seconds
};

// Date
static constexpr clock_date_cfg_t date_cfg {
  { 0, 6, muipp::coordinate_spec_t::absolute, muipp::coordinate_spec_t::grid, 1, 24 },  // grid placement at 3/8 on Y axis
  FONT_SMALL_U8G2,
  FONT_DEFAULT_COLOR, 0,  //  uint16_t color, bgcolor;
  2,                      // int font_date_size;
  true, false, false      //print_date, dow_short, month_short;
};

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
  {1, 24, 0, 1, 1, 16},           // grid x,y; box position on a grid, box size on a grid;
  {
    FONT_SMALL_U8G2,              // font
    RGB565_OLIVE, 0,              // color, bgcolor;
    1,                            // font size
    muipp::text_align_t::left, muipp::text_align_t::baseline, //  muipp::text_align_t halign{muipp::text_align_t::left}, valign{muipp::text_align_t::baseline};
    false                         // transp_bg
  },
  40                              // scroll speed
};

// Scroller 2 - song title or so
static constexpr scroller_cfg_t scroll_s2_cfg {
  {1, 24, 0, 2, 1, 16},           // grid x,y; box position on a grid, box size on a grid;
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
  {6, 24, 0, 8, 1, 16},           // grid x,y; box position on a grid, box size on a grid;
  10,                             // radius for round-shaped corners
  {
    FONT_SMALL_U8G2,              // font
    RGB565_CYAN, 0,               // color, bgcolor;
    1,                            // font size
    muipp::text_align_t::left, muipp::text_align_t::baseline, //  muipp::text_align_t halign{muipp::text_align_t::left}, valign{muipp::text_align_t::baseline};
    false                         // transp_bg
  },
  "%uk"                           // print format
};


};    // namespace   display_320x480