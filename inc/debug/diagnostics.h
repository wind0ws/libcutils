#pragma once
#ifndef LCU_DEBUG_DIAGNOSTICS_H
#define LCU_DEBUG_DIAGNOSTICS_H

// NOTE: This header does NOT include <stdlib.h> or <crtdbg.h> to allow callers
// (especially mem_debug.h) to control include order for _CRTDBG_MAP_ALLOC.
// Callers must ensure required headers are included before using CRT APIs.

#ifdef __cplusplus
extern "C" {
#endif

void lcu_diagnostics_init(void);
void lcu_diagnostics_deinit(void);
void lcu_diagnostics_write(const char *category, const char *message);
void lcu_diagnostics_writef(const char *category, const char *format, ...);

#if defined(_MSC_VER)
__declspec(noreturn)
#elif defined(__GNUC__)
__attribute__((noreturn))
#endif
void lcu_diagnostics_assert_fail(const char *expression, const char *function_name,
	const char *file_path, int file_line);

#if defined(_MSC_VER)
__declspec(noreturn)
#elif defined(__GNUC__)
__attribute__((noreturn))
#endif
void lcu_diagnostics_fatalf(const char *category, const char *format, ...);

#if defined(_WIN32) && defined(_DEBUG)

// Need CRT debug types and APIs for the inline functions below.
// If mem_debug.h included us, these headers are already in.
// Otherwise, include them now (won't affect _CRTDBG_MAP_ALLOC since it's too late anyway).
#if(!defined(_CRTDBG_H_) && !defined(_INC_CRTDBG))
#include <stdlib.h>
#include <crtdbg.h>
#endif

typedef struct lcu_diagnostics_crt_api
{
	int (__cdecl *set_report_mode)(int report_type, int report_mode);
	_HFILE (__cdecl *set_report_file)(int report_type, _HFILE report_file);
	int (__cdecl *set_report_hook)(int mode, _CRT_REPORT_HOOK hook);
	int (__cdecl *set_report_hook_wide)(int mode, _CRT_REPORT_HOOKW hook);
	int (__cdecl *set_debug_flag)(int debug_flag);
	int (__cdecl *dump_memory_leaks)(void);
	unsigned int (__cdecl *set_abort_behavior)(unsigned int flags, unsigned int mask);
} lcu_diagnostics_crt_api_t;

void lcu_diagnostics_register_crt(const lcu_diagnostics_crt_api_t *crt_api);
void lcu_diagnostics_unregister_crt(const lcu_diagnostics_crt_api_t *crt_api);

static inline lcu_diagnostics_crt_api_t lcu_diagnostics_current_crt_api(void)
{
	lcu_diagnostics_crt_api_t crt_api =
	{
		_CrtSetReportMode,
		_CrtSetReportFile,
		_CrtSetReportHook2,
		_CrtSetReportHookW2,
		_CrtSetDbgFlag,
		_CrtDumpMemoryLeaks,
		_set_abort_behavior
	};
	return crt_api;
}

static inline void lcu_diagnostics_register_current_crt(void)
{
	lcu_diagnostics_crt_api_t crt_api = lcu_diagnostics_current_crt_api();
	lcu_diagnostics_register_crt(&crt_api);
}

static inline void lcu_diagnostics_unregister_current_crt(void)
{
	lcu_diagnostics_crt_api_t crt_api = lcu_diagnostics_current_crt_api();
	lcu_diagnostics_unregister_crt(&crt_api);
}

#endif // defined(_WIN32) && defined(_DEBUG)

#ifdef __cplusplus
}
#endif

#endif // !LCU_DEBUG_DIAGNOSTICS_H
