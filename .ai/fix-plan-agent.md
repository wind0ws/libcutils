# libcutils 修复计划（Agent版）

**生成时间**: 2026-06-04  
**评审方法**: 5阶段对抗验证（现状核对→方案设计→蓝军攻击→Codex验证→综合Plan）  
**Token消耗**: 1,101,218（9个子Agent，79分钟）  

---

## 执行摘要

已对上一轮评审报告的所有问题点**逐文件实读代码核对**，推翻多个误判。经蓝军对抗攻击 + Codex 深度验证后，最终修复计划收敛为：

**P0（2项，可直接实施）**: CRITICAL-2-array 首行加 mem_debug.h；CRITICAL-2-hashmap 按实例注入 allocator_t（需修正递归边界）

**P1（4项）**: CRITICAL-1 foreach 回调守卫；HIGH-1 put 返回值文档；函数标注规范（四段式 pilot）

**P2（2项，opt-in）**: xlog 批量 flush（默认OFF）；CI mem_debug 检查（WARNING-first）

**降级为标注（3项）**: HIGH-3 thpool、HIGH-4 file_logger、HIGH-5 slog（前提不成立或保守优先）

**关键判定**: xlog 默认**不改变时序语义**（稳定性优先）；方案A"缩小临界区"经蓝军证明会破坏时序，仅作 opt-in 并明确标注降级。

---

## 现状核对（推翻上轮多个结论）

| 问题ID | 上轮结论 | 实际代码核实 | 状态 |
|--------|---------|-------------|------|
| CRITICAL-1 | foreach 迭代期间 rehash 无保护 | 已核实 foreach(389-403) 全程持锁，并发 put 会被阻塞 | ❌ 原结论不成立 |
| CRITICAL-2-hashmap | 绕过 mem_debug.h | 确认直接用 malloc/calloc/free，#undef 块有意绕过 | ✅ 确认 |
| CRITICAL-2-array | 绕过 mem_debug.h | 确认直接用 calloc/realloc/free，无 #undef 块 | ✅ 确认 |
| CRITICAL-2-list | 绕过 mem_debug.h | 已用 allocator_t→lcu_calloc（被追踪） | ❌ 实际已解决 |
| HIGH-1 | put 返回 UAF | 确认 fn_value_free 后返回悬垂指针，已有文档但不够直接 | ✅ 确认 |
| HIGH-2 | xlog 全局锁 | 确认锁内含格式化+IO（531-643全覆盖） | ✅ 确认 |
| HIGH-3 | thpool 单队列锁 | 已核实 jobqueue 有独立 rwmutex(412-431,438-461) | ❌ 原结论不成立 |
| HIGH-6 | ring_buffer_clear 无文档 | 已核实头文件(211-213)有明确 WARN | ❌ 文档已存在 |
| HIGH-7 | hashmap rehash 内存峰值 | 已核实 calloc 失败时优雅降级(154-156)，无泄漏 | ❌ 已正确处理 |

**核心事实**:
- xlog 锁范围: `XLOG_LOCK(531)` 到 `XLOG_UNLOCK(643)`，覆盖格式化+IO
- hashmap 迭代保护: `hashmap_enter(389)` 到 `hashmap_leave(403)` 包裹整个循环
- thpool 锁机制: jobqueue 有独立 `pthread_mutex_t rwmutex`(57)，push/pull 均持锁

---

## P0 - 立即修复（1-2周）

### 1. CRITICAL-2-array: 首行包含 mem_debug.h

**问题**: array.c 直接用 calloc/realloc/free，未包含 mem_debug.h，分配不被追踪

**修复**:
```c
// src/data/array.c 第1行插入
#include "mem/mem_debug.h"
// 删除第22行 #include <malloc.h>（mem_debug.h已含）
```

**验证**: 以 `-D_LCU_MEM_CHECK_FEATURE_ENABLE=1` 构建，故意制造泄漏，allocation_tracker 应报告

**风险**: 低。已核实 array.c 不在 allocation_tracker 依赖链上，无递归风险

**状态**: ✅ 可直接实施

---

### 2. CRITICAL-2-hashmap: 按实例注入 allocator_t

**问题**: hashmap 直接用 malloc/calloc/free 绕过追踪；但 #undef 有正当理由（allocation_tracker 内部 map 必须用 raw libc 避免递归）

**蓝军/Codex 修正**:
- ❌ 原方案遗漏 **Entry 分配路径**（private_create_entry@234）
- ❌ 原方案未明确 **buckets 显式 memset(0)**（allocator_t 无清零语义保证）
- ✅ 追踪器内部 map 必须用 `allocator_calloc_raw`（断递归）

