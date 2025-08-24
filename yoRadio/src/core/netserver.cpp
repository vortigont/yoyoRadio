#include "nvs_handle.hpp"
#include "netserver.h"
#include "EmbUI.h"
#include "basicui.h"
#include "const_strings.h"
#include "config.h"
#include "../displays/dspcore.h"
#include "options.h"
#include "evtloop.h"
#include "log.h"


#ifdef USE_SD
#include "sdmanager.h"
#endif
#ifndef MIN_MALLOC
#define MIN_MALLOC 24112
#endif
#ifndef NSQ_SEND_DELAY
  #define NSQ_SEND_DELAY       (TickType_t)100  //portMAX_DELAY?
#endif

//#define CORS_DEBUG //Enable CORS policy: 'Access-Control-Allow-Origin' (for testing)

static constexpr const char* P_payload = "payload";

/**
 * @brief numeric indexes for pages
 * it MUST not overlap with basicui::page index
 */
enum class page_t : uint16_t {
  radio = 50,                 // radio playback (front page)
  devprofile = 51             // page with device profile setup
};

NetServer netserver;

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void handleHTTPArgs(AsyncWebServerRequest * request);
// send playlist from LittleFS or from SD
void send_playlist(AsyncWebServerRequest * request);


// **************
// EmbUI handlers declarations
void ui_page_selector(Interface *interf, JsonVariantConst data, const char* action);
void ui_page_main(Interface *interf, JsonVariantConst data, const char* action);
void ui_page_radio(Interface *interf, JsonVariantConst data, const char* action);
// buld additional section under "Settings" page
void ui_block_usersettings(Interface *interf, JsonVariantConst data, const char* action);


// **************
// EmbUI handlers

/**
 * @brief index page callback for WebUI,
 * it loads on each new WebSocket connection
 * 
 */
void ui_page_main(Interface *interf, JsonVariantConst data, const char* action){
  // send application manifest
  interf->json_frame_interface();
  interf->json_section_manifest("yoRadio", embui.macid(), 0, "0.0.0 testing");       // app name/version manifest
  interf->json_section_end();

  // build side menu
  interf->json_section_uidata();
    interf->uidata_pick( "yo.menu" );
  //implicit interf->json_frame_flush();

  if(WiFi.getMode() & WIFI_MODE_STA){
    ui_page_radio(interf, {}, NULL);
  } else {
    // открываем страницу с настройками WiFi если контроллер не подключен к внешней AP
    basicui::page_settings_netw(interf, {}, NULL);
  }
}



// build page with radio (default page that opens on new conects)
void ui_page_radio(Interface *interf, JsonVariantConst data, const char* action){
  interf->json_frame_interface();
  interf->json_section_uidata();
    interf->uidata_pick( "yo.pages.radio" );
  interf->json_frame_flush();
}

// buld additional section under "Settings" page
void ui_block_usersettings(Interface *interf, JsonVariantConst data, const char* action){
  interf->json_section_uidata();
    interf->uidata_pick( "yo.blocks.settings" );

  // no need to flush this
}

// Buld a page to setup device profile
void ui_page_device_profile(Interface *interf, JsonVariantConst data, const char* action){
  interf->json_frame_interface();
  interf->json_section_uidata();
    interf->uidata_pick( "yo.pages.dev_profile" );
  interf->json_frame_flush();

  // send current value to UI
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(T_devcfg, NVS_READONLY, &err);
  if (err != ESP_OK) return;  // no NVS - no profiles

  size_t len{0};
  handle->get_item_size(nvs::ItemType::ANY, T_profile, len);
  if (!len) return;

  char profile[len];
  handle->get_string(T_profile, profile, len);
  interf->json_frame_value();
    interf->value(T_profile, profile);
  interf->json_frame_flush();
}

