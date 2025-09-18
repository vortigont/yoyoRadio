#include <Arduino_GFX_Library.h>
#include "generic.hpp"

namespace generic_boards {

    Arduino_GFX* create_display_dev_ILI9341(Arduino_DataBus *bus){

      bus = new Arduino_HWSPI(TFT_DC /* DC */, TFT_CS /* CS */);
      // gfx object
      return new Arduino_ILI9341(bus, TFT_RST /* RST */);
    }
}