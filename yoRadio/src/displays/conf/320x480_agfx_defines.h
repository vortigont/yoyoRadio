#pragma once
#include "clib/u8g2.h"

#define DSP_WIDTH       320
#define DSP_HEIGHT      480

// default unicode font
#define FONT_DEFAULT_U8G2   u8g2_font_6x12_t_cyrillic
#define FONT_DEFAULT_COLOR  62979

// what type of fonts are used for CLock widget (mandatory)
#define CLOCK_USE_AGFX_FONT
#define CLOCK_DATE_USE_U8G2_FONT

// clock fonts
#include "../fonts/DirectiveFour56.h"
#include "../fonts/DirectiveFour30.h"
#define FONT_CLOCK_H DirectiveFour56       // font for Hours
#define FONT_CLOCK_S DirectiveFour30       // font for Seconds

#define FONT_DATE           FONT_DEFAULT_U8G2