// process device profile setup form submission
void ui_set_device_profile(Interface *interf, JsonVariantConst data, const char* action){
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(T_devcfg, NVS_READWRITE, &err);
  if (err != ESP_OK || !data[T_profile].is<const char*>()) return;
  const char* profile = data[T_profile];
  handle->set_string(T_profile, profile);
  LOGI(T_WebUI, printf, "Apply device profile: %s\n", profile);
  // reboot device
  Task *t = new Task(TASK_SECOND * 3, TASK_ONCE, nullptr, &ts, false, nullptr, [](){ ESP.restart(); });
  t->enableDelayed();
}


// register web action handlers
void embui_actions_register(){
  embui.action.set_mainpage_cb(ui_page_main);                             // index page callback
  embui.action.add(T_ui_page_radio, ui_page_radio);                       // build page "radio" (via left page menu)
  embui.action.add(T_ui_page_any, ui_page_selector);                      // ui page switcher
  embui.action.set_settings_cb(ui_block_usersettings);                    // "settings" page, user block
  // setup device profile
  embui.action.add(A_dev_profile, ui_set_device_profile);

  // ***************
  // simple handlers
}

/**
 * @brief when action is called to display a specific page
 * this selector picks and calls correspoding method
 * using common selector simplifes and reduces a number of registered actions required 
 * 
 */
void ui_page_selector(Interface *interf, JsonVariantConst data, const char* action){
  // get a page index
  int idx = data;

  switch (static_cast<page_t>(idx)){
      case page_t::radio :              // страница воспроизведения радио
        return ui_page_radio(interf, {}, NULL);
      case page_t::devprofile :         // страница настройки профилей устройства
        return ui_page_device_profile(interf, {}, NULL);

      default:;                   // by default do nothing
  }
}


// ***********************************************************
//  NetServer method
// ***********************************************************

void handleIndex(AsyncWebServerRequest * request);
void handleNotFound(AsyncWebServerRequest * request);

bool  shouldReboot  = false;
#ifdef MQTT_ROOT_TOPIC
Ticker mqttplaylistticker;
bool  mqttplaylistblock = false;
void mqttplaylistSend() {
  mqttplaylistblock = true;
  mqttplaylistticker.detach();
  mqttPublishPlaylist();
  mqttplaylistblock = false;
}
#endif

NetServer::~NetServer(){
  _events_unsubsribe();
}

bool NetServer::begin(bool quiet) {
  if(!quiet) Serial.print("##[BOOT]#\tnetserver.begin\t");
  importRequest = IMDONE;
  irRecordEnable = false;

  nsQueue = xQueueCreate( 20, sizeof( nsRequestParams_t ) );

  // static content for WebUI
  set_static_http_handlers();
  // playlist serve ( no SD for now, so can skip)
  //embui.server.on(PLAYLIST_PATH, HTTP_GET, send_playlist);
  
  //embui.server.serveStatic("/data", LittleFS, "/data/")
  //  .setCacheControl(asyncsrv::T_no_cache);     // revalidate based on etag/IMS headers
    
  embui.server.onFileUpload(handleUpload);
    
#ifdef CORS_DEBUG
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), F("*"));
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Headers"), F("content-type"));
#endif

  // set EmbUI action handlers
  embui_actions_register();

  embui.begin();
  // change periodic WebUI publish interval
  embui.setPubInterval(30);

  // event bus subscriptions
  _events_subsribe();
  return true;
}

size_t NetServer::chunkedHtmlPageCallback(uint8_t* buffer, size_t maxLen, size_t index){
  File requiredfile;
  bool sdpl = strcmp(netserver.chunkedPathBuffer, PLAYLIST_SD_PATH) == 0;
  if(sdpl){
    requiredfile = config.SDPLFS()->open(netserver.chunkedPathBuffer, "r");
  }else{
    requiredfile = LittleFS.open(netserver.chunkedPathBuffer, "r");
  }
  if (!requiredfile) return 0;
  size_t filesize = requiredfile.size();
  size_t needread = filesize - index;
  if (!needread) {
    requiredfile.close();
    return 0;
  }
  size_t canread = (needread > maxLen) ? maxLen : needread;
  DBGVB("[%s] seek to %d in %s and read %d bytes with maxLen=%d", __func__, index, netserver.chunkedPathBuffer, canread, maxLen);
  requiredfile.seek(index, SeekSet);
  requiredfile.read(buffer, canread);
  index += canread;
  if (requiredfile) requiredfile.close();
  return canread;
}

