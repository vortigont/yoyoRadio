#include "dspcore.h"
#include "../core/network.h"

char* DspCoreBase::utf8Rus(const char* str, bool uppercase) {
    int index = 0;
    static char strn[BUFLEN];
    bool E = false;
    strlcpy(strn, str, BUFLEN);
    if (uppercase) {
      bool next = false;
      for (char *iter = strn; *iter != '\0'; ++iter)
      {
        if (E) {
          E = false;
          continue;
        }
        uint8_t rus = (uint8_t) * iter;
        if (rus == 208 && (uint8_t) * (iter + 1) == 129) { // ёКостыли
          *iter = (char)209;
          *(iter + 1) = (char)145;
          E = true;
          continue;
        }
        if (rus == 209 && (uint8_t) * (iter + 1) == 145) {
          *iter = (char)209;
          *(iter + 1) = (char)145;
          E = true;
          continue;
        }
        if (next) {
          if (rus >= 128 && rus <= 143) *iter = (char)(rus + 32);
          if (rus >= 176 && rus <= 191) *iter = (char)(rus - 32);
          next = false;
        }
        if (rus == 208) next = true;
        if (rus == 209) {
          *iter = (char)208;
          next = true;
        }
        *iter = toupper(*iter);
      }
    }
    if(L10N_LANGUAGE==EN) return strn;
    while (strn[index])
    {
      if (strn[index] >= 0xBF)
      {
        switch (strn[index]) {
          case 0xD0: {
              if (strn[index + 1] == 0x81) {
                strn[index] = 0xA8;
                break;
              }
              if (strn[index + 1] >= 0x90 && strn[index + 1] <= 0xBF) strn[index] = strn[index + 1] + 0x30;
              break;
            }
          case 0xD1: {
              if (strn[index + 1] == 0x91) {
                //strn[index] = 0xB7;
                strn[index] = 0xB8;
                break;
              }
              if (strn[index + 1] >= 0x80 && strn[index + 1] <= 0x8F) strn[index] = strn[index + 1] + 0x70;
              break;
            }
        }
        int sind = index + 2;
        while (strn[sind]) {
          strn[sind - 1] = strn[sind];
          sind++;
        }
        strn[sind - 1] = 0;
      }
      index++;
    }
    return strn;
}

void DspCoreBase::charSize(uint8_t textsize, uint8_t& width, uint16_t& height){
  width = textsize * CHARWIDTH;
  height = textsize * CHARHEIGHT;
}

void DspCoreBase::setClipping(clipArea ca){
  _cliparea = ca;
  _clipping = true;
}

uint16_t DspCoreBase::textWidth(const char *txt){
  uint16_t w = 0, l=strlen(txt);
  for(uint16_t c=0;c<l;c++) w+=_charWidth(txt[c]);
  return w;
}

void DspCoreBase::_getTimeBounds() {
  _timewidth = textWidth(_timeBuf);
  char buf[4];
  strftime(buf, 4, "%H", &network.timeinfo);
  _dotsLeft=textWidth(buf);
}
