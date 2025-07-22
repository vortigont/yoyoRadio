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
  Serial.println("##[BOOT]#\tSetup");

  // Start event loop task
  evt::start();

  if(REAL_LEDBUILTIN!=255) pinMode(REAL_LEDBUILTIN, OUTPUT);
  if (yoradio_on_setup) yoradio_on_setup();
  config.init();
  display.init();

  pm.on_setup();

  // cerate and init Player object
  create_player(dac_type_t::DAC_TYPE);
  player->init();

  network.begin();
  if (network.status != CONNECTED && network.status!=SDREADY) {
    netserver.begin();
    Serial.println("##[BOOT]#\tsn1");
    initControls();
    Serial.println("##[BOOT]#\tsn2");
    display.putRequest(DSP_START);
    Serial.println("##[BOOT]#\tsn3");
    while(!display.ready()) delay(10);
    Serial.println("##[BOOT]#\tsn4");
    return;
  }
  if(SDC_CS!=255) {
    Serial.println("##[BOOT]#\ts sd1");
    display.putRequest(WAITFORSD, 0);
    Serial.print("##[BOOT]#\tSD search\t");
  }
  Serial.println("##[BOOT]#\ts 2");
  config.initPlaylistMode();
  Serial.println("##[BOOT]#\ts 3");
  netserver.begin();
  Serial.println("##[BOOT]#\ts 4");
  telnet.begin();
  initControls();
  Serial.println("##[BOOT]#\ts 5");
  display.putRequest(DSP_START);
  while(!display.ready()) delay(10);
  Serial.println("##[BOOT]#\ts 6");
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
  Serial.println("##[BOOT]#\tSetup done!");
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
