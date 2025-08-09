#include "netserver.h"

#include "config.h"
#include "player.h"
#include "telnet.h"
#include "../displays/dspcore.h"
#include "options.h"
#include "network.h"
#include "mqtt.h"
#include "controls.h"
#include "commandhandler.h"
#include <Update.h>
#include <ESPmDNS.h>
#include "core/evtloop.h"
#include "log.h"


#if USE_OTA
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
#include <NetworkUdp.h>
#else
#include <WiFiUdp.h>
#endif
#include <ArduinoOTA.h>
#endif

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
static constexpr const char* P_id = "id";
static constexpr const char* P_value = "value";

NetServer netserver;

AsyncWebServer webserver(80);
AsyncWebSocket websocket("/ws");

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void handleIndex(AsyncWebServerRequest * request);
void handleNotFound(AsyncWebServerRequest * request);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void send_playlist(AsyncWebServerRequest * request);

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

void updateError(String& s) {
  s.reserve(200);
  s = "Update failed with code:";
  s += Update.getError();
  s += ", err:";
  s += Update.errorString();
}

NetServer::~NetServer(){
  _events_unsubsribe();
}

bool NetServer::begin(bool quiet) {
  if(network.status==SDREADY) return true;
  if(!quiet) Serial.print("##[BOOT]#\tnetserver.begin\t");
  importRequest = IMDONE;
  irRecordEnable = false;

  nsQueue = xQueueCreate( 20, sizeof( nsRequestParams_t ) );
  while(nsQueue==NULL){;}

  // server index
  webserver.on("/", HTTP_ANY, handleIndex);
  // playlist serve
  webserver.on(PLAYLIST_PATH, HTTP_GET, send_playlist);
  webserver.onNotFound(handleNotFound);
  webserver.onFileUpload(handleUpload);
  

  // server file content from filesystem
  webserver.serveStatic("/", LittleFS, "/www/")
    .setCacheControl(asyncsrv::T_no_cache);     // revalidate based on etag/IMS headers

  webserver.serveStatic("/data", LittleFS, "/data/")
    .setCacheControl(asyncsrv::T_no_cache);     // revalidate based on etag/IMS headers


#ifdef CORS_DEBUG
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), F("*"));
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Headers"), F("content-type"));
#endif
  webserver.begin();
  if(strlen(config.store.mdnsname)>0)
    MDNS.begin(config.store.mdnsname);

  websocket.onEvent(onWsEvent);
  webserver.addHandler(&websocket);
#if USE_OTA
  if(strlen(config.store.mdnsname)>0)
    ArduinoOTA.setHostname(config.store.mdnsname);
#ifdef OTA_PASS
  ArduinoOTA.setPassword(OTA_PASS);
#endif
  ArduinoOTA
    .onStart([]() {
      int32_t d = UPDATING;
      EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
      telnet.printf("Start OTA updating %s\n", ArduinoOTA.getCommand() == U_FLASH?"firmware":"filesystem");
    })
    .onEnd([]() {
      telnet.printf("\nEnd OTA update, Rebooting...\n");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      telnet.printf("Progress OTA: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      telnet.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        telnet.printf("Auth Failed\n");
      } else if (error == OTA_BEGIN_ERROR) {
        telnet.printf("Begin Failed\n");
      } else if (error == OTA_CONNECT_ERROR) {
        telnet.printf("Connect Failed\n");
      } else if (error == OTA_RECEIVE_ERROR) {
        telnet.printf("Receive Failed\n");
      } else if (error == OTA_END_ERROR) {
        telnet.printf("End Failed\n");
      }
    });
  ArduinoOTA.begin();
