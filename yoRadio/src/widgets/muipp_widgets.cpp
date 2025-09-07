#if __has_include("Arduino_GFX.h")
#include "muipp_widgets.hpp"
#include "components.hpp"
#include "agfx.h"
#include "locale/l10n.h"
#include "core/const_strings.h"
#include "core/log.h"

/**************************
      CLOCK WIDGET
 **************************/

bool ClockWidget::refresh_req() const {
  return std::time({}) != _last;
}

void ClockWidget::render(const MuiItem* parent, void* r){
  std::time_t time = std::time({});
  auto t = std::localtime(&time);
  _drawTime(t, static_cast<Arduino_GFX*>(r));
  _last = time;
  if (!_dcfg.print_date) return;
  // check if date has changed
  struct tm cur_date = *t;
  t = std::localtime(&_last_date);
  _drawDate(&cur_date, static_cast<Arduino_GFX*>(r));
  // draw date only if year of year day has changed (can't be applied here)
//  if ( cfg.print_date && (t->tm_yday != cur_date.tm_yday) || (t->tm_year != cur_date.tm_year) ){
//    _drawDate(&cur_date, static_cast<Arduino_GFX*>(r));
//    _last_date = time;
//  }
  //_reconfig(&cur_date);
};

void ClockWidget::_drawTime(tm* t, Arduino_GFX* dsp){
  _clear_clk(dsp);
  char buff[std::size("hh:mm")];
  
  std::strftime(std::data(buff), std::size(buff), "%R", t);    // "%R" equivalent to "%H:%M", t->tm_sec % 2 ? "%R" : "%H %M" would blink semicolon
  LOGV(T_Clock, println, buff );
  // recalculate area for clock and save it to be cleared later
  if (_tcfg.font_hours)
    dsp->setFont(_tcfg.font_hours);
  else if (_tcfg.font_gfx_hours)
    dsp->setFont(_tcfg.font_gfx_hours);

  int16_t x, y;
  std::tie(x, y) = _tcfg.place.getAbsoluteXY(dsp->width(), dsp->height());
  dsp->setTextSize(_tcfg.font_hours_size);
  dsp->getTextBounds(buff, x, y, &_time_block_x, &_time_block_y, &_time_block_w, &_time_block_h);

  //LOGV(T_Config, printf, "Draw clock grid: %d,%d, at: %d,%d\n", _tcfg.place.x, _tcfg.place.y, x, y);
  if (t->tm_sec % 2){   // draw ':' ?
    if (_tcfg.font_hours)
      gfxDrawText(dsp, x, y, buff, _tcfg.color, _tcfg.font_gfx_hours, _tcfg.font_hours_size);
    else if (_tcfg.font_gfx_hours)
      gfxDrawText(dsp, x, y, buff, _tcfg.color, _tcfg.font_hours, _tcfg.font_hours_size);
  } else {
    // let's draw time in parts so that ':' is drawn with background color maintaining same area dimensions
    std::strftime(std::data(buff), std::size(buff), "%H", t);
    if (_tcfg.font_hours)
      gfxDrawText(dsp, x, y, buff, _tcfg.color, _tcfg.font_hours, _tcfg.font_hours_size);
    else if (_tcfg.font_gfx_hours)
      gfxDrawText(dsp, x, y, buff, _tcfg.color, _tcfg.font_gfx_hours, _tcfg.font_hours_size);

    // write semicolon
    if (_tcfg.font_hours)
      gfxDrawText(dsp, dsp->getCursorX(), dsp->getCursorY(), ":", _tcfg.bgcolor, _tcfg.font_hours, _tcfg.font_hours_size);
    else if (_tcfg.font_gfx_hours)
      gfxDrawText(dsp, dsp->getCursorX(), dsp->getCursorY(), ":", _tcfg.bgcolor, _tcfg.font_gfx_hours, _tcfg.font_hours_size);
    //    dsp->drawChar(dsp->getCursorX(), dsp->getCursorY(), 0x3a /* ':' */, RGB565_RED, RGB565_RED);
    // write minutes
    std::strftime(std::data(buff), std::size(buff), "%M", t);
    if (_tcfg.font_hours)
      gfxDrawText(dsp, dsp->getCursorX(), dsp->getCursorY(), buff, _tcfg.color, _tcfg.font_hours, _tcfg.font_hours_size);
    else if (_tcfg.font_gfx_hours)
      gfxDrawText(dsp, dsp->getCursorX(), dsp->getCursorY(), buff, _tcfg.color, _tcfg.font_gfx_hours, _tcfg.font_hours_size);
  }

  if (!_tcfg.print_seconds) return;

  // make seconds
  std::strftime(std::data(buff), std::size(buff), "%S", t);
  if (_tcfg.font_seconds)
    dsp->setFont(_tcfg.font_seconds);
  else if (_tcfg.font_gfx_seconds)
    dsp->setFont(_tcfg.font_gfx_seconds);

  dsp->setTextSize(_tcfg.font_seconds_size);
  // recalculate area for clock and save it to be cleared later
  dsp->getTextBounds(buff, dsp->getCursorX() + _tcfg.sec_offset_x, dsp->getCursorY() + _tcfg.sec_offset_y, &_seconds_block_x, &_seconds_block_y, &_seconds_block_w, &_seconds_block_h);
  // print seconds
  if (_tcfg.font_seconds)
    gfxDrawText(dsp, dsp->getCursorX() + _tcfg.sec_offset_x, dsp->getCursorY() + _tcfg.sec_offset_y, buff, _tcfg.color, _tcfg.font_seconds, _tcfg.font_seconds_size);
  else if (_tcfg.font_gfx_seconds)
    gfxDrawText(dsp, dsp->getCursorX() + _tcfg.sec_offset_x, dsp->getCursorY() + _tcfg.sec_offset_y, buff, _tcfg.color, _tcfg.font_gfx_seconds, _tcfg.font_seconds_size);
}

