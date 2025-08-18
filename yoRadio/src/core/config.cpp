#include "config.h"

#include "../displays/dspcore.h"
#include "player.h"
#include "network.h"
#include "netserver.h"
#include "controls.h"
#ifdef USE_SD
#include "sdmanager.h"
#endif
#include <cstddef>
#include "evtloop.h"
#include "log.h"


Config config;

void u8fix(char *src){
  char last = src[strlen(src)-1]; 
  if ((uint8_t)last >= 0xC2) src[strlen(src)-1]='\0';
}

Config::~Config(){
  _events_unsubsribe();
}

bool Config::_isFSempty() {
  const char* reqiredFiles[] = {"dragpl.js.gz","ir.css.gz","irrecord.html.gz","ir.js.gz","logo.svg.gz","options.html.gz","player.html.gz","script.js.gz",
                                "style.css.gz","updform.html.gz","theme.css"};
  const uint8_t reqiredFilesSize = 11;
  char fullpath[28];
  if(LittleFS.exists("/www/settings.html")) LittleFS.remove("/www/settings.html");
  if(LittleFS.exists("/www/update.html")) LittleFS.remove("/www/update.html");
  if(LittleFS.exists("/www/index.html")) LittleFS.remove("/www/index.html");
  if(LittleFS.exists("/www/ir.html")) LittleFS.remove("/www/ir.html");
  if(LittleFS.exists("/www/elogo.png")) LittleFS.remove("/www/elogo.png");
  if(LittleFS.exists("/www/elogo84.png")) LittleFS.remove("/www/elogo84.png");
  for (uint8_t i=0; i<reqiredFilesSize; i++){
    sprintf(fullpath, "/www/%s", reqiredFiles[i]);
    if(!LittleFS.exists(fullpath)) {
      Serial.println(fullpath);
      return true;
    }
  }
  return false;
}

void Config::init() {
  EEPROM.begin(EEPROM_SIZE);
  sdResumePos = 0;
  screensaverTicks = 0;
  screensaverPlayingTicks = 0;
  newConfigMode = 0;
  isScreensaver = false;
  bootInfo();
#if RTCSUPPORTED
  _rtcFound = false;
  BOOTLOG("RTC begin(SDA=%d,SCL=%d)", RTC_SDA, RTC_SCL);
  if(rtc.init()){
    BOOTLOG("done");
    _rtcFound = true;
  }else{
    BOOTLOG("[ERROR] - Couldn't find RTC");
  }
#endif
  emptyFS = true;
#if IR_PIN!=255
    irindex=-1;
#endif
#if defined(SD_SPIPINS) || SD_HSPI
  #if !defined(SD_SPIPINS)
    SDSPI.begin();
  #else
    SDSPI.begin(SD_SPIPINS); // SCK, MISO, MOSI
  #endif
#endif
  eepromRead(EEPROM_START, store);
  
  if (store.config_set != 4262) {
    setDefaults();
  }
  if(store.version>CONFIG_VERSION) store.version=1;
  while(store.version!=CONFIG_VERSION) _setupVersion();
  BOOTLOG("CONFIG_VERSION\t%d", store.version);
  store.play_mode = store.play_mode & 0b11;
  if(store.play_mode>1) store.play_mode=PM_WEB;
  _initHW();
  if (!LittleFS.begin(true)) {
    Serial.println("##[ERROR]#\tLittleFS Mount Failed");
    return;
  }
  BOOTLOG("LittleFS mounted");
  emptyFS = _isFSempty();
  if(emptyFS) BOOTLOG("LittleFS is empty!");
  #ifdef USE_SD
  _SDplaylistFS = getMode()==PM_SDCARD?&sdman:(true?&LittleFS:_SDplaylistFS);
  #else
  _SDplaylistFS = &LittleFS;
  #endif
  _bootDone=false;
  _events_subsribe();
}

void Config::_setupVersion(){
  uint16_t currentVersion = store.version;
  switch(currentVersion){
    case 1:
      saveValue(&store.screensaverEnabled, false);
      saveValue(&store.screensaverTimeout, (uint16_t)20);
      break;
    case 2:
      char buf[MDNS_LENGTH];
      snprintf(buf, MDNS_LENGTH, "yoradio-%x", getChipId());
      saveValue(store.mdnsname, buf, MDNS_LENGTH);
      saveValue(&store.skipPlaylistUpDown, false);
      break;
    case 3:
      saveValue(&store.screensaverBlank, false);
      saveValue(&store.screensaverPlayingEnabled, false);
      saveValue(&store.screensaverPlayingTimeout, (uint16_t)5);
      saveValue(&store.screensaverPlayingBlank, false);
      break;
    default:
      break;
  }
  currentVersion++;
  saveValue(&store.version, currentVersion);
}

