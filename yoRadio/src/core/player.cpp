#include <string_view>
#include "options.h"
#include "player.h"
#include "config.h"
#include "../displays/dspcore.h"
#include "locale/l10n.h"
#include "sdmanager.h"
#include "netserver.h"
#include "evtloop.h"
#include "templates.hpp"
#include "log.h"


AudioController* player{nullptr};

AudioController::~AudioController(){
  _events_unsubsribe();
}


void create_player(dac_type_t dac){
  switch (dac){
    case   dac_type_t::esp32internal :
      player = new ESP32_Internal_DAC();
      break;
    case   dac_type_t::ES8311 :
      player = new ES8311Audio();
      break;
    default:
      player = new ESP32_I2S_Generic();
  }
}


void AudioController::init() {
  Serial.print("##[BOOT]#\tplayer.init\t");
  _resumeFilePos = 0;
  memset(_plError, 0, PLERR_LN);
  #ifdef MQTT_ROOT_TOPIC
  memset(burl, 0, MQTT_BURL_SIZE);
  #endif
  if(MUTE_PIN!=255)
    pinMode(MUTE_PIN, OUTPUT);
  setOutputPins(false);
  dac_init();
  setBalance(config.store.balance);
  setTone(config.store.bass, config.store.middle, config.store.trebble);
  _status = STOPPED;
  _volTimer=false;
  //randomSeed(analogRead(0));
  #if PLAYER_FORCE_MONO
    forceMono(true);
  #endif
  _loadVol(40);  // for now
  //_loadVol(config.store.volume);
  audio.setConnectionTimeout(1700, 3700);
  // event bus subscription
  _events_subsribe();
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

void AudioController::_stop(bool alreadyStopped){
  log_i("%s called", __func__);
  std::lock_guard<std::mutex> lock(_mtx);
/*
  if(config.getMode()==PM_SDCARD && !alreadyStopped)
    config.sdResumePos = player->getFilePos();  // getFilePos() is obsolete
*/
  _status = STOPPED;
  audio.stopSong();
  setOutputPins(false);

  // update stream's meta info
  evt::audio_into_t info{ 0, audio.getCodecname() };
  EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::playerAudioInfo), &info, sizeof(info));
  EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayPStop));

  //setDefaults();
  if(!lockOutput) stopInfo();
  if (player_on_stop_play) player_on_stop_play();
  pm.on_stop_play();
}

void AudioController::initHeaders(const char *file) {
  if(strlen(file)==0 || true) return; //TODO Read TAGs
  //audio.connecttoFS(sdman,file);
  //eofHeader = false;
  //while(!eofHeader) Audio::loop();
  //netserver.requestOnChange(SDPOS, 0);
  //setDefaults();
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
    _stop(true);

  // some volume save timer
  if(_volTimer){
    if((millis()-_volTicks)>3000){
      config.saveVolume();
      _volTimer=false;
    }
  }
#ifdef MQTT_ROOT_TOPIC
  if(strlen(burl)>0){
    browseUrl();
  }
#endif
}

void AudioController::setOutputPins(bool isPlaying) {
  if(REAL_LEDBUILTIN!=255) digitalWrite(REAL_LEDBUILTIN, LED_INVERT?!isPlaying:isPlaying);
  bool _ml = MUTE_LOCK ? !MUTE_VAL : ( isPlaying ? !MUTE_VAL : MUTE_VAL );
  if(MUTE_PIN!=255) digitalWrite(MUTE_PIN, _ml);
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
  if(config.getMode()!=PM_SDCARD) {
    EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayPStop));
  }
  setOutputPins(false);
  //config.setTitle(config.getMode()==PM_WEB?const_PlConnect:"");
  if(!config.loadStation(stationId)) return;
  //config.setTitle(config.getMode() == PM_WEB ? const_PlConnect:"[next track]");
  
  //_loadVol(config.store.volume);

  //EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayNewStation));

  netserver.requestOnChange(STATION, 0);
  // todo: why do this netserver loop calls from here??? it's a wrong thread to execute without proper locking!
  //netserver.loop();
  //netserver.loop();
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
  if(config.getMode()==PM_WEB)
    isConnected = audio.connecttohost(config.station.url);

  if(isConnected){
  //if (config.store.play_mode==PM_WEB?connecttohost(config.station.url):connecttoFS(SD,config.station.url,config.sdResumePos==0?_resumeFilePos:config.sdResumePos-player.sd_min)) {
    _status = PLAYING;
    if(config.getMode()==PM_SDCARD) {
      config.sdResumePos = 0;
      config.saveValue(&config.store.lastSdStation, stationId);
    }
    //config.setTitle("");
    if(config.store.smartstart!=2)
      config.setSmartStart(1);
    netserver.requestOnChange(MODE, 0);
    setOutputPins(true);

    // notify that device has switched to 'webstream' playback mode
    int32_t d = e2int(evt::yo_state::webstream);
    EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::devMode), &d, sizeof(d));

    if (player_on_start_play)
      player_on_start_play();
    pm.on_start_play();
  } else {
    //telnet.printf("##ERROR#:\tError connecting to %s\n", config.station.url);
    SET_PLAY_ERROR("Error connecting to %s", config.station.url);
    _stop(true);
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
  setOutputPins(false);
  config.setTitle(const_PlConnect);
  if (connecttohost(burl)){
    _status = PLAYING;
    config.setTitle("");
    netserver.requestOnChange(MODE, 0);
    setOutputPins(true);
    EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayPStart));
    if (player_on_start_play) player_on_start_play();
    pm.on_start_play();
  }else{
    //telnet.printf("##ERROR#:\tError connecting to %s\n", burl);
    SET_PLAY_ERROR("Error connecting to %s", burl);
    _stop(true);
  }
  memset(burl, 0, MQTT_BURL_SIZE);
}
#endif

