#pragma once

// NVS labels
static constexpr const char* T_balance = "balance";
static constexpr const char* T_devcfg = "devcfg";
static constexpr const char* T_equalizer = "equalizer";
static constexpr const char* T_volume = "volume";
static constexpr const char* T_station = "station";


static constexpr const char* T_ui_page_any = "ui_page_*";
static constexpr const char* T_ui_page_radio = "ui_page_radio";


// Player actions
static constexpr const char* T_player__all = "player_*";
static constexpr const char* T_player_ = "player_";             // webui element's ID prefix
static constexpr const char* T_getValues = "getValues";         // request for vol/EQ data from WebUI
static constexpr const char* T_prev = "prev";
static constexpr const char* T_next = "next";
static constexpr const char* T_toggle = "toggle";
static constexpr const char* T_volUp = "volUp";
static constexpr const char* T_volDown = "volDown";
static constexpr const char* T_playstation= "playstation";
static constexpr const char* T_trebble = "trebble";             // EQ
static constexpr const char* T_middle = "middle";               // EQ 
static constexpr const char* T_bass = "bass";                   // EQ

static constexpr const char* T_n_a = "n/a";                     // not available


// имена ключей конфигурации / акшены
static constexpr const char* A_ui_page_modules = "ui_page_modules";             // modules page
static constexpr const char* A_dev_profile = "dev_profile";                     // device profile setup

// Управление дисплеем
static constexpr const char* T_disp_brt = "disp_brt";               // display brightness


// Modules
static constexpr const char* T_mod_mgr_cfg = "/modules.json";
static constexpr const char* T_module = "module";                   // NVS namespace
static constexpr const char* T_set_modpreset_ = "set_modpreset_";   // prefix for profile selector DOM id
static constexpr const char* T_last_profile = "last_profile";
static constexpr const char* T_profile = "profile";
static constexpr const char* T_profiles = "profiles";


static constexpr const char* T_cfg = "cfg";
static constexpr const char* T_brightness = "brightness";
