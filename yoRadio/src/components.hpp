#pragma once
#include "core/player.h"

/*
    Project modules are shared objects that could be accesses via `extern` declaration
*/


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

// create and init devices for JC3248W535 board
void load_device_JC3248W535();

// create and init devices for JC1060P470 board
void load_device_JC1060P470();

// generic
void load_device_ILI9341();