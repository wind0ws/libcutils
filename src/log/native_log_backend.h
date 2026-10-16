#pragma once
#ifndef LCU_NATIVE_LOG_BACKEND_H
#define LCU_NATIVE_LOG_BACKEND_H

#include "log/logger_data.h" /* lcu LogLevel (OFF=0..UNKNOWN=6) */
#include <stddef.h>          /* size_t */

/* 平台 native 日志后端分发层.
 * 统一 xlog.c / slog.c 两处 native 后端调用为单一宏 LCU_NATIVE_LOG_PRINT.
 * body 为预格式化字符串(func:line/timestamp/tag/tid 由各自机制处理),
 * 故宏仅需 3 参数, 无 TRACE 变体. */

#if defined(__ANDROID__)
	#include <android/log.h>
#elif defined(__OHOS__)
	/* 前向声明 OH_LOG_Print, 不 include <hilog/log.h>: 后者 typedef enum{} LogLevel
	 * 与 lcu logger_data.h 的 LogLevel 重定义冲突. 用 int 参数声明(枚举按 int 传参,
	 * AAPCS/Itanium ABI 兼容; OHOS 无 -fshort-enums 实测确认).
	 * 保留 __format__(os_log,5,6) 属性: 实测 %{public}s + 单 char* 实参零警告,
	 *   且为未来误传提供编译期格式检查安全网(检查是收益非负担). */
	extern int OH_LOG_Print(int type, int level, unsigned int domain,
	                        const char *tag, const char *fmt, ...)
	    __attribute__((__format__(os_log, 5, 6)));
	#define LCU_OHOS_LOG_APP    0  /* LogType LOG_APP */
#endif

/* lcu LogLevel(0..6) -> native 日志级别表. 按 lcu LogLevel 枚举索引.
 * index 0 (LOG_LEVEL_OFF) 实际不可达(native 平台 min_level 默认 VERBOSE=1, 门控拦截), 故值无影响;
 *   用 ANDROID_LOG_UNKNOWN 对齐 slog.c 原值(index 6 由 clamp 拦截, 表内值同 ERROR).
 * OHOS 表用裸整数(LOG_DEBUG=3/INFO=4/WARN=5/ERROR=6), VERBOSE->DEBUG(OHOS 无 verbose) */
#if defined(__ANDROID__) || defined(__OHOS__)
static const int g_lcu_native_level[] = {
#if defined(__ANDROID__)
	ANDROID_LOG_UNKNOWN, ANDROID_LOG_VERBOSE, ANDROID_LOG_DEBUG,
	ANDROID_LOG_INFO, ANDROID_LOG_WARN, ANDROID_LOG_ERROR, ANDROID_LOG_ERROR
#else /* __OHOS__ */
	6, 3, 3, 4, 5, 6, 6  /* OFF->ERR, VERBOSE->DEBUG, DEBUG, INFO, WARN, ERR, UNKNOWN->ERR */
#endif
};

/* 钉死表大小覆盖 LogLevel 0..6, 防 off-by-one */
_Static_assert(sizeof(g_lcu_native_level) / sizeof(g_lcu_native_level[0]) == 7,
               "native level table must cover LogLevel 0..6");

/* 越界保护(对齐 slog.c 原 (level<0||level>5)?ERR:table[level] 防御语义):
 * level 实际来自 LogLevel 枚举常量(1..5)+min_level 门控, 不可越界, 但保留防御对称 slog.c.
 * static inline 单次求值, 可常量折叠, 无额外运行时开销 */
static inline int lcu_level_to_native(int lcu_level)
{
	const size_t n = sizeof(g_lcu_native_level) / sizeof(g_lcu_native_level[0]);
	if (lcu_level < 0 || (size_t)lcu_level >= n)
	{
		return g_lcu_native_level[n - 1]; /* out-of-range -> ERROR */
	}
	return g_lcu_native_level[lcu_level];
}
#endif /* __ANDROID__ || __OHOS__ */

/* 统一 native 后端分发宏. body 为预格式化字符串, 其内的 % 是数据不被后端 fmt 解释;
 * level 经 lcu_level_to_native inline clamp 保护.
 * lcu_level_to_native 仅在 native 平台定义; 非 native 平台宏为 no-op 不引用它, 零依赖. */
#if defined(__ANDROID__)
	#define LCU_NATIVE_LOG_PRINT(lcu_level, tag, body) \
	    __android_log_print(lcu_level_to_native(lcu_level), (tag), "%s", (body))
#elif defined(__OHOS__)
	extern unsigned int g_lcu_ohos_domain;  /* 定义在 xlog.c, 默认 0 */
	#define LCU_NATIVE_LOG_PRINT(lcu_level, tag, body) \
	    ((void)OH_LOG_Print(LCU_OHOS_LOG_APP, lcu_level_to_native(lcu_level), \
	                        g_lcu_ohos_domain, (tag), "%{public}s", (body)))
#else
	#define LCU_NATIVE_LOG_PRINT(lcu_level, tag, body) ((void)0)
#endif

#endif /* !LCU_NATIVE_LOG_BACKEND_H */
