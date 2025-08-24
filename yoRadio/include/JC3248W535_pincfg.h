#pragma once
#include <cstdint>

// defines for  480x320  3.5'  Guition JC3248W535 https://aliexpress.com/item/1005007593889279.html ESP32-S3
// AXS15231B TFT LCD
// https://dl.espressif.com/AE/esp_iot_solution/AXS15231B_Datasheet_V0.5_20230306.pdf
#define JC3248W535_TFT_WIDTH   320
#define JC3248W535_TFT_HEIGHT  480
#define JC3248W535_TFT_BLK     1           // display backlight
#define JC3248W535_TFT_BLK_ON_LEVEL 1 
#define JC3248W535_TFT_RST     -1
#define JC3248W535_TFT_CS      45
#define JC3248W535_TFT_SCK     47
#define JC3248W535_TFT_SDA0    21
#define JC3248W535_TFT_SDA1    48
#define JC3248W535_TFT_SDA2    40
#define JC3248W535_TFT_SDA3    39
#define JC3248W535_TFT_TE      38

#define JC3248W535_TOUCH_GPIO_I2C_SDA 4
#define JC3248W535_TOUCH_GPIO_I2C_SCL 8
#define JC3248W535_TOUCH_GPIO_INT 3
#define JC3248W535_TOUCH_GPIO_RST -1

// SDCARD
#define JC3248W535_SD_MMC_CMD  11      // MOSI     - CMD
#define JC3248W535_SD_MMC_CLK  12      // TF_CLK   - CLX
#define JC3248W535_SD_MMC_D0   13      // MISO     - DAT0
#define JC3248W535_SD_MMC_D1   -1      // Vcc
#define JC3248W535_SD_MMC_D2   -1      // Vcc
#define JC3248W535_SD_MMC_D3   10      // TF_CS    - CD/DAT3
#define JC3248W535_SD_MMC_CD   JC3248W535_SD_MMC_D3      // TF_CS    - CD/DAT3

// I2S
#define JC3248W535_AUDIO_I2S_PORT      I2S_NUM_0
#define JC3248W535_AUDIO_I2S_MCK_IO    -1      // MCK
#define JC3248W535_AUDIO_I2S_BCK_IO    42      // BCLK
#define JC3248W535_AUDIO_I2S_LRCK_IO   2       // L/R CLK
#define JC3248W535_AUDIO_I2S_DO_IO     41      // DIN

#define JC3248W535_BAT_ADC_PIN         5


// yoradio defines
namespace JC3248W535 {
  // I2S
  struct i2s_gpio_t {
    int32_t dout, bclk, lrclk, mclk;
    int32_t mute, mute_lvl;
  };
  static constexpr i2s_gpio_t i2s{JC3248W535_AUDIO_I2S_DO_IO, JC3248W535_AUDIO_I2S_BCK_IO, JC3248W535_AUDIO_I2S_LRCK_IO, -1};

  // Display
  struct display_t {
    uint16_t w, h;
    // gpios
    int32_t sda0, sda1, sda2, sda, sck, cs, rst, te, backlight, backlight_level;
  };
  static constexpr display_t display{
    JC3248W535_TFT_WIDTH, JC3248W535_TFT_HEIGHT,
    JC3248W535_TFT_SDA0, JC3248W535_TFT_SDA1, JC3248W535_TFT_SDA2, JC3248W535_TFT_SDA3,
    JC3248W535_TFT_SCK, JC3248W535_TFT_CS, JC3248W535_TFT_RST, JC3248W535_TFT_TE,
    JC3248W535_TFT_BLK, JC3248W535_TFT_BLK_ON_LEVEL
  };
};

/*
#define JC3248W535_I2S_DOUT     JC3248W535_AUDIO_I2S_DO_IO
#define JC3248W535_I2S_BCLK     JC3248W535_AUDIO_I2S_BCK_IO
#define JC3248W535_I2S_LRC      JC3248W535_AUDIO_I2S_LRCK_IO

#define JC3248W535_SDC_CS       JC3248W535_SD_MMC_CD

#define JC3248W535_TS_SCL       TOUCH_GPIO_I2C_SCL
#define JC3248W535_TS_SDA       TOUCH_GPIO_I2C_SDA
#define JC3248W535_TS_INT       TOUCH_GPIO_INT
#define JC3248W535_TS_RST       TOUCH_GPIO_RST
#define JC3248W535_TS_CS        -1
*/
#ifndef DSP_MODEL
#define JC3248W535_DSP_MODEL   DSP_JC3248W535
#endif

/*
Links:

platformio based build of the JC3248W535EN DEMO_LVGL Package
https://github.com/NorthernMan54/JC3248W535EN

A simple, lightweight library to drive the JC3248W535EN touch LCD display from Guition without requiring LVGL
https://github.com/AudunKodehode/JC3248W535EN-Touch-LCD


*/