#ifdef USE_SD

void Config::changeMode(int newmode){
  bool pir = player->isRunning();
  if(SDC_CS==255) return;
/*
  if(getMode()==PM_SDCARD) {
    sdResumePos = player->getFilePos();   // getFilePos() is not available in recent audio lib
  }
*/
  // wtf???
  if(/* network.status==SOFT_AP || */ display->mode()==LOST){
    saveValue(&store.play_mode, static_cast<uint8_t>(PM_SDCARD));
    delay(50);
    ESP.restart();
  }
  if(!sdman.ready && newmode!=PM_WEB) {
    if(!sdman.start()){
      Serial.println("##[ERROR]#\tSD Not Found");
      netserver.requestOnChange(GETPLAYERMODE, 0);
      sdman.stop();
      return;
    }
  }
  if(newmode<0||newmode>MAX_PLAY_MODE){
    store.play_mode++;
    if(getMode() > MAX_PLAY_MODE) store.play_mode=0;
  }else{
    store.play_mode=(playMode_e)newmode;
  }
  saveValue(&store.play_mode, store.play_mode, true, true);
  _SDplaylistFS = getMode()==PM_SDCARD?&sdman:(true?&LittleFS:_SDplaylistFS);
  if(getMode()==PM_SDCARD){
    if(pir){
      EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::playerStop));
    }
    int32_t d = SDCHANGE;
    EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
    while(display->mode()!=SDCHANGE)
      delay(10);
    delay(50);
  }
  if(getMode()==PM_WEB) {
    //if(network.status==SDREADY) ESP.restart();
    sdman.stop();
  }
  if(!_bootDone) return;
  if (pir){
    auto v = getMode()==PM_WEB?store.lastStation:store.lastSdStation;
    EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::playerStation), &v, sizeof(v));
  }

  netserver.resetQueue();
  //netserver.requestOnChange(GETPLAYERMODE, 0);
  netserver.requestOnChange(GETINDEX, 0);
  //netserver.requestOnChange(GETMODE, 0);
 // netserver.requestOnChange(CHANGEMODE, 0);
  display->resetQueue();
  
  int32_t d = SDCHANGE;
  EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
  EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewStation));
}

void Config::initSDPlaylist() {
  //store.countStation = 0;
  bool doIndex = !sdman.exists(INDEX_SD_PATH);
  if(doIndex) sdman.indexSDPlaylist();
  if (SDPLFS()->exists(INDEX_SD_PATH)) {
    File index = SDPLFS()->open(INDEX_SD_PATH, "r");
    //store.countStation = index.size() / 4;
    if(doIndex){
      lastStation(_randomStation());
      sdResumePos = 0;
    }
    index.close();
    //saveValue(&store.countStation, store.countStation, true, true);
  }
}

#endif //#ifdef USE_SD

bool Config::spiffsCleanup(){
  bool ret = (LittleFS.exists(PLAYLIST_SD_PATH)) || (LittleFS.exists(INDEX_SD_PATH)) || (LittleFS.exists(INDEX_PATH));
  if(LittleFS.exists(PLAYLIST_SD_PATH)) LittleFS.remove(PLAYLIST_SD_PATH);
  if(LittleFS.exists(INDEX_SD_PATH)) LittleFS.remove(INDEX_SD_PATH);
  if(LittleFS.exists(INDEX_PATH)) LittleFS.remove(INDEX_PATH);
  return ret;
}

void Config::_initHW(){
  loadTheme();
  #if IR_PIN!=255
  eepromRead(EEPROM_START_IR, ircodes);
  if(ircodes.ir_set!=4224){
    ircodes.ir_set=4224;
    memset(ircodes.irVals, 0, sizeof(ircodes.irVals));
  }
  #endif
  #if BRIGHTNESS_PIN!=255
    pinMode(BRIGHTNESS_PIN, OUTPUT);
    setBrightness(false);
  #endif
}

