#include "netserver.h"
#include "nvs_handle.hpp"
#include "EmbUI.h"
#include "basicui.h"
#include "const_strings.h"
#include "common.h"
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
  // request state reports, it should trigger sending WebUI updates for player state, screen brightness, etc...
  EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::reportStateAll));
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

NetServer::~NetServer(){
  _events_unsubsribe();
}

bool NetServer::begin(bool quiet) {
  if(!quiet) Serial.print("##[BOOT]#\tnetserver.begin\t");
  importRequest = IMDONE;

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

void NetServer::loop() {
  embui.handle();
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
  return false;
/*
  File tempfile = LittleFS.open(TMP_PATH, "r");
  if (!tempfile) {
    return false;
  }
  char sName[BUFLEN], sUrl[BUFLEN], linePl[BUFLEN*3];;
  int sOvol;
  // why two formats??? and why validate it here?
  // todoL refactor it
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
*/
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
  LOGV(T_WebUI, printf, "NetServer CHG event:%d\n", id);
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
      /*
      if(filename!="tempwifi.csv"){
        if(LittleFS.exists(PLAYLIST_PATH)) LittleFS.remove(PLAYLIST_PATH);
        if(LittleFS.exists(INDEX_PATH)) LittleFS.remove(INDEX_PATH);
        if(LittleFS.exists(PLAYLIST_SD_PATH)) LittleFS.remove(PLAYLIST_SD_PATH);
        if(LittleFS.exists(INDEX_SD_PATH)) LittleFS.remove(INDEX_SD_PATH);
      }
      */
      freeSpace = LittleFS.totalBytes() - LittleFS.usedBytes();
      request->_tempFile = LittleFS.open(filename, "w");
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
    //DBGVB("File: %s, size:%u bytes, index: %u, final: %s\n", filename.c_str(), len, index, final?"true":"false");
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
