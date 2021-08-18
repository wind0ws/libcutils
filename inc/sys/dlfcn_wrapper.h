#pragma once
#ifndef LCU_DLFCN_WRAPPER_H
#define LCU_DLFCN_WRAPPER_H

#ifdef _WIN32
#include "dlfcn_windows.h"
#else
#include <dlfcn.h>
#endif // _WIN32

#endif // !LCU_DLFCN_WRAPPER_H
