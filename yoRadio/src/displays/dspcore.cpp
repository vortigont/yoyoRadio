#include "nvs_handle.hpp"
#include "dspcore.h"
#include "core/const_strings.h"
#include "gfx_engine.h"
#include "locale/l10n.h"
#include "core/config.h"
#include "core/evtloop.h"
#include "core/log.h"


#ifdef NOT_NEEDED

char* DspCoreBase::utf8Rus(const char* str, bool uppercase) {
    int index = 0;
    static char strn[BUFLEN];
    bool E = false;
    strlcpy(strn, str, BUFLEN);
    if (uppercase) {
      bool next = false;
      for (char *iter = strn; *iter != '\0'; ++iter)
      {
        if (E) {
          E = false;
          continue;
        }
        uint8_t rus = (uint8_t) * iter;
        if (rus == 208 && (uint8_t) * (iter + 1) == 129) { // ёКостыли
          *iter = (char)209;
          *(iter + 1) = (char)145;
          E = true;
          continue;
        }
        if (rus == 209 && (uint8_t) * (iter + 1) == 145) {
          *iter = (char)209;
          *(iter + 1) = (char)145;
          E = true;
          continue;
        }
        if (next) {
          if (rus >= 128 && rus <= 143) *iter = (char)(rus + 32);
          if (rus >= 176 && rus <= 191) *iter = (char)(rus - 32);
          next = false;
        }
        if (rus == 208) next = true;
        if (rus == 209) {
          *iter = (char)208;
          next = true;
        }
        *iter = toupper(*iter);
      }
    }
    if(L10N_LANGUAGE==EN) return strn;
    while (strn[index])
    {
      if (strn[index] >= 0xBF)
      {
        switch (strn[index]) {
          case 0xD0: {
              if (strn[index + 1] == 0x81) {
                strn[index] = 0xA8;
                break;
              }
              if (strn[index + 1] >= 0x90 && strn[index + 1] <= 0xBF) strn[index] = strn[index + 1] + 0x30;
              break;
            }
          case 0xD1: {
              if (strn[index + 1] == 0x91) {
                //strn[index] = 0xB7;
                strn[index] = 0xB8;
                break;
              }
              if (strn[index + 1] >= 0x80 && strn[index + 1] <= 0x8F) strn[index] = strn[index + 1] + 0x70;
              break;
            }
        }
        int sind = index + 2;
        while (strn[sind]) {
          strn[sind - 1] = strn[sind];
          sind++;
        }
        strn[sind - 1] = 0;
      }
      index++;
    }
    return strn;
}

void DspCoreBase::charSize(uint8_t textsize, uint16_t& width, uint16_t& height){
  width = textsize * CHARWIDTH;
  height = textsize * CHARHEIGHT;
}

void DspCoreBase::setClipping(clipArea ca){
  _cliparea = ca;
  _clipping = true;
}

uint16_t DspCoreBase::textWidth(const char *txt){
  uint16_t w = 0, l=strlen(txt);
  for(uint16_t c=0;c<l;c++) w+=_charWidth(txt[c]);
  return w;
}

// Функция для вычисления ширины строки для стандартного шрифта Adafruit_GFX
uint16_t DspCoreBase::textWidthGFX(const char *txt, uint8_t textsize) {
  return strlen(txt) * CHARWIDTH * textsize;
}



/////////////////////
//DspCore_Arduino_GFX - extension methods
#ifdef _ARDUINO_GFX_H_

void DspCore_Arduino_GFX::gfxDrawText(int x, int y, const char* text, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font) {
  if (font)
    setFont(font);
  else {
#if defined FONT_DEFAULT_AGFX
    setFont(&FONT_DEFAULT_AGFX);
#elif defined  FONT_DEFAULT_U8G2
    setFont(FONT_DEFAULT_U8G2);
#endif  //  FONT_DEFAULT_U8G2
  }
  setTextColor(color, bgcolor);
  setTextSize(size);
  setCursor(x, y);
  print(text);
  //print(utf8Rus(text, true));
}

void DspCore_Arduino_GFX::gfxDrawNumber(int x, int y, int num, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", num);
  gfxDrawText(x, y, buf, color, bgcolor, size, font);
}

void DspCore_Arduino_GFX::gfxDrawFormatted(int x, int y, const char* fmt, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font, ...) {
  char buf[64];
  va_list args;
  va_start(args, font);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  gfxDrawText(x, y, buf, color, bgcolor, size, font);
}

void DspCore_Arduino_GFX::setFont(const GFXfont* font){
  if (font)
    Arduino_GFX::setFont(font);
#if defined FONT_DEFAULT_AGFX
  else
    Arduino_GFX::setFont(&FONT_DEFAULT_AGFX);
#elif defined FONT_DEFAULT_U8G2
  else {
    Arduino_GFX::setFont(FONT_DEFAULT_U8G2);
  }
#endif
}

