#include <TFT_eSPI.h> // Hardware-specific library
#include <Adafruit_GFX.h>


class TFT_eSPI_AdafruitGFX_Wrapper : public Adafruit_GFX {
    TFT_eSPI _tft;

    public:
    TFT_eSPI_AdafruitGFX_Wrapper(uint16_t w, uint16_t h) : Adafruit_GFX(w, h) {};
    void init(uint8_t tc = TAB_COLOUR){ _tft.init(tc); }
    void drawPixel(int16_t x, int16_t y, uint16_t color) override { _tft.drawPixel( x, y, color); }
    void fillScreen(uint16_t color) override { _tft.fillScreen(color); };

    void setRotation(uint8_t r) override { _tft.setRotation(r); }
    void invertDisplay(bool i) override { _tft.invertDisplay(i); };

    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override { _tft.drawFastVLine(x, y, h, color); };
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override { _tft.drawFastHLine(x, y, w, color); };;
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                          uint16_t color) override { _tft.fillRect(x, y, w, h, color); };
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                         uint16_t color) override { _tft.drawLine(x0, y0, x1, y1, color); };
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
                          uint16_t color) override { _tft.drawRect(x, y, w, h, color); };
  
};
