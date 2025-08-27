#pragma once
#include <ctime>
#include "muipp_agfx.hpp"
#include "core/common.h"
#include "core/evtloop.h"

/**
 * @brief Text-alike widget configuration
 * defines placement and text style config
 * 
 */
struct text_wdgt_t {
  muipp::item_position_t place;
  AGFX_text_t style;
};


/**
 * @brief defines configuration for scroller widgets
 * 
 */
struct scroller_cfg_t {
  muipp::grid_box box;
  AGFX_text_t style;
  uint32_t scroll_speed;
};

/**
 * @brief config for bitrate widget box
 * 
 */
struct bitrate_box_cfg_t {
  // widget box on a grid
  muipp::grid_box box;
  uint32_t radius;
  AGFX_text_t style;
  const char* bitrateFmt;
};


/**
 * @brief struct that defines clock position, fonts, etc...
 * use either u8g2 or agfx fonts for BOTH hours and secs!
 * The other must be set to NULL
 * @note it is here only because I like DirectiveFour font for clocks :)
 */
struct clock_time_cfg_t {
  muipp::item_position_t place;                         // widget's position
  const uint8_t *font_hours, *font_seconds;             // U8G2 font (NULL if not used)
  const GFXfont *font_gfx_hours, *font_gfx_seconds;     // Adafruit font (NULL if not used)
  uint16_t color, bgcolor;                              // font color, background fill
  int font_hours_size, font_seconds_size;               // font size multiplicator
  int16_t sec_offset_x, sec_offset_y;                   // pixel offset for seconds printing right after minute's cursor position
  bool print_seconds;                                   // print seconds or not
};

/**
 * @brief struct that defines clock's date position, fonts, etc...
 * 
 */
struct clock_date_cfg_t {
  muipp::item_position_t place;                         // widget's position
  const uint8_t* font;
  uint16_t color, bgcolor;
  int font_date_size;
  bool print_date, dow_short, month_short;
};


/**
 * @brief Widget prints clock on a screen
 * optional date string printing
 * 
 */
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


/**
 * @brief shows codec/bitrate text box
 * 
 */
class MuiItem_Bitrate_Widget : public MuiItem_Uncontrollable {
  int16_t _x, _y;
  uint16_t _w, _h;
  AGFX_text_t _tcfg;
  bool _pending;
  audio_info_t _info{0, nullptr};
  esp_event_handler_instance_t _hdlr_chg_evt{nullptr};

public:
  /**
   * @brief Construct a new MuiItem_U8g2_PageTitle object
   * 
   * @param id assigned id for the item
   * @param x, y Coordinates of the top left corner to start printing
   * @param w, h canvas size where data is printed
   * @param tcfg text decoration config
   */
  MuiItem_Bitrate_Widget(muiItemId id,
      int16_t x, int16_t y,
      uint16_t w, uint16_t h,
      AGFX_text_t tcfg = {});

  ~MuiItem_Bitrate_Widget();

  //void setText(const char* text, float speed){ _scroller.begin(text, speed, _tcfg.font); }
  void render(const MuiItem* parent, void* r = nullptr) override;
  bool refresh_req() const override { return _pending; };
  void setInfo(audio_info_t* i){ _info = *i; _pending = true; }
};
