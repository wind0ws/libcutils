#pragma once
#ifndef LCU_LOGGER_H
#define LCU_LOGGER_H

//don't forget "#define LOG_TAG "xxx" " before include this header
#ifndef LOG_TAG
#pragma message( "you are forget define LOG_TAG before include this header!!!" )
#endif // !LOG_TAG

#define _LCU_LOGGER_TYPE_XLOG   (0)
#define _LCU_LOGGER_TYPE_SLOG   (1)


#ifndef LCU_LOGGER_SELECTOR
#define LCU_LOGGER_SELECTOR  _LCU_LOGGER_TYPE_XLOG
//#define LCU_LOGGER_SELECTOR  _LCU_LOGGER_TYPE_SLOG
#endif // !LCU_LOGGER_SELECTOR

#if(_LCU_LOGGER_TYPE_SLOG == LCU_LOGGER_SELECTOR)

#include "log/logger_facade_slog.h"

#elif(_LCU_LOGGER_TYPE_XLOG == LCU_LOGGER_SELECTOR)
	
#include "log/logger_facade_xlog.h"

#else

#error "unsupport LCU_LOGGER_SELECTOR type"

#endif // LCU_LOGGER_SELECTOR


#endif // !LCU_LOGGER_H
