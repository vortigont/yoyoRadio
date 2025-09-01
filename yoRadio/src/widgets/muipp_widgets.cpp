#if __has_include("Arduino_GFX.h")
#include "muipp_widgets.hpp"
#include "agfx.h"
#include "locale/l10n.h"
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

MuiItem_Bitrate_Widget::MuiItem_Bitrate_Widget(muiItemId id, const bitrate_box_cfg_t *cfg, int16_t screen_w, int16_t screen_h) : MuiItem_Uncontrollable(id){
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

#endif  // #if __has_include("Arduino_GFX.h")
