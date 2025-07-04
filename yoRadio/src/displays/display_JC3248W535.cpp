#include "../core/options.h"
#if DSP_MODEL==DSP_JC3248W535

#include "display_JC3248W535.h"
#include "fonts/bootlogo.h"
#include "../core/config.h"
//#include "../core/network.h"
#include "tools/utf8RusGFX.h"

static Arduino_DataBus *bus{nullptr};
DspCore* dsp{nullptr};

bool create_display_dev(){
  if (bus == nullptr){
    bus = new Arduino_ESP32QSPI(TFT_CS, TFT_SCK, TFT_SDA0, TFT_SDA1, TFT_SDA2, TFT_SDA3);
  }

  if (bus == nullptr){
    Serial.println("Can't create bus!");
    return false;
  }

  if (dsp == nullptr ){
    dsp = new DspCore(bus);
    Serial.println("create dsp object");
    // backlight
    #ifdef TFT_BLK
      pinMode(TFT_BLK, OUTPUT);
      digitalWrite(TFT_BLK, TFT_BLK_ON_LEVEL);
    #endif
  }
  return dsp != nullptr;
}

DspCore::DspCore(Arduino_DataBus *b): Arduino_AXS15231B(b, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, true /* IPS */, TFT_WIDTH /* width */, TFT_HEIGHT /* height */) {}

void DspCore::initDisplay() {
  begin();

  setTextWrap(false);
  setTextSize(1);
  fillScreen(63492);
  delay(250);
  //fillScreen(0x0000);
  //invert();
  //flip();
  
  plItemHeight = playlistConf.widget.textsize*(CHARHEIGHT-1)+playlistConf.widget.textsize*4;
  plTtemsCount = round((float)height()/plItemHeight);
  if(plTtemsCount%2==0) plTtemsCount++;
  plCurrentPos = plTtemsCount/2;
  plYStart = (height() / 2 - plItemHeight / 2) - plItemHeight * (plTtemsCount - 1) / 2 + playlistConf.widget.textsize*2;
}

void DspCore::drawLogo(uint16_t top) {
  fillRect(0, top, width(), 88, config.theme.background);
  draw16bitRGBBitmap((width() - 99) / 2, top, bootlogo2, 99, 64);
}
//void DspCore::drawLogo(uint16_t top) { drawRGBBitmap((width() - 99) / 2, top, bootlogo2, 99, 64); }

void DspCore::printPLitem(uint8_t pos, const char* item, ScrollWidget& current){
  if (pos == plCurrentPos) {
    current.setText(item);
  } else {
    uint8_t plColor = (abs(pos - plCurrentPos)-1)>4?4:abs(pos - plCurrentPos)-1;
    fillRect(0, plYStart + pos * plItemHeight - 1, width(), plItemHeight - 2, config.theme.background);
    // Обрезка строки по ширине без троеточия
    const char* rus = utf8Rus(item, true);
    int len = strlen(rus);
    char buf[128];
    int maxWidth = playlistConf.width;
    uint8_t textsize = playlistConf.widget.textsize;
    if (textWidthGFX(rus, textsize) <= maxWidth) {
      strncpy(buf, rus, sizeof(buf)-1);
      buf[sizeof(buf)-1] = 0;
    } else {
      int cut = len;
      while (cut > 0) {
        char tmp[128];
        strncpy(tmp, rus, cut);
        tmp[cut] = 0;
        if (textWidthGFX(tmp, textsize) <= maxWidth) break;
        cut--;
      }
      strncpy(buf, rus, cut);
      buf[cut] = 0;
    }
    gfxDrawText(
      gfx,
      TFT_FRAMEWDT,
      plYStart + pos * plItemHeight,
      buf,
      config.theme.playlist[plColor],
      config.theme.background,
      textsize
    );
  }
}

void DspCore::drawPlaylist(uint16_t currentItem) {
  uint8_t lastPos = config.fillPlMenu(currentItem - plCurrentPos, plTtemsCount);
  if(lastPos<plTtemsCount){
    fillRect(0, lastPos*plItemHeight+plYStart, width(), height()/2, config.theme.background);
  }
}

void DspCore::clearDsp(bool black) { fillScreen(black?0:config.theme.background); }

uint8_t DspCore::_charWidth(unsigned char c){
  GFXglyph *glyph = pgm_read_glyph_ptr(&DS_DIGI56pt7b, c - 0x20);
  return pgm_read_byte(&glyph->xAdvance);
}

void DspCore::_clockSeconds(){
  setTextSize(4);
  setTextColor(config.theme.seconds, config.theme.background);
  setCursor(width() - 8 - clockRightSpace - CHARWIDTH*4*2, clockTop-clockTimeHeight+1);
  sprintf(_bufforseconds, "%02d", network.timeinfo.tm_sec);
  if(!config.isScreensaver) print(_bufforseconds);                                      /* print seconds */
  setTextSize(1);
  setFont(&DS_DIGI56pt7b);
  setTextColor((network.timeinfo.tm_sec % 2 == 0) ? config.theme.clock : (CLOCKFONT_MONO?config.theme.clockbg:config.theme.background), config.theme.background);
  setCursor(_timeleft+_dotsLeft, clockTop);
  print(":");                                     /* print dots */
  setFont();
}

