#pragma once
#include "displays/dspcore.h"

/* Display */
// Модуль Guition ESP32-4848S040 (ST7701 RGB 480x480)
// https://devices.esphome.io/devices/Guition-ESP32-S3-4848S040
// https://github.com/moononournation/Arduino_GFX/discussions/35
// основано на наработках https://github.com/Witaliy76/Yoradio_JC3248W535C_3_30/tree/4848S040

// Пины дисплея 
// Командная шина ST7701 (SWSPI)
#define G_4848S040_CS   39
#define G_4848S040_SCK  48
#define G_4848S040_SDA  47
// RGB синхросигналы
#define G_4848S040_DE     18
#define G_4848S040_VSYNC  17
#define G_4848S040_HSYNC  16
#define G_4848S040_PCLK   21
// R0..R4 (LSB→MSB) — согласно вендору
#define G_4848S040_R0   11
#define G_4848S040_R1   12
#define G_4848S040_R2   13
#define G_4848S040_R3   14
#define G_4848S040_R4   0
// G0..G5
#define G_4848S040_G0   8
#define G_4848S040_G1   20
#define G_4848S040_G2   3
#define G_4848S040_G3   46
#define G_4848S040_G4   9
#define G_4848S040_G5   10
// B0..B4 (LSB→MSB) — согласно вендору
#define G_4848S040_B0   4
#define G_4848S040_B1   5
#define G_4848S040_B2   6
#define G_4848S040_B3   7
#define G_4848S040_B4   15
// Подсветка
#define G_4848S040_BL   38

/*  I2S DAC    */
#define G_4848S040_I2S_DOUT      40
#define G_4848S040_I2S_BCLK      1
#define G_4848S040_I2S_LRC       2


/* Основные настройки */
#define PLAYER_FORCE_MONO true
#define L10N_LANGUAGE RU
#define BITRATE_FULL  true

/*
// Включение Spectrum Analyzer
#define SPECTRUM_ENABLED       true    // !!Включить Spectrum Analyzer
#define SPECTRUM_USE_PSRAM     true    // Использовать PSRAM для FFT буферов
#define SPECTRUM_BANDS         15      // Количество полос спектра (уменьшено до 15)
#define SPECTRUM_FFT_SIZE      64      // Размер FFT (уменьшен для простоты)
#define SPECTRUM_SMOOTHING     0.90f    // Сглаживание (увеличено для более плавных переходов)
#define SPECTRUM_PEAK_HOLD_TIME 300.0f // Время удержания пиков (мс, уменьшено)
#define SPECTRUM_LOGARITHMIC   false   // Логарифмическая шкала частот (отключено)
#define SPECTRUM_STEREO        true   // Стерео режим (отключено - моно)
#define SPECTRUM_REPLACE_VU    true   // !!Заменить VU-метр на SA
#define SPECTRUM_GAIN          0.03f    // Общее усиление спектра (1.0 = без усиления, 0.05 = -95%)
*/


/*  SDCARD  */
//#define USE_SD                              /* Отключаем поддержку SD карты */
/*  MISO is the same as D0, MOSI is the same as D1 */
/*  SD VSPI PINS. SD SCK must be connected to pin 18
                  SD MISO must be connected to pin 19
                  SD MOSI must be connected to pin 23  */
/*  SD HSPI PINS. SD SCK must be connected to pin 14
                  SD MISO must be connected to pin 12 (+20 KOm на GND)
                  SD MOSI must be connected to pin 13  */
/*  SD PINS согласно схеме:
    io42 - TF(D3) - Chip Select
    io47 - SPICLK_P - MOSI (Master Out Slave In)
    io48 - SPICLK_N - SCK (Clock)
    io41 - TF(D1) - MISO (Master In Slave Out)  */
//#define SDC_CS        42              /* Chip Select */
//#define SD_SPIPINS    48, 41, 47      /* SCK, MISO, MOSI */
//#define SD_HSPI       false           /* use VSPI for SD (по умолчанию) */
/* **************************************** */

/*  TOUCHSCREEN  */
/* Touchscreen Configuration для 4848S040 */
#define G_4848S040_TS_MODEL              TS_MODEL_GT911  /* GT911 Capacitive I2C touch screen */
#define G_4848S040_TS_SDA                19              /* Touch screen SDA pin */
#define G_4848S040_TS_SCL                45              /* Touch screen SCL pin */
#define G_4848S040_TS_INT                255             /* Touch screen INT pin (отключен) */
#define G_4848S040_TS_RST                255             /* Touch screen RST pin (отключен) */

// Специальные настройки для квадратного экрана 480x480
#define SQUARE_SCREEN_OPTIMIZATION true

/*  Resistive SPI touch screen  */
/*  TS VSPI PINS. CLK must be connected to pin 18
                  DIN must be connected to pin 23
                  DO  must be connected to pin 19
                  IRQ - not connected */
//#define TS_CS                 255           /*  Touch screen CS pin  */
/*  TS HSPI PINS. CLK must be connected to pin 14
                  DIN must be connected to pin 13
                  DO  must be connected to pin 12
                  IRQ - not connected */

/*  Capacitive I2C touch screen GT911  */
#define G_4848S040_TS_X_MIN              0               /*  Минимальное значение X координаты  */
#define G_4848S040_TS_X_MAX              480             /*  Максимальное значение X координаты  */
#define G_4848S040_TS_Y_MIN              0               /*  Минимальное значение Y координаты  */
#define G_4848S040_TS_Y_MAX              480             /*  Максимальное значение Y координаты  */
/******************************************/

// yoradio defines
namespace Guition_4848S040 {
  // I2S configuration
  struct i2s_gpio_t {
    int32_t dout, bclk, lrclk, mclk;
    int32_t mute, mute_lvl;
  };
  static constexpr i2s_gpio_t i2s{G_4848S040_I2S_DOUT, G_4848S040_I2S_BCLK, G_4848S040_I2S_LRC, -1, -1, 0};

  // Display configuration
  struct display_t {
    uint16_t w, h;
    // command bus
    int32_t sck, cs, sda;
    // RGB synch
    int32_t de, vsync, hsync, pclk;
    // colors
    int32_t r0, r1, r2, r3, r4;
    int32_t g0, g1, g2, g3, g4;
    int32_t b0, b1, b2, b3, b4;
    int32_t backlight, backlight_level;
  };

  static constexpr display_t display{
    G_4848S040_CS, G_4848S040_SCK, G_4848S040_SDA,
    G_4848S040_DE, G_4848S040_VSYNC, G_4848S040_HSYNC, G_4848S040_PCLK,
    G_4848S040_R0,
    G_4848S040_R1,
    G_4848S040_R2,
    G_4848S040_R3,
    G_4848S040_R4,
    G_4848S040_G0,
    G_4848S040_G1,
    G_4848S040_G2,
    G_4848S040_G3,
    G_4848S040_G4,
    G_4848S040_G5,
    G_4848S040_B0,
    G_4848S040_B1,
    G_4848S040_B2,
    G_4848S040_B3,
    G_4848S040_B4,
  };


  // function to create gfx instance
  Arduino_GFX* create_display_dev(const display_t &cfg, Arduino_DataBus *bus);

  // Module device cointrol
  class Dsp_4848S040 : public DisplayControl {
    int32_t _backlight_gpio;
  public:
    Dsp_4848S040(int32_t backlight_gpio = -1);
  };


};  // namespace Guition_4848S040 {