**最终方案**:
1. 新增 `allocator_calloc_raw/allocator_malloc_raw`（inc/mem/allocator.h + src/mem/allocator.c）
2. 新增 `hashmap_create_ex(..., const allocator_t*)`（inc/data/hashmap.h）
3. `hashmap_create` 改为转调 `hashmap_create_ex(..., &allocator_calloc)`（默认被追踪）
4. struct Hashmap 增 `const allocator_t* allocator` 字段（不透明，ABI安全）
5. **覆盖三条分配路径**:
   - struct Hashmap(83): `map->allocator->alloc(sizeof(hashmap_t))` + memset
   - buckets(108,151): `map->allocator->alloc(bucketCount*sizeof(Entry*))` + **显式 memset(0)**
   - Entry(234): `map->allocator->alloc(sizeof(Entry))`（private_create_entry 改签名）
6. allocation_tracker.c(101-102) 改用 `hashmap_create_ex(..., &allocator_calloc_raw)`

**关键约束**:
- buckets 必须显式 memset(0)，不依赖 allocator 清零语义
- 追踪器内部 map 用 raw 分配器断递归（lcu_malloc→tracker→hashmap_put→lcu_malloc）
- 在 tracker 改动处加注释固化"必须用 raw 避免递归"

**验证**:
```bash
# 递归安全（关键）
cmake -D_LCU_MEM_CHECK_FEATURE_ENABLE=1 ..
# 向空 hashmap put 触发 Entry 分配 + 足够多 key 触发 rehash
# 确认不栈溢出/不死锁（旧 raw 路径若误接 lcu_calloc 会立即崩）
```

**状态**: ⚠️ 需修正后实施（三路径全覆盖 + 显式 memset）

---

## P1 - 短期修复（1个月）

### 3. CRITICAL-1: hashmap_foreach 回调守卫

**蓝军判定**: ❌ "仅文档不充分" - 回调重入会死锁或 UAF，需 fail-fast 机制

**修复**:
1. struct Hashmap 增 `#ifdef _DEBUG int iterating; #endif`（opaque 结构，ABI 安全）
2. foreach 循环前 `++map->iterating`，循环后 `--map->iterating`（仅 Debug）
3. put/remove/clear/expand 入口加 `ASSERT(0==map->iterating)`（Debug 捕获非法重入）
4. 头文件按四段式补全契约文档（见下）

**状态**: ⚠️ 需修正后实施

---

### 4. HIGH-1: hashmap_put 返回值文档强化

**问题**: fn_value_free 非空时返回悬垂指针，现有文档不够直接

**修复**: 纯注释，按四段式改写 hashmap.h(122-135)

```c
/**
 * @par Thread Safety:
 *      CONDITIONALLY-SAFE - serialized only if a lock was supplied at create.
 *
 * @warning Return-value lifetime: if fn_value_free was set, the replaced old
 *      value is freed INSIDE this call before it returns. The returned pointer
 *      is then DANGLING - you may ONLY compare it to NULL (to learn whether a
 *      previous mapping existed) and nothing else.
 *
 * @note Correct usage:
 * @code
 *      bool replaced = (hashmap_put(map, k, v) != NULL);
 * @endcode
 */
```

**状态**: ✅ 可直接实施

---

### 5-6. 函数标注规范（四段式 Doxygen）

**范围**: 本轮 pilot 限定 hashmap/ring_buffer（不过度承诺全模块）

**规范**:
```c
/**
 * @par Thread Safety:
 *      <SAFE | CONDITIONALLY-SAFE | UNSAFE> - <机制一句话>
 *
 * @pre <调用方必须保证的前置条件>
 *
 * @warning Misuse: <违反后的不可挽回后果>
 *
 * @note Correct usage:
 * @code
 *      <最小正确示例>
 * @endcode
 */
```

**应用清单**:
- hashmap.h 文件级 Thread Safety 块
- hashmap_foreach（@pre 禁回调内改 map + @warning rehash 致 UAF）
- hashmap_put（@warning 返回值悬垂仅可比 NULL）
- hashmap_remove/clear/get/size（补 @par Thread Safety 行）
- ring_buffer_clear（@par UNSAFE + @pre 静默所有读写线程）

**状态**: ✅ 可直接实施

---

## P2 - 中期优化（2-3个月）

### 7. xlog 批量 flush（opt-in，默认 OFF）

**蓝军判定**: ❌ 方案A"缩小临界区"**破坏时序**（格式化窗口导致乱序）

**最终决策**: 
- 默认**不改变 xlog 行为**（保持时序语义，稳定性优先）
- 仅提供方案C（批量 flush）作为 opt-in，默认 OFF

**方案C 细节**:
- 仅 LOG_FLUSH_MODE_EVERY 下生效
- 每 N 条（N=8）触发一次 fflush（纯计数，不引入锁内 time_util 调用）
- ERROR 级强制立即 flush
- `#if XLOG_BATCH_FLUSH` 条件编译，默认 OFF

**时序保持证明**: flush 仅影响落盘时机，不影响 printf 写入 libc buffer 的顺序

**状态**: ⚠️ 需修正后实施（放弃 time_util 锁内调用）

