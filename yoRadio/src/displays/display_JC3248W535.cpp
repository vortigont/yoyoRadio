#include "../core/options.h"
#if DSP_MODEL==DSP_JC3248W535
#include "display/Arduino_AXS15231B.h"  // ArduinoGFX driver
#include "canvas/Arduino_Canvas.h"
#include "databus/Arduino_ESP32QSPI.h"
#include "display_JC3248W535.h"
#include "fonts/bootlogo.h"
#include "../core/config.h"
#include "../core/network.h"
#include "tools/l10n.h"

static Arduino_DataBus *bus{nullptr};
static Arduino_AXS15231B *g{nullptr};
DspCore* dsp{nullptr};

bool create_display_dev(){
  if (bus == nullptr){
    bus = new Arduino_ESP32QSPI(TFT_CS, TFT_SCK, TFT_SDA0, TFT_SDA1, TFT_SDA2, TFT_SDA3);
  }

  if (bus == nullptr){
    Serial.println("Can't create GFX bus!");
    return false;
  }

  if (dsp == nullptr ){
    g = new Arduino_AXS15231B(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, false /* IPS */, TFT_WIDTH, TFT_HEIGHT);
    dsp = new DspCore(g);

    //dsp = new DspCore(bus);

    Serial.println("create dsp object");
    // backlight
    #ifdef TFT_BLK
      //pinMode(TFT_BLK, OUTPUT);
      //digitalWrite(TFT_BLK, TFT_BLK_ON_LEVEL);
      ledcAttach(TFT_BLK, 1000, 8);
      //ledcOutputInvert(TFT_BLK, true);
      ledcWrite(TFT_BLK, 200);
    #endif
  }
  return dsp != nullptr;
}


void DspCore::initDisplay() {
  fillScreen(0x07e0); // green
  delay(500);
  fillScreen(0x0);    // black
#ifdef  U8G2_FONT_SUPPORT
  setUTF8Print(true);
#endif  // U8G2_FONT_SUPPORT


#ifdef CPU_LOAD
  // Инициализация CPU виджета
  cpuWidget.init(cpuConf, 20, false, config.theme.rssi, config.theme.background);
  cpuWidget.setActive(true);
  // Запускаем мониторинг CPU
  perfmon_start();
#endif

  plItemHeight = playlistConf.widget.textsize*(CHARHEIGHT-1)+playlistConf.widget.textsize*4;
  plTtemsCount = round((float)height()/plItemHeight);
  if (plTtemsCount%2==0) plTtemsCount++;
  plCurrentPos = plTtemsCount/2;
  plYStart = (height() / 2 - plItemHeight / 2) - plItemHeight * (plTtemsCount - 1) / 2 + playlistConf.widget.textsize*2;
  
  Serial.println("[AXS15231B] initDisplay completed successfully");
}

void DspCore::drawLogo(uint16_t top) {
  // Очищаем область под логотип
  //gfxFillRect(0, top, width(), 88, config.theme.background);
  
  // Рисуем логотип
  draw16bitRGBBitmap((width() - 99) / 2, top, const_cast<uint16_t*>(bootlogo2), 99, 64);
  delay(1000);
}

void DspCore::printPLitem(uint8_t pos, const char* item, ScrollWidget& current){
  if (pos == plCurrentPos) {
    current.setText(item);
  } else {
    uint8_t plColor = (abs(pos - plCurrentPos)-1)>4?4:abs(pos - plCurrentPos)-1;
    gfxFillRect(0, plYStart + pos * plItemHeight - 1, width(), plItemHeight - 2, config.theme.background);
    // Обрезка строки по ширине без троеточия
    const char* rus = utf8Rus(item, true);
    int len = strlen(rus);
    char buf[128];
    int maxWidth = playlistConf.width;
    uint8_t textsize = playlistConf.widget.textsize;
    if (textWidthGFX(rus, textsize) <= maxWidth) {
      strncpy(buf, rus, sizeof(buf)-1);
      buf[sizeof(buf)-1] = 0;
    } else {
      int cut = len;
      while (cut > 0) {
        char tmp[128];
        strncpy(tmp, rus, cut);
        tmp[cut] = 0;
        if (textWidthGFX(tmp, textsize) <= maxWidth) break;
        cut--;
      }
      strncpy(buf, rus, cut);
      buf[cut] = 0;
    }
    gfxDrawText(
      TFT_FRAMEWDT,
      plYStart + pos * plItemHeight,
      buf,
      config.theme.playlist[plColor],
      config.theme.background,
      textsize
    );
  }
}

