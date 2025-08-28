#include "nvs_handle.hpp"
#include "components.hpp"
#include "displays/dspcore.h"
// device defines
#include "devboards/JC3248W535.hpp"
#include "devboards/JC1060P470C.hpp"
// presets
#include "widgets/presets.hpp"
#include "core/log.h"

#define   MAX_BOOT_FAIL_CNT   5

static constexpr const char* T_dev_JC3248W535         = "JC3248W535";     //  Guition JC3248W535
static constexpr const char* T_dev_JC1060P470         = "JC1060P470";     //  Guition JC1060P470 ESP32-P4

// var to keep boot couter across reboots
RTC_DATA_ATTR int setupCnt = 0;

// Audio player controller
AudioController* player{nullptr};
// Display
Display* display{nullptr};
DisplayControl* dctrl{nullptr};
// ArduinoGFX objects
Arduino_DataBus* bus{nullptr};
Arduino_GFX* agfx{nullptr};


// Module Manager instance
ModuleManager zookeeper;



void load_hwcomponets_configuration(){
  ++setupCnt;
  
  if (setupCnt > MAX_BOOT_FAIL_CNT){
    // looks like a boot loop - abort hw setup
    esp_err_t err;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(T_devcfg, NVS_READWRITE, &err);
    // erase NVS namespace
    if (err == ESP_OK)
      handle->erase_all();

    LOGE(T_profile, printf, "Bootloop count:%u, aborting device configuration, pls login to WebIU and revise your setup", setupCnt);
    return;
  }
  
  
  load_device_profile_from_NVS();
  
  // load config from json TODO
  
  // reset boot counter if reached here, todo: set a timer here
  setupCnt = 0;
}

void load_device_profile_from_NVS(){
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(T_devcfg, NVS_READONLY, &err);

  if (err != ESP_OK) return;  // no NVS - no profiles

  size_t len{0};
  handle->get_item_size(nvs::ItemType::ANY, T_profile, len);
  if (!len) return;
  
  char profile[len];
  handle->get_string(T_profile, profile, len);
  LOGI(T_devcfg, printf, "Loading device profile: '%s'\n", profile);
  std::string_view sv(profile);
  if (sv.compare(T_dev_JC3248W535) == 0) return load_device_JC3248W535();
  if (sv.compare(T_dev_JC1060P470) == 0) return load_device_JC1060P470();
}

void load_device_JC3248W535(){
  LOGD(T_devcfg, println, "Creating devices for JC3248W535");
  // JC3248W535 uses generic esp32 DAC
  player = new ESP32_I2S_Generic(JC3248W535::i2s.bclk, JC3248W535::i2s.lrclk, JC3248W535::i2s.dout, JC3248W535::i2s.mclk);
  player->init();

  // Display
  // create display hw controller
  dctrl = new DisplayControl_AGFX_PWM(JC3248W535::display.backlight, JC3248W535::display.backlight_level, agfx);
  dctrl->init();

  // create Agfx object
  agfx = JC3248W535::create_display_dev(JC3248W535::display, bus);

  // link the above into Display object
  display = new DisplayGFX(agfx);
  // Init the display UI
  display->init();

  // apply widget preset for 320x240
  display->load_main_preset(display_320x480::cfg1);
}

void load_device_JC1060P470(){
  LOGD(T_devcfg, println, "Creating devices for JC1060P470");
  // JC1060P470 uses ES8311 DAC chip
  player = new ES8311Audio(JC1060P470::i2s.bclk, JC1060P470::i2s.lrclk, JC1060P470::i2s.dout, JC1060P470::i2s.mclk, JC1060P470::i2s.mute);
  player->init();

  // Display

  // Create and init display device controller
  dctrl = new DisplayControl_AGFX_PWM(JC1060P470::display.backlight, JC1060P470::display.backlight_level, agfx);
  dctrl->init();

  // create Agfx object
  agfx = JC1060P470::create_display_dev(JC1060P470::display);
    // link the above into Display object
  display = new DisplayGFX(agfx);
  // Init the display UI
  display->init();
  
  // apply widget preset for 1024x600
  display->load_main_preset(display_1024x600::cfg1);
}