---

### 8. CI mem_debug.h 检查（WARNING-first）

**蓝军判定**: ❌ 正则会误报/漏报（函数指针、宏包装、注释中 free 字样）

**修正方案**:
- 规则: 扫描 src/**/*.c，首个 include 非 mem_debug.h 且有直接 libc 分配调用
- 白名单: {allocator.c, allocation_tracker.c}（deliberate raw-allocation）
- 初版 `message(WARNING)` 试运行，确认零误报后再升级 `FATAL_ERROR`

**状态**: ⚠️ 需进一步验证

---

## P3 - 降级为标注

### 9. HIGH-3: thpool 单队列锁

**Codex 核实**: jobqueue 已有独立 rwmutex(412-432, 438-461)，原"单队列锁瓶颈"结论不成立

**最终方案**: 从 fixes 移除，作为"已知限制"文档化（不引入 work-stealing）

**状态**: 降级为标注

---

### 10. HIGH-4: file_logger 职责过载

**保守理由**: SRP 拆分风险大于收益（需设计 3 模块生命周期同步）

**最终方案**: 仅文档化职责边界 + 补单测

**状态**: 降级为标注

---

### 11. HIGH-5: slog 循环依赖

**Codex 核实**: 真实代码无运行期递归（slog.h 不 include file_logger）

**最终方案**: 澄清无循环，不做宏改造（原方案为 no-op）

**状态**: 降级为标注

---

## xlog 决策（重点说明）

### 时序保持判定

已核实 `__xlog_internal_print` 在 `XLOG_LOCK(531)` 到 `XLOG_UNLOCK(643)` 间完成格式化+emit。

**蓝军 blocker**: 方案A"缩小临界区"会破坏时序
- 当前: 跨线程顺序 = 锁授予顺序（先进临界区先 emit）
- 方案A: 跨线程顺序 = 格式化完成顺序（乱序窗口被格式化耗时放大）
- 同一线程内保序，单条日志不交错，但跨线程可能乱序

**推荐方案**:
- **默认**: 保持现状（不改时序语义）
- **opt-in**: 方案C 批量 flush（-DXLOG_BATCH_FLUSH=1）
- **不采用**: 方案A（时序降级，仅文档标注可选）、方案B（稳定性风险高）

---

## 稳定性保障

### ABI 兼容性

| 改动项 | ABI 影响 | 证明 |
|--------|---------|------|
| hashmap_create | 零影响 | 签名不变，改为转调 _ex |
| hashmap_create_ex | 零影响 | 新增符号 |
| struct Hashmap.allocator | 零影响 | 不透明类型，仅 hashmap.c 可见 |
| xlog 优化 | 零影响 | g_xlog_cfg/g_xlog_mutex 为 file-static |
| 纯注释 | 零影响 | 不改任何符号 |

### 验证矩阵

| 构建模式 | 验证点 |
|---------|-------|
| 常规构建 | 编译通过，nm 检查符号，ctest 全绿 |
| mem-check 构建 | -D_LCU_MEM_CHECK=1，递归安全测试，追踪有效性 |
| xlog opt-in | 默认 OFF vs ON 双版本回归 |

### 回滚策略

- hashmap: 3 commit（allocator raw / _ex / tracker），最小回滚仅还原 tracker 一行
- xlog: 条件编译，`-DXLOG_*=0` 即时回退
- 纯注释: `git checkout` 零残留

---

## Open Risks

1. **mem-check 开启后新暴露泄漏**: 原未追踪的 hashmap/array 进入追踪表，既有泄漏会新报告（预期正向）
2. **递归约束依赖注释**: 未来新增 hashmap 实例忘记用 raw 分配器会重新引入递归
3. **xlog 方案A 时序降级**: opt-in 启用后跨线程可能微秒级乱序
4. **xlog 方案C 丢日志窗口**: 最多丢 N-1 条（ERROR 级除外）
5. **CI lint 误报/漏报**: 正则非静态分析，初版仅 WARNING
6. **标注规范未全覆盖**: 本轮 pilot 仅 hashmap/ring_buffer，thpool/list/array 等待后续
7. **方案B 未实施**: 若需严格 FIFO + 高吞吐，现有方案均无法满足

---

## 实施顺序

```
Week 1-2 (P0):
├─ array.c 首行 mem_debug.h ✓
└─ hashmap allocator_t 注入（修正三路径+memset）

Week 3-4 (P1):
├─ foreach 回调守卫（debug iterating 标志）
├─ put 返回值文档
└─ 函数标注规范（四段式 pilot）

Month 2-3 (P2):
├─ xlog 批量 flush（opt-in，默认 OFF）
└─ CI mem_debug 检查（WARNING-first）

Month 3+ (P3):
└─ 文档标注全模块推广
```

---

**报告生成**: 2026-06-04  
**完整数据**: `E:\Users\admin\Temp\claude\...\wszqks71y.output`
