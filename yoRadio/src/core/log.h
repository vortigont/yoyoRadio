/*
LOG macro will enable/disable logs to serial depending on YO_DEBUG build-time flag
*/
#pragma once

#ifndef YO_DEBUG_PORT
#define YO_DEBUG_PORT Serial
#endif

// undef possible LOG macros
#ifdef LOG
  #undef LOG
#endif
#ifdef LOGV
  #undef LOGV
#endif
#ifdef LOGD
  #undef LOGD
#endif
#ifdef LOGI
  #undef LOGI
#endif
#ifdef LOGW
  #undef LOGW
#endif
#ifdef LOGE
  #undef LOGE
#endif

static constexpr const char* S_V = "V: ";
static constexpr const char* S_D = "D: ";
static constexpr const char* S_I = "I: ";
static constexpr const char* S_W = "W: ";
static constexpr const char* S_E = "E: ";

#if defined(YO_DEBUG_LEVEL) && YO_DEBUG_LEVEL == 5
	#define LOGV(tag, func, ...) YO_DEBUG_PORT.print(S_V); YO_DEBUG_PORT.print(tag); YO_DEBUG_PORT.print((char)0x9); YO_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGV(...)
#endif

#if defined(YO_DEBUG_LEVEL) && YO_DEBUG_LEVEL > 3
	#define LOGD(tag, func, ...) YO_DEBUG_PORT.print(S_D); YO_DEBUG_PORT.print(tag); YO_DEBUG_PORT.print((char)0x9); YO_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGD(...)
#endif

#if defined(YO_DEBUG_LEVEL) && YO_DEBUG_LEVEL > 2
	#define LOGI(tag, func, ...) YO_DEBUG_PORT.print(S_I); YO_DEBUG_PORT.print(tag); YO_DEBUG_PORT.print((char)0x9); YO_DEBUG_PORT.func(__VA_ARGS__)
	// compat macro
	#define LOG(func, ...) YO_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGI(...)
	// compat macro
	#define LOG(...)
#endif

#if defined(YO_DEBUG_LEVEL) && YO_DEBUG_LEVEL > 1
	#define LOGW(tag, func, ...) YO_DEBUG_PORT.print(S_W); YO_DEBUG_PORT.print(tag); YO_DEBUG_PORT.print((char)0x9); YO_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGW(...)
#endif

#if defined(YO_DEBUG_LEVEL) && YO_DEBUG_LEVEL > 0
	#define LOGE(tag, func, ...) YO_DEBUG_PORT.print(S_E); YO_DEBUG_PORT.print(tag); YO_DEBUG_PORT.print((char)0x9); YO_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGE(...)
#endif


// LOG tags
static constexpr const char* T_BOOT = "##[BOOT]#";
static constexpr const char* T_Player = "Player";
static constexpr const char* T_Module = "Module";
static constexpr const char* T_WebUI = "WebUI";
static constexpr const char* T_ModMGR = "ModMGR";
static constexpr const char* T_Display = "Display";
static constexpr const char* T_Clock = "Clock";
