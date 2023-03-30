#pragma once
#ifndef LCU_DIRENT_WRAPPER_H
#define LCU_DIRENT_WRAPPER_H

#ifdef _WIN32
#include "dirent_windows.h"
#else
#include <dirent.h>
#endif // _WIN32

#endif // !LCU_DIRENT_WRAPPER_H
