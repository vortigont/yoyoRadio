#ifndef player_h
#define player_h
#include "options.h"
#include <mutex>
#include "Audio.h"
#include "es8311.h"
#include "EmbUI.h"

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
  esp32internal,
  ES8311
};

enum plStatus_e : uint8_t{ PLAYING = 1, STOPPED = 2 };


/**
 * @brief generic class to manage Audio lib
 * it connects Audio lib with playlists, manages states, sends / receives control messages via event bus
 * HW specific implementations could be implemented in derived classes, i.e. for amplifiers control, etc...
 * 
 */
class AudioController {
    uint32_t    _resumeFilePos;
    plStatus_e  _status;
    char        _plError[PLERR_LN];

    void _stop(bool alreadyStopped = false);
    void _play(uint16_t stationId);
    // restore volume value from nvram
    void _loadVol();

protected:
  // audio processing obj
  Audio audio;
  // current raw volume value (unscaled), i.e. one that is supplied from external calls
  int32_t volume;

  /**
   * @brief set DAC volume
   * must be implemented in derived class
   * 
   * @param vol 
   * @param curve 
   */
  virtual void setDACVolume(uint8_t vol, uint8_t curve = 0) = 0;
  virtual uint8_t getDACVolume() = 0;

  /**
   * @brief adjusts station's volume according to playlist's correction value
   * i.e. poor man's normalizer
   * 
   * @param volume 
   * @return uint8_t 
   */
  uint8_t volume_level_adjustment(uint8_t volume);

public:
    bool resumeAfterUrl = false;
    uint32_t sd_min, sd_max;
    #ifdef MQTT_ROOT_TOPIC
    char      burl[MQTT_BURL_SIZE];  /* buffer for browseUrl  */
    #endif

    AudioController() = default;
    virtual ~AudioController();

    // virtual methods
    virtual void init();
    // volume, tembre control - maps to Audio lib methods by default
    virtual void setBalance(int8_t bal = 0){ audio.setBalance(bal); };
    virtual void setTone(int8_t low, int8_t band, int8_t high){ audio.setTone(low, band, high); }

    /**
     * @brief Set player's Volume
     * @note volume range is 0-255 (defined by Audio lib)
     * @note saves applied volume value to nvram
     * @param vol 
     */
    void setVolume(int32_t vol);

    /**
     * @brief Get player's Volume
     * should do reverse scaling (not implemented yet)
     * 
     * @return int32_t 
     */
    int32_t getVolume() { return getDACVolume(); };

    /**
     * @brief step adjust volume up or down
     * 
     * @param step value to increment/decrement volume
     */
    void stepVolume(int32_t step){ setVolume( volume + step); };
    
    //virtual void setVolumeSteps(uint8_t steps){ audio.setVolumeSteps(steps); };

    /**
     * @brief mute DAC/amplifier
     * 
     * @param mute 
     */
    virtual void setMute(bool mute) = 0;
    // get mute state
    virtual bool getMute() const = 0;

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


    void stopInfo();
    void setResumeFilePos(uint32_t pos) { _resumeFilePos = pos; }

    // publishing methods (a hack due to missing callbacks in Audio lib)
    void pubCodecInfo();
    // needed in Config::changeMode for some reason
    bool isRunning(){ return audio.isRunning(); }
    bool setFilePos(uint32_t pos){ return audio.setFilePos(pos); };
    // needed in src/core/netserver.cpp
    uint32_t getFileSize(){ return audio.getFileSize(); };
    uint32_t getAudioCurrentTime(){ return audio.getAudioCurrentTime(); };
    uint32_t getAudioFileDuration(){ return getAudioFileDuration(); };



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

    // EmbUI handlers
    void _embui_actions_register();
    void _embui_actions_unregister();
    // process "player_*" messages from EmbUI
    void _embui_player_commands(Interface *interf, JsonVariantConst data, const char* action);
};

// generic I2S DAC for ESP32 with software volume control
class ESP32_I2S_Generic : public AudioController {
  int32_t _mute_gpio;
  bool _mute_level, _mute_state; // soft mute state
  // stashed volume value for software mute (if no mute gpio defined)
  uint8_t _soft_mute_volume;

  void setDACVolume(uint8_t vol, uint8_t curve = 0) override { audio.setVolume( vol, curve); }
  uint8_t getDACVolume() override { return audio.getVolume(); }

public:
  ESP32_I2S_Generic(int32_t mute_gpio = -1, bool mute_level = HIGH) : _mute_gpio(mute_gpio), _mute_level(mute_level) {};
  ESP32_I2S_Generic(int32_t bclk, int32_t lcr,  int32_t dout, int32_t mclk = -1, int32_t mute_gpio = -1, bool mute_level = HIGH) : _mute_gpio(mute_gpio), _mute_level(mute_level) { audio.setPinout(bclk, lcr, dout, mclk); };

  void init() override;

  void setMute(bool mute) override;
  // get mute state
  bool getMute() const override;
};

// ES8311 chip DAC with volume control over i2c
class ES8311Audio : public AudioController {
  int32_t _mute_gpio;
  bool _mute_state; // mute state
  ES8311 _es;

  void setDACVolume(uint8_t vol, uint8_t curve = 0) override { _es.setVolume(vol); };
  uint8_t getDACVolume() override { return _es.getVolume(); }

public:
  ES8311Audio(int32_t bclk, int32_t lcr,  int32_t dout, int32_t mclk, int32_t mute_gpio) : _mute_gpio(mute_gpio) { audio.setPinout(bclk, lcr, dout, mclk); };

  void init() override;

  void setMute(bool mute) override;
  // get mute state
  bool getMute() const override { return _mute_state; };
};

// ******************
extern AudioController* player;

extern __attribute__((weak)) void player_on_start_play();
extern __attribute__((weak)) void player_on_stop_play();
extern __attribute__((weak)) void player_on_track_change();
extern __attribute__((weak)) void player_on_station_change();

void create_player(dac_type_t dac);

#endif
