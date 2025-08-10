#pragma once
#include <array>
/*************************************************************************************
    HOWTO:
    Copy this file to yoRadio/locale/displayL10n_custom.h
    and modify it
*************************************************************************************/
static const char mon[] = "пн";
static const char tue[] = "вт";
static const char wed[] = "ср";
static const char thu[] = "чт";
static const char fri[] = "пт";
static const char sat[] = "сб";
static const char sun[] = "вс";

static const char monf[] = "понедельник";
static const char tuef[] = "вторник";
static const char wedf[] = "среда";
static const char thuf[] = "четверг";
static const char frif[] = "пятница";
static const char satf[] = "суббота";
static const char sunf[] = "воскресенье";

static const char jan[] = "января";
static const char feb[] = "февраля";
static const char mar[] = "марта";
static const char apr[] = "апреля";
static const char may[] = "мая";
static const char jun[] = "июня";
static const char jul[] = "июля";
static const char aug[] = "августа";
static const char sep[] = "сентября";
static const char octt[] = "октября";
static const char nov[] = "ноября";
static const char decc[] = "декабря";

static const char wn_N[]      = "СЕВ";
static const char wn_NNE[]    = "ССВ";
static const char wn_NE[]     = "СВ";
static const char wn_ENE[]    = "ВСВ";
static const char wn_E[]      = "ВОСТ";
static const char wn_ESE[]    = "ВЮВ";
static const char wn_SE[]     = "ЮВ";
static const char wn_SSE[]    = "ЮЮВ";
static const char wn_S[]      = "ЮЖ";
static const char wn_SSW[]    = "ЮЮЗ";
static const char wn_SW[]     = "ЮЗ";
static const char wn_WSW[]    = "ЗЮЗ";
static const char wn_W[]      = "ЗАП";
static const char wn_WNW[]    = "ЗСЗ";
static const char wn_NW[]     = "СЗ";
static const char wn_NNW[]    = "ССЗ";

static const char* const dow[]     = { sun, mon, tue, wed, thu, fri, sat };
static const char* const dowf[]    = { sunf, monf, tuef, wedf, thuf, frif, satf };
static const char* const mnths[]   = { jan, feb, mar, apr, may, jun, jul, aug, sep, octt, nov, decc };
static const char* const wind[]    = { wn_N, wn_NNE, wn_NE, wn_ENE, wn_E, wn_ESE, wn_SE, wn_SSE, wn_S, wn_SSW, wn_SW, wn_WSW, wn_W, wn_WNW, wn_NW, wn_NNW, wn_N };

// device states, must match to evt::yo_state enum
static constexpr std::array<const char*,2> device_state_literal = {
"[простой]",
"[web-поток]"
};

static const char    const_PlReady[]    = "[готов]";
static const char  const_PlStopped[]    = "[остановлено]";
static const char  const_PlConnect[]    = "[соединение]";
static const char  const_DlgVolume[]    = "ГРОМКОСТЬ";
static const char    const_DlgLost[]    = "ОТКЛЮЧЕНО";
static const char  const_DlgUpdate[]    = "ОБНОВЛЕНИЕ";
static const char const_DlgNextion[]    = "NEXTION";
static const char const_getWeather[]    = "";
static const char  const_waitForSD[]    = "ИНДЕКС SD";

static const char        apNameTxt[]    = "ТОЧКА ДОСТУПА";
static const char        apPassTxt[]    = "ПАРОЛЬ";
static const char       bootstrFmt[]    = "Соединяюсь с %s";
static const char        apSettFmt[]    = "НАСТРОЙКИ: HTTP://%s/";
#if EXT_WEATHER
static const char       weatherFmt[]    = "%s, %.1f\011C \007 ощущается: %.1f\011C \007 давление: %d мм \007 влажность: %s%% \007 ветер: %.1f м/с [%s]";
#else
static const char       weatherFmt[]    = "%s, %.1f\011C \007 давление: %d mm \007 влажность: %s%%";
#endif
static const char     weatherUnits[]    = "metric";   /* standard, metric, imperial */
static const char      weatherLang[]    = "ru";       /* https://openweathermap.org/current#multi */

