#include "../gfx_engine.h"
#if DSP_MODEL!=DSP_DUMMY

#include "widgets.h"
#include "../../core/player.h"    //  for VU widget
#include "../tools/l10n.h"

static constexpr const char* P_na = "n/a";

/************************
      FILL WIDGET
 ************************/
void FillWidget::init(FillConfig conf, uint16_t bgcolor){
  Widget::init(conf.widget, bgcolor, bgcolor);
  _width = conf.width;
  _height = conf.height;
  
}

void FillWidget::_draw(){
  if(!_active) return;
  dsp->fillRect(_config.left, _config.top, _width, _height, _bgcolor);
}

void FillWidget::setHeight(uint16_t newHeight){
  _height = newHeight;
  //_draw();
}
/************************
      TEXT WIDGET
 ************************/

void TextWidget::init(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor) {
  Widget::init(wconf, fgcolor, bgcolor);
  text.reserve(buffsize);
  dsp->charSize(_config.textsize, charWidth, textheight);
  textwidth = oldtextwidth = oldleft = 0;
  _uppercase = uppercase;
}

void TextWidget::setText(const char* txt) {
  text = dsp->utf8Rus(txt, _uppercase);
  textwidth = text.length() * charWidth;
  if (text.compare(oldtext) == 0) return;
  if (_active) dsp->fillRect(oldleft == 0 ? realLeft() : min(oldleft, realLeft()),  _config.top, max(oldtextwidth, textwidth), textheight, _bgcolor);
  oldtextwidth = textwidth;
  oldleft = realLeft();
  if (_active) _draw();
}

void TextWidget::setText(int val, const char *format){
  snprintf(text.data(), text.size(), format, val);
}

void TextWidget::setText(const char* txt, const char *format){
  snprintf(text.data(), text.size(), format, txt);
  // todo: do I need uft8 conversion here for compatibility with an old code?
//  setText(buf);
}

uint16_t TextWidget::realLeft() {
  switch (_config.align) {
    case WA_CENTER: return (dsp->width() - textwidth) / 2; break;
    case WA_RIGHT: return (dsp->width() - textwidth - _config.left); break;
    default: return _config.left; break;
  }
}

void TextWidget::draw(){
  if(!_active) return;
  dsp->setTextColor(_fgcolor, _bgcolor);
  dsp->setCursor(realLeft(), _config.top);
  dsp->setFont();
  dsp->setTextSize(_config.textsize);
  dsp->print(text.c_str());
  oldtext = text;
}

/************************
      SCROLL WIDGET
 ************************/
ScrollWidget::ScrollWidget(const char* separator, ScrollConfig conf, uint16_t fgcolor, uint16_t bgcolor) {
  init(separator, conf, fgcolor, bgcolor);
}

void ScrollWidget::init(const char* separator, ScrollConfig conf, uint16_t fgcolor, uint16_t bgcolor) {
  TextWidget::init(conf.widget, conf.buffsize, conf.uppercase, fgcolor, bgcolor);
  _sep.reserve(4);
  snprintf(_sep.data(), 4, " %.*s ", 1, separator);
  _x = conf.widget.left;
  _startscrolldelay = conf.startscrolldelay;
  _scrolldelta = conf.scrolldelta;
  _scrolltime = conf.scrolltime;
  // calculate char size for the text
  // todo: this considers monospace fonts only!!! to be refactored for variable fonts
  dsp->charSize(_config.textsize, charWidth, textheight);
  _sepwidth = _sep.length() * charWidth;
  _width = conf.width;
  _backMove.width = _width;
  _window.reserve(MAX_WIDTH / charWidth + 1);
  _doscroll = false;
}

void ScrollWidget::_setTextParams() {
  if (_config.textsize == 0) return;
  dsp->setTextSize(_config.textsize);
  dsp->setTextColor(_fgcolor, _bgcolor);
  dsp->setFont(font);
}

