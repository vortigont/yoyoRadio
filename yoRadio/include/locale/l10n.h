#ifndef _display_l10n_h
#define _display_l10n_h

//==================================================
#if L10N_LANGUAGE==RU
  #define L10N_PATH "displayL10n_ru.h"
#else
  #define L10N_PATH "displayL10n_en.h"
#endif

#if __has_include("displayL10n_custom.h")
  #include "displayL10n_custom.h"
#else
  #include L10N_PATH
#endif
//==================================================

#endif