uint16_t Config::color565(uint8_t r, uint8_t g, uint8_t b)
{
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void Config::loadTheme(){
  theme.background    = color565(COLOR_BACKGROUND);
  theme.meta          = color565(COLOR_STATION_NAME);
  theme.metabg        = color565(COLOR_STATION_BG);
  theme.metafill      = color565(COLOR_STATION_FILL);
  theme.title1        = color565(COLOR_SNG_TITLE_1);
  theme.title2        = color565(COLOR_SNG_TITLE_2);
  theme.digit         = color565(COLOR_DIGITS);
  theme.div           = color565(COLOR_DIVIDER);
  theme.weather       = color565(COLOR_WEATHER);
  theme.vumax         = color565(COLOR_VU_MAX);
  theme.vumin         = color565(COLOR_VU_MIN);
  theme.clock         = color565(COLOR_CLOCK);
  theme.clockbg       = color565(COLOR_CLOCK_BG);
  theme.seconds       = color565(COLOR_SECONDS);
  theme.dow           = color565(COLOR_DAY_OF_W);
  theme.date          = color565(COLOR_DATE);
  theme.heap          = color565(COLOR_HEAP);
  theme.buffer        = color565(COLOR_BUFFER);
  theme.ip            = color565(COLOR_IP);
  theme.vol           = color565(COLOR_VOLUME_VALUE);
  theme.rssi          = color565(COLOR_RSSI);
  theme.bitrate       = color565(COLOR_BITRATE);
  theme.volbarout     = color565(COLOR_VOLBAR_OUT);
  theme.volbarin      = color565(COLOR_VOLBAR_IN);
  theme.plcurrent     = color565(COLOR_PL_CURRENT);
  theme.plcurrentbg   = color565(COLOR_PL_CURRENT_BG);
  theme.plcurrentfill = color565(COLOR_PL_CURRENT_FILL);
  theme.playlist[0]   = color565(COLOR_PLAYLIST_0);
  theme.playlist[1]   = color565(COLOR_PLAYLIST_1);
  theme.playlist[2]   = color565(COLOR_PLAYLIST_2);
  theme.playlist[3]   = color565(COLOR_PLAYLIST_3);
  theme.playlist[4]   = color565(COLOR_PLAYLIST_4);
  #include "../displays/tools/tftinverttitle.h"
}

template <class T> int Config::eepromWrite(int ee, const T& value) {
  const uint8_t* p = (const uint8_t*)(const void*)&value;
  int i;
  for (i = 0; i < sizeof(value); i++)
    EEPROM.write(ee++, *p++);
  EEPROM.commit();
  return i;
}

template <class T> int Config::eepromRead(int ee, T& value) {
  uint8_t* p = (uint8_t*)(void*)&value;
  int i;;
  for (i = 0; i < sizeof(value); i++)
    *p++ = EEPROM.read(ee++);
  return i;
}

void Config::reset(){
  setDefaults();
  delay(500);
  ESP.restart();
}
void Config::enableScreensaver(bool val){
  saveValue(&store.screensaverEnabled, val);
#ifndef DSP_LCD
  int32_t d = PLAYER;
  EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
#endif
}
void Config::setScreensaverTimeout(uint16_t val){
  val=constrain(val,5,65520);
  saveValue(&store.screensaverTimeout, val);
#ifndef DSP_LCD
  int32_t d = PLAYER;
  EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
#endif
}
void Config::setScreensaverBlank(bool val){
  saveValue(&store.screensaverBlank, val);
#ifndef DSP_LCD
  int32_t d = PLAYER;
  EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
#endif
}
void Config::setScreensaverPlayingEnabled(bool val){
  saveValue(&store.screensaverPlayingEnabled, val);
#ifndef DSP_LCD
  int32_t d = PLAYER;
  EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
#endif
}
void Config::setScreensaverPlayingTimeout(uint16_t val){
  val=constrain(val,1,1080);
  config.saveValue(&config.store.screensaverPlayingTimeout, val);
#ifndef DSP_LCD
  int32_t d = PLAYER;
  EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
#endif
}
void Config::setScreensaverPlayingBlank(bool val){
  saveValue(&store.screensaverPlayingBlank, val);
#ifndef DSP_LCD
  int32_t d = PLAYER;
  EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
#endif
}

void Config::setShowweather(bool val){
  config.saveValue(&config.store.showweather, val);
  //network.trueWeather=false;
  //network.forceWeather = true;
  EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayShowWeather));

}
void Config::setWeatherKey(const char *val){
  saveValue(store.weatherkey, val, WEATHERKEY_LENGTH);
  //network.trueWeather=false;
  int32_t d = CLEAR;
  EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
  d = PLAYER;
  EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
}
void Config::setSDpos(uint32_t val){
  if (getMode()==PM_SDCARD){
    sdResumePos = 0;
    if(!player->isRunning()){
      player->setResumeFilePos(val - player->sd_min);
      EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::playerStation), &config.store.lastSdStation, sizeof(config.store.lastSdStation));
    }else{
      player->setFilePos(val - player->sd_min);
    }
  }
}
#if IR_PIN!=255
void Config::setIrBtn(int val){
  irindex = val;
  netserver.irRecordEnable = (irindex >= 0);
  irchck = 0;
  netserver.irValsToWs();
  if (irindex < 0) saveIR();
}
#endif
void Config::resetSystem(const char *val, uint8_t clientId){
  if (strcmp(val, "system") == 0) {
    saveValue(&store.smartstart, (uint8_t)2, false);
    saveValue(&store.audioinfo, false, false);
    saveValue(&store.vumeter, false, false);
    saveValue(&store.softapdelay, (uint8_t)0, false);
    snprintf(store.mdnsname, MDNS_LENGTH, "yoradio-%x", getChipId());
    saveValue(store.mdnsname, store.mdnsname, MDNS_LENGTH, true, true);
    int32_t d = CLEAR;
    EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
    d = PLAYER;
    EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
    netserver.requestOnChange(GETSYSTEM, clientId);
    return;
  }
  if (strcmp(val, "screen") == 0) {
    saveValue(&store.flipscreen, false, false);
    display->flip();
    saveValue(&store.invertdisplay, false, false);
    display->invert();
    saveValue(&store.dspon, true, false);
    store.brightness = 100;
    setBrightness(false);
    saveValue(&store.contrast, (uint8_t)55, false);
    display->setContrast();
    saveValue(&store.numplaylist, false);
    saveValue(&store.screensaverEnabled, false);
    saveValue(&store.screensaverTimeout, (uint16_t)20);
    saveValue(&store.screensaverBlank, false);
    saveValue(&store.screensaverPlayingEnabled, false);
    saveValue(&store.screensaverPlayingTimeout, (uint16_t)5);
    saveValue(&store.screensaverPlayingBlank, false);
    int32_t d = CLEAR;
    EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
    d = PLAYER;
    EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
    netserver.requestOnChange(GETSCREEN, clientId);
    return;
  }

  if (strcmp(val, "weather") == 0) {
    saveValue(&store.showweather, false, false);
    saveValue(store.weatherlat, "55.7512", 10, false);
    saveValue(store.weatherlon, "37.6184", 10, false);
    saveValue(store.weatherkey, "", WEATHERKEY_LENGTH);
    //network.trueWeather=false;
    int32_t d = CLEAR;
    EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
    d = PLAYER;
    EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
    netserver.requestOnChange(GETWEATHER, clientId);
    return;
  }
  if (strcmp(val, "controls") == 0) {
    saveValue(&store.volsteps, (uint8_t)1, false);
    saveValue(&store.fliptouch, false, false);
    saveValue(&store.dbgtouch, false, false);
    saveValue(&store.skipPlaylistUpDown, false);
    setEncAcceleration(200);
    setIRTolerance(40);
    netserver.requestOnChange(GETCONTROLS, clientId);
    return;
  }
  if (strcmp(val, "1") == 0) {
    config.reset();
    return;
  }
}



