#pragma once

#define TFT_WIDTH   480
#define TFT_HEIGHT  320
#define TFT_BLK_ON_LEVEL 1
#define TFT_BLK     1
#define TFT_RST     -1
#define TFT_CS      45
#define TFT_SCK     47
#define TFT_SDA0    21
#define TFT_SDA1    48
#define TFT_SDA2    40
#define TFT_SDA3    39
#define TFT_TE      38

#define TOUCH_PIN_NUM_I2C_SCL 8
#define TOUCH_PIN_NUM_I2C_SDA 4
#define TOUCH_PIN_NUM_INT 3
#define TOUCH_PIN_NUM_RST -1

// SDCARD
#define SD_MMC_CD   10      // TF_CS    - CD/DAT3
#define SD_MMC_CMD  11      // MOSI     - CMD
#define SD_MMC_CLK  12      // TF_CLK   - CLX
#define SD_MMC_D0   13      // MISO     - DAT0

// I2S
#define AUDIO_I2S_PORT I2S_NUM_0
#define AUDIO_I2S_MCK_IO    -1      // MCK
#define AUDIO_I2S_BCK_IO    42      // BCLK
#define AUDIO_I2S_LRCK_IO   2       // L/R CLK
#define AUDIO_I2S_DO_IO     41      // DIN

#define BAT_ADC_PIN         5


// yoradio defines
#define I2S_DOUT    AUDIO_I2S_DO_IO
#define I2S_BCLK    AUDIO_I2S_BCK_IO
#define I2S_LRC     AUDIO_I2S_LRCK_IO

#define SDC_CS      SD_MMC_CD

#define TS_SCL  TOUCH_PIN_NUM_I2C_SCL
#define TS_SDA  TOUCH_PIN_NUM_I2C_SDA
#define TS_INT  TOUCH_PIN_NUM_INT
#define TS_RST  TOUCH_PIN_NUM_RST -1
#define TS_CS   -1


/*
Links:

platformio based build of the JC3248W535EN DEMO_LVGL Package
https://github.com/NorthernMan54/JC3248W535EN

A simple, lightweight library to drive the JC3248W535EN touch LCD display from Guition without requiring LVGL
https://github.com/AudunKodehode/JC3248W535EN-Touch-LCD


*/