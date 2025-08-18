#include "Arduino.h"
#include "core/options.h"
#include "core/config.h"
#include "core/player.h"
#include "displays/dspcore.h"
#include "core/network.h"
#include "core/netserver.h"
#include "core/controls.h"
#include "core/mqtt.h"
#include "core/optionschecker.h"
#include "core/evtloop.h"
#include "core/log.h"

#if DSP_HSPI || TS_HSPI || VS_HSPI
SPIClass  SPI2(HOOPSENb);
#endif

extern __attribute__((weak)) void yoradio_on_setup();

void setup() {
  Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT==1
  // let USB serial to settle
  delay(2000);
#endif
  LOGI(T_BOOT, println, "Setup...");

  // Start event loop task
  evt::start();

  if(REAL_LEDBUILTIN!=255)
    pinMode(REAL_LEDBUILTIN, OUTPUT);
  if (yoradio_on_setup)
    yoradio_on_setup();

  LOGI(T_BOOT, println, "Init Config");
  config.init();
  LOGI(T_BOOT, println, "Init Display");
  if (!create_display()){
    LOGE(T_BOOT, println, "Can't create display interface! Boot failed!");
    return;
  }
  display->init();

  // cerate and init Player object
  LOGI(T_BOOT, println, "Init Player");
  create_player(DAC_TYPE);
  player->init();

  LOGI(T_BOOT, println, "Start WebServer");
  netserver.begin();
  //telnet.begin();

  if(SDC_CS!=255) {
    LOGI(T_BOOT, println, "Wait for SDCARD");
    display->putRequest(WAITFORSD, 0);
  }

  LOGI(T_BOOT, println, "Init Controls");
  initControls();

  LOGI(T_BOOT, println, "Start Display");
  display->putRequest(DSP_START);
  LOGI(T_BOOT, println, "Wait for Display");
  //while(!display->ready()) delay(10);

  #ifdef MQTT_ROOT_TOPIC
    mqttInit();
  #endif

  /*
  // this autoplay is wrong anyway
  #ifndef NO_AUTOPLAY_ONBOOT
  if (config.store.smartstart == 1) {
    LOGI(T_BOOT, println, "Resume last station playback");
    auto v = config.lastStation();
    EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::playerStation), &v, sizeof(v));
  }
  #endif
*/
  LOGD(T_BOOT, println, "Setup complete");
}

void loop() {
  //telnet.loop();
  player->loop();
  loopControls();
  netserver.loop();
  vTaskDelay(1);
}
