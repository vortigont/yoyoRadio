#include "nvs_handle.hpp"
#include "config.h"
#include "common.h"
#include "player.h"
#include "const_strings.h"
#include "locale/l10n.h"
#include "sdmanager.h"
#include "netserver.h"
#include "evtloop.h"
#include "log.h"

AudioController::~AudioController(){
  _events_unsubsribe();
  _embui_actions_unregister();
}

void AudioController::init() {
  Serial.print("##[BOOT]#\tplayer.init\t");
  _resumeFilePos = 0;
  memset(_plError, 0, PLERR_LN);
  #ifdef MQTT_ROOT_TOPIC
  memset(burl, 0, MQTT_BURL_SIZE);
  #endif
  _status = STOPPED;
  #if PLAYER_FORCE_MONO
    forceMono(true);
  #endif
  audio.setVolumeSteps(100);                  // set volume steps to 100 to have some common groud for different DACs or so...
  audio.setConnectionTimeout(1700, 3700);
  _loadValues();  // restore volume/tone value from NVS
  // load playlist index
  _pls.loadIndex();
  // event bus subscription
  _events_subsribe();
  // EmbUI messages
  _embui_actions_register();
  // Audio callbacks
  audio.setLiteralCallback([this](const char* msg, audiolib::callback_type_t type){ _audio_cb_generic(msg, type); });
  // Enable all type of literal callbacks (most verbose one)
  audio.enableCallbackType(audiolib::callback_type_t::all, true);
}

void AudioController::stopInfo() {
  config.setSmartStart(0);
  netserver.requestOnChange(MODE, 0);
}

void AudioController::setError(const char *e){
  LOGE(T_Player, printf, "Some error:%s\n", e);
  //strlcpy(_plError, e, PLERR_LN);
  //if(hasError()) {
  //  config.setTitle(_plError);
  //}
}

void AudioController::_stop(){
  log_i("%s called", __func__);
  std::lock_guard<std::mutex> lock(_mtx);
  _status = STOPPED;
  audio.stopSong();
  setMute(true);

  // update stream's meta info
  audio_info_t info{ 0, T_n_a };
  EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::playerAudioInfo), &info, sizeof(info));
}

#ifndef PL_QUEUE_TICKS
  #define PL_QUEUE_TICKS 0
#endif
#ifndef PL_QUEUE_TICKS_ST
  #define PL_QUEUE_TICKS_ST 15
#endif
void AudioController::loop() {
  audio.loop();
  if(!audio.isRunning() && _status==PLAYING)    // todo: remove this and rely on internal logical state instead. It may lead to problems with async events
    _stop();

#ifdef MQTT_ROOT_TOPIC
  if(strlen(burl)>0){
    browseUrl();
  }
#endif
}