void Config::setDefaults() {
  store.config_set = 4262;
  store.version = CONFIG_VERSION;
  store.lastStation = 0;
  store.countStation = 0;
  store.lastSSID = 0;
  store.audioinfo = false;
  store.smartstart = 2;

  store.vumeter=false;
  store.softapdelay=0;
  store.flipscreen=false;
  store.invertdisplay=false;
  store.numplaylist=false;
  store.fliptouch=false;
  store.dbgtouch=false;
  store.dspon=true;
  store.brightness=100;
  store.contrast=55;
  store.showweather=false;
  strlcpy(store.weatherlat,"55.7512", 10);
  strlcpy(store.weatherlon,"37.6184", 10);
  strlcpy(store.weatherkey,"", WEATHERKEY_LENGTH);
  store._reserved = 0;
  store.lastSdStation = 0;
  store.sdsnuffle = false;
  store.volsteps = 1;
  store.encacc = 200;
  store.play_mode = 0;
  store.irtlp = 35;
  store.btnpullup = true;
  store.btnlongpress = 200;
  store.btnclickticks = 300;
  store.btnpressticks = 500;
  store.encpullup = false;
  store.enchalf = false;
  store.enc2pullup = false;
  store.enc2half = false;
  store.forcemono = false;
  store.i2sinternal = false;
  store.rotate90 = false;
  store.screensaverEnabled = false;
  store.screensaverTimeout = 20;
  store.screensaverBlank = false;
  snprintf(store.mdnsname, MDNS_LENGTH, "yoradio-%x", getChipId());
  store.skipPlaylistUpDown = false;
  store.screensaverPlayingEnabled = false;
  store.screensaverPlayingTimeout = 5;
  store.screensaverPlayingBlank = false;
  eepromWrite(EEPROM_START, store);
}