void NetServer::chunkedHtmlPage(const String& contentType, AsyncWebServerRequest *request, const char * path) {
  memset(chunkedPathBuffer, 0, sizeof(chunkedPathBuffer));
  strlcpy(chunkedPathBuffer, path, sizeof(chunkedPathBuffer)-1);
  AsyncWebServerResponse *response;
  response = request->beginChunkedResponse(contentType, chunkedHtmlPageCallback);
  request->send(response);
}

#ifndef DSP_NOT_FLIPPED
  #define DSP_CAN_FLIPPED true
#else
  #define DSP_CAN_FLIPPED false
#endif
#if !defined(HIDE_WEATHER) && (!defined(DUMMYDISPLAY) && !defined(USE_NEXTION))
  #define SHOW_WEATHER  true
#else
  #define SHOW_WEATHER  false
#endif

#ifndef NS_QUEUE_TICKS
  #define NS_QUEUE_TICKS 0
#endif
/*
const char *getFormat(BitrateFormat _format) {
  switch (_format) {
    case BF_MP3:  return "MP3";
    case BF_AAC:  return "AAC";
    case BF_FLAC: return "FLC";
    case BF_OGG:  return "OGG";
    case BF_WAV:  return "WAV";
    default:      return "bitrate";
  }
}
*/

