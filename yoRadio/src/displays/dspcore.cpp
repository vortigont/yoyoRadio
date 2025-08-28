#include "nvs_handle.hpp"
#include "EmbUI.h"
#include "dspcore.h"
#include "locale/l10n.h"
#include "core/const_strings.h"
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
  _state = state_t::empty;
  if (_gfx->begin()){
    _gfx->fillScreen(RGB565_BLUE);
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
  //_bootScreen();

  // create runner task
  xTaskCreatePinnedToCore(
    [](void* self){ static_cast<DisplayGFX*>(self)->_loopDspTask(); },
    "DspTask",
    CORE_STACK_SIZE,
    static_cast<void*>(this),
    DISPLAY_GFX_TASK_PRIO,
    &_dspTask,
    CONFIG_ARDUINO_RUNNING_CORE);

  _mode = PLAYER;
  _state = state_t::normal;

  _events_subsribe();
  LOGI(T_BOOT, println, "display->init");
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

          default: break;
        }

      // check if there are more messages waiting in the Q, in this case break the loop() and go
      // for another round to evict next message, do not waste time to redraw the screen, etc...
      if (uxQueueMessagesWaiting(_displayQueue))
        continue;
    }

    // refresh screen items if needed
    if (_mpp.refresh(_gfx)){
      _gfx->flush();
    }
  }
  vTaskDelete( NULL );
  _dspTask=NULL;
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
    //case evt::yo_event_t::displayPStop :
    //  _layoutChange(false);
    //  break;

    default:;
  }
}

// notifications events
void DisplayGFX::_events_chg_hndlr(int32_t id, void* data){
  LOGV(T_Display, printf, "chg event rcv:%d\n", id);

  switch (static_cast<evt::yo_event_t>(id)){
    // device mode change - update "title_status" widget (todo: this should be done from inside the widget)
    case evt::yo_event_t::devMode : {
      int32_t v = *static_cast<int32_t*>(data);
      if (_title_status && v >= 0 && v < device_state_literal.size()){
        _title_status->setName(device_state_literal.at(v));
      }
    }
    break;

    // new station title - update "title_status" widget
    case evt::yo_event_t::playerStationTitle : {
      // this is not thread-safe, to be fixed later (todo: this should be done from inside the widget)
      if (_scroll_title1)
        _scroll_title1->setText(static_cast<const char*>(data));
    }
    break;

    // new track title - update "title_status" widget
    case evt::yo_event_t::playerTrackTitle : {
      // this is not thread-safe, to be fixed later
      if (_scroll_title2)
        _scroll_title2->setText(static_cast<const char*>(data));
    }
    break;
    
    default:;
  }

}