void Config::setSnuffle(bool sn){
  saveValue(&store.sdsnuffle, sn);
  if(store.sdsnuffle) player->next();
}

#if IR_PIN!=255
void Config::saveIR(){
  eepromWrite(EEPROM_START_IR, ircodes);
}
#endif

void Config::setSmartStart(uint8_t ss) {
  saveValue(&store.smartstart, ss);
}

uint8_t Config::setLastSSID(uint8_t val) {
  saveValue(&store.lastSSID, val);
  return store.lastSSID;
}
/*
uint8_t Config::fillPlMenu(int from, uint8_t count, bool fromNextion) {
  int     ls      = from;
  uint8_t c       = 0;
  bool    finded  = false;
  if (playlistLength() == 0) {
    return 0;
  }
  File playlist = SDPLFS()->open(REAL_PLAYL, "r");
  File index = SDPLFS()->open(REAL_INDEX, "r");
  while (true) {
    if (ls < 1) {
      ls++;
      if(!fromNextion) display->printPLitem(c, "");
  #ifdef USE_NEXTION
    if(fromNextion) nextion.printPLitem(c, "");
  #endif
      c++;
      continue;
    }
    if (!finded) {
      index.seek((ls - 1) * 4, SeekSet);
      uint32_t pos;
      index.readBytes((char *) &pos, 4);
      finded = true;
      index.close();
      playlist.seek(pos, SeekSet);
    }
    bool pla = true;
    while (pla) {
      pla = playlist.available();
      String stationName = playlist.readStringUntil('\n');
      stationName = stationName.substring(0, stationName.indexOf('\t'));
      if(config.store.numplaylist && stationName.length()>0) stationName = String(from+c)+" "+stationName;
      if(!fromNextion) display->printPLitem(c, stationName.c_str());
      #ifdef USE_NEXTION
        if(fromNextion) nextion.printPLitem(c, stationName.c_str());
      #endif
      c++;
      if (c >= count) break;
    }
    break;
  }
  playlist.close();
  return c;
}
*/
void Config::escapeQuotes(const char* input, char* output, size_t maxLen) {
  size_t j = 0;
  for (size_t i = 0; input[i] != '\0' && j < maxLen - 1; ++i) {
    if (input[i] == '"' && j < maxLen - 2) {
      output[j++] = '\\';
      output[j++] = '"';
    } else {
      output[j++] = input[i];
    }
  }
  output[j] = '\0';
}

bool Config::parseJSON(const char* line, char* name, char* url, int &ovol) {
  char* tmps, *tmpe;
  const char* cursor = line;
  char port[8], host[246], file[254];
  tmps = strstr(cursor, "\":\"");
  if (tmps == NULL) return false;
  tmpe = strstr(tmps, "\",\"");
  if (tmpe == NULL) return false;
  strlcpy(name, tmps + 3, tmpe - tmps - 3 + 1);
  if (strlen(name) == 0) return false;
  cursor = tmpe + 3;
  tmps = strstr(cursor, "\":\"");
  if (tmps == NULL) return false;
  tmpe = strstr(tmps, "\",\"");
  if (tmpe == NULL) return false;
  strlcpy(host, tmps + 3, tmpe - tmps - 3 + 1);
  if (strlen(host) == 0) return false;
  if (strstr(host, "http://") == NULL && strstr(host, "https://") == NULL) {
    sprintf(file, "http://%s", host);
    strlcpy(host, file, strlen(file) + 1);
  }
  cursor = tmpe + 3;
  tmps = strstr(cursor, "\":\"");
  if (tmps == NULL) return false;
  tmpe = strstr(tmps, "\",\"");
  if (tmpe == NULL) return false;
  strlcpy(file, tmps + 3, tmpe - tmps - 3 + 1);
  cursor = tmpe + 3;
  tmps = strstr(cursor, "\":\"");
  if (tmps == NULL) return false;
  tmpe = strstr(tmps, "\",\"");
  if (tmpe == NULL) return false;
  strlcpy(port, tmps + 3, tmpe - tmps - 3 + 1);
  int p = atoi(port);
  if (p > 0) {
    sprintf(url, "%s:%d%s", host, p, file);
  } else {
    sprintf(url, "%s%s", host, file);
  }
  cursor = tmpe + 3;
  tmps = strstr(cursor, "\":\"");
  if (tmps == NULL) return false;
  tmpe = strstr(tmps, "\"}");
  if (tmpe == NULL) return false;
  strlcpy(port, tmps + 3, tmpe - tmps - 3 + 1);
  ovol = atoi(port);
  return true;
}

