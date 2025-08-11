#ifndef player_h
#define player_h
#include "options.h"
#include <mutex>
#include "Audio.h"
#include "es8311.h"

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

    uint32_t    _volTicks;   /* delayed volume save  */
    bool        _volTimer;   /* delayed volume save  */
    uint32_t    _resumeFilePos;
    plStatus_e  _status;
    char        _plError[PLERR_LN];

    void _stop(bool alreadyStopped = false);
    void _play(uint16_t stationId);
    // restore volume value from nvram
    void _loadVol(uint8_t volume);

protected:
  Audio audio;
  /**
   * @brief DAC initialization, procedure
   * must be implemented in derived classes
   */
  virtual void dac_init() = 0;
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
    bool lockOutput = true;
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
    
    //virtual void setVolumeSteps(uint8_t steps){ audio.setVolumeSteps(steps); };


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

    void stopInfo();
    void setOutputPins(bool isPlaying);
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
};

// 
/**
 * @brief ESP32 internal DAC
 * @note only for classic ESP32 boards! Not applicable for S2/S3 and newer
 * 
 */
class ESP32_Internal_DAC : public AudioController {
  void dac_init() override {};
  void setDACVolume(uint8_t vol, uint8_t curve = 0) override { audio.setVolume( vol, curve); }
  uint8_t getDACVolume() override { return audio.getVolume(); }

public:
  ESP32_Internal_DAC() = default;
};

// generic I2S DAC for ESP32 with software volume control
class ESP32_I2S_Generic : public AudioController {
  void dac_init() override { audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT); };
  void setDACVolume(uint8_t vol, uint8_t curve = 0) override { audio.setVolume( vol, curve); }
  uint8_t getDACVolume() override { return audio.getVolume(); }

public:
  ESP32_I2S_Generic() = default;
};

// ES8311 chip DAC with volume control over i2c
class ES8311Audio : public AudioController {
  ES8311 _es;

  void dac_init() override { audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT, I2S_MCLK); };
  void setDACVolume(uint8_t vol, uint8_t curve = 0) override { _es.setVolume(vol); };
  uint8_t getDACVolume() override { return _es.getVolume(); }

public:
  ES8311Audio() = default;
  void init() override;
};

// ******************
extern AudioController* player;

extern __attribute__((weak)) void player_on_start_play();
extern __attribute__((weak)) void player_on_stop_play();
extern __attribute__((weak)) void player_on_track_change();
extern __attribute__((weak)) void player_on_station_change();

void create_player(dac_type_t dac);

#endif