void DspCore_Arduino_GFX::setFont(const uint8_t* font){
  if (font)
    Arduino_GFX::setFont(font);
#if defined FONT_DEFAULT_AGFX
  else
    Arduino_GFX::setFont(&FONT_DEFAULT_AGFX);
#elif defined FONT_DEFAULT_U8G2
  else {
    Arduino_GFX::setFont(FONT_DEFAULT_U8G2);
  }
#endif
}
#endif // NOT_NEEDED



#endif  // _ARDUINO_GFX_H_


// some old legacy from AudioEx.h
#ifndef AUDIOBUFFER_MULTIPLIER2                                                                                                                               
#define AUDIOBUFFER_MULTIPLIER2    8                                                                                                                          
#endif

//============================================================================================================================

#ifndef DSQ_SEND_DELAY
  #define DSQ_SEND_DELAY portMAX_DELAY
#endif

#define DISPLAY_GFX_TASK_PRIO        3
#ifndef CORE_STACK_SIZE
  #define CORE_STACK_SIZE       1024*3
#endif
#ifndef DSP_TASK_DELAY
  #define DSP_TASK_DELAY  pdMS_TO_TICKS(20)   // cap for 50 fps
#endif
// will use DSP_QUEUE_TICKS as delay interval for display task runner when there are no msgs in a queue to process
#define DSP_QUEUE_TICKS DSP_TASK_DELAY

#if !((DSP_MODEL==DSP_ST7735 && DTYPE==INITR_BLACKTAB) || DSP_MODEL==DSP_ST7789 || DSP_MODEL==DSP_ST7796 || DSP_MODEL==DSP_ILI9488 || DSP_MODEL==DSP_ILI9486 || DSP_MODEL==DSP_ILI9341 || DSP_MODEL==DSP_ILI9225)
  #undef  BITRATE_FULL
  #define BITRATE_FULL     false
#endif

void returnPlayer(){
  display->putRequest(NEWMODE, PLAYER);
}

DisplayGFX::~DisplayGFX(){
  _events_unsubsribe();
}

void DisplayGFX::init() {
  LOGI(T_BOOT, println, "display->init");
#if LIGHT_SENSOR!=255
  analogSetAttenuation(ADC_0db);
#endif
  _state = state_t::empty;
  if (_gfx->begin()){
    _gfx->fillScreen(0);
    _gfx->setUTF8Print(true);
  } else {
    LOGE(T_BOOT, println, "DisplayGFX.init FAILED!");
    return;
  }

  _displayQueue = xQueueCreate( 5, sizeof( requestParams_t ) );
  if(_displayQueue == nullptr){
    LOGE(T_Display, println, "Can't create msg queue");
  }

  //_pager.begin();
  _bootScreen();

  // create runner task
  xTaskCreatePinnedToCore(
    [](void* self){ static_cast<DisplayGFX*>(self)->_loopDspTask(); },
    "DspTask",
    CORE_STACK_SIZE,
    static_cast<void*>(this),
    DISPLAY_GFX_TASK_PRIO,
    &_dspTask,
    CONFIG_ARDUINO_RUNNING_CORE);

  _events_subsribe();
  // load main page
  putRequest(DSP_START);
}

void DisplayGFX::_bootScreen(){
  _state = state_t::bootlogo;
  
  // Очищаем экран перед отображением boot page
  /*  
  dsp->clearDsp(false); // false = цвет фона
  dsp->loop(true); // Принудительное обновление
  _boot = new Page();
  _boot->addWidget(new ProgressWidget(bootWdtConf, bootPrgConf, BOOT_PRG_COLOR, 0));
  _bootstring = (TextWidget*) &_boot->addWidget(new TextWidget(bootstrConf, 50, true, BOOT_TXT_COLOR, 0));
  _pager.addPage(_boot);
  _pager.setPage(_boot, true);
  dsp->drawLogo(bootLogoTop);
  */
}

