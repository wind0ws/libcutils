#pragma once
#ifndef LCU_LOG_SANITIZER_H
#define LCU_LOG_SANITIZER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 原地脱敏日志消息中的敏感字段
 *
 * 扫描从 body_offset 开始的消息体，匹配注册的敏感键模式（如 token=, apikey:）
 * 并将值替换为固定 "***"。不碰 body_offset 之前的头部区域（时间戳/tag/func:line）。
 *
 * @param msg           日志消息缓冲区（NUL 结尾，可写）
 * @param body_offset   消息体起点偏移（头部长度）
 * @return              脱敏后的新字符串长度（不含 NUL，可能小于原长度）
 *
 * @note 线程安全：运行期纯只读模式表，无锁。
 * @note 模式表为空时立即返回原长度（零开销）。
 */
size_t log_sanitize_in_place(char* msg, size_t body_offset);

/**
 * @brief 注册敏感字段模式（裸键名，如 "device_sn"）
 *
 * @param pattern  敏感键名（库会 strdup 内部拷贝）
 * @return         0 成功，-1 失败（重复或内存不足）
 *
 * @note 并发契约：仅在 lcu_global_init() 期间单线程调用，运行期禁止。
 * @note 生命周期：模式表在 lcu_global_cleanup() -> xlog_global_cleanup() 中被
 *       log_sanitize_clear_patterns() 释放。cleanup 完成后严禁再调用
 *       log_sanitize_in_place（否则与 clear 并发会 use-after-free）。
 *       生产侧默认仅用内置表，如需业务自定义键，须在 init 期间注册。
 */
int log_sanitize_register_pattern(const char* pattern);

/**
 * @brief 清空整张模式表（包括内置默认模式）
 *
 * 释放所有动态分配的模式字符串。清空后可重新注册，实现完全自定义模式表。
 *
 * @note lcu_global_cleanup() 会自动调用此函数。
 * @note 并发契约：仅在 init/cleanup 单线程上下文调用。
 */
void log_sanitize_clear_patterns(void);

/**
 * @brief 恢复内置默认模式表（测试/调试用）
 *
 * 在 clear_patterns 之后调用可恢复内置模式。生产代码不应调用此函数。
 */
void log_sanitize_restore_builtins(void);

#ifdef __cplusplus
}
#endif

#endif // LCU_LOG_SANITIZER_H