void DspCore::drawPlaylist(uint16_t currentItem) {
  return;
  uint8_t lastPos = config.fillPlMenu(currentItem - plCurrentPos, plTtemsCount);
  if(lastPos<plTtemsCount){
    gfxFillRect(0, lastPos*plItemHeight+plYStart, width(), height()/2, config.theme.background);
  }
}

void DspCore::clearDsp(bool black) { fillScreen( black ? 0 : config.theme.background); }

uint8_t DspCore::_charWidth(unsigned char c){
    GFXglyph *glyph = pgm_read_glyph_ptr(&CLK_FONT1, c - 0x20);
  return pgm_read_byte(&glyph->xAdvance);
}

void DspCore::_clockSeconds(){
  // Секунды

  char secbuf[8];
  snprintf(secbuf, sizeof(secbuf), "%02d", network.timeinfo.tm_sec);
  gfxDrawText(
    width()  - clockRightSpace - CHARWIDTH*4*2-17,
    clockTop-clockTimeHeight+94,
    secbuf,
    config.theme.seconds,
    config.theme.background,
    1,
    &CLK_FONT2
  );
  
  // Очищаем область под двоеточием (только область самого двоеточия) - для старого шрифта
 //gfxFillRect(gfx, _timeleft+_dotsLeft+5, clockTop-CHARHEIGHT+5, 15, 65, config.theme.background);
  
  // Двоеточие с прозрачным фоном
  gfxDrawText(
    _timeleft+_dotsLeft -4,
    clockTop-CHARHEIGHT+46,
    ":",
    (network.timeinfo.tm_sec % 2 == 0) ? config.theme.clock : (CLOCKFONT_MONO ? config.theme.clockbg : config.theme.background),
    (network.timeinfo.tm_sec % 2 == 0) ? config.theme.clock : (CLOCKFONT_MONO ? config.theme.clockbg : config.theme.background),
    1,
    &CLK_FONT1
  );
#ifdef BATTERY_ON
  if(!config.isScreensaver) {
    // Мигалки и батарейка
    char batbuf[8] = "";
    uint16_t batcolor = 0;
    if (Charging) {
      batcolor = color565(0, 255, 255);
      if (g == 1) strcpy(batbuf, "\xA0\xA2\x9E\x9F");
      if (g == 2) strcpy(batbuf, "\xA0\x9E\x9E\xA3");
      if (g == 3) strcpy(batbuf, "\x9D\x9E\xA2\xA3");
      if (g >= 4) {g = 0; strcpy(batbuf, "\x9D\xA2\xA2\x9F");}
      g++;
    } else if (Volt < 2.8) {
      batcolor = color565(255, 0, 0);
      if (g == 1) strcpy(batbuf, "\xA0\xA2\xA2\xA3");
      if (g >= 2) {g = 0; strcpy(batbuf, "\x9D\x9E\x9E\x9F");}
      g++;
    } else {
      // Статическая батарейка
      if (Volt >= 3.82)      { batcolor = color565(100, 255, 150); strcpy(batbuf, "\xA0\xA2\xA2\xA3"); }
      else if (Volt >= 3.72) { batcolor = color565(50, 255, 100);  strcpy(batbuf, "\x9D\xA2\xA2\xA3"); }
      else if (Volt >= 3.61) { batcolor = color565(0, 255, 0);     strcpy(batbuf, "\x9D\xA1\xA2\xA3"); }
      else if (Volt >= 3.46) { batcolor = color565(75, 255, 0);    strcpy(batbuf, "\x9D\x9E\xA2\xA3"); }
      else if (Volt >= 3.33) { batcolor = color565(150, 255, 0);   strcpy(batbuf, "\x9D\x9E\xA1\xA3"); }
      else if (Volt >= 3.20) { batcolor = color565(255, 255, 0);   strcpy(batbuf, "\x9D\x9E\x9E\xA3"); }
      else if (Volt >= 2.8)  { batcolor = color565(255, 0, 0);     strcpy(batbuf, "\x9D\x9E\x9E\x9F"); }
    }
    gfxDrawText(gfx, BatX, BatY, batbuf, batcolor, config.theme.background, BatFS);
#ifndef HIDE_VOLT
    char voltbuf[16];
    snprintf(voltbuf, sizeof(voltbuf), "%.3fv", Volt);
    gfxDrawText(gfx, VoltX, VoltY, voltbuf, batcolor, config.theme.background, VoltFS);
#endif
    char procbuf[8];
    snprintf(procbuf, sizeof(procbuf), "%3i%%", ChargeLevel);
    gfxDrawText(gfx, ProcX, ProcY, procbuf, batcolor, config.theme.background, ProcFS);
  }
#endif
}