void NetServer::processQueue(){
  if(nsQueue==NULL) return;
  nsRequestParams_t request;
  if(xQueueReceive(nsQueue, &request, NS_QUEUE_TICKS)){
    uint8_t clientId = request.clientId;
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    switch (request.type) {
      case PLAYLIST:        getPlaylist(clientId); break;
      case GETACTIVE: {
          bool dbgact = false, nxtn=false;
          JsonArray act = obj["act"].to<JsonArray>();
          act.add("group_wifi");
            act.add("group_system");
            if (BRIGHTNESS_PIN != 255 || DSP_CAN_FLIPPED || DSP_MODEL == DSP_NOKIA5110 || dbgact)
              act.add("group_display");
          #ifdef USE_NEXTION
            act.add("group_nextion");
            if (!SHOW_WEATHER || dbgact)
              act.add("group_weather");
            nxtn=true;
          #endif
          #if defined(LCD_I2C) || defined(DSP_OLED)
            act.add("group_oled");
          #endif
          #ifndef HIDE_VU
            act.add("group_vu");
          #endif
            if (BRIGHTNESS_PIN != 255 || nxtn || dbgact)
              act.add("group_brightness");
            if (DSP_CAN_FLIPPED || dbgact)
              act.add("group_tft");
            if (TS_MODEL != TS_MODEL_UNDEFINED || dbgact)
              act.add("group_touch");
            if (DSP_MODEL == DSP_NOKIA5110)
              act.add("group_nokia");

            act.add("group_timezone");
            if (SHOW_WEATHER || dbgact)
              act.add("group_weather");

            act.add("group_controls");
            if (ENC_BTNL != 255 || ENC2_BTNL != 255 || dbgact)
              act.add("group_encoder");
            if (IR_PIN != 255 || dbgact)
              act.add("group_ir");
          break;
        }
      //case STARTUP:       sprintf (wsbuf, "{\"command\":\"startup\", \"payload\": {\"mode\":\"%s\", \"version\":\"%s\"}}", network.status == CONNECTED ? "player" : "ap", YOVERSION); break;
      case GETINDEX:      {
          requestOnChange(STATION, clientId);
          requestOnChange(TITLE, clientId);
          requestOnChange(VOLUME, clientId);
          requestOnChange(EQUALIZER, clientId);
          requestOnChange(BALANCE, clientId);
          requestOnChange(MODE, clientId);
          requestOnChange(SDINIT, clientId);
          requestOnChange(GETPLAYERMODE, clientId); 
          if (config.getMode()==PM_SDCARD) { requestOnChange(SDPOS, clientId); requestOnChange(SDLEN, clientId); requestOnChange(SDSNUFFLE, clientId); } 
          return; 
          break;
        }
      case GETSYSTEM:
        obj["sst"] = config.store.smartstart != 2;
        obj["aif"] = config.store.audioinfo;
        obj["vu"] = config.store.vumeter;
        obj["softr"] = config.store.softapdelay;
        obj["vut"] = config.vuThreshold;
        obj["mdns"] = config.store.mdnsname;
        break;

      case GETSCREEN:
        obj["flip"] = config.store.flipscreen;
        obj["inv"] =  config.store.invertdisplay;
        obj["nump"] =  config.store.numplaylist;
        obj["tsf"] =  config.store.fliptouch;
        obj["tsd"] =  config.store.dbgtouch;
        obj["dspon"] =  config.store.dspon;
        obj["br"] =  config.store.brightness;
        obj["con"] =  config.store.contrast;
        obj["scre"] =  config.store.screensaverEnabled;
        obj["scrt"] =  config.store.screensaverTimeout;
        obj["scrb"] =  config.store.screensaverBlank;
        obj["scrpe"] =  config.store.screensaverPlayingEnabled;
        obj["scrpt"] =  config.store.screensaverPlayingTimeout;
        obj["scrpb"] =  config.store.screensaverPlayingBlank;
        break;

      case GETWEATHER:
        obj["wen"] = config.store.showweather;
        obj["wlat"] = config.store.weatherlat;
        obj["wlon"] = config.store.weatherlon;
        obj["wkey"] = config.store.weatherkey;
        break;

      case GETCONTROLS:
        obj["vols"] =  config.store.volsteps;
        obj["enca"] =  config.store.encacc;
        obj["irtl"] =  config.store.irtlp;
        obj["skipup"] =  config.store.skipPlaylistUpDown;
        break;
      case DSPON:
        obj["dspontrue"] = true;

      case STATION:
        requestOnChange(STATIONNAME, clientId); requestOnChange(ITEM, clientId); break;
/*
      case STATIONNAME: {
        JsonArray a = obj[P_payload].to<JsonArray>();
        JsonObject o = a.add<JsonObject>();
        o["id"] = "nameset";
        o["value"] = config.station.name;
        break;
      }
*/
      case NRSSI: {
        JsonArray a = obj[P_payload].to<JsonArray>();
        JsonObject o = a.add<JsonObject>();
        o["id"] = "volume";
        o["value"] = rssi;
        break;
      }
/*
      case SDPOS:
        //obj["sdpos"] = player->getFilePos();    >getFilePos() is obsolete
        obj["sdend"] = player->getFileSize();
        obj["sdtpos"] = player->getAudioCurrentTime();
        obj["sdtend"] = player->getAudioFileDuration();
        break;
      case SDLEN: // not sure what is this
        obj["sdmin"] = player->sd_min;
        obj["sdmax"] = player->sd_max;
        break;
        case SDSNUFFLE:
        obj["snuffle"] = config.store.sdsnuffle;
        break;
*/

      case SDINIT:
        obj["sdinit"] = SDC_CS!=255;
        break;

      case GETPLAYERMODE:
        obj["playermode"] = config.getMode()==PM_SDCARD?"modesd":"modeweb";
        break;

      #ifdef USE_SD
      case CHANGEMODE:
          config.changeMode(config.newConfigMode);
          return;
      #endif

      default:;
    }

    //_send_ws_message(obj, clientId);
    if (!doc.isNull()){
      size_t length = measureJson(obj);
      auto buffer = embui.ws.makeBuffer(length);
      if (buffer){
        serializeJson(obj, (char*)buffer->get(), length);
      }
      if (clientId == 0)
        { embui.ws.textAll(buffer); }
      else
        { embui.ws.text(clientId, buffer); }

      #ifdef MQTT_ROOT_TOPIC
        if (clientId == 0 && (request.type == STATION || request.type == ITEM || request.type == TITLE || request.type == MODE)) mqttPublishStatus();
        if (clientId == 0 && request.type == VOLUME) mqttPublishVolume();
      #endif
    }
  }
}