/*
void DisplayGFX::_buildPager(){
  _meta.init("*", metaConf, config.theme.meta, config.theme.metabg);
  _title1.init("*", title1Conf, config.theme.title1, config.theme.background);
  _clock.init(clockConf, 0, 0);
  _clock._datecfg = &dateConf;    // dirty hack with date config
  #if DSP_MODEL==DSP_NOKIA5110
    _plcurrent.init("*", playlistConf, 0, 1);
  #else
    _plcurrent.init("*", playlistConf, config.theme.plcurrent, config.theme.plcurrentbg);
  #endif
  #if !defined(DSP_LCD)
    _plcurrent.moveTo({TFT_FRAMEWDT, (uint16_t)(dsp->plYStart+dsp->plCurrentPos*dsp->plItemHeight), (int16_t)playlistConf.width});
  #endif
  #ifndef HIDE_TITLE2
    _title2 = new ScrollWidget("*", title2Conf, config.theme.title2, config.theme.background);
  #endif
  #if !defined(DSP_LCD) && DSP_MODEL!=DSP_NOKIA5110
    _plbackground = new FillWidget(playlBGConf, config.theme.plcurrentfill);
    #if DSP_INVERT_TITLE || defined(DSP_OLED)
      _metabackground = new FillWidget(metaBGConf, config.theme.metafill);
    #else
      _metabackground = new FillWidget(metaBGConfInv, config.theme.metafill);
    #endif
  #endif
  #if DSP_MODEL==DSP_NOKIA5110
    _plbackground = new FillWidget(playlBGConf, 1);
    //_metabackground = new FillWidget(metaBGConf, 1);
  #endif
  #ifndef HIDE_VU
    _vuwidget = new VuWidget(vuConf, bandsConf, config.theme.vumax, config.theme.vumin, config.theme.background);
  #endif
  #ifndef HIDE_VOLBAR
    _volbar = new SliderWidget(volbarConf, config.theme.volbarin, config.theme.background, 254, config.theme.volbarout);
  #endif
  #ifndef HIDE_HEAPBAR
    _heapbar = new SliderWidget(heapbarConf, config.theme.buffer, config.theme.background, psramInit()?300000:1600 * AUDIOBUFFER_MULTIPLIER2);
  #endif
  #ifndef HIDE_VOL
    _voltxt = new TextWidget(voltxtConf, 10, false, config.theme.vol, config.theme.background);
  #endif
  #ifndef HIDE_IP
    _volip = new TextWidget(iptxtConf, 30, false, config.theme.ip, config.theme.background);
  #endif
  #ifndef HIDE_RSSI
    _rssi = new TextWidget(rssiConf, 20, false, config.theme.rssi, config.theme.background);
  #endif
  _nums.init(numConf, 10, false, config.theme.digit, config.theme.background);
  #ifndef HIDE_WEATHER
    _weather = new ScrollWidget("\007", weatherConf, config.theme.weather, config.theme.background);
  #endif
  
  if(_volbar)   _footer.addWidget( _volbar);
  if(_voltxt)   _footer.addWidget( _voltxt);
  if(_volip)    _footer.addWidget( _volip);
  if(_rssi)     _footer.addWidget( _rssi);
  if(_heapbar)  _footer.addWidget( _heapbar);
  
  if(_metabackground) _pages.at(PG_PLAYER)->addWidget( _metabackground);
  _pages.at(PG_PLAYER)->addWidget(&_meta);
  _pages.at(PG_PLAYER)->addWidget(&_title1);
  if(_title2) _pages.at(PG_PLAYER)->addWidget(_title2);
  if(_weather) _pages.at(PG_PLAYER)->addWidget(_weather);
  #if BITRATE_FULL
    _fullbitrate = new BitrateWidget(fullbitrateConf, config.theme.bitrate, config.theme.background);
    _pages.at(PG_PLAYER)->addWidget( _fullbitrate);
  #else
    _bitrate = new TextWidget(bitrateConf, 30, false, config.theme.bitrate, config.theme.background);
    _pages.at(PG_PLAYER)->addWidget( _bitrate);
  #endif
  if(_vuwidget) _pages.at(PG_PLAYER)->addWidget( _vuwidget);
  _pages.at(PG_PLAYER)->addWidget(&_clock);
  _pages.at(PG_SCREENSAVER)->addWidget(&_clock);
  _pages.at(PG_PLAYER)->addPage(&_footer);

  if(_metabackground) _pages.at(PG_DIALOG)->addWidget( _metabackground);
  _pages.at(PG_DIALOG)->addWidget(&_meta);
  _pages.at(PG_DIALOG)->addWidget(&_nums);
  
  #if !defined(DSP_LCD) && DSP_MODEL!=DSP_NOKIA5110
    _pages.at(PG_DIALOG)->addPage(&_footer);
  #endif
  #if !defined(DSP_LCD)
  if(_plbackground) {
    _pages.at(PG_PLAYLIST)->addWidget( _plbackground);
    _plbackground->setHeight(dsp->plItemHeight);
    _plbackground->moveTo({0,(uint16_t)(dsp->plYStart+dsp->plCurrentPos*dsp->plItemHeight-playlistConf.widget.textsize*2), (int16_t)playlBGConf.width});
  }
  #endif
  _pages.at(PG_PLAYLIST)->addWidget(&_plcurrent);

  for(const auto& p: _pages) _pager.addPage(p);
}
*/