void AudioController::prev() {
  uint16_t lastStation = config.lastStation();
  if(config.getMode()==PM_WEB || !config.store.sdsnuffle){;
    if (lastStation == 1)
      config.lastStation(config.playlistLength());
    else
      config.lastStation(lastStation - 1);
  }
  _play_station_from_playlist(config.lastStation());
}

void AudioController::next() {
  uint16_t lastStation = config.lastStation();
  if(config.getMode()==PM_WEB || !config.store.sdsnuffle){
    if (lastStation == config.playlistLength())
      config.lastStation(1);
    else
      config.lastStation(lastStation + 1);
  } else {
    config.lastStation(random(1, config.playlistLength()));
  }
  _play_station_from_playlist(config.lastStation());
}

void AudioController::toggle() {
  if (_status == PLAYING) {
    _stop();
  } else {
    _play_station_from_playlist(config.lastStation());
  }
}

void AudioController::stepVol(bool up) {
  if (up) {
    if (config.store.volume <= 254 - config.store.volsteps) {
      setVolume(config.store.volume + config.store.volsteps);
    }else{
      setVolume(254);
    }
  } else {
    if (config.store.volume >= config.store.volsteps) {
      setVolume(config.store.volume - config.store.volsteps);
    }else{
      setVolume(0);
    }
  }
}

uint8_t AudioController::volume_level_adjustment(uint8_t volume) {
  // some kind of normalizer ?
  int vol = map(volume, 0, 254 - config.station.ovol * 3 , 0, 254);
  if (vol > 254) vol = 254;
  if (vol < 0) vol = 0;
  return vol;
}

void AudioController::_loadVol(uint8_t volume) {
  setDACVolume(volume_level_adjustment(volume));
}

void AudioController::setVolume(int32_t volume) {
  LOGD(T_Player, printf, "setVol:%d\n", volume);
  _volTicks = millis();
  _volTimer = true;
  config.setVolume(volume);
  setDACVolume(volume_level_adjustment(volume));
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
  switch (static_cast<evt::yo_event_t>(id)){

    // Play radio station from a playlist
    case evt::yo_event_t::plsStation : {
      _play_station_from_playlist(*reinterpret_cast<int32_t*>(data));
      break;
    }

    // Stop player
    case evt::yo_event_t::playerStop :
      _stop();
      break;

    // volume control
    case evt::yo_event_t::playerVolume :
      setVolume(*reinterpret_cast<int32_t*>(data));
      break;

    default:;
  }
}

void AudioController::_play_station_from_playlist(int idx){
  if (idx > 0)
    config.setLastStation(idx);

   _play(abs(idx));

  EVT_POST(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::playerPlay));
  if (player_on_station_change)   // todo: this should be moved to event handling
    player_on_station_change();
  pm.on_station_change();   // todo: this should be moved to event handling  
}

void AudioController::pubCodecInfo(){
  evt::audio_into_t info{ audio.getBitRate() / 1000, audio.getCodecname() };
  EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::playerAudioInfo), &info, sizeof(info));
}


//  **************************
//  *** ESP32_Internal_DAC ***
//  **************************


//  **************************
//  *** ES8311 DAC         ***
//  **************************

void ES8311Audio::init(){
  if(!_es.begin(I2C_SDA, I2C_SCL, 400000)){
    log_e("ES8311 begin failed");
    return;
  }    
  AudioController::init();
  _es.setBitsPerSample(16);
  // set program vol to max
  audio.setVolume(255);
}



