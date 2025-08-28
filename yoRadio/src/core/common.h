#ifndef COMMON_H
#define COMMON_H
#include <cstdint>

#define PLAYLIST_PATH     "/data/playlist.csv"
#define SSIDS_PATH        "/data/wifi.csv"
#define TMP_PATH          "/data/tmpfile.txt"
#define INDEX_PATH        "/data/index.dat"

#define PLAYLIST_SD_PATH     "/data/playlistsd.csv"
#define INDEX_SD_PATH        "/data/indexsd.dat"

enum displayMode_e { PLAYER, VOL, STATIONS, NUMBERS, LOST, UPDATING, INFO, SETTINGS, TIMEZONE, WIFI, CLEAR, SLEEPING, SDCHANGE, SCREENSAVER, SCREENBLANK };
//enum class pages_e : uint8_t  { PG_PLAYER=0, PG_DIALOG=1, PG_PLAYLIST=2, PG_SCREENSAVER=3 };

enum displayRequestType_e { BOOTSTRING, NEWMODE, NEWTITLE, NEWSTATION, NEXTSTATION, DRAWPLAYLIST, DRAWVOL, DBITRATE, AUDIOINFO, SHOWVUMETER, DSPRSSI, SHOWWEATHER, NEWWEATHER, PSTOP, PSTART, DSP_START, WAITFORSD, SDFILEINDEX, NEWIP, NOPE };
struct requestParams_t
{
  displayRequestType_e type;
  int payload;
};

enum controlEvt_e { EVT_NONE=255, EVT_BTNLEFT=0, EVT_BTNCENTER=1, EVT_BTNRIGHT=2, EVT_ENCBTNB=3, EVT_BTNUP=4, EVT_BTNDOWN=5, EVT_ENC2BTNB=6, EVT_BTNMODE=7 };


// structs that could be used as event payload

/**
 * @brief playing stream/data meta
 * 
 */
struct audio_info_t {
  uint32_t bitRate;
  const char *codecName;
};

// Audiolib equalizer values
struct equalizer_tone_t {
  int8_t low, band, high;
};



// a simple constrain function
template<typename T>
T clamp(T value, T min, T max){
  return (value < min)? min : (value > max)? max : value;
}

#endif