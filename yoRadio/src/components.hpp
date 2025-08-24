#pragma once
#include "core/player.h"

/*
    Project modules are shared objects that could be accesses via `extern` declaration
*/


// Manager that creates dynamicaly loaded components
#include "modules/mod_manager.hpp"
extern ModuleManager zookeeper;

// ******************
extern AudioController* player;



/**
 * @brief Loads device profile and user configuration from FS
 * creates defined devices and components
 * 
 */
void load_hwcomponets_configuration();

/**
 * @brief select and load specific device profile
 * the profile name is stored in NVS, based on the profile a corresponding
 * set of device componets are created
 * 
 */
void load_device_profile_from_NVS();


void load_device_JC3248W535();
void load_device_JC1060P470();


void create_player(dac_type_t dac);
