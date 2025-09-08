/*
  This file is a part of yoyoRadio project
  https://github.com/vortigont/yoyoRadio/

  a fork of yoRadio project from https://github.com/e2002/yoradio

  Copyright © 2025 Emil Muratov (Vortigont)
*/

/*
  Widget controllers - an EmbUI Units that stich MuiPP derived widget objects
  with configuration features and EmbUI
  are used to:
    - serialize/deserialize widgets configuration and config presets
    - provide Web-based controls for widget class
    - interact between WebUI and Widget
    - interact between event bus and Widget additionaly if needed

*/

#pragma once
#include "embui_units.hpp"
#include "muipp_widgets.hpp"

class SpectrumAnalyser_Controller : public EmbUIUnit_Presets {
  std::shared_ptr<SpectrumAnalyser_Widget> _unit;

public:
  SpectrumAnalyser_Controller(const char* label, const char* name_space, std::shared_ptr<SpectrumAnalyser_Widget> unit) :
    EmbUIUnit_Presets(label, name_space), _unit(unit) {}

  // start or initialize unit
  void start() override {};

  // stop or deactivate unit without destroying it
  void stop() override {};

private:

  /**
   * @brief derived method should generate object's configuration into provided JsonVariant
   * 
   * @param cfg 
   * @return JsonVariantConst 
   */
  void generate_cfg(JsonVariant cfg) const override;

  /**
   * @brief load configuration from a json object
   * method should be implemented in derived class to process
   * class specific json object
   * @param cfg 
   */
  void load_cfg(JsonVariantConst cfg) override;

};