#endif

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
      case PLAYLISTSAVED:   {
        #ifdef USE_SD
        if(config.getMode()==PM_SDCARD) {
        //  config.indexSDPlaylist();
          config.initSDPlaylist();
        }
        #endif
        if(config.getMode()==PM_WEB){
          config.indexPlaylist(); 
          config.initPlaylist(); 
        }
        getPlaylist(clientId); break;
      }
      case GETACTIVE: {
          bool dbgact = false, nxtn=false;
          JsonArray act = obj["act"].to<JsonArray>();
          act.add("group_wifi");
          if (network.status == CONNECTED) {
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
          }
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

      case GETTIMEZONE:
        obj["tzh"] =  config.store.tzHour;
        obj["tzm"] =  config.store.tzMin;
        obj["sntp1"] =  config.store.sntp1;
        obj["sntp2"] =  config.store.sntp2;
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

      case STATIONNAME: {
        JsonArray a = obj[P_payload].to<JsonArray>();
        JsonObject o = a.add<JsonObject>();
        o["id"] = "nameset";
        o["value"] = config.station.name;
        break;
      }
      case ITEM:
        obj["current"] = config.lastStation();
        break;

      case TITLE: {
        JsonArray a = obj[P_payload].to<JsonArray>();
        JsonObject o = a.add<JsonObject>();
        o["id"] = "meta";
        o["value"] = config.station.title;
        telnet.printf("##CLI.META#: %s\n> ", config.station.title);
        break;
      }
      case VOLUME: {
        JsonArray a = obj[P_payload].to<JsonArray>();
        JsonObject o = a.add<JsonObject>();
        o["id"] = "volume";
        o["value"] = config.store.volume;
        telnet.printf("##CLI.VOL#: %d\n", config.store.volume);
        break;
      }
      case NRSSI: {
        JsonArray a = obj[P_payload].to<JsonArray>();
        JsonObject o = a.add<JsonObject>();
        o["id"] = "volume";
        o["value"] = rssi;
        break;
      }
      case SDPOS:
        //obj["sdpos"] = player->getFilePos();    >getFilePos() is obsolete
        obj["sdend"] = player->getFileSize();
        obj["sdtpos"] = player->getAudioCurrentTime();
        obj["sdtend"] = player->getAudioFileDuration();
        break;

      case SDLEN:
        obj["sdmin"] = player->sd_min;
        obj["sdmax"] = player->sd_max;
        break;

      case SDSNUFFLE:
        obj["snuffle"] = config.store.sdsnuffle;
        break;

      case MODE: {
        JsonArray a = obj[P_payload].to<JsonArray>();
        JsonObject o = a.add<JsonObject>();
          o["id"] = "playerwrap";
          o["value"] = player->status() == PLAYING ? "playing" : "stopped";
          telnet.info();
          break;
      }
      case EQUALIZER: {
        JsonArray a = obj[P_payload].to<JsonArray>();
        JsonObject o = a.add<JsonObject>();
        o["id"] = "bass";
        o["value"] = config.store.bass;
        JsonObject o2 = a.add<JsonObject>();
        o["id"] = "middle";
        o["value"] = config.store.middle;
        JsonObject o3 = a.add<JsonObject>();
        o["id"] = "trebble";
        o["value"] = config.store.trebble;
        break;
      }

      case BALANCE: {
        JsonArray a = obj[P_payload].to<JsonArray>();
        JsonObject o = a.add<JsonObject>();
        o["id"] = "balance";
        o["value"] = config.store.balance;
        break;
      }

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

    _send_ws_message(obj, clientId);
    if (!doc.isNull()){
/*
      size_t length = measureJson(obj);
      auto buffer = websocket.makeBuffer(length);
      if (buffer){
        serializeJson(obj, (char*)buffer->get(), length);
      }
      if (clientId == 0)
        { websocket.textAll(buffer); }
      else
        { websocket.text(clientId, buffer); }
*/
      #ifdef MQTT_ROOT_TOPIC
        if (clientId == 0 && (request.type == STATION || request.type == ITEM || request.type == TITLE || request.type == MODE)) mqttPublishStatus();
        if (clientId == 0 && request.type == VOLUME) mqttPublishVolume();
      #endif
    }
  }
}

void NetServer::loop() {
  if(network.status==SDREADY) return;
  if (shouldReboot) {
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }
  websocket.cleanupClients();
  switch (importRequest) {
    case IMPL:    importPlaylist();  importRequest = IMDONE; break;
    case IMWIFI:  config.saveWifi(); importRequest = IMDONE; break;
    default:      break;
  }
  processQueue();
#if USE_OTA
  ArduinoOTA.handle();
#endif
}

#if IR_PIN!=255
void NetServer::irToWs(const char* protocol, uint64_t irvalue) {
  char buf[BUFLEN] = { 0 };
  sprintf (buf, "{\"ircode\": %llu, \"protocol\": \"%s\"}", irvalue, protocol);
  websocket.textAll(buf);
}
void NetServer::irValsToWs() {
  if (!irRecordEnable) return;
  char buf[BUFLEN] = { 0 };
  sprintf (buf, "{\"irvals\": [%llu, %llu, %llu]}", config.ircodes.irVals[config.irindex][0], config.ircodes.irVals[config.irindex][1], config.ircodes.irVals[config.irindex][2]);
  websocket.textAll(buf);
}
#endif

void NetServer::onWsMessage(void *arg, uint8_t *data, size_t len, uint8_t clientId) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    // pring received message in debug mode
    LOGD("WS MSG: ", println, reinterpret_cast<const char*>(data));
    char comnd[65], val[65];
    if (config.parseWsCommand((const char*)data, comnd, val, 65)) {
      if (strcmp(comnd, "trebble") == 0) {
        int8_t valb = atoi(val);
        config.setTone(config.store.bass, config.store.middle, valb);
        return;
      }
      if (strcmp(comnd, "middle") == 0) {
        int8_t valb = atoi(val);
        config.setTone(config.store.bass, valb, config.store.trebble);
        return;
      }
      if (strcmp(comnd, "bass") == 0) {
        int8_t valb = atoi(val);
        config.setTone(valb, config.store.middle, config.store.trebble);
        return;
      }
      if (strcmp(comnd, "submitplaylistdone") == 0) {
#ifdef MQTT_ROOT_TOPIC
        mqttplaylistticker.attach(5, mqttplaylistSend);
#endif
        if (player->isRunning()){
          auto v = config.lastStation();
          EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::plsStation), &v, sizeof(v));
        }

        return;
      }
      
      if(cmd.exec(comnd, val, clientId)){
        return;
      }
    }
  }
}