void DisplayGFX::_apScreen() {
/*
  if(_boot) _pager.removePage(_boot);
  #ifndef DSP_LCD
    _boot = new Page();
    #if DSP_MODEL!=DSP_NOKIA5110
      #if DSP_INVERT_TITLE || defined(DSP_OLED)
      _boot->addWidget(new FillWidget(metaBGConf, config.theme.metafill));
      #else
      _boot->addWidget(new FillWidget(metaBGConfInv, config.theme.metafill));
      #endif
    #endif
    ScrollWidget *bootTitle = (ScrollWidget*) &_boot->addWidget(new ScrollWidget("*", apTitleConf, config.theme.meta, config.theme.metabg));
    bootTitle->setText("ёRadio AP Mode");
    TextWidget *apname = (TextWidget*) &_boot->addWidget(new TextWidget(apNameConf, 30, false, config.theme.title1, config.theme.background));
    apname->setText(apNameTxt);
    TextWidget *apname2 = (TextWidget*) &_boot->addWidget(new TextWidget(apName2Conf, 30, false, config.theme.clock, config.theme.background));
    apname2->setText(apSsid);
    TextWidget *appass = (TextWidget*) &_boot->addWidget(new TextWidget(apPassConf, 30, false, config.theme.title1, config.theme.background));
    appass->setText(apPassTxt);
    TextWidget *appass2 = (TextWidget*) &_boot->addWidget(new TextWidget(apPass2Conf, 30, false, config.theme.clock, config.theme.background));
    appass2->setText(apPassword);
    ScrollWidget *bootSett = (ScrollWidget*) &_boot->addWidget(new ScrollWidget("*", apSettConf, config.theme.title2, config.theme.background));
    bootSett->setText(WiFi.softAPIP().toString().c_str(), apSettFmt);
    _pager.addPage(_boot);
    _pager.setPage(_boot);
  #else
    dsp->apScreen();
  #endif
*/
}

void DisplayGFX::_start() {
  _mpp.clear();
  _build_main_screen();
  _mode = PLAYER;
  _state = state_t::normal;
  LOGD(T_Display, println, "DisplayGFX::_started");
}

void DisplayGFX::_showDialog(const char *title){
/*
  dsp->setScrollId(NULL);
  _pager.setPage( _pages.at(PG_DIALOG));
  #ifdef META_MOVE
    _meta.moveTo(metaMove);
  #endif
  _meta.setAlign(WA_CENTER);
  _meta.setText(title);
*/
}

void DisplayGFX::_setReturnTicker(uint8_t time_s){
  _returnTicker.detach();
  _returnTicker.once(time_s, returnPlayer);
}

void DisplayGFX::_swichMode(displayMode_e newmode) {
/*
  if (newmode == _mode || (network.status != CONNECTED && network.status != SDREADY)) return;
  _mode = newmode;
  dsp->setScrollId(NULL);

  // enable / disable clock widget
  _clock.setActive(newmode == PLAYER || newmode == SCREENSAVER);
  
  if (newmode == PLAYER) {
    if(player->isRunning())
      _clock.moveTo(clockMove);
    else
      _clock.moveBack();
    #ifdef DSP_LCD
      dsp->clearDsp();
    #endif
    numOfNextStation = 0;
    _returnTicker.detach();
    #ifdef META_MOVE
      _meta.moveBack();
    #endif
    _meta.setAlign(metaConf.widget.align);
    _meta.setText(config.station.name);
    _nums.setText("");
    config.isScreensaver = false;
    _pager.setPage( _pages.at(PG_PLAYER));
    config.setDspOn(config.store.dspon, false);
  }
  if (newmode == SCREENSAVER || newmode == SCREENBLANK) {
    config.isScreensaver = true;
    _pager.setPage( _pages.at(PG_SCREENSAVER));
    if (newmode == SCREENBLANK) {
      //dsp->clearClock();  TODO
      config.setDspOn(false, false);
    }
  }else{
    config.screensaverTicks=SCREENSAVERSTARTUPDELAY;
    config.screensaverPlayingTicks=SCREENSAVERSTARTUPDELAY;
    config.isScreensaver = false;
  }
  if (newmode == VOL) {
    #ifndef HIDE_IP
      _showDialog(const_DlgVolume);
    #else
      _showDialog(WiFi.localIP().toString().c_str());
    #endif
    _nums.setText(config.store.volume, numtxtFmt);
  }
  if (newmode == LOST)      _showDialog(const_DlgLost);
  if (newmode == UPDATING)  _showDialog(const_DlgUpdate);
  if (newmode == SLEEPING)  _showDialog("SLEEPING");
  if (newmode == SDCHANGE)  _showDialog(const_waitForSD);
  if (newmode == INFO || newmode == SETTINGS || newmode == TIMEZONE || newmode == WIFI) _showDialog(const_DlgNextion);
  if (newmode == NUMBERS) _showDialog("");
  if (newmode == STATIONS) {
    _pager.setPage( _pages.at(PG_PLAYLIST));
    _plcurrent.setText("");
    currentPlItem = config.lastStation();
    _drawPlaylist();
  }
*/  
}