void DspCore::_clockDate(){
  if(_olddateleft>0)
    gfxFillRect(_olddateleft,  clockTop+70, _olddatewidth, CHARHEIGHT*2, config.theme.background); //очистка надписи даты
  gfxDrawText(_dateleft+70, clockTop+70, _dateBuf, config.theme.date, config.theme.background, 2);
  strlcpy(_oldDateBuf, _dateBuf, sizeof(_dateBuf));
  _olddatewidth = _datewidth;
  _olddateleft = _dateleft;
  // День недели
  gfxDrawText(
    //width() - clockRightSpace - CHARWIDTH*4*2+13-20,
     8,
    //clockTop-CHARHEIGHT+44,
    clockTop+70,
    utf8Rus(dow[network.timeinfo.tm_wday], false),
    config.theme.dow,
    config.theme.background,
    2
  );
}

void DspCore::_clockTime(){
  if(_oldtimeleft>0 && !CLOCKFONT_MONO)
    gfxFillRect(_oldtimeleft, clockTop-clockTimeHeight+1, _oldtimewidth, clockTimeHeight, config.theme.background);
  _timeleft = width()-clockRightSpace-CHARWIDTH*4*2-24-_timewidth;
  // Время
  gfxDrawText(
    _timeleft-4,
    clockTop + 38,
    _timeBuf,
    config.theme.clock,
    config.theme.background,
    1,
    &CLK_FONT1
  );
  strlcpy(_oldTimeBuf, _timeBuf, sizeof(_timeBuf));
  _oldtimewidth = _timewidth;
  _oldtimeleft = _timeleft;
  // Вертикальный разделитель
  //gfxDrawLine(gfx, width()-clockRightSpace-CHARWIDTH*4*2-25, clockTop +5, width()-clockRightSpace-CHARWIDTH*4*2-25, clockTop +5 + clockTimeHeight-3, config.theme.div);
  // Горизонтальный разделитель
  //gfxDrawLine(gfx, width()-clockRightSpace-CHARWIDTH*4*2+10, clockTop+32, width()-clockRightSpace-CHARWIDTH*4*2+10+62-1, clockTop+32, config.theme.div);
  sprintf(_buffordate, "%2d %s %d", network.timeinfo.tm_mday,mnths[network.timeinfo.tm_mon], network.timeinfo.tm_year+1900);
  strlcpy(_dateBuf, utf8Rus(_buffordate, true), sizeof(_dateBuf));
  _datewidth = strlen(_dateBuf) * CHARWIDTH*2;
  _dateleft = width() - clockRightSpace - _datewidth - 80;
  
//if(!config.isScreensaver){    
//   gfxDrawBitmap(30, 226, bootlogo2, 99, 64);
//  }
 
}

