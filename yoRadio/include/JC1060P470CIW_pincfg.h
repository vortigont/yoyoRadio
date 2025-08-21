#pragma once

// defines for 1024x600 7'    Guition JC1060P470 ESP32-P4

// Display driver JD9165BA-DS
#define LCD_H_RES   1024
#define LCD_V_RES   600
#define LCD_RST     27
#define LCD_LED     23

// Touch
#define TP_I2C_SDA  7
#define TP_I2C_SCL  8
#define TP_RST      22
#define TP_INT      21


// I2S codec ES8311     http://www.everest-semi.com/pdf/ES8311%20PB.pdf
// https://components.espressif.com/components/espressif/esp_codec_dev
#define CODEC_I2S0_DSDIN        9   // I2S_DO
#define CODEC_I2S0_MCLK         13  // I2S_MCK

// ES7210 ADC           https://e2e.ti.com/cfs-file/__key/communityserver-discussions-components-files/6/7563.ES7210.pdf
#define ES7210_MCLK             13
#define ES7210_SCLK             12  //  I2S_BCK
#define ES7210_LRCK             10  //  I2S_WS
#define ES7210_SDOUT            11  //  I2S_DI

#define PA_ENABLE               20  //  NS4160 PA control: H - output active, L - mute

// I2C
#define I2C_SDA                 7
#define I2C_SCL                 8
#define GT911_I2C_ADDR          0x5d
#define RTC_I2C_ADDR            0x32
#define ES8311_I2C_ADDR         0x18

// SD CARD
#define SD_DATA0                39
#define SD_DATA1                40
#define SD_DATA2                41
#define SD_DATA3                42
#define SD_CLK                  43
#define SD_CMD                  44


// WS2812
#define WS2812_RGB_LED          26

#define BAT_VOLTAGE_SENSOR_PIN  52





// yoradio defines
#define IGNORE_BOARD_CHECKS

#define I2S_MCLK    ES7210_MCLK
#define I2S_DOUT    CODEC_I2S0_DSDIN
#define I2S_BCLK    ES7210_SCLK
#define I2S_LRC     ES7210_LRCK
#define MUTE_PIN    PA_ENABLE
#define MUTE_VAL    LOW
#define DAC_TYPE    dac_type_t::ES8311
#ifndef DSP_MODEL
#define DSP_MODEL   DSP_JC1060P470
#endif
#define DSP_WIDTH       LCD_H_RES
#define DSP_HEIGHT      LCD_V_RES

#define TFT_BLK     LCD_LED           // display backlight