void DisplayGFX::resetQueue(){
  if(_displayQueue!=NULL) xQueueReset(_displayQueue);
}

void DisplayGFX::_drawPlaylist() {
//  dsp->drawPlaylist(currentPlItem);
  _setReturnTicker(30);
}

void DisplayGFX::_drawNextStationNum(uint16_t num) {
  _setReturnTicker(30);
  //_meta.setText(config.stationByNum(num));
  //_nums.setText(num, "%d");
}

//void DisplayGFX::printPLitem(uint8_t pos, const char* item){
  //dsp->printPLitem(pos, item, _plcurrent);
//}

void DisplayGFX::putRequest(displayRequestType_e type, int payload){
  if(_displayQueue==NULL) return;
  requestParams_t request;
  request.type = type;
  request.payload = payload;
  xQueueSend(_displayQueue, &request, DSQ_SEND_DELAY);
}

void DisplayGFX::_layoutChange(bool played){
/*
  if(config.store.vumeter){
    if(played){
      if(_vuwidget) _vuwidget->unlock();
      _clock.moveTo(clockMove);
      if(_weather) _weather->moveTo(weatherMoveVU);
    }else{
      if(_vuwidget) if(!_vuwidget->locked()) _vuwidget->lock();
      _clock.moveBack();
      if(_weather) _weather->moveBack();
    }
  }else{
    if(played){
      if(_weather) _weather->moveTo(weatherMove);
      _clock.moveBack();
    }else{
      if(_weather) _weather->moveBack();
      _clock.moveBack();
    }
  }
*/
}

void DisplayGFX::_loopDspTask() {
  while(true){
    requestParams_t request;
    if(xQueueReceive(_displayQueue, &request, DSP_QUEUE_TICKS)){
      bool pm_result = true;
      if(pm_result)
        switch (request.type){
          case NEWMODE: _swichMode((displayMode_e)request.payload); break;
          case NEWTITLE: _title(); break;
          case NEWSTATION: _station(); break;
          case NEXTSTATION: _drawNextStationNum(request.payload); break;
          case DRAWPLAYLIST: _drawPlaylist(); break;
          case DRAWVOL: _volume(); break;
          case AUDIOINFO:
            //if(_heapbar)  { _heapbar->lock(!config.store.audioinfo); _heapbar->setValue(player->inBufferFilled()); }
            break;
          case SHOWVUMETER: {
            /*
            if(_vuwidget){
              _vuwidget->lock(!config.store.vumeter); 
              _layoutChange(player->isRunning());
            }
            */
          }
          break;
/*
          case SHOWWEATHER: {
            if(_weather) _weather->lock(!config.store.showweather);
            if(!config.store.showweather){
              #ifndef HIDE_IP
              if(_volip) _volip->setText(WiFi.localIP().toString().c_str(), iptxtFmt);
              #endif
            }else{
              if(_weather) _weather->setText(const_getWeather);
            }
            break;
          }
          case NEWWEATHER: {
            if(_weather && network.weatherBuf) _weather->setText(network.weatherBuf);
            break;
          }
          case BOOTSTRING: {
            if(_bootstring) _bootstring->setText(config.ssids[request.payload].ssid, bootstrFmt);
            break;
          }
          case WAITFORSD: {
            if(_bootstring) _bootstring->setText(const_waitForSD);
            break;
          }
          case SDFILEINDEX: {
            if(_mode == SDCHANGE) _nums.setText(request.payload, "%d");
            break;
          }
*/
          //case DSPRSSI: if(_rssi){ _setRSSI(request.payload); } if (_heapbar && config.store.audioinfo) _heapbar->setValue(player->isRunning()?player->inBufferFilled():0); break;
          case PSTART: _layoutChange(true);   break;
          case PSTOP:  _layoutChange(false);  break;
          case DSP_START: _start();  break;

          default: break;
        }

      // check if there are more messages waiting in the Q, in this case break the loop() and go
      // for another round to evict next message, do not waste time to redraw the screen, etc...
      if (uxQueueMessagesWaiting(_displayQueue))
        continue;
    }

    // refresh screen items if needed
    if (_mpp.refresh(_gfx))
      _gfx->flush();
  
    #if I2S_DOUT==255
    player.computeVUlevel();
    #endif
  }
  vTaskDelete( NULL );
  _dspTask=NULL;
}

