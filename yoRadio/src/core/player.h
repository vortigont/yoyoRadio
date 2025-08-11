#ifndef player_h
#define player_h
#include "options.h"
#include <mutex>

#if I2S_DOUT!=255 || I2S_INTERNAL
  #include "Audio.h"
  #include "es8311.h"
#else
  #include "../audioVS1053/audioVS1053Ex.h"
#endif

#ifndef MQTT_BURL_SIZE
  #define MQTT_BURL_SIZE  512
#endif

#ifndef PLQ_SEND_DELAY
	#define PLQ_SEND_DELAY portMAX_DELAY
#endif

#define PLERR_LN        64
#define SET_PLAY_ERROR(...) {char buff[512 + 64]; sprintf(buff,__VA_ARGS__); setError(buff);}

enum class dac_type_t {
  generic = 0,
  ES8311
};

enum plStatus_e : uint8_t{ PLAYING = 1, STOPPED = 2 };

class Player: public Audio {

    uint32_t    _volTicks;   /* delayed volume save  */
    bool        _volTimer;   /* delayed volume save  */
    uint32_t    _resumeFilePos;
    plStatus_e  _status;
    char        _plError[PLERR_LN];

    void _stop(bool alreadyStopped = false);
    void _play(uint16_t stationId);
    void _loadVol(uint8_t volume);

protected:
  virtual void dac_init();

public:
    bool lockOutput = true;
    bool resumeAfterUrl = false;
    uint32_t sd_min, sd_max;
    #ifdef MQTT_ROOT_TOPIC
    char      burl[MQTT_BURL_SIZE];  /* buffer for browseUrl  */
    #endif

    Player();
    virtual ~Player();

    virtual void init();
    void loop();
    void initHeaders(const char *file);
    void setError(const char *e);
    bool hasError() { return strlen(_plError)>0; }
    #ifdef MQTT_ROOT_TOPIC
    void browseUrl();
    #endif
    bool remoteStationName = false;
    plStatus_e status() { return _status; }
    void prev();
    void next();
    void toggle();
    void stepVol(bool up);
    void setVol(int32_t volume);
    uint8_t volToI2S(uint8_t volume);
    void stopInfo();
    void setOutputPins(bool isPlaying);
    void setResumeFilePos(uint32_t pos) { _resumeFilePos = pos; }


private:
    std::mutex _mtx;

    /**
     * @brief play station from a playlist by index
     * index is resolved via Config instance
     * 
     * @param idx 
     */
    void _play_station_from_playlist(int idx);

    // event function handlers
    esp_event_handler_instance_t _hdlr_cmd_evt{nullptr};

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
    void _events_cmd_hndlr(int32_t id, void* data);
};


class PlayerES8311 : public Player {
  ES8311 _es;
  void dac_init() override;

  public:
  PlayerES8311() = default;
  void init() override;

};

// ******************
extern Player* player;

extern __attribute__((weak)) void player_on_start_play();
extern __attribute__((weak)) void player_on_stop_play();
extern __attribute__((weak)) void player_on_track_change();
extern __attribute__((weak)) void player_on_station_change();

void create_player(dac_type_t dac);

#endif
