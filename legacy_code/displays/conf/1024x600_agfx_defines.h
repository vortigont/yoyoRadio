/*************************************************************************************
    1024х600  displays configuration file.
*************************************************************************************/

#pragma once
#include "clib/u8g2.h"

#ifndef DSP_WIDTH
#define DSP_WIDTH       1024
#endif
#ifndef DSP_HEIGHT
#define DSP_HEIGHT      600
#endif

// default unicode font
#define FONT_DEFAULT_U8G2   u8g2_font_10x20_t_cyrillic
#define FONT_MEDIUM_U8G2    u8g2_font_10x20_t_cyrillic
#define FONT_DEFAULT_COLOR  62979

// what type of fonts are used for CLock widget (mandatory)
#define CLOCK_USE_AGFX_FONT
#define CLOCK_DATE_USE_U8G2_FONT

// clock fonts
#include "fonts/DirectiveFour56.h"
#include "fonts/DirectiveFour30.h"
#define FONT_CLOCK_H DirectiveFour56       // font for Hours
#define FONT_CLOCK_S DirectiveFour30       // font for Seconds

#define FONT_DATE           FONT_DEFAULT_U8G2
