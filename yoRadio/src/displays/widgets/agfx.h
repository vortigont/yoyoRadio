// Helper functions for ArduinoGFX lib
#include "Arduino_GFX.h"

// draw text with transparent background using GFXfont
template <typename T>
void gfxDrawText(Arduino_GFX* g, int x, int y, const char* text, uint16_t color, const T* font = nullptr, uint8_t size = 0){
  g->setCursor(x, y);
  if (font)
    g->setFont(font);

  g->setTextColor(color);
  if (size) g->setTextSize(size);
  g->print(text);
  //print(utf8Rus(text, true));
}

// draw text with background fill using GFXfont
template <typename T>
void gfxDrawTextFill(Arduino_GFX* g, int x, int y, const char* text, uint16_t color, uint16_t bgcolor, const T* font = nullptr, uint8_t size = 0){
  g->setCursor(x, y);
  if (font)
    g->setFont(font);

  g->setTextColor(color, bgcolor);
  if (size) g->setTextSize(size);
  g->print(text);
  //print(utf8Rus(text, true));
}