void DspCore::printClock(uint16_t top, uint16_t rightspace, uint16_t timeheight, bool redraw){
  // Ограничиваем верхнюю границу
  if(top < TFT_FRAMEWDT) {
    top = TFT_FRAMEWDT;
  }
  
  // Ограничиваем нижнюю границу
  // Учитываем:
  // - высоту времени (clockTimeHeight)
  // - высоту даты (CHARHEIGHT*2)
  // - высоту дня недели (CHARHEIGHT*3)
  // - отступы между элементами (78 пикселей для даты, 44 для дня недели)
  uint16_t totalHeight = clockTimeHeight + CHARHEIGHT*2 + CHARHEIGHT*3 + 78 ;
  
  if(top + totalHeight > height() - TFT_FRAMEWDT) {
    top = height() - TFT_FRAMEWDT - totalHeight;
  }
  
  clockTop = top;
  clockRightSpace = rightspace;
  clockTimeHeight = timeheight;
  strftime(_timeBuf, sizeof(_timeBuf), "%H:%M", &network.timeinfo);
  if(strcmp(_oldTimeBuf, _timeBuf)!=0 || redraw){
    _getTimeBounds();
    _clockTime();
    _clockDate();
  }
  _clockSeconds();
}

void DspCore::clearClock(){  
  // Очищаем область под текущими часами
  gfxFillRect(_timeleft, clockTop, MAX_WIDTH, clockTimeHeight+12+CHARHEIGHT, config.theme.background);
  
  // Если есть старое положение часов (при перемещении), очищаем и его
  if(_oldtimeleft > 0) {
    gfxFillRect(_oldtimeleft, clockTop-clockTimeHeight+20, _oldtimewidth+CHARWIDTH*3*2+80, clockTimeHeight+CHARHEIGHT+60, config.theme.background);
  }
}


void DspCore::sleep(void) { 
  Serial.println("DspCore::sleep");
  std::lock_guard<std::mutex> lock(_mtx);
  displayOff();
  ledcWrite(0, 0); // Выключаем подсветку через PWM
}

void DspCore::wake(void) {
  Serial.println("DspCore::wake");
  std::lock_guard<std::mutex> lock(_mtx);
  displayOn();
  ledcWrite(0, map(config.store.brightness, 0, 100, 0, 255)); // Устанавливаем яркость через PWM
}

void DspCore::writePixel(int16_t x, int16_t y, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x > _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height)) return;
  }
  drawPixel(x, y, color);
}

void DspCore::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x >= _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height))  return;
  }
  writeFillRect(x, y, w, h, color);
}

void DspCore::setNumFont(){
  #if CLOCKFONT_MONO
    setFont(&CLK_FONT1);
  #else
    setFont(&DS_DIGI56pt7b);
  #endif
  setTextSize(1);
}

void DspCore::loop(bool force) {
  static uint32_t lastCpuUpdate = 0;
  static uint32_t lastValue = 0;
  
#ifdef BATTERY_ON
  static uint32_t lastBatteryUpdate = 0;
  if (millis() - lastBatteryUpdate >= 1000) { // Обновляем каждую секунду
      readBattery();
      lastBatteryUpdate = millis();
  }
#endif
  
#ifdef CPU_LOAD
  // Обновляем каждую секунду
  if (millis() - lastCpuUpdate >= 1000) {
      uint32_t cpuUsage = _calculateCpuUsage();
      
      // Обновляем виджет только если значение изменилось
      if (cpuUsage != lastValue || force) {
          char buf[20];
          snprintf(buf, sizeof(buf), "CPU: %d%%", cpuUsage);
          cpuWidget.setText(buf);
          lastValue = cpuUsage;
          
          // Принудительно обновляем виджет
          if (force) {
              cpuWidget.setActive(true);
          }
      }
      lastCpuUpdate = millis();
  }
  // Проверяем, находимся ли мы на странице плеера
  if (display.mode() == PLAYER) {
      cpuWidget.setActive(true);
  } else {
      cpuWidget.setActive(false);
  }
#endif
}