bool ScrollWidget::_checkIsScrollNeeded() {
  return textwidth > _width;
}

void ScrollWidget::setText(const char* txt) {
  text = txt;
  //strlcpy(_text, dsp->utf8Rus(txt, _uppercase), _buffsize - 1);
  if (text.compare(oldtext) == 0) return;
  textwidth = text.length() * charWidth;
  _x = _config.left;
  _doscroll = _checkIsScrollNeeded();
  if (dsp->getScrollId() == this) dsp->setScrollId(NULL);
  _scrolldelay = millis();
  if (_active) {
    _setTextParams();
    if (_doscroll) {
        dsp->fillRect(_config.left,  _config.top, _width, textheight, _bgcolor);
        dsp->setCursor(_config.left, _config.top);
        _window = text;
        //snprintf(_window.data(), _window.size(), "%s", text.c_str()); //TODO _width / charWidth + 1
        dsp->setClipping({_config.left, _config.top, _width, textheight});
        dsp->print(_window.c_str());
        dsp->clearClipping();
    } else {
      dsp->fillRect(_config.left, _config.top, _width, textheight, _bgcolor);
      dsp->setCursor(realLeft(), _config.top);
      //dsp->setClipping({_config.left, _config.top, _width, _textheight});
      dsp->print(text.c_str());
      //dsp->clearClipping();
    }
    oldtext = text;
  }
}

void ScrollWidget::setText(const char* txt, const char *format){
  size_t l = strlen(txt) + 32;
  char buf[l];   // what's the purpose of this???
  snprintf(buf, l, format, txt);
  setText(buf);
}

void ScrollWidget::loop() {
  if(_locked) return;
  if (!_doscroll || _config.textsize == 0 || (dsp->getScrollId() != NULL && dsp->getScrollId() != this)) return;
  if (_checkDelay(_x == _config.left ? _startscrolldelay : _scrolltime, _scrolldelay)) {
    _calcX();
    if (_active) _draw();
  }
}

void ScrollWidget::_clear(){
  dsp->fillRect(_config.left, _config.top, _width, textheight, _bgcolor);
}

void ScrollWidget::_draw() {
  if(!_active || _locked) return;
  _setTextParams();
  if (_doscroll) {
    size_t _newx = _config.left - _x;
    const char* _cursor = text.c_str() + _newx / charWidth;
    uint16_t hiddenChars = _cursor - text.c_str();
    if (hiddenChars < text.length()) {
      snprintf(_window.data(), _window.size(), "%s%s%s", _cursor, _sep, text.c_str());
    } else {
      const char* _scursor = _sep.c_str() + (_cursor - (text.c_str() + text.length()));
      snprintf(_window.data(), _window.size(), "%s%s", _scursor, text.c_str());
    }
    dsp->setCursor(_x + hiddenChars * charWidth, _config.top);
    dsp->setClipping({_config.left, _config.top, _width, textheight});
    //Serial.printf("scrollw:%s", _window); ==HERE
    //dsp->print(_window);
    #ifndef DSP_LCD
      dsp->print(" ");
    #endif
    dsp->clearClipping();
  } else {
    dsp->fillRect(_config.left, _config.top, _width, textheight, _bgcolor);
    dsp->setCursor(realLeft(), _config.top);
    dsp->setClipping({realLeft(), _config.top, _width, textheight});
    dsp->print(text.c_str());
    dsp->clearClipping();
  }
}

void ScrollWidget::_calcX() {
  if (!_doscroll || _config.textsize == 0) return;
  _x -= _scrolldelta;
  if (-_x > textwidth + _sepwidth - _config.left) {
    _x = _config.left;
    dsp->setScrollId(NULL);
  } else {
    dsp->setScrollId(this);
  }
}