void ClockWidget::_drawDate(tm* t, Arduino_GFX* dsp){
  dsp->fillRect(_date_block_x, _date_block_y, _date_block_w, _date_block_h, _dcfg.bgcolor);
  char buff[100];
  // date format "1 января / понедельник"
  snprintf(buff, std::size(buff), "%d %s / %s", t->tm_mday, mnths[t->tm_mon], _dcfg.dow_short ? dowf[t->tm_wday] : dowf[t->tm_wday]);
  //LOGD(T_Clock, println, buff);
  // recalculate area for clock and save it to be cleared later
  dsp->setFont(_dcfg.font);
  dsp->setTextSize(_dcfg.font_date_size);
  int16_t x, y;
  std::tie(x, y) = _dcfg.place.getAbsoluteXY(dsp->width(), dsp->height());
  dsp->getTextBounds(buff, x, y, &_date_block_x, &_date_block_y, &_date_block_w, &_date_block_h);

  // draw date
  gfxDrawText(dsp, x, y, buff, _dcfg.color, _dcfg.font, _dcfg.font_date_size);
}

void ClockWidget::_clear_clk(Arduino_GFX* dsp){
  // Очищаем область под текущими часами
  dsp->fillRect(_time_block_x, _time_block_y, _time_block_w, _time_block_h, _tcfg.bgcolor);  // RGB565_DARKGREY); //
  if (_tcfg.print_seconds)
    dsp->fillRect(_seconds_block_x, _seconds_block_y, _seconds_block_w, _seconds_block_h, _tcfg.bgcolor); // RGB565_DARKGREEN); //config.theme.background);
}


MuiItem_Bitrate_Widget::MuiItem_Bitrate_Widget(muiItemId id,
  int16_t x, int16_t y,
  uint16_t w, uint16_t h,
  AGFX_text_t tcfg)
    : MuiItem_Uncontrollable(id), _x(x), _y(y), _w(w), _h(h),  _tcfg(tcfg) { _events_subsribe(); };