void AudioController::_play(uint16_t stationId) {
  // todo - this mutex here is taking too much, need to split this method into more atomic calls
  std::lock_guard<std::mutex> lock(_mtx);
  log_i("%s called, stationId=%d", __func__, stationId);
  LOGD(T_Player, printf, "_play:%u\n", stationId);

  setError("");
  //setDefaults();    this is private in a new version of ESP32-audioI2S lib
  remoteStationName = false;
  config.setDspOn(1);
  config.vuThreshold = 0;
  config.screensaverTicks=SCREENSAVERSTARTUPDELAY;
  config.screensaverPlayingTicks=SCREENSAVERSTARTUPDELAY;
  setMute(false);
  //config.setTitle(config.getMode()==PM_WEB?const_PlConnect:"");
  if(!config.loadStation(stationId)){
    LOGW(T_Player, printf, "Can't load station index:%u\n", stationId);
    return;
  }

  if(config.store.smartstart!=2)
    config.setSmartStart(0);
  bool isConnected = false;
  audio.stopSong();
  if(config.getMode()==PM_SDCARD && SDC_CS!=255){
    isConnected = false;
    // disable it for now
    //isConnected = audio.connecttoFS(sdman,config.station.url,config.sdResumePos==0?_resumeFilePos:config.sdResumePos - sd_min);
  } else {
    config.saveValue(&config.store.play_mode, static_cast<uint8_t>(PM_WEB));
  }
  // try to connect to remote host
  //if(config.getMode()==PM_WEB)

  if(isConnected){
  //if (config.store.play_mode==PM_WEB?connecttohost(config.station.url):connecttoFS(SD,config.station.url,config.sdResumePos==0?_resumeFilePos:config.sdResumePos-player.sd_min)) {
    _status = PLAYING;
    if(config.getMode()==PM_SDCARD) {
      config.sdResumePos = 0;
      config.saveValue(&config.store.lastSdStation, stationId);
    }
    if(config.store.smartstart!=2)
      config.setSmartStart(1);
    netserver.requestOnChange(MODE, 0);

    // notify that device has switched to 'webstream' playback mode
    int32_t d = e2int(evt::yo_state::webstream);
    EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::devMode), &d, sizeof(d));
  } else {
    //telnet.printf("##ERROR#:\tError connecting to %s\n", config.station.url);
    _stop();
  };
}

#ifdef MQTT_ROOT_TOPIC
void AudioController::browseUrl(){
  setError("");
  remoteStationName = true;
  config.setDspOn(1);
  resumeAfterUrl = _status==PLAYING;
  EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayPStop));
  EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayPStart));

//  setDefaults();
  config.setTitle(const_PlConnect);
  if (connecttohost(burl)){
    _status = PLAYING;
    config.setTitle("");
    netserver.requestOnChange(MODE, 0);
    EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayPStart));
  }else{
    //telnet.printf("##ERROR#:\tError connecting to %s\n", burl);
    SET_PLAY_ERROR("Error connecting to %s", burl);
    _stop(true);
  }
  memset(burl, 0, MQTT_BURL_SIZE);
}
#endif

void AudioController::prev() {
  if (_curStationIndex <= 1)
    return _play_station_from_playlist(_pls.getCount());

  _play_station_from_playlist(_curStationIndex - 1);
}

void AudioController::next() {
  if (_curStationIndex >= _pls.getCount())
    return _play_station_from_playlist(1);

  _play_station_from_playlist(_curStationIndex + 1);
}

void AudioController::toggle() {
  if (audio.isRunning()) {
    _stop();
  } else {
    _play_station_from_playlist(_curStationIndex);
  }
}

uint8_t AudioController::volume_level_adjustment(uint8_t volume) {
  return volume;
  // some kind of normalizer, in playlist it is all zero, who fills this values?
  //int vol = map(volume, 0, 254 - config.station.ovol * 3 , 0, 254);
  //if (vol > 254) vol = 254;
  //if (vol < 0) vol = 0;
  //return vol;
}

void AudioController::_loadValues() {
  // load volume/tone/balance value from nvs
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(T_Player, NVS_READONLY, &err);
  if (err != ESP_OK){
    LOGW(T_Player, println, "Can't load volume/EQ values");
    return;
  }
  // volume
  volume = -1;
  handle->get_item(T_volume, volume);
  // restore volume if any  pos value was set
  if (volume > -1)
    setDACVolume(volume_level_adjustment(volume));
  // balance
  balance = 0;
  handle->get_item(T_balance, balance);
  setDACBalance(balance);
  // EQ tone
  tone = {0, 0, 0};
  handle->get_blob(T_equalizer, &tone, sizeof(tone));
  setDACTone(tone.low, tone.band, tone.high);
  // restore last played station's playlist position
  handle->get_item(T_station, _curStationIndex);
}

