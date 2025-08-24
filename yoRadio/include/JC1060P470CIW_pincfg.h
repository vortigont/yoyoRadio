#pragma once

// defines for 1024x600 7'    Guition JC1060P470 ESP32-P4

// Display driver JD9165BA-DS
#define JC1060P470_LCD_H_RES   1024
#define JC1060P470_LCD_V_RES   600
#define JC1060P470_LCD_RST     27
#define JC1060P470_LCD_LED     23

// Touch
#define JC1060P470_TP_I2C_SDA  7
#define JC1060P470_TP_I2C_SCL  8
#define JC1060P470_TP_RST      22
#define JC1060P470_TP_INT      21


// I2S codec ES8311     http://www.everest-semi.com/pdf/ES8311%20PB.pdf
// https://components.espressif.com/components/espressif/esp_codec_dev
#define JC1060P470_CODEC_I2S0_DSDIN        9   // I2S_DO
#define JC1060P470_CODEC_I2S0_MCLK         13  // I2S_MCK

// ES7210 ADC           https://e2e.ti.com/cfs-file/__key/communityserver-discussions-components-files/6/7563.ES7210.pdf
#define JC1060P470_ES7210_MCLK             13
#define JC1060P470_ES7210_SCLK             12  //  I2S_BCK
#define JC1060P470_ES7210_LRCK             10  //  I2S_WS
#define JC1060P470_ES7210_SDOUT            11  //  I2S_DI

#define JC1060P470_PA_ENABLE               20  //  NS4160 PA control: H - output active, L - mute

// I2C
#define JC1060P470_I2C_SDA                 7
#define JC1060P470_I2C_SCL                 8
#define JC1060P470_GT911_I2C_ADDR          0x5d
#define JC1060P470_RTC_I2C_ADDR            0x32
#define JC1060P470_ES8311_I2C_ADDR         0x18

// SD CARD
#define JC1060P470_SD_DATA0                39
#define JC1060P470_SD_DATA1                40
#define JC1060P470_SD_DATA2                41
#define JC1060P470_SD_DATA3                42
#define JC1060P470_SD_CLK                  43
#define JC1060P470_SD_CMD                  44


// WS2812
#define JC1060P470_WS2812_RGB_LED          26

#define JC1060P470_BAT_VOLTAGE_SENSOR_PIN  52





// yoradio defines
namespace JC1060P470 {
    // I2S
    struct i2s_gpio_t {
      int32_t dout, bclk, lrclk, mclk, mute, mute_level;
    };
    static constexpr i2s_gpio_t i2s{
        JC1060P470_CODEC_I2S0_DSDIN,
        JC1060P470_ES7210_SCLK,
        JC1060P470_ES7210_LRCK,
        JC1060P470_ES7210_MCLK,
        JC1060P470_PA_ENABLE,
        LOW
    };
  
    // Display
    struct display_t {
      uint16_t w, h;
      // gpios
      int32_t rst, backlight, backlight_level;
    };
    static constexpr display_t display{
      JC1060P470_LCD_H_RES, JC1060P470_LCD_V_RES,
      JC1060P470_LCD_RST,
      JC1060P470_LCD_LED, HIGH
    };
  };
  
  

#ifndef DSP_MODEL
#define JC1060P470_DSP_MODEL   DSP_JC1060P470
#endif