void DisplayGFX::_setRSSI(int rssi) {
/*
  if(!_rssi) return;
#if RSSI_DIGIT
  _rssi->setText(rssi, rssiFmt);
  return;
#endif
  char rssiG[3];
  int rssi_steps[] = {RSSI_STEPS};
  if(rssi >= rssi_steps[0]) strlcpy(rssiG, "\004\006", 3);
  if(rssi >= rssi_steps[1] && rssi < rssi_steps[0]) strlcpy(rssiG, "\004\005", 3);
  if(rssi >= rssi_steps[2] && rssi < rssi_steps[1]) strlcpy(rssiG, "\004\002", 3);
  if(rssi >= rssi_steps[3] && rssi < rssi_steps[2]) strlcpy(rssiG, "\003\002", 3);
  if(rssi <  rssi_steps[3] || rssi >=  0) strlcpy(rssiG, "\001\002", 3);
  _rssi->setText(rssiG);
*/
}

void DisplayGFX::_station() {
  //_meta.setAlign(metaConf.widget.align);
  //_meta.setText("АБВГД-еёжзиклм_123");
  //_meta.setText(config.station.name);
}

char *split(char *str, const char *delim) {
  char *dmp = strstr(str, delim);
  if (dmp == NULL) return NULL;
  *dmp = '\0'; 
  return dmp + strlen(delim);
}

void DisplayGFX::_title() {
/*
  if (strlen(config.station.title) > 0) {
    char tmpbuf[strlen(config.station.title)+1];
    strlcpy(tmpbuf, config.station.title, strlen(config.station.title)+1);
    char *stitle = split(tmpbuf, " - ");
    if(stitle && _title2){
      _title1.setText(tmpbuf);
      _title2->setText(stitle);
    }else{
      _title1.setText(config.station.title);
      if(_title2) _title2->setText("");
    }
    
  } else {
    _title1.setText("");
    if(_title2) _title2->setText("");
  }
*/
}

void DisplayGFX::_volume() {
/*
  if(_volbar) _volbar->setValue(config.store.volume);
  #ifndef HIDE_VOL
    if(_voltxt) _voltxt->setText(config.store.volume, voltxtFmt);
  #endif
  if(_mode==VOL) {
    _setReturnTicker(3);
    _nums.setText(config.store.volume, numtxtFmt);
  }
*/
}

/*
void  DisplayGFX::setContrast(){
  #if DSP_MODEL==DSP_NOKIA5110
    dsp->setContrast(config.store.contrast);
  #endif
}

bool DisplayGFX::deepsleep(){
#if defined(LCD_I2C) || defined(DSP_OLED) || BRIGHTNESS_PIN!=255
  dsp->sleep();
  return true;
#endif
  return false;
}

void DisplayGFX::wakeup(){
#if defined(LCD_I2C) || defined(DSP_OLED) || BRIGHTNESS_PIN!=255
  dsp->wake();
#endif
}
*/
void DisplayGFX::_events_subsribe(){
  // command events
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_CMD_EVENTS, ESP_EVENT_ANY_ID,
    [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<DisplayGFX*>(self)->_events_cmd_hndlr(id, data); },
    this, &_hdlr_cmd_evt
  );

  // state change events
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_CHG_STATE_EVENTS, ESP_EVENT_ANY_ID,
    [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<DisplayGFX*>(self)->_events_chg_hndlr(id, data); },
    this, &_hdlr_chg_evt
  );
}

void DisplayGFX::_events_unsubsribe(){
  esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_CMD_EVENTS, ESP_EVENT_ANY_ID, _hdlr_cmd_evt);
  esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_CHG_STATE_EVENTS, ESP_EVENT_ANY_ID, _hdlr_chg_evt);
}

