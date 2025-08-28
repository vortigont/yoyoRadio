#include "Arduino.h"
#include "core/options.h"
#include "core/config.h"
#include "core/player.h"
#include "displays/dspcore.h"
#include "core/netserver.h"
#include "core/controls.h"
#include "core/mqtt.h"
#include "core/evtloop.h"
#include "components.hpp"
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

  LOGI(T_BOOT, println, "Init Config");
  config.init();

  LOGI(T_BOOT, println, "Creating hw configuration");
  load_hwcomponets_configuration();

  LOGI(T_BOOT, println, "Start WebServer");
  netserver.begin();
  //telnet.begin();
/*
  if(SDC_CS!=255) {
    LOGI(T_BOOT, println, "Wait for SDCARD");
    display->putRequest(WAITFORSD, 0);
  }
*/

  //LOGI(T_BOOT, println, "Init Controls");
  //initControls();

  #ifdef MQTT_ROOT_TOPIC
    mqttInit();
  #endif

  // spawn Modules instances from saved configurations, this must be done AFTER display initialization
  zookeeper.start();
  zookeeper.setHandlers();

  LOGD(T_BOOT, println, "Setup complete");
}

void loop() {
  //telnet.loop();
  if (player)
    player->loop();
  //loopControls();
  netserver.loop();
  vTaskDelay(1);
}
