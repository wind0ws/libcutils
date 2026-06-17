#pragma once
#ifndef LCU_DEBUG_DIAGNOSTICS_H
#define LCU_DEBUG_DIAGNOSTICS_H

#include <stddef.h>

#if defined(_WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#include <stdlib.h>
#endif

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

#endif

#ifdef __cplusplus
}
#endif

#endif