MuiItem_Bitrate_Widget::MuiItem_Bitrate_Widget(muiItemId id, const bitrate_box_cfg_t *cfg, int16_t screen_w, int16_t screen_h, const char* label) : MuiItem_Uncontrollable(id, label){
  std::tie(_x, _y, _w, _h) = cfg->box.getBoxDimensions(screen_w, screen_h);
  _radius = cfg->radius;
  _tcfg = cfg->style;
  _bitrateFmt = cfg->bitrateFmt;
  _events_subsribe();
};

MuiItem_Bitrate_Widget::~MuiItem_Bitrate_Widget(){
  esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_CMD_EVENTS, e2int(evt::yo_event_t::playerAudioInfo), _hdlr_chg_evt);
}

void MuiItem_Bitrate_Widget::render(const MuiItem* parent, void* r){
  Arduino_GFX* g = static_cast<Arduino_GFX*>(r);

  g->fillRect(_x, _y, _w, _h, _tcfg.bgcolor);
  if (_radius){
    // draw rounded rect
    g->drawRoundRect(_x, _y, _w, _h, _radius, _tcfg.color);
    g->fillRoundRect(_x, _y + _h/2, _w, _h/2, _radius, _tcfg.color);
  } else {
    g->drawRect(_x, _y, _w, _h, _tcfg.color);
    g->fillRect(_x, _y + _h/2, _w, _h/2, _tcfg.color);
  }

  g->setFont(_tcfg.font);
  g->setTextSize(_tcfg.font_size);
  g->setTextColor(_tcfg.color, _tcfg.bgcolor);

  char buff[16];
  snprintf(buff, 16, _bitrateFmt, _info.bitRate);
  // text block
  int16_t  xx{0}, yy{0};
  uint16_t ww{0}, hh{0};
  g->getTextBounds(buff, _x, _y + _h, &xx, &yy, &ww, &hh);
  g->setCursor(_x + _w/2 - ww/2, (_y + _h/4) + hh/2);
  g->printf(_bitrateFmt, _info.bitRate);

  std::string_view a(_info.codecName ? _info.codecName : "n/a");
  g->getTextBounds(a.data(), _x, _y, &xx, &yy, &ww, &hh);
  
  // align text by h/v center 
  g->setCursor(_x + _w/2 - ww/2, _y + _h - _h/4 + hh/2);
  g->setTextColor(_tcfg.bgcolor);
  g->print(a.data());
  _pending = false;
}

void MuiItem_Bitrate_Widget::_events_subsribe(){
  // bitrate state change event picker
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::playerAudioInfo),
    [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<MuiItem_Bitrate_Widget*>(self)->setInfo(static_cast<audio_info_t*>(data)); },
    this, &_hdlr_chg_evt
  );
}

// ***** SpectrumAnalyser_Widget ***** //
// The function convert value to RGB565 color value
constexpr int8_t colors[3][3] = { {0, 0, 31}, {0, 63, 0}, {31, 0, 0} };
uint16_t convert_to_rgb(uint8_t minval, uint8_t maxval, int8_t val){
    uint16_t result;

    float i_f = (float)(val - minval) / (float)(maxval - minval) * 2;

    int Ii = i_f;
    float If = i_f - Ii;

    const int8_t *c1 = colors[Ii];
    const int8_t *c2 = colors[Ii + 1];
    uint16_t res_colors[3];

    res_colors[0] = c1[0] + If * (c2[0] - c1[0]);
    res_colors[1] = c1[1] + If * (c2[1] - c1[1]);
    res_colors[2] = c1[2] + If * (c2[2] - c1[2]);
    result = res_colors[2] | (res_colors[1] << 5) | (res_colors[0] << 11);
    return result;
}

SpectrumAnalyser_Widget::SpectrumAnalyser_Widget(muiItemId id, const muipp::grid_box &cfg, int16_t screen_w, int16_t screen_h) :
  MuiItem_Uncontrollable(id, nullptr) {
    std::tie(_x, _y, _w, _h) = cfg.getBoxDimensions(screen_w, screen_h);
    if (_spectradsp.init()){
      // set callback and subscribe only if mem allocation was done
      player->setDSPCallback([this](const int16_t* outBuff, int32_t validSamples, bool *continueI2S){ _spectradsp.data_sink(outBuff, validSamples); *continueI2S = true; } );
      _events_subsribe();
    }
  };