void NetServer::getPlaylist(uint8_t clientId) {
  char buf[160] = {0};
  sprintf(buf, "{\"file\": \"http://%s%s\"}", WiFi.localIP().toString().c_str(), PLAYLIST_PATH);
  if (clientId == 0) { websocket.textAll(buf); } else { websocket.text(clientId, buf); }
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
  _readPlaylistLine(tempfile, linePl, sizeof(linePl)-1);
  if (config.parseCSV(linePl, sName, sUrl, sOvol)) {
    tempfile.close();
    LittleFS.rename(TMP_PATH, PLAYLIST_PATH);
    requestOnChange(PLAYLISTSAVED, 0);
    return true;
  }
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
  auto buffer = websocket.makeBuffer(length);
  if (buffer){
    serializeJson(v, (char*)buffer->get(), length);
  }
  if (clientId == 0)
    { websocket.textAll(buffer); }
  else
    { websocket.text(clientId, buffer); }
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
      evt::audio_into_t* i = reinterpret_cast<evt::audio_into_t*>(data);
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
  }else if(request->url()=="/update"){
    if (!index) {
      int target = (request->getParam("updatetarget", true)->value() == "LittleFS") ? U_SPIFFS : U_FLASH;
      Serial.printf("Update Start: %s\n", filename.c_str());
      EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::playerStop));
      int32_t d = UPDATING;
      EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewMode), &d, sizeof(d));
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, target)) {
        Update.printError(Serial);
        String err;
        updateError(err);
        request->send(200, asyncsrv::T_text_html, err);
      }
    }
    if (!Update.hasError()) {
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
        String err;
        updateError(err);
        request->send(200, asyncsrv::T_text_html, err);
      }
    }
    if (final) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %uB\n", index + len);
      } else {
        Update.printError(Serial);
        String err;
        updateError(err);
        request->send(200, asyncsrv::T_text_html, err);
      }
    }
  }else{ // "/webboard"
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
      if(filename=="playlist.csv") config.indexPlaylist();
    }
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT: /*netserver.requestOnChange(STARTUP, client->id()); */if (config.store.audioinfo) Serial.printf("[WEBSOCKET] client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str()); break;
    case WS_EVT_DISCONNECT: if (config.store.audioinfo) Serial.printf("[WEBSOCKET] client #%u disconnected\n", client->id()); break;
    case WS_EVT_DATA: netserver.onWsMessage(arg, data, len, client->id()); break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
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
  if (config.getMode()==PM_SDCARD)    // redirect to SDCARD's path
    return request->redirect(PLAYLIST_SD_PATH);

  // last resort - send playlist from LittleFS
  request->send(LittleFS, request->url().c_str(), asyncsrv::T_text_plain);
}

void handleNotFound(AsyncWebServerRequest * request) {
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

    if(request->url()=="/update"){ // <--upload firmware
      shouldReboot = !Update.hasError();
      String err;
      updateError(err);
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : err.c_str());
      response->addHeader("Connection", "close");
      request->send(response);
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
#if defined(HTTP_USER) && defined(HTTP_PASS)
  if(network.status == CONNECTED)
    if (!request->authenticate(HTTP_USER, HTTP_PASS)) {
      return request->requestAuthentication();
    }
#endif
  if (strcmp(request->url().c_str(), "/") == 0 && request->params() == 0) {
    if(network.status == CONNECTED) request->send(200, asyncsrv::T_text_html, index_html); else request->redirect("/settings.html");
    return;
  }
  if(network.status == CONNECTED){
    int paramsNr = request->params();
    if(paramsNr==1){
      const AsyncWebParameter* p = request->getParam(0);
      if(cmd.exec(p->name().c_str(),p->value().c_str())) {
        if(p->name()=="reset" || p->name()=="clearLittleFS")
          return request->redirect("/");
        
      request->send(200, asyncsrv::T_text_plain);
        if(p->name()=="clearLittleFS")
          { delay(100); ESP.restart(); }
        return;
      }
    }
    if (request->hasArg("trebble") && request->hasArg("middle") && request->hasArg("bass")) {
      config.setTone(request->getParam("bass")->value().toInt(), request->getParam("middle")->value().toInt(), request->getParam("trebble")->value().toInt());
      request->send(200, asyncsrv::T_text_plain);
      return;
    }
    if (request->hasArg("sleep")) {
      int sford = request->getParam("sleep")->value().toInt();
      int safterd = request->hasArg("after")?request->getParam("after")->value().toInt():0;
      if(sford > 0 && safterd >= 0){
        request->send(200, asyncsrv::T_text_plain);
        config.sleepForAfter(sford, safterd);
        return;
      }
    }
  }

  request->send(404, asyncsrv::T_text_plain, "Not found");  
}