#ifdef CPU_LOAD
uint32_t DspCore::_calculateCpuUsage() {
    static uint32_t lastUpdate = 0;
    if (millis() - lastUpdate >= 500) { // Обновляем каждые 500мс
        lastUpdate = millis();
        return perfmon_get_cpu_usage(0); // Получаем загрузку первого ядра
    }
    return 0;
}
#endif

#ifdef BATTERY_ON
void DspCore::readBattery() {
    static uint32_t lastRead = 0;
    if (millis() - lastRead < 100) return; // Ограничиваем частоту чтений
    lastRead = millis();

    float tempmVolt = 0;
    for(uint8_t i = 0; i < BATTERY_SAMPLES; i++) {
        tempmVolt += esp_adc_cal_raw_to_voltage(adc1_get_raw(USER_ADC_CHAN), &adc1_chars);
    }
    float mVolt = (tempmVolt / BATTERY_SAMPLES) / 1000;
    float rawVolt = (mVolt + 0.0028 * mVolt * mVolt + 0.0096 * mVolt - 0.051) / (ADC_R2 / (ADC_R1 + ADC_R2)) + DELTA;
    if (rawVolt < 0) rawVolt = 0;

    // Первый уровень сглаживания - скользящее среднее по 5 последним измерениям
    Volt5 = Volt4;
    Volt4 = Volt3;
    Volt3 = Volt2;
    Volt2 = Volt1;
    Volt1 = rawVolt;
    float firstSmooth = (Volt1 + Volt2 + Volt3 + Volt4 + Volt5) / 5;

    // Второй уровень сглаживания - циклический буфер из 10 измерений
    smoothBuffer[smoothIndex] = firstSmooth;
    smoothIndex = (smoothIndex + 1) % BATTERY_SMOOTH_COUNT;
    
    float sum = 0;
    for(uint8_t i = 0; i < BATTERY_SMOOTH_COUNT; i++) {
        sum += smoothBuffer[i];
    }
    Volt = sum / BATTERY_SMOOTH_COUNT;

    // Определение зарядки на основе изменения напряжения
    if (millis() - chargingState.lastCheck >= BATTERY_CHECK_INTERVAL) {
        chargingState.lastCheck = millis();
        float voltDelta = Volt - chargingState.lastVoltage;
        
        if (abs(voltDelta) < BATTERY_STABLE_THRESHOLD) {
            // Напряжение стабильно
            chargingState.stableCount++;
            if (chargingState.stableCount >= 3) { // Если напряжение стабильно 3 раза подряд
                chargingState.chargeCount = 0;
                chargingState.dischargeCount = 0;
                Charging = false; // Нет зарядки при стабильном напряжении
            }
        } else {
            chargingState.stableCount = 0; // Сбрасываем счетчик стабильности
            
            if (voltDelta > BATTERY_VOLTAGE_THRESHOLD && Volt >= BATTERY_CHARGE_MIN_VOLTAGE) {
                chargingState.chargeCount++;
                chargingState.dischargeCount = 0;
                if (chargingState.chargeCount >= 2) {
                    Charging = true;
                }
            } else if (voltDelta < -BATTERY_VOLTAGE_THRESHOLD) {
                chargingState.dischargeCount++;
                if (chargingState.dischargeCount >= 2) {
                    chargingState.chargeCount = 0;
                    chargingState.dischargeCount = 0;
                    Charging = false;
                }
            } else {
                chargingState.chargeCount = 0;
                chargingState.dischargeCount = 0;
            }
        }
        
        chargingState.lastVoltage = Volt;
    }

    // Расчет процента заряда
    uint8_t idx = 0;
    while (true) {
        if (Volt < vs[idx]) {ChargeLevel = 0; break;}
        if (Volt < vs[idx+1]) {
            mVolt = Volt - vs[idx];
            ChargeLevel = idx * 5 + round(mVolt /((vs[idx+1] - vs[idx]) / 5 ));
            break;
        }
        else {idx++;}
    }
    if (ChargeLevel < 0) ChargeLevel = 0;
    if (ChargeLevel > 100) ChargeLevel = 100;
}
#endif

#endif