void NetServer::loop() {
  if (shouldReboot) {
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }
  switch (importRequest) {
    case IMPL:    importPlaylist();  importRequest = IMDONE; break;
    //case IMWIFI:  config.saveWifi(); importRequest = IMDONE; break;
    default:      break;
  }
  processQueue();
  embui.handle();
}

#if IR_PIN!=255
void NetServer::irToWs(const char* protocol, uint64_t irvalue) {
  char buf[BUFLEN] = { 0 };
  sprintf (buf, "{\"ircode\": %llu, \"protocol\": \"%s\"}", irvalue, protocol);
  embui.ws.textAll(buf);
}
void NetServer::irValsToWs() {
  if (!irRecordEnable) return;
  char buf[BUFLEN] = { 0 };
  sprintf (buf, "{\"irvals\": [%llu, %llu, %llu]}", config.ircodes.irVals[config.irindex][0], config.ircodes.irVals[config.irindex][1], config.ircodes.irVals[config.irindex][2]);
  embui.ws.textAll(buf);
}
#endif

void NetServer::getPlaylist(uint8_t clientId) {
  char buf[160] = {0};
  sprintf(buf, "{\"file\": \"http://%s%s\"}", WiFi.localIP().toString().c_str(), PLAYLIST_PATH);
  if (clientId == 0) { embui.ws.textAll(buf); } else { embui.ws.text(clientId, buf); }
}

int NetServer::_readPlaylistLine(File &file, char * line, size_t size){
  int bytesRead = file.readBytesUntil('\n', line, size);
  if(bytesRead>0){
    line[bytesRead] = 0;
    if(line[bytesRead-1]=='\r') line[bytesRead-1]=0;
  }
  return bytesRead;
}

bool NetServer::importPlaylist() {
  if(config.getMode()==PM_SDCARD) return false;
  File tempfile = LittleFS.open(TMP_PATH, "r");
  if (!tempfile) {
    return false;
  }
  char sName[BUFLEN], sUrl[BUFLEN], linePl[BUFLEN*3];;
  int sOvol;
  // why two formats??? and why validate it here?
  // todoL refactor it
  return false;
  _readPlaylistLine(tempfile, linePl, sizeof(linePl)-1);
/*
  if (config.parseCSV(linePl, sName, sUrl, sOvol)) {
    tempfile.close();
    LittleFS.rename(TMP_PATH, PLAYLIST_PATH);
    requestOnChange(PLAYLISTSAVED, 0);
    return true;
  }
*/
  if (config.parseJSON(linePl, sName, sUrl, sOvol)) {
    File playlistfile = LittleFS.open(PLAYLIST_PATH, "w");
    snprintf(linePl, sizeof(linePl)-1, "%s\t%s\t%d", sName, sUrl, 0);
    playlistfile.println(linePl);
    while (tempfile.available()) {
      _readPlaylistLine(tempfile, linePl, sizeof(linePl)-1);
      if (config.parseJSON(linePl, sName, sUrl, sOvol)) {
        snprintf(linePl, sizeof(linePl)-1, "%s\t%s\t%d", sName, sUrl, 0);
        playlistfile.println(linePl);
      }
    }
    playlistfile.flush();
    playlistfile.close();
    tempfile.close();
    LittleFS.remove(TMP_PATH);
    requestOnChange(PLAYLISTSAVED, 0);
    return true;
  }
  tempfile.close();
  LittleFS.remove(TMP_PATH);
  return false;
}

void NetServer::requestOnChange(requestType_e request, uint8_t clientId) {
  if(nsQueue==NULL) return;
  nsRequestParams_t nsrequest;
  nsrequest.type = request;
  nsrequest.clientId = clientId;
  xQueueSend(nsQueue, &nsrequest, NSQ_SEND_DELAY);
}