//=============================================//
//              Audio handlers                 //
//=============================================//

void audio_info(const char *info) {
  if(player->lockOutput) return;
  LOGI(T_Player, printf, "Audio info:\t%s\n", info);
  //if(config.store.audioinfo) telnet.printf("##AUDIO.INFO#: %s\n", info);
  #ifdef USE_NEXTION
    nextion.audioinfo(info);
  #endif

  /*
    'BitsPerSample' string is a part of Audio::showCodecParams(), it is called when all playback params are already known
    so we can use it as a generic trigget to update metadata
  */
  if (strstr(info, "BitsPerSample")  != NULL){
    // a hackish call
    player->pubCodecInfo();
  }

  if (strstr(info, "Account already in use") != NULL || strstr(info, "HTTP/1.0 401") != NULL) {
    player->setError(info);
  }
/*
  if (strstr(info, "format is aac")  != NULL) { config.setBitrateFormat(BF_AAC); EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayDrawBitRatte)); }
  if (strstr(info, "format is flac") != NULL) { config.setBitrateFormat(BF_FLAC); EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayDrawBitRatte)); }
  if (strstr(info, "format is mp3")  != NULL) { config.setBitrateFormat(BF_MP3); EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayDrawBitRatte)); }
  if (strstr(info, "format is wav")  != NULL) { config.setBitrateFormat(BF_WAV); EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::displayDrawBitRatte)); }
  char* ici; char b[20]={0};
  if ((ici = strstr(info, "BitRate: ")) != NULL) {
    strlcpy(b, ici + 9, 50);
    audio_bitrate(b);
  }
*/
}
/*
void audio_bitrate(const char *info)
{
  if(config.store.audioinfo) telnet.printf("%s %s\n", "##AUDIO.BITRATE#:", info);
  int bitrate = atoi(info) / 1000;
  EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::playerBitRatte), &bitrate, sizeof(bitrate));
}
*/
bool printable(const char *info) {
  if(L10N_LANGUAGE!=RU) return true;
  bool p = true;
  for (int c = 0; c < strlen(info); c++)
  {
    if ((uint8_t)info[c] > 0x7e || (uint8_t)info[c] < 0x20) p = false;
  }
  if (!p) p = (uint8_t)info[0] >= 0xC2 && (uint8_t)info[1] >= 0x80 && (uint8_t)info[1] <= 0xBF;
  return p;
}

void audio_showstation(const char *info) {
  LOGI(T_Player, printf, "Station title: %s\n", info);
  // copy by value including null terminator
  EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::playerStationTitle), info, strlen(info)+1);
}

void audio_showstreamtitle(const char *info) {
  LOGI(T_Player, printf, "Stream title: %s\n", info);
  EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::playerTrackTitle), info, strlen(info)+1);
}

void audio_error(const char *info) {
  //config.setTitle(info);
  player->setError(info);
  LOGE(T_Player, println, info);
  //telnet.printf("##ERROR#:\t%s\n", info);
}

void audio_id3artist(const char *info){
  LOGI(T_Player, printf, "id3artist: %s\n", info);
  //if(printable(info)) config.setStation(info);
  //display->putRequest(NEWSTATION);
  //netserver.requestOnChange(STATION, 0);
}

void audio_id3album(const char *info){
  LOGI(T_Player, printf, "id3album: %s\n", info);
}

void audio_id3title(const char *info){
  LOGI(T_Player, printf, "id3title: %s\n", info);
  //audio_id3album(info);
}

void audio_beginSDread(){
  //config.setTitle("n/a");
}

void audio_id3data(const char *info){  //id3 metadata
  LOGI(T_Player, printf, "id3data: %s\n", info);
  //telnet.printf("##AUDIO.ID3#: %s\n", info);
}

void audio_eof_mp3(const char *info){  //end of file
  config.sdResumePos = 0;
  player->next();
}

void audio_eof_stream(const char *info){
  if(!player->resumeAfterUrl){
    EVT_POST(YO_CMD_EVENTS, e2int(evt::yo_event_t::playerStop));
    return;
  }
  if (config.getMode()!=PM_WEB){
    player->setResumeFilePos( config.sdResumePos==0?0:config.sdResumePos - player->sd_min);
  }

  auto v = config.lastStation();
  EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::plsStation), &v, sizeof(v));
}

void audio_progress(uint32_t startpos, uint32_t endpos){
  player->sd_min = startpos;
  player->sd_max = endpos;
  netserver.requestOnChange(SDLEN, 0);
}