void AudioController::setVolume(int32_t vol) {
  // since Audio lib takes uint8_t for volume, let's clamp it here anyway
  volume = clamp(vol, 0L, 255L);
  LOGI(T_Player, printf, "setVolume:%d\n", volume);
  setDACVolume(volume_level_adjustment(volume));
  // save volume value to nvs
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(T_Player, NVS_READWRITE, &err);
  if (err == ESP_OK){
    handle->set_item(T_volume, volume);
  }
  // publish volume change notification to event bus
  EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::audioVolume), &volume, sizeof(volume));
}

void AudioController::setBalance(int8_t bal){
  LOGI(T_Player, printf, "set Balance:%d\n", volume);
  setDACBalance(bal);
  // save volume value to nvs
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(T_Player, NVS_READWRITE, &err);
  if (err == ESP_OK){
    handle->set_item(T_balance, bal);
  }
  // publish balance change notification to event bus
  EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::audioBalance), &bal, sizeof(bal));
}

void AudioController::setTone(equalizer_tone_t t){
  LOGI(T_Player, printf, "set Tone low:%d, mid:%d, high:%d\n", tone.low, tone.band, tone.high);
  tone = t;
  setDACTone(tone.low, tone.band, tone.high);
  // save volume value to nvs
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(T_Player, NVS_READWRITE, &err);
  if (err == ESP_OK){
    handle->set_blob(T_balance, &tone, sizeof(tone));
  }
  // publish tone change notification to event bus
  EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::audioBalance), &tone, sizeof(tone));
}

void AudioController::_events_subsribe(){
  // command events
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_CMD_EVENTS, ESP_EVENT_ANY_ID,
    [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<AudioController*>(self)->_events_cmd_hndlr(id, data); },
    this, &_hdlr_cmd_evt
  );
}

void AudioController::_events_unsubsribe(){
  esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_CMD_EVENTS, ESP_EVENT_ANY_ID, _hdlr_cmd_evt);

}

void AudioController::_events_cmd_hndlr(int32_t id, void* data){
// process command events received via event loop bus
  switch (static_cast<evt::yo_event_t>(id)){

    // Play radio station from a playlist
    case evt::yo_event_t::playerStation : {
      _play_station_from_playlist(*reinterpret_cast<int32_t*>(data));
      break;
    }

    case evt::yo_event_t::playerStop :
      _stop();
      break;

    case evt::yo_event_t::playerToggle :
      toggle();
      break;

    case evt::yo_event_t::playerPrev :
      prev();
      break;

    case evt::yo_event_t::playerNext :
      next();
      break;

    case evt::yo_event_t::audioVolume :
      setVolume(*reinterpret_cast<int32_t*>(data));
      _embui_publish_audio_values();
      break;

    case evt::yo_event_t::audioBalance :
      setBalance(*reinterpret_cast<int8_t*>(data));
      _embui_publish_audio_values();
      break;

    case evt::yo_event_t::audioTone :
      setTone(*reinterpret_cast<equalizer_tone_t*>(data));
      _embui_publish_audio_values();
      break;

    default:;
  }
}

void AudioController::_play_station_from_playlist(int idx){
  if ( !_pls.switchTrack(abs(idx))){
    LOGW(T_Player, printf, "Can't switch playlist to station:%d\n", idx);
    return;
  }
  // stop previous playback if any
  audio.stopSong();
  setMute(false);
  // connect to a station
  if (audio.connecttohost(_pls.getURL())){
    // connection successfull, let's save new playlist position and send notifications
    _curStationIndex = abs(idx);
    esp_err_t err;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(T_Player, NVS_READONLY, &err);
    if (err == ESP_OK){
      handle->set_item(T_station, _curStationIndex);
    }
    int32_t d = e2int(evt::yo_state::webstream);
    EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::devMode), &d, sizeof(d));
    EVT_POST(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::playerPlay));
    // notify with a new playlist index
    EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::playerStation), &_curStationIndex, sizeof(_curStationIndex));    
  } else {
    // todo: handle error
  }
}