bool Config::parseWsCommand(const char* line, char* cmd, char* val, uint8_t cSize) {
  char *tmpe;
  tmpe = strstr(line, "=");
  if (tmpe == NULL) return false;
  memset(cmd, 0, cSize);
  strlcpy(cmd, line, tmpe - line + 1);
  //if (strlen(tmpe + 1) == 0) return false;
  memset(val, 0, cSize);
  strlcpy(val, tmpe + 1, strlen(line) - strlen(cmd) + 1);
  return true;
}

bool Config::parseSsid(const char* line, char* ssid, char* pass) {
  char *tmpe;
  tmpe = strstr(line, "\t");
  if (tmpe == NULL) return false;
  uint16_t pos = tmpe - line;
  if (pos > 29 || strlen(line) > 71) return false;
  memset(ssid, 0, 30);
  strlcpy(ssid, line, pos + 1);
  memset(pass, 0, 40);
  strlcpy(pass, line + pos + 1, strlen(line) - pos);
  return true;
}

bool Config::saveWifiFromNextion(const char* post){
  File file = LittleFS.open(SSIDS_PATH, "w");
  if (!file) {
    return false;
  } else {
    file.print(post);
    file.close();
    ESP.restart();
    return true;
  }
}

void Config::setBrightness(bool dosave){
#if BRIGHTNESS_PIN!=255
  if(!store.dspon && dosave) {
    display->wakeup();
  }
  analogWrite(BRIGHTNESS_PIN, map(store.brightness, 0, 100, 0, 255));
  if(!store.dspon) store.dspon = true;
  if(dosave){
    saveValue(&store.brightness, store.brightness, false, true);
    saveValue(&store.dspon, store.dspon, true, true);
  }
#endif
#ifdef USE_NEXTION
  nextion.wake();
  char cmd[15];
  snprintf(cmd, 15, "dims=%d", store.brightness);
  nextion.putcmd(cmd);
  if(!store.dspon) store.dspon = true;
  if(dosave){
    saveValue(&store.brightness, store.brightness, false, true);
    saveValue(&store.dspon, store.dspon, true, true);
  }
#endif
}

void Config::setDspOn(bool dspon, bool saveval){
  if(saveval){
    store.dspon = dspon;
    saveValue(&store.dspon, store.dspon, true, true);
  }
#ifdef USE_NEXTION
  if(!dspon) nextion.sleep();
  else nextion.wake();
#endif
  if(!dspon){
#if BRIGHTNESS_PIN!=255
  analogWrite(BRIGHTNESS_PIN, 0);
#endif
    display->deepsleep();
  }else{
    display->wakeup();
#if BRIGHTNESS_PIN!=255
  analogWrite(BRIGHTNESS_PIN, map(store.brightness, 0, 100, 0, 255));
#endif
  }
}

void Config::doSleep(){
  if(BRIGHTNESS_PIN!=255) analogWrite(BRIGHTNESS_PIN, 0);
  display->deepsleep();
#ifdef USE_NEXTION
  nextion.sleep();
#endif
#if !defined(ARDUINO_ESP32C3_DEV)
#if !defined(CONFIG_IDF_TARGET_ESP32P4)
 if(WAKE_PIN!=255) esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_PIN, LOW);
#else
  if(WAKE_PIN!=255) esp_sleep_enable_ext1_wakeup((gpio_num_t)WAKE_PIN, ESP_EXT1_WAKEUP_ANY_LOW);
