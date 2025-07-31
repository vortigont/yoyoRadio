#include "../core/options.h"
#if DSP_MODEL==DSP_JC1060P470

#include "display_JC1060P470C.h"
#include "display/Arduino_DSI_Display.h"
#include "fonts/bootlogo.h"
#include "../core/config.h"
#include "../core/network.h"

#include "tools/l10n.h"

#define DSI_PANEL

static Arduino_ESP32DSIPanel *dsipanel{nullptr};
//static Arduino_DSI_Display *gfx{nullptr};
DspCore* dsp{nullptr};

bool create_display_dev(){
  if (!dsipanel){
    dsipanel = new Arduino_ESP32DSIPanel(
      40 /* hsync_pulse_width */,
      160 /* hsync_back_porch */,
      160 /* hsync_front_porch */,
      10 /* vsync_pulse_width */,
      23 /*vsync_back_porch  */,
      12 /* vsync_front_porch */,
      48000000 /* prefer_speed */);
  }

  if (!dsipanel){
    Serial.println("Can't create DSI Palel");
    return false;
  }
  
  dsp = new DspCore(dsipanel);
  return dsp != nullptr;
}


void DspCore::initDisplay() {
  ledcAttach(LCD_LED, 1000, 8);
  //ledcOutputInvert(TFT_BLK, true);
  ledcWrite(LCD_LED, 200);

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
  
  Serial.println("[JC1060P470C] initDisplay completed successfully");
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

void DspCore::sleep(void) { 
  Serial.println("DspCore::sleep");
  //std::lock_guard<std::mutex> lock(_mtx);
  displayOff();
  ledcWrite(0, 0); // Выключаем подсветку через PWM
}

void DspCore::wake(void) {
  Serial.println("DspCore::wake");
  //std::lock_guard<std::mutex> lock(_mtx);
  displayOn();
  ledcWrite(0, map(config.store.brightness, 0, 100, 0, 255)); // Устанавливаем яркость через PWM
}

void DspCore::writePixelPreclipped(int16_t x, int16_t y, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x > _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height)) return;
  }
  Arduino_DSI_Display::writePixelPreclipped(x, y, color);
}

void DspCore::writeFillRectPreclipped(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x >= _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height))  return;
  }

  Arduino_DSI_Display::writeFillRectPreclipped(x, y, w, h, color);
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