void DisplayGFX::_events_cmd_hndlr(int32_t id, void* data){
  LOGV(T_Display, printf, "cmd event rcv:%d\n", id);
  switch (static_cast<evt::yo_event_t>(id)){

    // Play radio station from a playlist
    case evt::yo_event_t::displayNewMode :
      _swichMode(*reinterpret_cast<displayMode_e*>(data));
      break;
/*
    case evt::yo_event_t::displayClock :
      //if(_mode==PLAYER || _mode==SCREENSAVER) _time();
      if(_mode==PLAYER || _mode==SCREENSAVER) display->putRequest(CLOCK);
      break;
*/
    case evt::yo_event_t::displayNewTitle :
      _title();
      break;

    case evt::yo_event_t::displayNewStation :
      _station();
      break;

    case evt::yo_event_t::displayNextStation :
      _drawNextStationNum(*reinterpret_cast<int32_t*>(data));
      break;

    case evt::yo_event_t::displayDrawPlaylist :
      _drawPlaylist();
      break;

    case evt::yo_event_t::displayDrawVol :
      _volume();
      break;

    case evt::yo_event_t::displayShowVUMeter :
/*
      if(_vuwidget){
        _vuwidget->lock(!config.store.vumeter); 
        _layoutChange(player->isRunning());
      }
*/
    break;

    case evt::yo_event_t::displayShowWeather : /*{
      if(_weather)
        _weather->lock(!config.store.showweather);
      if(!config.store.showweather){
        #ifndef HIDE_IP
        if(_volip) _volip->setText(WiFi.localIP().toString().c_str(), iptxtFmt);
        #endif
      } else {
        if(_weather) _weather->setText(const_getWeather);
      }
    }*/
    break;
/*
    case evt::yo_event_t::displayNewWeather :
      if(_weather && network.weatherBuf)
        _weather->setText(network.weatherBuf);
      break;

    case evt::yo_event_t::displayBootstring : {
      auto idx = *reinterpret_cast<int32_t*>(data);
      if (idx >= sizeof(config.ssids)/sizeof(neworkItem) || idx < 0)
        return;
      if(_bootstring)
        _bootstring->setText(config.ssids[idx].ssid, bootstrFmt);
      break;
    }

    case evt::yo_event_t::displayWait4SD :
      if(_bootstring)
        _bootstring->setText(const_waitForSD);
      break;

    case evt::yo_event_t::displaySDFileIndex :
      if (_mode == SDCHANGE)
        _nums.setText(*reinterpret_cast<int32_t*>(data), "%d");
      break;

    case evt::yo_event_t::displayShowRSSI :
      if(_rssi)
        { _setRSSI(*reinterpret_cast<int32_t*>(data)); }
      if (_heapbar && config.store.audioinfo)
        _heapbar->setValue(player->isRunning() ? player->inBufferFilled() : 0);
      break;
*/
    case evt::yo_event_t::displayPStart :
      _layoutChange(true);
      break;

    case evt::yo_event_t::displayPStop :
      _layoutChange(false);
      break;

    case evt::yo_event_t::displayStart :
      _start();
      break;

    #ifndef HIDE_IP
    case evt::yo_event_t::displayNewIP :
      //if(_volip)
      //  _volip->setText(WiFi.localIP().toString().c_str(), iptxtFmt);
      break;
    #endif

    default:;
  }
}

// notifications events
void DisplayGFX::_events_chg_hndlr(int32_t id, void* data){
  LOGV(T_Display, printf, "chg event rcv:%d\n", id);

  switch (static_cast<evt::yo_event_t>(id)){

    // process metadata about playing codec
    case evt::yo_event_t::playerAudioInfo : {
      audio_info_t* i = reinterpret_cast<audio_info_t*>(data);
      char buf[20];
      snprintf(buf, 20, bitrateFmt, i->codecName);
      break;
    }

    // device mode change - update "title_status" widget
    case evt::yo_event_t::devMode : {
      int32_t v = *static_cast<int32_t*>(data);
      if (_title_status && v >= 0 && v < device_state_literal.size()){
        _title_status->setName(device_state_literal.at(v));
      }
    }
    break;

    // new station title - update "title_status" widget
    case evt::yo_event_t::playerStationTitle : {
      // this is not thread-safe, to be fixed later
      const char* c = static_cast<const char*>(data);
      if (_scroll_title1)
        _scroll_title1->setText(static_cast<const char*>(data), SCROLLER_STATION_SPEED);
    }
    break;

    // new track title - update "title_status" widget
    case evt::yo_event_t::playerTrackTitle : {
      // this is not thread-safe, to be fixed later
      const char* c = static_cast<const char*>(data);
      if (_scroll_title2)
        _scroll_title2->setText(static_cast<const char*>(data), SCROLLER_TRACK_SPEED);
    }
    break;
    
    default:;
  }

}

