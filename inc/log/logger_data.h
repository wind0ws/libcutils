#pragma once
#ifndef LCU_LOGGER_DATA_H
#define LCU_LOGGER_DATA_H

//if your os(for example, RTOS) not support freopen printf, enable this macro
//#define _LCU_LOGGER_UNSUPPORT_PRINTF_REDIRECT  1

#ifndef _PRINTF_FUNC
#include <stdio.h>
#define _PRINTF_FUNC printf
#endif // !_PRINTF_FUNC

#ifndef _SUFFIX_LOG
#define _SUFFIX_LOG "\n"
#endif // !_SUFFIX_LOG

#ifdef _WIN32
#define _STDOUT_NODE ("CON")
#ifndef __func__
#define __func__ __FUNCTION__
#endif // !__func__
#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCSIG__ 
#endif // !__PRETTY_FUNCTION__
#else
#define _STDOUT_NODE ("/dev/tty")
#endif // _WIN32

#define LOG_STAR_LINE "****************************************************************"

#if(defined(_MSC_VER) && defined(__cplusplus))
#pragma warning(disable:26812) //for disable enum class warning in c++
#endif // _MSC_VER && __cplusplus

// DO NOT CHANGE ORDER, or maybe trigger bug!
typedef enum
{
	LOG_LEVEL_OFF = 0,
	LOG_LEVEL_VERBOSE = 1,
	LOG_LEVEL_DEBUG = 2,
	LOG_LEVEL_INFO = 3,
	LOG_LEVEL_WARN = 4,
	LOG_LEVEL_ERROR = 5,
	LOG_LEVEL_UNKNOWN
} LogLevel;


#endif // !LCU_LOGGER_DATA_H
