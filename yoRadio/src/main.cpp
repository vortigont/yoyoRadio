#include "Arduino.h"
#include "core/options.h"
#include "core/config.h"
#include "core/telnet.h"
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
    LOGD(T_BOOT, println, "Start NetServer");
#endif
  LOGD(T_BOOT, println, "Setup...");

  // Start event loop task
  evt::start();

  if(REAL_LEDBUILTIN!=255)
    pinMode(REAL_LEDBUILTIN, OUTPUT);
  if (yoradio_on_setup)
    yoradio_on_setup();

  LOGD(T_BOOT, println, "Init Config");
  config.init();
  LOGD(T_BOOT, println, "Init Display");
  display.init();

  pm.on_setup();

  // cerate and init Player object
  LOGD(T_BOOT, println, "Init Player");
  create_player(dac_type_t::DAC_TYPE);
  player->init();

  LOGD(T_BOOT, println, "Start NetWork");
  network.begin();
  if (network.status != CONNECTED && network.status!=SDREADY) {
    LOGD(T_BOOT, println, "Start NetServer");
    netserver.begin();
    LOGD(T_BOOT, println, "Init Controls");
    initControls();

    LOGD(T_BOOT, println, "Wait for Display");
    display.putRequest(DSP_START);
    while(!display.ready()) delay(10);
    return;
  }

  if(SDC_CS!=255) {
    Serial.println("Wait for SDCARD");
    display.putRequest(WAITFORSD, 0);
  }
  Serial.println("Load playlist");
  config.initPlaylistMode();

  Serial.println("Starting WebServer");
  netserver.begin();
  telnet.begin();

  LOGD(T_BOOT, println, "Init Controls");
  initControls();

  LOGD(T_BOOT, println, "Start Display");
  display.putRequest(DSP_START);
  LOGD(T_BOOT, println, "Wait for Display");
  while(!display.ready()) delay(10);

  #ifdef MQTT_ROOT_TOPIC
    mqttInit();
  #endif
  if (config.getMode()==PM_SDCARD) player->initHeaders(config.station.url);
  player->lockOutput=false;
  #ifndef NO_AUTOPLAY_ONBOOT
  if (config.store.smartstart == 1) {
    delay(99);
    auto v = config.lastStation();
    EVT_POST_DATA(YO_CMD_EVENTS, e2int(evt::yo_event_t::plsStation), &v, sizeof(v));
  }
  #endif
  pm.on_end_setup();
  LOGD(T_BOOT, println, "Setup complete");
}

void loop() {
  telnet.loop();
  if (network.status == CONNECTED || network.status==SDREADY) {
    player->loop();
    //loopControls();
  }
  loopControls();
  netserver.loop();
  vTaskDelay(1);
}
