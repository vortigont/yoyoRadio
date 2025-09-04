#pragma once
#include <ctime>
#include "muipp_agfx.hpp"
#include "core/common.h"
#include "core/evtloop.h"
#include "core/spectrum.hpp"

// a list of known widget types
enum class yoyo_wdgt_t {
  unknown = 0,
  bitrate,
  clock,
  textStatic,
  textScroller,
  spectrumAnalyzer
};

/**
 * @brief this struct is a configuration placeholder that carries a widget type and an abstract pointer to it's config struct
 * used to define static configuration presets
 * @note mapping widget's label to it's class type is done via 'widgets_map' struct in "widget_dispatcher.hpp"
 */
struct widget_cfgitem_t {
    // widget's label
    const char* wlabel;
    // a pointer to it's configuration
    const void* cfg;
};


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

// combined ptr struct for Clock
struct clock_cfg_t {
  const clock_time_cfg_t *clk;
  const clock_date_cfg_t *date;
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
  clock_time_cfg_t _tcfg;
  clock_date_cfg_t _dcfg;
  
public:
  ClockWidget(muiItemId id, const clock_time_cfg_t& clk, const clock_date_cfg_t& date): MuiItem_Uncontrollable(id, nullptr), _tcfg(clk), _dcfg(date) {};

  void render(const MuiItem* parent, void* r = nullptr) override;
  bool refresh_req() const override;

private:
  void _drawTime(tm* t, Arduino_GFX* dsp);
  void _drawDate(tm* t, Arduino_GFX* dsp);
  void _clear_clk(Arduino_GFX* dsp);
  void _clear_date(Arduino_GFX* dsp);
};


/**
 * @brief shows codec/bitrate text box
 * 
 */
class MuiItem_Bitrate_Widget : public MuiItem_Uncontrollable {
  int16_t _x, _y;
  uint16_t _w, _h;
  uint32_t _radius;
  AGFX_text_t _tcfg;
  const char* _bitrateFmt;
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

  MuiItem_Bitrate_Widget(muiItemId id, const bitrate_box_cfg_t *cfg, int16_t screen_w, int16_t screen_h);

  ~MuiItem_Bitrate_Widget();

  //void setText(const char* text, float speed){ _scroller.begin(text, speed, _tcfg.font); }
  void render(const MuiItem* parent, void* r = nullptr) override;
  bool refresh_req() const override { return _pending; };
  void setInfo(audio_info_t* i){ _info = *i; _pending = true; }

private:

  /**
   * @brief subscribe to event mesage bus
   * 
   */
  void _events_subsribe();

};


// *****  SpectrumAnalyser_Widget
/**
 * @brief config for Spectrum Analyzer widget box
 * 
 */
struct spectrum_box_cfg_t {
  // widget box on a grid
  muipp::grid_box box;
};

/**
 * @brief graphics spectrum analyzer
 * 
 */
class SpectrumAnalyser_Widget : public MuiItem_Uncontrollable {
  int16_t _x, _y;
  uint16_t _w, _h;
  // y-offset for spectrogram
  uint16_t _hh{0};

public:
  SpectrumAnalyser_Widget(muiItemId id, const muipp::grid_box &cfg, int16_t screen_w, int16_t screen_h);
  ~SpectrumAnalyser_Widget();

  void render(const MuiItem* parent, void* r = nullptr);
  bool refresh_req() const override { return _running; };

  enum class visual_t { bands, spectrogram };

private:
  bool _running{false}, _cleanup{false};
  // visualization type
  enum visual_t _v{visual_t::bands};

  esp_event_handler_instance_t _hdlr_chg_evt{nullptr};
  
  SpectraDSP _spectradsp;

  void _draw_spectrum(Arduino_GFX* g);

  void _draw_bands(Arduino_GFX* g);

  // cleanup widget area when audio stops
  void _clean_canvas(Arduino_GFX* g);

  /**
   * @brief subscribe to event mesage bus
   * 
   */
  void _events_subsribe();

  // change events
  void _events_chg_hndlr(int32_t id, void* data);
};