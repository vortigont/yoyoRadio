#pragma once
#include "muipp_agfx.hpp"
#include "../gfx_lib.h"
#include <ctime>

// struct that defines clock position
struct clock_time_cfg_t {
  int16_t x, y;   // clock print position
#ifdef CLOCK_USE_U8G2_FONT
  const uint8_t* font_hours, font_seconds;
#elif defined CLOCK_USE_AGFX_FONT
  const GFXfont* font_hours;
  const GFXfont* font_seconds;
#endif
  uint16_t color, bgcolor;
  int font_hours_size, font_seconds_size;
  int16_t sec_offset_x, sec_offset_y;   // offset for seconds print after minite's position
  bool print_seconds, print_date;
};

struct clock_date_cfg_t {
  int16_t x, y;     // date print position
#ifdef CLOCK_DATE_USE_U8G2_FONT
  const uint8_t* font;
#elif defined CLOCK_USE_AGFX_FONT
  const GFXfont* font;
#endif
  uint16_t color, bgcolor;
  int font_date_size;
  bool print_date, dow_short, month_short;
};

class ClockWidget: public MuiItem_Uncontrollable {
  std::time_t _last{0}, _last_date{0};
  // vars to save time block bounds, needed to clear time blocks on next run
  int16_t _time_block_x{0}, _time_block_y{0}, _seconds_block_x{0}, _seconds_block_y{0}, _date_block_x{0}, _date_block_y{0};
  uint16_t  _time_block_w{0}, _time_block_h{0}, _seconds_block_w{0}, _seconds_block_h{0}, _date_block_w{0}, _date_block_h{0};
  
public:
  ClockWidget(muiItemId id): MuiItem_Uncontrollable(id, nullptr){};

  void render(const MuiItem* parent, void* r = nullptr) override;
  bool refresh_req() const override;

  clock_time_cfg_t cfg;
  clock_date_cfg_t dcfg;

private:
  void _drawTime(tm* t, Arduino_GFX* dsp);
  void _drawDate(tm* t, Arduino_GFX* dsp);
  void _clear_clk(Arduino_GFX* dsp);
  void _clear_date(Arduino_GFX* dsp);
  // move clock in screensaver mode / measure brightness
  //void _reconfig(tm* t);
};