void DisplayGFX::_build_main_screen(){
  muiItemId root_page = _mpp.makePage();  // root page
  // Status title
  _title_status = std::make_shared<MuiItem_AGFX_StaticText>( _mpp.nextIndex(), device_state_literal.at(0), TITLE_STATUS_POSITION_X, TITLE_STATUS_POSITION_Y, title_status_cfg);
  _mpp.addMuippItem(_title_status, root_page);

  // Clock
  ClockWidget* clk = new ClockWidget(_mpp.nextIndex());
  // pick configs from display-specific include file (conf/display_*)
  clk->cfg = clock_cfg;
  clk->dcfg = date_cfg;
  // move clock object to root page
  _mpp.addMuippItem(clk, root_page);

  // Scroller - radio title
  _scroll_title1 = std::make_shared<MuiItem_AGFX_TextScroller>(_mpp.nextIndex(), SCROLLER_STATION_POSITION_X, SCROLLER_STATION_POSITION_Y, SCROLLER_STATION_POSITION_W, SCROLLER_STATION_POSITION_H, scroller_station_cfg);
  // let it scroll "ёRadio" by default :)
  _scroll_title1->setText("ёRadio", SCROLLER_STATION_SPEED);
  _mpp.addMuippItem(_scroll_title1, root_page);

  // Scroller - track title
  _scroll_title2 = std::make_shared<MuiItem_AGFX_TextScroller>(_mpp.nextIndex(), SCROLLER_TRACK_POSITION_X, SCROLLER_TRACK_POSITION_Y, SCROLLER_TRACK_POSITION_W, SCROLLER_TRACK_POSITION_H, scroller_track_cfg);
  _mpp.addMuippItem(_scroll_title2, root_page);

  // BitRate Widget
  _mpp.addMuippItem(new MuiItem_Bitrate_Widget(_mpp.nextIndex(), BITRATE_WDGT_POSITION_X, BITRATE_WDGT_POSITION_Y, BITRATE_WDGT_W, BITRATE_WDGT_H, bitrate_wdgt_cfg), root_page);


  // this is not a real menu, so no need to activate the items
  //pageAutoSelect(root_page, some_id);
  // start menu from page mainmenu
  _mpp.menuStart(root_page);
  // render newly created screen
  _mpp.render(_gfx);
  _gfx->flush();
}

// ****************
//  DisplayControl methods
// ****************
void DisplayControl::init(){
  // load brightness value from nvs
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(T_Display, NVS_READONLY, &err);
  if (err != ESP_OK)
    return;
  handle->get_item(T_brightness, brt);
  setDevBrightness(brt);
}

void DisplayControl::setBrightness(int32_t val){
  brt = clamp(val, 0L, 100L);   // limit to 100%
  LOGI(T_Display, printf, "setBrightness:%u\n", brt);
  setDevBrightness(brt);

  // save brightness value to nvs
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(T_Display, NVS_READWRITE, &err);
  if (err == ESP_OK){
    handle->set_item(T_brightness, brt);
  }
  // publish brightness change notification to event bus
  // TODO: webpublish
  //EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::audioVolume), &volume, sizeof(volume));
}

// ****************
//  DisplayControl_AGFX_PWM methods
// ****************
DisplayControl_AGFX_PWM::DisplayControl_AGFX_PWM(int32_t backlight_gpio, int32_t level, Arduino_GFX* device, int32_t pwm_bit, int32_t pwm_hz) : _bcklight(backlight_gpio), _level(level), _dev(device), _pwm_bit(pwm_bit) {
  if (_bcklight > -1){
    // backlight
    ledcAttach(_bcklight, pwm_hz, pwm_bit);
    if (!level)
      ledcOutputInvert(_bcklight, true);
  }
};

void DisplayControl_AGFX_PWM::displaySuspend(bool state){
  if (_dev){
    state ? _dev->displayOff() : _dev->displayOn();
  }
  if (state){
    // save current duty
    _duty = ledcRead(_bcklight);
    ledcWrite(_bcklight, 0);    // this ugly Arduino API does not allow to just pause PWM, todo: replace it with native IDF ledc API
  } else {
    ledcWrite(_bcklight, _duty);
  }
}


// ****************
//  Dummy display methods
// ****************

void DisplayDummy::putRequest(displayRequestType_e type, int payload){
  if(type==NEWMODE) mode((displayMode_e)payload);
};


// ****************
//  Nextion display methods
// ****************
void DisplayNextion::_start(){
  //nextion.putcmd("page player");
  nextion.start();
  //config.setTitle(const_PlReady);
}

void DisplayNextion::putRequest(displayRequestType_e type, int payload){
  if(type==DSP_START) _start();
  requestParams_t request;
  request.type = type;
  request.payload = payload;
  nextion.putRequest(request);
}

void DisplayNextion::_station() {
  //nextion.newNameset(config.station.name);
  //nextion.bitrate(config.station.bitrate);
  nextion.bitratePic(ICON_NA);
}
    /*#ifdef USE_NEXTION
      nextion.newTitle(config.station.title);
    #endif*/
  /*#ifdef USE_NEXTION
    nextion.setVol(config.store.volume, _mode == VOL);
  #endif*/
      /*#ifdef USE_NEXTION
        char buf[50];
        snprintf(buf, 50, bootstrFmt, config.ssids[request.payload].ssid);
        nextion.bootString(buf);
      #endif*/