void NetServer::resetQueue(){
  if(nsQueue!=NULL) xQueueReset(nsQueue);
}

void NetServer::_send_ws_message(JsonVariantConst v, int32_t clientId){
  if (v.isNull()) return;

  size_t length = measureJson(v);
  auto buffer = embui.ws.makeBuffer(length);
  if (buffer){
    serializeJson(v, (char*)buffer->get(), length);
  }
  if (clientId == 0)
    { embui.ws.textAll(buffer); }
  else
    { embui.ws.text(clientId, buffer); }
}

void NetServer::_events_subsribe(){
  // command events
/*  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_CMD_EVENTS, ESP_EVENT_ANY_ID,
    [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<NetServer*>(self)->_events_cmd_hndlr(id, data); },
    this, &_hdlr_cmd_evt
  );
*/
  // state change events
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_CHG_STATE_EVENTS, ESP_EVENT_ANY_ID,
    [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<NetServer*>(self)->_events_chg_hndlr(id, data); },
    this, &_hdlr_chg_evt
  );
}

void NetServer::_events_unsubsribe(){
  //esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_CMD_EVENTS, ESP_EVENT_ANY_ID, _hdlr_cmd_evt);
  esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_CHG_STATE_EVENTS, ESP_EVENT_ANY_ID, _hdlr_chg_evt);
}

void NetServer::_events_chg_hndlr(int32_t id, void* data){
  JsonDocument doc;
  JsonObject obj = doc.to<JsonObject>();

  switch (static_cast<evt::yo_event_t>(id)){

    // process metadata about playing codec
    case evt::yo_event_t::playerAudioInfo : {
      audio_info_t* i = reinterpret_cast<audio_info_t*>(data);
      JsonArray a = doc[P_payload].to<JsonArray>();
      JsonObject o = a.add<JsonObject>();
        o[P_id] = "bitrate";
        o[P_value] = i->bitRate;
      JsonObject o2 = a.add<JsonObject>();
        o2[P_id] = "fmt";
        o2[P_value] = i->codecName;
        break;
    }

    default:;
  }

  _send_ws_message(obj);
}




int freeSpace;
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if(request->url()=="/upload"){
    if (!index) {
      if(filename!="tempwifi.csv"){
        if(LittleFS.exists(PLAYLIST_PATH)) LittleFS.remove(PLAYLIST_PATH);
        if(LittleFS.exists(INDEX_PATH)) LittleFS.remove(INDEX_PATH);
        if(LittleFS.exists(PLAYLIST_SD_PATH)) LittleFS.remove(PLAYLIST_SD_PATH);
        if(LittleFS.exists(INDEX_SD_PATH)) LittleFS.remove(INDEX_SD_PATH);
      }
      freeSpace = (float)LittleFS.totalBytes()/100*68-LittleFS.usedBytes();
      request->_tempFile = LittleFS.open(TMP_PATH , "w");
    }
    if (len) {
      if(freeSpace>index+len){
        request->_tempFile.write(data, len);
      }
    }
    if (final) {
      request->_tempFile.close();
    }
  } else { // "/webboard"
    DBGVB("File: %s, size:%u bytes, index: %u, final: %s\n", filename.c_str(), len, index, final?"true":"false");
    if (!index) {
      String spath("/www/");
      if(filename=="playlist.csv" || filename=="wifi.csv") spath = "/data/";
      request->_tempFile = LittleFS.open(spath + filename , "w");
    }
    if (len) {
      request->_tempFile.write(data, len);
    }
    if (final) {
      request->_tempFile.close();
      //if(filename=="playlist.csv") config.indexPlaylist();
      // todo: send a notify to reload a playlist
    }
  }
}