bool ScrollWidget::_checkDelay(int m, uint32_t &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

void ScrollWidget::_reset(){
  dsp->setScrollId(NULL);
  _x = _config.left;
  _scrolldelay = millis();
  _doscroll = _checkIsScrollNeeded();
}

/************************
      SLIDER WIDGET
 ************************/
void SliderWidget::init(FillConfig conf, uint16_t fgcolor, uint16_t bgcolor, uint32_t maxval, uint16_t oucolor) {
  Widget::init(conf.widget, fgcolor, bgcolor);
  _width = conf.width; _height = conf.height; _outlined = conf.outlined; _oucolor = oucolor, _max = maxval;
  _oldvalwidth = _value = 0;
}

void SliderWidget::setValue(uint32_t val) {
  _value = val;
  if (_active && !_locked) _drawslider();

}

void SliderWidget::_drawslider() {
  uint16_t valwidth = map(_value, 0, _max, 0, _width - _outlined * 2);
  if (_oldvalwidth == valwidth) return;
  dsp->fillRect(_config.left + _outlined + min(valwidth, _oldvalwidth), _config.top + _outlined, abs(_oldvalwidth - valwidth), _height - _outlined * 2, _oldvalwidth > valwidth ? _bgcolor : _fgcolor);
  _oldvalwidth = valwidth;
}

void SliderWidget::_draw() {
  if(_locked) return;
  _clear();
  if(!_active) return;
  if (_outlined) dsp->drawRect(_config.left, _config.top, _width, _height, _oucolor);
  uint16_t valwidth = map(_value, 0, _max, 0, _width - _outlined * 2);
  dsp->fillRect(_config.left + _outlined, _config.top + _outlined, valwidth, _height - _outlined * 2, _fgcolor);
}

void SliderWidget::_clear() {
//  _oldvalwidth = 0;
  dsp->fillRect(_config.left, _config.top, _width, _height, _bgcolor);
}
void SliderWidget::_reset() {
  _oldvalwidth = 0;
}
/************************
      VU WIDGET
 ************************/
#if !defined(DSP_LCD) && !defined(DSP_OLED)
VuWidget::~VuWidget() {
  delete _canvas;
}

void VuWidget::init(WidgetConfig wconf, VUBandsConfig bands, uint16_t vumaxcolor, uint16_t vumincolor, uint16_t bgcolor) {
  Widget::init(wconf, bgcolor, bgcolor);
  _vumaxcolor = vumaxcolor;
  _vumincolor = vumincolor;
  _bands = bands;
  _canvas = new Canvas(_bands.width * 2 + _bands.space, _bands.height, dsp);
}


void VuWidget::_draw(){
  if(!_active || _locked) return;
#if !defined(USE_NEXTION) && I2S_DOUT==255
/*  static uint8_t cc = 0;
  cc++;
  if(cc>0){
    player.getVUlevel();
    cc=0;
  }*/
#endif
  static uint16_t measL, measR;
  uint16_t bandColor;
  uint16_t dimension = _config.align?_bands.width:_bands.height;
  uint16_t vulevel = player->getVUlevel();
  
  uint8_t L = (vulevel >> 8) & 0xFF;
  uint8_t R = vulevel & 0xFF;
  
  bool played = player->isRunning();
  if(played){
    measL=(L>=measL)?measL + _bands.fadespeed:L;
    measR=(R>=measR)?measR + _bands.fadespeed:R;
  }else{
    if(measL<dimension) measL += _bands.fadespeed;
    if(measR<dimension) measR += _bands.fadespeed;
  }
  if(measL>dimension) measL=dimension;
  if(measR>dimension) measR=dimension;
  uint8_t h=(dimension/_bands.perheight)-_bands.vspace;
  _canvas->fillRect(0,0,_bands.width * 2 + _bands.space,_bands.height, _bgcolor);
  for(int i=0; i<dimension; i++){
    if(i%(dimension/_bands.perheight)==0){
      if(_config.align){
        #ifndef BOOMBOX_STYLE
          bandColor = (i>_bands.width-(_bands.width/_bands.perheight)*4)?_vumaxcolor:_vumincolor;
          _canvas->fillRect(i, 0, h, _bands.height, bandColor);
          _canvas->fillRect(i + _bands.width + _bands.space, 0, h, _bands.height, bandColor);
        #else
          bandColor = (i>(_bands.width/_bands.perheight))?_vumincolor:_vumaxcolor;
          _canvas->fillRect(i, 0, h, _bands.height, bandColor);
          bandColor = (i>_bands.width-(_bands.width/_bands.perheight)*3)?_vumaxcolor:_vumincolor;
          _canvas->fillRect(i + _bands.width + _bands.space, 0, h, _bands.height, bandColor);
        #endif
      }else{
        bandColor = (i<(_bands.height/_bands.perheight)*3)?_vumaxcolor:_vumincolor;
        _canvas->fillRect(0, i, _bands.width, h, bandColor);
        _canvas->fillRect(_bands.width + _bands.space, i, _bands.width, h, bandColor);
      }
    }
  }
  if(_config.align){
    #ifndef BOOMBOX_STYLE
      _canvas->fillRect(_bands.width-measL, 0, measL, _bands.width, _bgcolor);
      _canvas->fillRect(_bands.width * 2 + _bands.space - measR, 0, measR, _bands.width, _bgcolor);
      dsp->draw16bitRGBBitmap(_config.left, _config.top, _canvas->getFramebuffer(), _bands.width * 2 + _bands.space, _bands.height);
      //dsp->drawRGBBitmap(_config.left, _config.top, _canvas->getBuffer(), _bands.width * 2 + _bands.space, _bands.height);
    #else
      _canvas->fillRect(0, 0, _bands.width-(_bands.width-measL), _bands.width, _bgcolor);
      _canvas->fillRect(_bands.width * 2 + _bands.space - measR, 0, measR, _bands.width, _bgcolor);
      dsp->drawRGBBitmap(_config.left, _config.top, _canvas->getBuffer(), _bands.width * 2 + _bands.space, _bands.height);
    #endif
  }else{
    _canvas->fillRect(0, 0, _bands.width, measL, _bgcolor);
    _canvas->fillRect(_bands.width + _bands.space, 0, _bands.width, measR, _bgcolor);
    dsp->draw16bitRGBBitmap(_config.left, _config.top, _canvas->getFramebuffer(), _bands.width * 2 + _bands.space, _bands.height);
    //dsp->drawRGBBitmap(_config.left, _config.top, _canvas->getBuffer(), _bands.width * 2 + _bands.space, _bands.height);
  }
}

void VuWidget::loop(){
  if(_active || !_locked) _draw();
}

void VuWidget::_clear(){
  dsp->fillRect(_config.left, _config.top, _bands.width * 2 + _bands.space, _bands.height, _bgcolor);
}
#else // DSP_LCD
VuWidget::~VuWidget() { }
void VuWidget::init(WidgetConfig wconf, VUBandsConfig bands, uint16_t vumaxcolor, uint16_t vumincolor, uint16_t bgcolor) {
  Widget::init(wconf, bgcolor, bgcolor);
}
void VuWidget::_draw(){ }
void VuWidget::loop(){ }
void VuWidget::_clear(){ }
#endif
/************************
      NUM WIDGET
 ************************/
void NumWidget::init(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor) {
  Widget::init(wconf, fgcolor, bgcolor);
  text.reserve(buffsize);
  textwidth = oldtextwidth = oldleft = 0;
  _uppercase = uppercase;
  textheight = wconf.textsize;
}

void NumWidget::setText(const char* txt) {
  text = txt;
  _getBounds();
  if (text.compare(oldtext) == 0) return;
  uint16_t realth = textheight;
#if defined(DSP_OLED) && DSP_MODEL!=DSP_SSD1322
  realth = textheight*CHARHEIGHT;
#endif
  if (_active) dsp->fillRect(oldleft == 0 ? realLeft() : min(oldleft, realLeft()),  _config.top - textheight + 1, max(oldtextwidth, textwidth), realth, _bgcolor);
  oldtextwidth = textwidth;
  oldleft = realLeft();
  if (_active) _draw();
}

void NumWidget::setText(int val, const char *format){
  char buf[16];   // what's the purpose of this???
  snprintf(buf, 16, format, val);
  setText(buf);
}

void NumWidget::_getBounds() {
  textwidth= dsp->textWidth(text.c_str());
}

void NumWidget::_draw() {
  if(!_active) return;
  dsp->setNumFont(); // --------------SetBigFont
  //dsp->setTextSize(1);
  dsp->setTextColor(_fgcolor, _bgcolor);
  dsp->setCursor(realLeft(), _config.top);
  dsp->print(text.c_str());
  oldtext = text;
  dsp->setFont();
}

/**************************
      PROGRESS WIDGET
 **************************/
void ProgressWidget::_progress() {
  char buf[_width + 1];
  snprintf(buf, _width, "%*s%.*s%*s", _pg <= _barwidth ? 0 : _pg - _barwidth, "", _pg <= _barwidth ? _pg : 5, ".....", _width - _pg, "");
  _pg++; if (_pg >= _width + _barwidth) _pg = 0;
  setText(buf);
}

bool ProgressWidget::_checkDelay(int m, uint32_t &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

void ProgressWidget::loop() {
  if (_checkDelay(_speed, _scrolldelay)) {
    _progress();
  }
}

/**************************
      CLOCK WIDGET
 **************************/

bool ClockWidget::run(bool force){
  std::time_t time = std::time({});
  auto t = std::localtime(&time);
  if (time == _last && !force){
    return false;
  }
  dsp->lock();
  _drawTime(t);
  _last = time;
  // check if date has changed
  struct tm cur_date = *t;
  t = std::localtime(&_last_date);
  // draw date only if year of year day has changed
  if ( force || (t->tm_yday != cur_date.tm_yday || t->tm_year != cur_date.tm_year) ){
    _drawDate(&cur_date);
    _last_date = time;
  }
  dsp->unlock();
  return true;
};

void ClockWidget::_drawTime(tm* t){
  _clear_clk();
  char buff[std::size("hh:mm")];
  
  std::strftime(std::data(buff), std::size(buff), "%R", t);    // "%R" equivalent to "%H:%M", t->tm_sec % 2 ? "%R" : "%H %M" would blink semicolon
  Serial.printf("Time:%s\n", buff);
  // recalculate area for clock and save it to be cleared later
  dsp->getTextBounds(buff, _config.left, _config.top, &_time_block_x, &_time_block_y, &_time_block_w, &_time_block_h);

  if (t->tm_sec % 2){   // draw ':'
    dsp->gfxDrawText(_config.left, _config.top, buff, config.theme.clock, config.theme.background, _config.textsize, &CLK_FONT1);
  } else {
    // let's draw time in parts so that ':' is drawn with background color maintaining same area dimensions
    std::strftime(std::data(buff), std::size(buff), "%H", t);
    dsp->gfxDrawText(_config.left, _config.top, buff, config.theme.clock, config.theme.background, _config.textsize, &CLK_FONT1);
    // write semicolon
    dsp->gfxDrawText(dsp->getCursorX(), dsp->getCursorY(), ":", config.theme.background, config.theme.background, _config.textsize, &CLK_FONT1);
//    dsp->drawChar(dsp->getCursorX(), dsp->getCursorY(), 0x3a /* ':' */, RGB565_RED, RGB565_RED);
    std::strftime(std::data(buff), std::size(buff), "%M", t);
    dsp->gfxDrawText(dsp->getCursorX(), dsp->getCursorY(), buff, config.theme.clock, config.theme.background, _config.textsize, &CLK_FONT1);
  }

  // make seconds
  std::strftime(std::data(buff), std::size(buff), "%S", t);
  dsp->setFont(&CLK_FONT2);
  // recalculate area for clock and save it to be cleared later
  dsp->getTextBounds(buff, dsp->getCursorX() + CLOCK_SECONDS_X_OFFSET, dsp->getCursorY() + CLOCK_SECONDS_Y_OFFSET, &_seconds_block_x, &_seconds_block_y, &_seconds_block_w, &_seconds_block_h);
  // print seconds
  dsp->gfxDrawText(dsp->getCursorX() + CLOCK_SECONDS_X_OFFSET, dsp->getCursorY() + CLOCK_SECONDS_Y_OFFSET, buff, config.theme.clock, config.theme.background, _config.textsize, &CLK_FONT2);
}

void ClockWidget::_drawDate(tm* t){
  _clear_date();
  char buff[40];
  // date format "1 января 2025 / понедельник"
  snprintf(buff, std::size(buff), "%u %s %u / %s", t->tm_mday, mnths[t->tm_mon], t->tm_year + 1900, dowf[t->tm_wday]);
  // recalculate area for clock and save it to be cleared later
  dsp->getTextBounds(buff, _config.left, _config.top, &_date_block_x, &_date_block_y, &_date_block_w, &_date_block_h);
  // draw date with default font
  dsp->gfxDrawText(
    _datecfg->left,
    _datecfg->top,
    buff,
    config.theme.date,
    config.theme.background,
    _datecfg->textsize
  );
}

void ClockWidget::_clear_clk(){
  // Очищаем область под текущими часами
  dsp->gfxFillRect(_time_block_x, _time_block_y, _time_block_w, _time_block_h, config.theme.background);  // RGB565_DARKGREY); // 
  dsp->gfxFillRect(_seconds_block_x, _seconds_block_y, _seconds_block_w, _seconds_block_h, config.theme.background); // RGB565_DARKGREEN); //config.theme.background);
}

void ClockWidget::_clear_date(){
  // Очищаем область под датой
  dsp->gfxFillRect(_date_block_x, _date_block_y, _date_block_w, _date_block_h, config.theme.background);  // RGB565_DARKGREY); // 
}

void BitrateWidget::init(BitrateConfig bconf, uint16_t fgcolor, uint16_t bgcolor){
  Widget::init(bconf.widget, fgcolor, bgcolor);
  _dimension = bconf.dimension;
  _bitrate = 0;
  _format = P_na;
  dsp->charSize(bconf.widget.textsize, _charWidth, _textheight);
  memset(_buf, 0, 6);
}

void BitrateWidget::setBitrate(uint32_t bitrate){
  _bitrate = bitrate;
  if(_bitrate>999) _bitrate=999;
  _draw();
}

void BitrateWidget::setFormat(const char* format){
  _format = format;
  _draw();
}

void BitrateWidget::_draw(){
  _clear();
  if(!_active || _bitrate==0) return;
  dsp->drawRect(_config.left, _config.top, _dimension, _dimension, _fgcolor);
  dsp->fillRect(_config.left, _config.top + _dimension/2, _dimension, _dimension/2, _fgcolor);
  dsp->setFont();
  dsp->setTextSize(_config.textsize);
  dsp->setTextColor(_fgcolor, _bgcolor);
  snprintf(_buf, 6, "%d", _bitrate);
  dsp->setCursor(_config.left + _dimension/2 - _charWidth*strlen(_buf)/2 + 1, _config.top + _dimension/4 - _textheight/2+1);
  dsp->print(_buf);
  dsp->setTextColor(_bgcolor, _fgcolor);
  dsp->setCursor(_config.left + _dimension/2 - _charWidth*3/2 + 1, _config.top + _dimension - _dimension/4 - _textheight/2);
  dsp->print(_format);
}

void BitrateWidget::_clear() {
  dsp->fillRect(_config.left, _config.top, _dimension, _dimension, _bgcolor);
}

#endif // #if DSP_MODEL!=DSP_DUMMY
