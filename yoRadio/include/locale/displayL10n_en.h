#pragma once
#include <array>
/*************************************************************************************
    HOWTO:
    Copy this file to yoRadio/locale/displayL10n_custom.h
    and modify it
*************************************************************************************/
static const char mon[] = "mo";
static const char tue[] = "tu";
static const char wed[] = "we";
static const char thu[] = "th";
static const char fri[] = "fr";
static const char sat[] = "sa";
static const char sun[] = "su";

static const char monf[] = "monday";
static const char tuef[] = "tuesday";
static const char wedf[] = "wednesday";
static const char thuf[] = "thursday";
static const char frif[] = "friday";
static const char satf[] = "saturday";
static const char sunf[] = "sunday";

static const char jan[] = "january";
static const char feb[] = "february";
static const char mar[] = "march";
static const char apr[] = "april";
static const char may[] = "may";
static const char jun[] = "june";
static const char jul[] = "july";
static const char aug[] = "august";
static const char sep[] = "september";
static const char octt[] = "october";
static const char nov[] = "november";
static const char decc[] = "december";

static const char wn_N[]      = "NORTH";
static const char wn_NNE[]    = "NNE";
static const char wn_NE[]     = "NE";
static const char wn_ENE[]    = "ENE";
static const char wn_E[]      = "EAST";
static const char wn_ESE[]    = "ESE";
static const char wn_SE[]     = "SE";
static const char wn_SSE[]    = "SSE";
static const char wn_S[]      = "SOUTH";
static const char wn_SSW[]    = "SSW";
static const char wn_SW[]     = "SW";
static const char wn_WSW[]    = "WSW";
static const char wn_W[]      = "WEST";
static const char wn_WNW[]    = "WNW";
static const char wn_NW[]     = "NW";
static const char wn_NNW[]    = "NNW";

static const char* const dow[]     = { sun, mon, tue, wed, thu, fri, sat };
static const char* const dowf[]    = { sunf, monf, tuef, wedf, thuf, frif, satf };
static const char* const mnths[]   = { jan, feb, mar, apr, may, jun, jul, aug, sep, octt, nov, decc };
static const char* const wind[]    = { wn_N, wn_NNE, wn_NE, wn_ENE, wn_E, wn_ESE, wn_SE, wn_SSE, wn_S, wn_SSW, wn_SW, wn_WSW, wn_W, wn_WNW, wn_NW, wn_NNW, wn_N };

// device states, must match to evt::yo_state enum
static constexpr std::array<const char*,2> device_state_literal = {
    "[clock]",
    "[web-stream]"
    };
    
static const char    const_PlReady[]    = "[ready]";
static const char  const_PlStopped[]    = "[stopped]";
static const char  const_PlConnect[]    = "[connecting]";
static const char  const_DlgVolume[]    = "VOLUME";
static const char    const_DlgLost[]    = "* LOST *";
static const char  const_DlgUpdate[]    = "* UPDATING *";
static const char const_DlgNextion[]    = "* NEXTION *";
static const char const_getWeather[]    = "";
static const char  const_waitForSD[]    = "INDEX SD";

static const char        apNameTxt[]    = "AP NAME";
static const char        apPassTxt[]    = "PASSWORD";
static const char       bootstrFmt[]    = "Trying to %s";
static const char        apSettFmt[]    = "SETTINGS PAGE ON: HTTP://%s/";
#if EXT_WEATHER
static const char       weatherFmt[]    = "%s, %.1f\011C \007 feels like: %.1f\011C \007 pressure: %d мм \007 humidity: %s%% \007 wind: %.1f m/s [%s]";
#else
static const char       weatherFmt[]    = "%s, %.1f\011C \007 pressure: %d mm \007 humidity: %s%%";
#endif
static const char     weatherUnits[]    = "metric";   /* standard, metric, imperial */
static const char      weatherLang[]    = "en";       /* https://openweathermap.org/current#multi */

