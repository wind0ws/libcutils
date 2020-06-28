#pragma once
#ifndef __LCU_DLFCN_WRAPPER_H
#define __LCU_DLFCN_WRAPPER_H

#ifdef _WIN32
#include "dlfcn_windows.h"
#else
#include <dlfcn.h>
#endif //_WIN32

#endif //__LCU_DLFCN_WRAPPER_H