SpectrumAnalyser_Widget::~SpectrumAnalyser_Widget(){
  player->setDSPCallback(nullptr);
  esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_CHG_STATE_EVENTS, ESP_EVENT_ANY_ID, _hdlr_chg_evt);
}

void SpectrumAnalyser_Widget::render(const MuiItem* parent, void* r){
  if (_cleanup) {
    _running = false;
    return _clean_canvas(static_cast<Arduino_GFX*>(r));
  }

  switch (_v){
    case visual_t::bands :
      _draw_bands(static_cast<Arduino_GFX*>(r));
      break;

      case visual_t::spectrogram :
      _draw_spectrum(static_cast<Arduino_GFX*>(r));
      break;

    default:;
  }
};

void SpectrumAnalyser_Widget::_draw_spectrum(Arduino_GFX* g){
  const float* result_data = _spectradsp.getData();
  size_t len = _spectradsp.getDataSize();

  // Add left channel to the screen
  // The order of the values inverted
  for (int x = 0 ; x != len / 2; x++) {
    // Left channel:
    // invert the order of values and clamp it to 0-127
    float data = result_data[len / 2 - x - 1];

    // Convert input value in dB to the color - this will draw "waterfall spectrogram"
    g->writePixel(x, _y + _hh, convert_to_rgb(0, 128, clamp(data, 0.0F, 128.0F)));

    // Right channel:
    data = result_data[len / 2 + x];
    // Convert input value in dB to the color - this will draw "waterfall spectrogram"
    g->writePixel(x + len / 2, _y + _hh, convert_to_rgb(0, 128, clamp(data, 0.0F, 128.0F)));
  }
  
  // move spectrogram offset
  ++_hh;
  // reset y-offset
  if (_hh > _h) _hh = 0;
}

void SpectrumAnalyser_Widget::_draw_bands(Arduino_GFX* g){
  const float* result_data = _spectradsp.getData();
  size_t len = _spectradsp.getDataSize();

  // clear amplitude area
  g->fillRect(_x, _y, _w, _h, 0);

  // Add left channel to the screen
  // The order of the values inverted
  for (int x = 0 ; x != len / 2; x++) {
    // Left channel:
    // invert the order of values and clamp it to area size
    float data = clamp(result_data[len / 2 - x - 1], 0.0F, (float)_h);
    // this will draw dot amplitudes (with inverted Y axis)
    g->writePixel(_x + x, _y + _h - data, RGB565_CYAN);

    // Right channel:
    data = clamp( result_data[len / 2 + x], 0.0F, (float)_h);
    // this will draw dot amplitudes (with inverted Y axis)
    g->writePixel(_x + len / 2 + x, _y + _h - data, RGB565_CYAN);
  }
}

void SpectrumAnalyser_Widget::_events_subsribe(){
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_CHG_STATE_EVENTS, ESP_EVENT_ANY_ID,
    [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<SpectrumAnalyser_Widget*>(self)->_events_chg_hndlr(id, data); },
    this, &_hdlr_chg_evt
  );

}

void SpectrumAnalyser_Widget::_events_chg_hndlr(int32_t id, void* data){
  // process command events received via event loop bus
  switch (static_cast<evt::yo_event_t>(id)){

    // Play radio station from a playlist
    case evt::yo_event_t::playerPlay :
      LOGV(T_spectre, println, "Starting analyzer" );
      _running = true;
      break;

    case evt::yo_event_t::playerPause :
    case evt::yo_event_t::playerStop :
      LOGV(T_spectre, println, "Stopping analyzer" );
      _cleanup = true;
      break;

    default:;
  }
}

void SpectrumAnalyser_Widget::_clean_canvas(Arduino_GFX* g){
  g->fillRect(_x, _y, _w, _h, 0);
  _cleanup = false;
}



#endif  // #if __has_include("Arduino_GFX.h")
