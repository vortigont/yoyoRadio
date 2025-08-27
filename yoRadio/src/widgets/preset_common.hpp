#pragma once
#include "muipp_widgets.hpp"

// Шрифты для часов
#include "fonts/DirectiveFour56.h"
#include "fonts/DirectiveFour30.h"
#define FONT_CLOCK_DOTS_H DirectiveFour56       // font for Hours
#define FONT_CLOCK_DOTS_S DirectiveFour30       // font for Seconds

// Шрифты текст
#define FONT_VERY_SMALL_U8G2    u8g2_font_6x12_t_cyrillic
#define FONT_SMALL_U8G2         u8g2_font_10x20_t_cyrillic
#define FONT_DEFAULT_COLOR      62979

/**
 * @brief this struct is a configuration placeholder that carries a widget type and an abstract pointer to it's config struct
 * used to define static configuration presets
 * 
 */
struct widget_cfgitem_t {
    // type of the widget
    yoyo_wdgt_t wtype;
    // a pointer to it's configuration
    const void* cfg;
};
