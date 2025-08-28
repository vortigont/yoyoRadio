#include "databus/Arduino_SWSPI.h"
#include "display/Arduino_RGB_Display.h"  // ArduinoGFX driver
#include "canvas/Arduino_Canvas.h"
#include "guition_4848S040.hpp"
//#include "core/log.h"

namespace Guition_4848S040 {

// panel driver
Arduino_ESP32RGBPanel *rgbpanel;
// GFX object
Arduino_RGB_Display *rgbgfx;

Arduino_GFX* create_display_dev(const display_t &cfg, Arduino_DataBus *bus){
//  delete bus; // destruct previous object, if any
  bus = new Arduino_SWSPI(GFX_NOT_DEFINED /* DC */, cfg.cs, cfg.sck, cfg.sda /* MOSI */, GFX_NOT_DEFINED /* MISO */);

  // Отправка обязательных команд контроллеру
  if (bus) {
    // Формат пикселя: RGB565
    bus->sendCommand(0x3A);
    bus->sendData(0x55);
    delay(5);
    // Инверсия выкл
    bus->sendCommand(0x20);
    delay(2);
    // Страница 0x10: настройка сканирования (C7=0x00 нормаль)
    bus->beginWrite();
    bus->writeCommand(0xFF); bus->write(0x77); bus->write(0x01); bus->write(0x00); bus->write(0x00); bus->write(0x10);
    bus->writeCommand(0xC7); bus->write(0x00);
    // Возврат на страницу 0x00 и установка MADCTL (0x36) = 0x00 (RGB порядок)
    bus->writeCommand(0xFF); bus->write(0x77); bus->write(0x01); bus->write(0x00); bus->write(0x00); bus->write(0x00);
    bus->writeCommand(0x36); bus->write(0x00);
    bus->endWrite();
  } else return nullptr;

  rgbpanel = new Arduino_ESP32RGBPanel(
        cfg.de /* DE */, cfg.vsync /* VSYNC */, cfg.hsync /* HSYNC */, cfg.pclk /* PCLK */,
        // Пины КРАСНОГО канала (R0-R4)
        cfg.r0, cfg.r1, cfg.r2, cfg.r3, cfg.r4,
        // Пины ЗЕЛЕНОГО канала (G0-G5)
        cfg.g0, cfg.g1, cfg.g2, cfg.g3, cfg.g4, cfg.g5,
        // Пины СИНЕГО канала (B0-B4)
        cfg.b0, cfg.b1, cfg.b2, cfg.b3, cfg.b4,
        // Параметры таймингов (горизонтальные и вертикальные)
        /* hsync_polarity */ 1, /* hsync_front_porch */ 10, /* hsync_pulse_width */ 8, /* hsync_back_porch */ 50,
        /* vsync_polarity */ 1, /* vsync_front_porch */ 10, /* vsync_pulse_width */ 8, /* vsync_back_porch */ 20,
        // Параметры тактирования и формата данных
        /* pclk_active_neg */ 0, /* prefer_speed */ 6000000UL, /* big endian */ false,
        /* de_idle_high */ 0, /* pclk_idle_high */ 0, /* bounce_buffer_size_px */ 0);

  rgbgfx = new Arduino_RGB_Display(cfg.w, cfg.h, rgbpanel, 0 /* rotation */, false /* auto flush */,
        bus, GFX_NOT_DEFINED /* RST */, st7701_type1_init_operations, sizeof(st7701_type1_init_operations));

  // this screen does not have frame buffer, so it needs full redraw each time from MCU
  // gfx object
  return new Arduino_Canvas(cfg.w, cfg.h, rgbgfx);
}


};  //  namespace Guition_4848S040 {
