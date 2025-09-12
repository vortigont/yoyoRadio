#include "Arduino.h"
#include "core/options.h"
#include "core/player.h"
#include "displays/dspcore.h"
#include "core/netserver.h"
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

  // bring up radio firts (it would also mount LittleFS)
  LOGI(T_BOOT, println, "Start WebServer");
  netserver.begin();

  LOGI(T_BOOT, println, "Loading hw configuration");
  load_hwcomponets_configuration();

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
