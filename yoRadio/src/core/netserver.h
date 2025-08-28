#ifndef netserver_h
#define netserver_h
#include "ArduinoJson.h"
#include "ESPAsyncWebServer.h"

enum requestType_e : uint8_t  { PLAYLIST=1, STATION=2, STATIONNAME=3, ITEM=4, TITLE=5, VOLUME=6, NRSSI=7, MODE=9, EQUALIZER=10, BALANCE=11, PLAYLISTSAVED=12, STARTUP=13, GETINDEX=14, GETACTIVE=15, GETSYSTEM=16, GETSCREEN=17, GETTIMEZONE=18, GETWEATHER=19, GETCONTROLS=20, DSPON=21, SDPOS=22, SDLEN=23, SDSNUFFLE=24, SDINIT=25, GETPLAYERMODE=26, CHANGEMODE=27 };
enum import_e      : uint8_t  { IMDONE=0, IMPL=1, IMWIFI=2 };

// set handlers for embedded resources
void set_static_http_handlers();

class NetServer {
  private:
    bool resumePlay;
    char chunkedPathBuffer[40];
  public:
    NetServer() {};
    ~NetServer();

    bool begin(bool quiet=false);
    void loop();

  import_e importRequest;

  private:
    requestType_e request;
    void getPlaylist(uint8_t clientId);
    bool importPlaylist();
    int _readPlaylistLine(File &file, char * line, size_t size);

    void _send_ws_message(JsonVariantConst v, int32_t clientId = 0);

    // event function handlers
    //esp_event_handler_instance_t _hdlr_cmd_evt{nullptr};
    esp_event_handler_instance_t _hdlr_chg_evt{nullptr};

    /**
     * @brief subscribe to event mesage bus
     * 
     */
    void _events_subsribe();

    /**
     * @brief unregister from event loop
     * 
     */
    void _events_unsubsribe();

    // command events handler
    //void _events_cmd_hndlr(int32_t id, void* data);

    // state change events handler
    void _events_chg_hndlr(int32_t id, void* data);
};

extern NetServer netserver;

#endif
