/**
 * include this header file at your source file first line.
 * On windows, in debug mode, it will help you find memory leak.
 */

#pragma once
#ifndef __LCU_STDAFX_H
#define __LCU_STDAFX_H

#ifdef _WIN32
#ifdef _DEBUG
// must keep next 3 line on your top source file,
// otherwise it won't tell you leak memory on which file with line number.
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
//allocations to be of _CLIENT_BLOCK type
#define __MYDEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new __MYDEBUG_NEW

//only need call once on your main function first line!
#define  enable_memleak_check()\
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF)

#else

#define  enable_memleak_check() 

#endif // _DEBUG
#endif // _WIN32

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#endif // __LCU_STDAFX_H