void DspCore::_clockDate(){
  if(_olddateleft>0)
    fillRect(_olddateleft,  clockTop+14, _olddatewidth, CHARHEIGHT*2, config.theme.background);

  setTextColor(config.theme.date, config.theme.background);
  setCursor(_dateleft, clockTop+15);
  setTextSize(2);
  if(!config.isScreensaver) print(_dateBuf);                                            /* print date */
  strlcpy(_oldDateBuf, _dateBuf, sizeof(_dateBuf));
  _olddatewidth = _datewidth;
  _olddateleft = _dateleft;
  setTextSize(4);
  setTextColor(config.theme.dow, config.theme.background);
  setCursor(width() - 8 - clockRightSpace - CHARWIDTH*4*2, clockTop-CHARHEIGHT*4+4);
  print(utf8Rus(dow[network.timeinfo.tm_wday], false));       /* print dow */
}

void DspCore::_clockTime(){
  if(_oldtimeleft>0 && !CLOCKFONT_MONO)
    fillRect(_oldtimeleft, clockTop-clockTimeHeight+1, _oldtimewidth, clockTimeHeight, config.theme.background);

  _timeleft = width()-clockRightSpace-CHARWIDTH*4*2-24-_timewidth;
  setTextSize(1);
  setFont(&DS_DIGI56pt7b);
  
  if(CLOCKFONT_MONO) {
    setCursor(_timeleft, clockTop);
    setTextColor(config.theme.clockbg, config.theme.background);
    print("88:88");
  }
  setTextColor(config.theme.clock, config.theme.background);
  setCursor(_timeleft, clockTop);
  print(_timeBuf);
  setFont();
  strlcpy(_oldTimeBuf, _timeBuf, sizeof(_timeBuf));
  _oldtimewidth = _timewidth;
  _oldtimeleft = _timeleft;
  if(!config.isScreensaver) drawFastVLine(width()-clockRightSpace-CHARWIDTH*4*2-18, clockTop-clockTimeHeight, clockTimeHeight+4, config.theme.div);  /*divider vert*/
  if(!config.isScreensaver) drawFastHLine(width()-clockRightSpace-CHARWIDTH*4*2-18, clockTop-clockTimeHeight+37, 59, config.theme.div);              /*divider hor*/
  sprintf(_buffordate, "%2d %s %d", network.timeinfo.tm_mday,mnths[network.timeinfo.tm_mon], network.timeinfo.tm_year+1900);
  strlcpy(_dateBuf, utf8Rus(_buffordate, true), sizeof(_dateBuf));
  _datewidth = strlen(_dateBuf) * CHARWIDTH*2;
  _dateleft = width() - 10 - clockRightSpace - _datewidth;
}

void DspCore::printClock(uint16_t top, uint16_t rightspace, uint16_t timeheight, bool redraw){
  clockTop = top;
  clockRightSpace = rightspace;
  clockTimeHeight = timeheight;
  strftime(_timeBuf, sizeof(_timeBuf), "%H:%M", &network.timeinfo);
  if(strcmp(_oldTimeBuf, _timeBuf)!=0 || redraw){
    _getTimeBounds();
    _clockTime();
    if(!config.isScreensaver)
      if(strcmp(_oldDateBuf, _dateBuf)!=0 || redraw) _clockDate();
  }
  _clockSeconds();
}

void DspCore::clearClock(){
  fillRect(_timeleft,  clockTop-clockTimeHeight, _timewidth+CHARWIDTH*3*2+24, clockTimeHeight+12+CHARHEIGHT, config.theme.background);
}

void DspCore::startWrite(void) {
  Arduino_AXS15231B::startWrite();
}

void DspCore::endWrite(void) { 
  Arduino_AXS15231B::endWrite();
}
  
void DspCore::loop(bool force) {

}

void DspCore::charSize(uint8_t textsize, uint8_t& width, uint16_t& height){
  width = textsize * CHARWIDTH;
  height = textsize * CHARHEIGHT;
}
/*
void DspCore::setTextSize(uint8_t s){
  Arduino_AXS15231B::setTextSize(s);
}
*/
void DspCore::flip(){
  setRotation(config.store.flipscreen?3:1);
}

void DspCore::invert(){
  invertDisplay(config.store.invertdisplay);
}

void DspCore::sleep(void) { 
  //sendCommand(ILI9488_SLPIN); delay(150); sendCommand(ILI9488_DISPOFF); delay(150);
}

void DspCore::wake(void) { 
  //sendCommand(ILI9488_DISPON); delay(150); sendCommand(ILI9488_SLPOUT); delay(150);
}

void DspCore::writePixel(int16_t x, int16_t y, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x > _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height)) return;
  }
  Arduino_AXS15231B::drawPixel(x, y, color);
}

void DspCore::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x >= _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height))  return;
  }
  Arduino_AXS15231B::writeFillRect(x, y, w, h, color);
}

void DspCore::setClipping(clipArea ca){
  _cliparea = ca;
  _clipping = true;
}

void DspCore::clearClipping(){
  _clipping = false;
}

void DspCore::setNumFont(){
  setFont(&DS_DIGI56pt7b);
  setTextSize(1);
}

// Функция для вычисления ширины строки для стандартного шрифта Adafruit_GFX
uint16_t DspCore::textWidthGFX(const char *txt, uint8_t textsize) {
  return strlen(txt) * CHARWIDTH * textsize;
}

#endif
