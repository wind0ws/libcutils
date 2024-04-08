#pragma once
#ifndef LCU_LOGGER_H
#define LCU_LOGGER_H

//don't forget to '#define LOG_TAG "xxx" ' before include this header
#ifndef LOG_TAG
#error you forgot to 'define LOG_TAG' before include "logger.h"!
#endif // !LOG_TAG
#ifdef _LOG_TAG
#error DO NOT 'define LOG_TAG', define LOG_TAG instead!
#endif // _LOG_TAG

#ifndef LOG_TAG_PREFIX
#define LOG_TAG_PREFIX ""
#endif // !LOG_TAG_PREFIX

#ifndef LOG_TAG_SUFFIX
#define LOG_TAG_SUFFIX ""
#endif // !LOG_TAG_SUFFIX

// the real LOG TAG for macro to use.
#define _LOG_TAG   LOG_TAG_PREFIX LOG_TAG LOG_TAG_SUFFIX 

#define  _LCU_LOGGER_TYPE_XLOG  (0)
#define  _LCU_LOGGER_TYPE_SLOG  (1)

#ifndef LCU_LOGGER_SELECTOR
#define LCU_LOGGER_SELECTOR  _LCU_LOGGER_TYPE_XLOG
//#define LCU_LOGGER_SELECTOR  _LCU_LOGGER_TYPE_SLOG
#endif // !LCU_LOGGER_SELECTOR

#if(LCU_LOGGER_SELECTOR == _LCU_LOGGER_TYPE_SLOG)

#include "log/logger_facade_slog.h"

#elif(LCU_LOGGER_SELECTOR == _LCU_LOGGER_TYPE_XLOG)
	
#include "log/logger_facade_xlog.h"

#else

#error unsupport this LCU_LOGGER_SELECTOR type

#endif // LCU_LOGGER_SELECTOR


#endif // !LCU_LOGGER_H