void AudioController::_embui_actions_register(){
  // process "player_*" messages from EmbUI
  embui.action.add(T_player__all, [this](Interface *interf, JsonVariantConst data, const char* action){ _embui_player_commands(interf, data, action); } );
 
}

void AudioController::_embui_actions_unregister(){
  embui.action.remove(T_player__all);
}

void AudioController::_embui_player_commands(Interface *interf, JsonVariantConst data, const char* action){
  // extract message action
  std::string_view a(action);
  a.remove_prefix(std::string_view(T_player__all).length()-1); // chop off prefix before '*'

  if(a.compare(T_prev) == 0) return prev();

  if(a.compare(T_next) == 0) return next();

  if(a.compare(T_toggle) == 0) return toggle();

  if(a.compare(T_volUp) == 0) return stepVolume(5);        // for now set the step to 5, todo: make con configurable

  if(a.compare(T_volDown) == 0) return stepVolume(-5);     // for now set the step to -5, todo: make con configurable

  if(a.compare(T_volume) == 0) return setVolume(data.as<int32_t>());     // volume slider

  if(a.compare(T_playstation) == 0) return _play_station_from_playlist(data.as<int>());

  if(a.compare(T_balance) == 0) return setBalance(data.as<int8_t>());

  if(a.compare(T_equalizer) == 0) return setTone(data[T_bass], data[T_middle], data[T_trebble]);
  // request for Volume/EQ values
  if(a.compare(T_getValues) == 0) return _embui_publish_audio_values(interf);
}

void AudioController::_embui_publish_audio_values(Interface* interf){
  // here now only WS publish, todo: MQTT

  // this method could be called with or without Interface object
  // with - when WebUI requests for data from it's side
  // without - when setting vol/EQ via internal methods and new values needs to be published to web UI
  // so I'll use double indirect here and temp object could be safely destroyed when going out of scope 
  std::unique_ptr<Interface> i;
  if (!interf){
    if (!embui.ws.count()) return;                  // do not publish anything if there are no active clients
    i = std::make_unique<Interface>(&embui.ws);     // only websocket publish, or else need to use `embui.feeders()`
    interf = i.get();
  }


  interf->json_frame_value();
  // vol
  std::string s(T_player_);      // common control's prefix, required for webUI's element ID's
  s.append(T_volume);
  interf->value(s, volume);

  // balance
  s = T_player_;
  s.append(T_balance);
  interf->value(s, balance);

  // tone
  s = T_player_;
  s.append(T_bass);
  interf->value(s, tone.low);
  s = T_player_;
  s.append(T_middle);
  interf->value(s, tone.band);
  s = T_player_;
  s.append(T_trebble);
  interf->value(s, tone.high);
  // send the data to WebUI
  interf->json_frame_flush();
}

void AudioController::_audio_cb_generic(const char* msg, audiolib::callback_type_t type){
  switch (type){
    case audiolib::callback_type_t::id3data :
      LOGI(T_Player, print, "id3data: ");
      LOG(println, msg);
      break;

    // track title
    case audiolib::callback_type_t::streamtitle : {
      LOGI(T_Player, print, "Stream title: ");
      LOG(println, msg);
      // copy by value including null terminator
      // todo: check if it is safe to pass by pointer
      EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::playerTrackTitle), msg, strlen(msg)+1);
    } break;

    // station title
    case audiolib::callback_type_t::station : {
      LOGI(T_Player, print, "Station title: ");
      LOG(println, msg);
      // copy by value including null terminator
      // todo: check if it is safe to pass by pointer
      EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::playerStationTitle), msg, strlen(msg)+1);
    } break;

    case audiolib::callback_type_t::bitrate : {
        LOGI(T_Player, print, "BitRate: ");
        LOG(println, msg);
        audio_info_t info{ audio.getBitRate() / 1000, audio.getCodecname() };
        EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::playerAudioInfo), &info, sizeof(info));
    } break;