#endif
  esp_sleep_enable_timer_wakeup(config.sleepfor * 60 * 1000000ULL);
  esp_deep_sleep_start();
#endif
}

void Config::doSleepW(){
  if(BRIGHTNESS_PIN!=255) analogWrite(BRIGHTNESS_PIN, 0);
  display->deepsleep();
#ifdef USE_NEXTION
  nextion.sleep();
#endif
#if !defined(ARDUINO_ESP32C3_DEV)
#if !defined(CONFIG_IDF_TARGET_ESP32P4)
  if(WAKE_PIN!=255) esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_PIN, LOW);
#else
  if(WAKE_PIN!=255) esp_sleep_enable_ext1_wakeup((gpio_num_t)WAKE_PIN, ESP_EXT1_WAKEUP_ANY_LOW);
#endif
  esp_deep_sleep_start();
#endif
}

void Config::sleepForAfter(uint16_t sf, uint16_t sa){
  sleepfor = sf;
  if(sa > 0) _sleepTimer.attach(sa * 60, doSleep);
  else doSleep();
}

void Config::bootInfo() {
  BOOTLOG("************************************************");
  BOOTLOG("*               ёPadio v%s                *", YOVERSION);
  BOOTLOG("************************************************");
  BOOTLOG("------------------------------------------------");
  BOOTLOG("arduino:\t%d", ARDUINO);
  BOOTLOG("compiler:\t%s", __VERSION__);
  BOOTLOG("esp32core:\t%d.%d.%d", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
  uint32_t chipId = 0;
  for(int i=0; i<17; i=i+8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  BOOTLOG("chip:\t\tmodel: %s | rev: %d | id: %d | cores: %d | psram: %d", ESP.getChipModel(), ESP.getChipRevision(), chipId, ESP.getChipCores(), ESP.getPsramSize());
  BOOTLOG("display:\t%d", DSP_MODEL);
  BOOTLOG("audio:\t\t%s (%d, %d, %d)", "I2S", I2S_DOUT, I2S_BCLK, I2S_LRC);
  BOOTLOG("audioinfo:\t%s", store.audioinfo?"true":"false");
  BOOTLOG("smartstart:\t%d", store.smartstart);
  BOOTLOG("vumeter:\t%s", store.vumeter?"true":"false");
  BOOTLOG("softapdelay:\t%d", store.softapdelay);
  BOOTLOG("flipscreen:\t%s", store.flipscreen?"true":"false");
  BOOTLOG("invertdisplay:\t%s", store.invertdisplay?"true":"false");
  BOOTLOG("showweather:\t%s", store.showweather?"true":"false");
  BOOTLOG("buttons:\tleft=%d, center=%d, right=%d, up=%d, down=%d, mode=%d, pullup=%s", 
          BTN_LEFT, BTN_CENTER, BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_MODE, BTN_INTERNALPULLUP?"true":"false");
  BOOTLOG("encoders:\tl1=%d, b1=%d, r1=%d, pullup=%s, l2=%d, b2=%d, r2=%d, pullup=%s", 
          ENC_BTNL, ENC_BTNB, ENC_BTNR, ENC_INTERNALPULLUP?"true":"false", ENC2_BTNL, ENC2_BTNB, ENC2_BTNR, ENC2_INTERNALPULLUP?"true":"false");
  BOOTLOG("ir:\t\t%d", IR_PIN);
  if(SDC_CS!=255) BOOTLOG("SD:\t\t%d", SDC_CS);
  BOOTLOG("------------------------------------------------");
}

void Config::_events_subsribe(){
  // command events
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_CMD_EVENTS, ESP_EVENT_ANY_ID,
    [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<Config*>(self)->_events_cmd_hndlr(id, data); },
    this, &_hdlr_cmd_evt
  );
}

void Config::_events_unsubsribe(){
  esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_CMD_EVENTS, ESP_EVENT_ANY_ID, _hdlr_cmd_evt);

}

void Config::_events_cmd_hndlr(int32_t id, void* data){
  switch (static_cast<evt::yo_event_t>(id)){

    #ifdef USE_SD
    // check player mode (ex. PR_CHECKSD)
    case evt::yo_event_t::playerMode :
      if(config.getMode()==PM_SDCARD){
        if(!sdman.cardPresent()){
          sdman.stop();
          changeMode(PM_WEB);
        }
      }
      break;
    #endif  // USE_SD

    default:;
  }
}