void DisplayGFX::load_main_preset(const std::vector<widget_cfgitem_t>& preset){
  // purge entire container - now I use only one set, so should be OK untill multiple sets of pages are introduced
  _mpp.clear();
  // purge existing objects if any
  _title_status.reset();
  _scroll_title1.reset();
  _scroll_title2.reset();

  muiItemId root_page = _mpp.makePage();  // root page

  // parse the preset and populate widgets set
  for (auto i : preset){
    switch (i.wtype){
      // BitRate Widget
      case yoyo_wdgt_t::bitrate :
        _mpp.addMuippItem(new MuiItem_Bitrate_Widget(_mpp.nextIndex(), reinterpret_cast<const bitrate_box_cfg_t*>(i.cfg), _gfx->width(), _gfx->height()), root_page);
        break;

      // Clock
      case yoyo_wdgt_t::clock :
        _mpp.addMuippItem(new ClockWidget(_mpp.nextIndex(), *reinterpret_cast<const clock_cfg_t*>(i.cfg)->clk, *reinterpret_cast<const clock_cfg_t*>(i.cfg)->date), root_page);
        break;

      // Status title
      case yoyo_wdgt_t::text :
        _title_status = std::make_shared<MuiItem_AGFX_StaticText>(
          _mpp.nextIndex(),
          device_state_literal.at(0) /* "idle" */,
          reinterpret_cast<const text_wdgt_t*>(i.cfg)->place.getAbsoluteXY(_gfx->width(), _gfx->height()),  // unwrap to real position
          reinterpret_cast<const text_wdgt_t*>(i.cfg)->style
        );
        _mpp.addMuippItem(_title_status, root_page);
        break;

      // Scroller - main title / radio title
      case yoyo_wdgt_t::scrollerStation :
        _scroll_title1 = std::make_shared<MuiItem_AGFX_TextScroller>(
            _mpp.nextIndex(),
            reinterpret_cast<const scroller_cfg_t*>(i.cfg)->box.getBoxDimensions(_gfx->width(), _gfx->height()),  // unwrap into absolute position
            reinterpret_cast<const scroller_cfg_t*>(i.cfg)->scroll_speed,
            reinterpret_cast<const scroller_cfg_t*>(i.cfg)->style);
        // let it scroll "ёёRadio" by default :)
        _scroll_title1->setText("ёёRadio");
        _mpp.addMuippItem(_scroll_title1, root_page);
        break;

      // Scroller - track ttile (kind of dublicate, but will do for now)
      case yoyo_wdgt_t::scrollerTitle :
        _scroll_title2 = std::make_shared<MuiItem_AGFX_TextScroller>(
            _mpp.nextIndex(),
            reinterpret_cast<const scroller_cfg_t*>(i.cfg)->box.getBoxDimensions(_gfx->width(), _gfx->height()),  // unwrap into absolute position
            reinterpret_cast<const scroller_cfg_t*>(i.cfg)->scroll_speed,
            reinterpret_cast<const scroller_cfg_t*>(i.cfg)->style);
        _mpp.addMuippItem(_scroll_title2, root_page);
        break;

      default:;
    }
  }

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
DisplayControl::~DisplayControl(){
  _events_unsubsribe();
  _embui_actions_unregister();
}

void DisplayControl::init(){
  // load brightness value from nvs
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(T_Display, NVS_READONLY, &err);
  if (err != ESP_OK){
    // if no saved value exist then set bightness to 75% by default
    setDevBrightness(75);
    return;
  }
  handle->get_item(T_brightness, brt);
  setDevBrightness(brt);
  _embui_actions_register();
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
  // send notification to event bus
  if (embui.feeders.available()){
    // publish changed value to EmbUI feeds
    Interface interf(&embui.feeders);
    interf.json_frame_value();
    interf.value(T_brightness, val);
    interf.json_frame_flush();
  }
  // publish brightness change notification to event bus
  EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::brightness), &val, sizeof(val));  
}

void DisplayControl::_events_subsribe(){
  // command events
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_CMD_EVENTS, ESP_EVENT_ANY_ID,
    [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<DisplayControl*>(self)->_events_cmd_hndlr(id, data); },
    this, &_hdlr_cmd_evt
  );

  // state change events
  /*
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_CHG_STATE_EVENTS, ESP_EVENT_ANY_ID,
    [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<DisplayControl*>(self)->_events_chg_hndlr(id, data); },
    this, &_hdlr_chg_evt
  );
*/
}

void DisplayControl::_events_unsubsribe(){
  esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_CMD_EVENTS, ESP_EVENT_ANY_ID, _hdlr_cmd_evt);
//  esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_CHG_STATE_EVENTS, ESP_EVENT_ANY_ID, _hdlr_chg_evt);
}

void DisplayControl::_events_cmd_hndlr(int32_t id, void* data){
  LOGV(T_Display, printf, "cmd event rcv:%d\n", id);
  switch (static_cast<evt::yo_event_t>(id)){

    // brightness control
    case evt::yo_event_t::displayBrightness :
      setBrightness(*reinterpret_cast<int32_t*>(data));
      break;

    default:;
  }
}

void DisplayControl::_embui_actions_register(){
  // brightness control from EmbUI
  embui.action.add(T_disp_brt, [this](Interface *interf, JsonVariantConst data, const char* action){ setBrightness(data); } );
}

void DisplayControl::_embui_actions_unregister(){
  embui.action.remove(T_disp_brt);
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