void send_playlist(AsyncWebServerRequest * request){
#ifdef MQTT_ROOT_TOPIC    // very ugly check
  // if MQTT something is in progress, ask client to return later
  if (mqttplaylistblock){
    AsyncWebServerResponse *response = request->beginResponse(429);
    response->addHeader(asyncsrv::T_retry_after, 1);
    return request->send(response);
  }
#endif
  // no sdcard for now
  //if (config.getMode()==PM_SDCARD)    // redirect to SDCARD's path
  //  return request->redirect(PLAYLIST_SD_PATH);

  // last resort - send playlist from LittleFS
  request->send(LittleFS, request->url().c_str(), asyncsrv::T_text_plain);
}

void handleNotFound(AsyncWebServerRequest * request) {}
/*
#if defined(HTTP_USER) && defined(HTTP_PASS)
  if(network.status == CONNECTED)
    if (request->url() == "/logout") {
      request->send(401);
      return;
    }
    if (!request->authenticate(HTTP_USER, HTTP_PASS)) {
      return request->requestAuthentication();
    }
#endif
  // emergency static page
  if(request->url()=="/emergency")
    { request->send(200, asyncsrv::T_text_html, emergency_form); return; }

  // redirect to root page if posting to /webboard on empty FS
  if(config.emptyFS && request->method() == HTTP_POST && request->url()=="/webboard")
    { request->redirect("/"); ESP.restart(); return; }
  
  if (request->method() == HTTP_POST) {
    if(request->url()=="/webboard")
      { request->redirect("/"); return; } // <--post files from /data/www

    if(request->url()=="/upload"){ // <--upload playlist.csv or wifi.csv
      if (request->hasParam("plfile", true, true)) {
        netserver.importRequest = IMPL;
        request->send(200);
      } else if (request->hasParam("wifile", true, true)) {
        netserver.importRequest = IMWIFI;
        request->send(200);
      } else {
        request->send(404);
      }
      return;
    }

  }// if (request->method() == HTTP_POST)
  
  if (request->url() == "/favicon.ico") {
    request->send(200, "image/x-icon", "data:,");
    return;
  }
  if (request->url() == "/variables.js") {
    char varjsbuf[BUFLEN];
    sprintf (varjsbuf, "var yoVersion='%s';\nvar formAction='%s';\nvar playMode='%s';\n", YOVERSION, (network.status == CONNECTED && !config.emptyFS)?"webboard":"", (network.status == CONNECTED)?"player":"ap");
    request->send(200, asyncsrv::T_application_javascript, varjsbuf);
    return;
  }
  if (strcmp(request->url().c_str(), "/settings.html") == 0 || strcmp(request->url().c_str(), "/update.html") == 0 || strcmp(request->url().c_str(), "/ir.html") == 0){
    request->send(200, asyncsrv::T_text_html, index_html);
    return;
  }
  if (request->method() == HTTP_GET && request->url() == "/webboard") {
    request->send(200, asyncsrv::T_text_html, emptyfs_html);
    return;
  }
  Serial.print("Not Found: ");
  Serial.println(request->url());
  request->send(404, "text/plain", "Not found");
}
*/

void handleIndex(AsyncWebServerRequest * request) {
  if(config.emptyFS){
    //webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ if (r->args()) return handleHTTPArgs(r); r->redirect( network.status == CONNECTED ? "/index.html" : "/settings.html");});

    if(request->method() == HTTP_GET ) { request->send(200, asyncsrv::T_text_html, emptyfs_html); return; }
    if(request->method() == HTTP_POST) {
      if(request->arg("ssid")!="" && request->arg("pass")!=""){
        char buf[BUFLEN];
        memset(buf, 0, BUFLEN);
        snprintf(buf, BUFLEN, "%s\t%s", request->arg("ssid").c_str(), request->arg("pass").c_str());
        request->redirect("/");
        config.saveWifiFromNextion(buf);
        return;
      }
      request->redirect("/"); 
      ESP.restart();
      return;
    }
    Serial.print("Not Found: ");
    Serial.println(request->url());
    request->send(404, "text/plain", "Not found");
    return;
  } // end if(config.emptyFS)


  request->send(404, asyncsrv::T_text_plain, "Not found");  
}