/*
    case audiolib::callback_type_t::id3lyrics :
      break;
    case audiolib::callback_type_t::commercial :
      break;
    case audiolib::callback_type_t::icyurl :
      break;
    case audiolib::callback_type_t::icylogo :
      break;
    case audiolib::callback_type_t::icydescr :
      break;
    case audiolib::callback_type_t::lasthost :
      // passes connection URL
      break;
*/
    case audiolib::callback_type_t::eof :
      LOGI(T_Player, println, "End of file reached");
      break;

    default: {
      // default is just print the message
      LOGI(T_Player, printf, "Audio %u:", static_cast<size_t>(type));
      LOG(println, msg);
      #ifdef USE_NEXTION
        //nextion.audioinfo(info);    // nextion print debug messages???
      #endif
    }

  }

}



//  **************************
//  *** ESP32 DAC ***
//  **************************
void ESP32_I2S_Generic::init(){
  if (_mute_gpio != -1){
    pinMode(_mute_gpio, OUTPUT);
  }
  AudioController::init();
};

void ESP32_I2S_Generic::setMute(bool mute){
  if (_mute_gpio == -1){
    // soft mute
    // save current DAC volume
    if (mute){
      _soft_mute_volume = getDACVolume();
      setDACVolume(0);
    } else {
      // this could be 0 when boot-start
      if (_soft_mute_volume) setDACVolume(_soft_mute_volume);
    }
    _mute_state = mute;
  } else {
    digitalWrite(_mute_gpio, mute ? _mute_level : !_mute_level);
  } 
}

// get mute state
bool ESP32_I2S_Generic::getMute() const {
  if (_mute_gpio == -1){
    // soft mute
    return _mute_state;
  } else {
    return digitalRead(_mute_gpio);
  } 
}

//  **************************
//  *** ES8311 DAC         ***
//  **************************

void ES8311Audio::init(){
  if(!_es.begin(I2C_SDA, I2C_SCL, 400000)){
    log_e("ES8311 begin failed");
    return;
  }
  _es.setBitsPerSample(16);
  // set program vol to max
  pinMode(_mute_gpio, OUTPUT);
  AudioController::init();
  audio.setVolume(100);     // 100 is adjusted scale
}

void ES8311Audio::setMute(bool mute){
  digitalWrite(_mute_gpio, mute ? LOW : HIGH);    // NS4150's mute level is LOW
  _mute_state = mute;
}


// ********* Playlist ***********

void RadioPlaylist::loadIndex(){
  File playlist = LittleFS.open(PLAYLIST_PATH);
  if (!playlist) {
    return;
  }
  while (playlist.available()) {
    _index.push_back(playlist.position());
    LOGV(T_Player, printf, "pls position %u:%u\n", _index.size(), _index.back());
    playlist.find('\n');
  }
  LOGI(T_Player, printf, "Loaded %u items in playlist index\n", _index.size());
}

bool RadioPlaylist::switchTrack(size_t t){
  // o(zero) track is not possible, we count from 1
  if (!t || t > _index.size())
    return false;

  File playlist = LittleFS.open(PLAYLIST_PATH);

  playlist.seek(_index.at(t-1), SeekSet);

  String l(playlist.readStringUntil('\n'));   // this readStringUntil is very inefficient
  std::string_view line(l.c_str());
  //LOGD(T_Player, printf, "parse pls line:%s\n", line);

  auto lenTitle = line.find_first_of("\t");
  if (lenTitle == std::string_view::npos) return false;
  _title = line.substr(0, lenTitle);

  line.remove_prefix(lenTitle+1);
  auto lenURL = line.find_first_of("\t");
  if (lenURL == std::string_view::npos) return false;

  _url = line.substr(0, lenURL);
  _pos = t;
  LOGI(T_Player, printf, "Swith pls to: %u. %s - %s\n", t, _title.c_str(), _url.c_str());
  return true;
}
