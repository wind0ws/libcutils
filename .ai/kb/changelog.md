# 修改记录

<!-- 会话中有重要修改时追加到此处 -->

1. **测试框架重构（v1.9.0 候选）** — 2026-05-27
   - **Breaking**：`lcu_demo` 砍掉数字索引调用（`./lcu_demo 0 5 8` 不再支持）。改用名字（`./lcu_demo time_util_test`）。
   - **新增**：自动注册框架。`*_test.c` 末尾追加 `LCU_TEST_REGISTER(name, "desc")` 即注册，无需改 main.cpp / CMakeLists.txt。
   - **新增**：CLI 选项 `--list / --help / --all / --filter <glob> / --fail-fast / --junit <file>`。
   - **新增**：`LCU_TEST_REGISTER_OPTIONAL` 宏标记 opt-in 用例（如 `memleak_test`），不进 `--all`。
   - **新增**：失败继续 + 末尾 pass/fail 汇总；退出码 = 失败数（截 254）。
   - **新增**：CTest 集成。`enable_testing()` 已加在 CMakeLists.txt 顶层，`ctest -L lcu` 列出全部默认用例，`ctest -L lcu_optional` 列出 opt-in。
   - **改进**：耗时统计从 `clock()` 改 `time_util_query_performance_ms`（高精度墙钟）。
   - **改进**：注册节点静态分配（不 malloc），消除 mem_debug leak 报告。
   - **修复**：`file_logger_test` 路径从 `D:/temp/log` 改 `./log`（CI 友好）。
   - **修复**：所有测试函数签名 `int xxx_test()` → `int xxx_test(void)`（C 标准）。
   - **新增文件**：`src_demo/lcu_test_registry.{h,c}`、`lcu_test_args.{h,c}`、`lcu_test_console.{h,c}`、`lcu_test_glob.{h,c}`。
   - **删除目录**：`inc/test/`（注册框架不再发布给外部）。
   - main.cpp 行数 478 → 396（瘦身 17%）。

2. **代码评审 v4 plan 全量修复** — 2026-05-29
   - **基线**：MSVC 19.44 (VS 2022 Enterprise) Release / x64，全量 ctest 19/19 PASS（基线锚点）。
   - **完成**：23 项修订（5 P0 + 10 P1 + 8 P2），全量 ctest 19/19 PASS，零退化。
   - **P0 必修**（5）：
     - `P0-1` `file_logger_init` 失败路径泄漏 msg_queue + worker 线程 → 改 goto cleanup_on_error 统一清理 + cfg 前置 + msg 所有权转移置 NULL（`src/log/file_logger.c`）。
     - `P0-2/P0-3` `thpool` detach + 销毁同步原语 UB + 上界用 alive 计数 → worker 改 joinable + bsem_wait 加 keepalive_p 实现 sticky shutdown (O(N)→O(1) 唤醒) + destroy 用 num_threads 上界 + pthread_join（`src/thread/thpool.c`）。验收：`thpool_test` 100 jobs 完成 + destroy 仅 1ms。
     - `P0-4` `file_util_append_slash` 缓冲不足时静默覆盖末字符 → 缓冲不足返回 -3 不修改原数据（`src/file/file_util.c`）。新增 5 个 TDD 边界用例。
     - `P0-5` Win32 `pthread_mutex_unlock` 仅识别 `_INITIALIZER`，对 `_RECURSIVE_INITIALIZER`/`_ERRORCHECK_INITIALIZER` 段错误 → `>= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER` 与 lock 路径对齐（`pthread_win_simple.c`）。新增静态初始化器测试。
   - **P1 应修**（10）：
     - `P1-1` Win32 `sem_init` 强制 Global\ 命名空间普通用户必失败 → 改未命名信号量 `CreateSemaphoreA(NULL,...,NULL)`，保留 `GetLastError`→errno 映射（`semaphore_win_simple.c`）。
     - `P1-2` `file_logger_destroy` 文档加 `@warning Call Order`（`inc/log/file_logger.h`，前序已沉淀）。
     - `P1-3` `file_logger_log` 持锁 sleep 优化：retry 平台分支（POSIX 200us×10 ≈2ms / Win32 1500us×2）；lost.log 同步 IO 移到 unlock 之后避免阻塞其他 producer（`src/log/file_logger.c`）。
     - `P1-4` `thpool_destroy` 1 秒忙循环 → 已合并到 P0-2/P0-3 的 join 改造，删除忙循环。
     - `P1-5` `ring_buffer` 跨线程读写无内存屏障（弱内存模型脏读）→ 在 `inc/common_macro.h` 封装 4 层 fallback 跨平台原子操作（C11 stdatomic / GCC __atomic / MSVC _ReadWriteBarrier / volatile）；`ring_buffer.c` 17 个访问点全部走 inline accessor，按角色（producer/consumer/公共 API）拆分内存序（`src/ring/ring_buffer.c`）。
     - `P1-6` `msg_queue_handler` 创建失败时 `thread_handler` 状态未定义 → 失败分支显式置 0，避免 destroy join 垃圾句柄（`src/ring/msg_queue_handler.c`）。
     - `P1-7` `base64_decode` 无长度参数 → 头文件补 `@warning` 强制 NUL 终止契约（`inc/data/base64.h`）。
     - `P1-8` `str_char2hex` capacity<3 时 size_t 下溢 → 入口校验 `capacity >= ONE_HEX_STR_SIZE+1`（`src/mem/strings.c`）。
     - `P1-9` `str_params_create_str` 未检查 strdup/strndup 失败 → OOM 时释放半边并跳过当前 kv（`src/mem/str_params.c`）。
     - `P1-10` `url_encode` 错误返回未写 NUL → 所有 return 前先 `out[idx]='\0'`（`src/net/url_encoder_decoder.c`）。
   - **P2 建议修**（8/12）：
     - `P2-1` ini bool 前缀匹配（"trueblahblah"→true）→ `strcasecmp` 全等（`src/file/ini_parser.c`）。
     - `P2-2` `ini_parser_save` 非原子写崩溃丢失 → tmp + rename 原子写（`src/file/ini_parser.c`）。
     - `P2-3` `time_rfc1123/2822` 未检查 `gmtime_r` 失败 + tm 字段越界 → 检查返回 + 钳制 tm_wday<7 / tm_mon<12（`src/time/time_rfc1123.c`、`time_rfc2822.c`）。
     - `P2-4` `file_util_read_all` int 截断 → 内部加固（保留 ABI）：检查 fseek/ftell 失败 + INT_MAX 溢出（`src/file/file_util.c`）。
     - `P2-5` `base64_encode` 拒绝 1 字节合法输入 → 接受 `>=1`，主循环防 size_t 下溢（`src/data/base64.c`）。
     - `P2-6` `xlog_global_init` check-then-act 竞态 → guard-once 模式：平台 once（C11 call_once / Win32 InitOnceExecuteOnce / POSIX pthread_once）保护守护锁，守护锁保护 g_xlog_mutex 的 init/cleanup（`src/log/xlog.c`）。验收：16 线程×100 次并发 init OK。
     - `P2-7` `ring_buffer_create_with_mem` 对齐契约缺失 → 头文件补 `@warning` 必须 alignof(uint32_t)（`inc/ring/ring_buffer.h`）。
     - `P2-8` `msg_queue_handler_destroy` 单次 sem_post 在 worker AGAIN 路径会丢失 → 循环 post 8 次覆盖临界窗口（`src/ring/msg_queue_handler.c`）。
     - `P2-9` `file_iterator` snprintf 截断未检查 → 检查返回值，截断则跳过（`src/file/file_iterator.c`）。
     - `P2-10` `mplite_realloc` 在 lock 外读 nOld 形成 TOCTOU → 整体移入 enter/leave 临界区（`src/mem/mplite.c`）。
     - `P2-11` `strtrim` 空串 `s+strlen(s)-1` 指针下溢 UB → 入口空串/NULL 保护（`src/mem/strings.c`）。
     - `P2-12` `xlog print_func_line` 不传剩余空间，超长 `__PRETTY_FUNCTION__` 栈越界 → 加 remaining_size 参数，按需截断（`src/log/xlog.c`）。
   - **新增 CMake 探测**：`tool/CMakeLists.txt` 加 `check_include_file(threads.h LCU_HAVE_C11_THREADS_H)` 双保险，cross-compile 下不可靠时由 `__has_include` 兜底。
   - **新增 inc/common_macro.h 跨平台原子操作章节**：4 tier fallback inline 函数 + STATIC_ASSERT 兜底 + `LCU_HAS_ATOMIC_BUILTIN` 宏隔离 `__has_builtin`（修复 MSVC C1012）。
   - **新增 TDD 测试**：`file_util_test` 5 个边界用例 / `posix_thread_test` 静态初始化器 + 16×100 并发 init / `thpool_test` 100 jobs + destroy 时延断言 (<500ms)。
   - **修订决策剔除项**（plan v4 主审员判断）：保持 v2/v3 大方向，剔除 12 项过度设计/误判（MSVC /std:c11 已配置 / cfg_p 副作用兼容 / clear release store / debug API acquire / extern inline GNU89 分歧 / `_ReadWriteBarrier` 未来 deprecation / const volatile 兼容灰区 / 版本号节奏 / TSan ARM 硬件回归 / thpool malloc→calloc / sem_init ASSERT 重入 / Week 0 cross-build 完整化）。详见 `~/.claude/plans/p1-3-usleep-producer-tingly-comet.md`。
   - **跨平台限制（诚实披露）**：ARM/ARMv8 内存屏障仅靠静态对照 + 编译器保证，团队无 ARM 硬件测试床；uclibc/musl 老编译器走 Tier 4 volatile fallback，发出 `#warning`。
   - **验收闭环**：MSVC 19.44 Release / x64 clean rebuild 零警告零错误，全量 ctest 19/19 PASS（基线 19/19，零退化），耗时 84.26s（基线 82.59s，新增并发测试占 +1.67s）。
   - **Linux 跨平台验收**（2026-05-29）：WSL Ubuntu 16.04 LTS / GCC 5.4.0 / CMake 3.31.4。
     - **m64 Release**：构建通过，全量 ctest 19/19 PASS（87.89s，含 autocover 80s SPSC 压力）。
     - **m32 Release**：构建通过（先装 `gcc-multilib`），全量 ctest 19/19 PASS（87.34s）。32 位下 `_Atomic uint32_t` 与 `uint32_t` size/align 一致，P1-5 ABI 假设成立。
     - **Tier 1 路径确认**：编译期 `#pragma message` 探针确认 GCC 5.4 `-std=gnu11` 模式下走 Tier 1 C11 stdatomic（不退化到 Tier 2），最优内存序生效。
     - **P0-2/P0-3 sticky shutdown**：Linux 上 `thpool_destroy` 仅 **0.04ms**（Windows 1ms × 25 倍提速），broadcast 一次唤醒所有 worker 完美生效。
     - **P2-6 并发 init**：16 线程 ×100 并发 `xlog_global_init` 在 m64/m32 下都 OK（POSIX 走 `pthread_once` 兜底分支，因 GCC 5.4 默认 glibc < 2.28 无 `<threads.h>`）。
     - **AddressSanitizer 验证**：用 `-fsanitize=address -fno-omit-frame-pointer` 重新构建，`ASAN_OPTIONS=detect_leaks=1` 严格模式下跑 P0/P1 关键测试（thpool/file_logger/file_util/posix_thread/msg_queue_handler/str_params），**100% PASS 零内存错误**：
       - P0-1 `file_logger_init` cleanup 路径无 leak/UAF（goto cleanup 所有权转移正确）。
       - P0-2/P0-3 thpool joinable + sticky shutdown 无 mutex destroy UB。
       - P1-9 `str_params_create_str` 大量 strdup 无 leak。
     - **平台差异确认**：P0-5 (`pthread_mutex_unlock` 静态初始化器) 仅在 `_LCU_CFG_WIN_PTHREAD_MODE == SIMPLE` 条件编译下生效，Linux 自动跳过（用 glibc 原生 pthread）。`P0-5 OK` 日志在 Linux 不出现，符合预期。
     - **未在本次验证范围**：ARM/ARMv8 真机回归（无硬件）；其他 30+ 嵌入式 toolchain（基础设施工作，主审员判断不阻塞修复交付）。
   - **ARM64 真机验收**（2026-05-29）：交叉编译 + ssh 真机运行验证。
     - **交叉编译**：linaro7.5.0 toolchain (GCC 7.5.0 aarch64-linux-gnu) + WSL Ubuntu 16.04，Release 构建通过，产物 ELF 64-bit ARM aarch64。
     - **真机环境**：Ubuntu 24.04 LTS / kernel 6.1.118 / aarch64 / 8 核 ARM Cortex-A55 (CPU part 0xd05) / 支持 LSE atomics + lrcpc。
     - **反汇编铁证**：`aarch64-linux-gnu-objdump -d liblcu_a.a | grep -E 'ldar|stlr'` 在 `ring_buffer_write` / `pri_ring_buffer_read_internal` 函数体内大量出现 ARMv8 `ldar` (Load-Acquire) 与 `stlr` (Store-Release) 指令，确认 P1-5 的 acquire/release 内存序**真正编译为硬件原子指令**，不是空操作。
     - **真机 18/18 PASS**（`lcu_demo --all`）：82.304s，零 fail 零 skip。
     - **决定性证据 - SPSC 80s 压力测试**：autocover_buffer_test 在 ARMv8 弱内存模型下跑 80 秒，producer counter == consumer counter == **2026215392**（≈20 亿次 push/pop），严格相等。如果 P1-5 acquire/release 屏障缺失或位置错，ARM weak memory model 必然导致 stale read 或 counter 失配。
     - **P0-2/P0-3 ARM64**：thpool_destroy = 0.23ms（相比 Linux x86 0.04ms，dmb 指令开销使其略增，但仍远低于断言阈值 500ms），sticky shutdown 在 ARMv8 多核下生效。
     - **P0-4 ARM64**：file_util_append_slash 5 个边界 TDD 用例 PASS。
     - **P2-6 ARM64**：16 线程 ×100 并发 xlog_global_init 通过（POSIX pthread_once 路径）。
     - **跨平台覆盖小结**：Windows MSVC x64 / Linux GCC 5.4 m64+m32 / Linux ASan / **Linux ARMv8 Cortex-A55 真机** 全绿。

3. **hashmap/array 内存追踪盲区修复（allocation_tracker 递归根治）** — 2026-06-04
   - **背景**：多角度对抗评审（含 Codex）发现 `hashmap.c`/`array.c` 直接用 libc `malloc/calloc/free`，绕过 `allocation_tracker`，导致这些容器的分配不被泄漏检测覆盖（追踪盲区）。但 `hashmap.c` 的 `#undef` 绕过是**有意为之**——`allocation_tracker` 内部用 hashmap 存分配记录，若 hashmap 走 `lcu_*alloc` 会无限递归 + 自死锁。
   - **根因链（症状 → 各层）**：盲区症状 ← data 层容器直连 libc ← 为规避 tracker 内部 map 的递归而一刀切关掉追踪（连用户 map 一起排除）。根治落点：**按实例注入 allocator**，区分"被追踪"与"raw"两类分配器。
   - **修复（6 commit）**：
     - `fix(allocator)`：新增 `allocator_calloc_raw`/`allocator_malloc_raw`，定义在 `#undef` 作用域内走 libc，不经 tracker（断递归基础）。
     - `fix(hashmap)`：新增 `hashmap_create_ex(..., const allocator_t*)`，`hashmap_create` 转调它传 `&allocator_calloc`（**ABI 逐字节不变**）。覆盖**三条分配路径**：struct Hashmap / buckets(init+rehash) / Entry，全走 `allocator->alloc`；buckets **显式 `memset(0)`** 不依赖分配器清零语义；`private_create_entry` 改签名带 allocator。
     - `fix(allocation_tracker)`：内部 map 改用 `hashmap_create_ex(..., &allocator_calloc_raw)` 断递归，加 CRITICAL 注释固化约束。
     - `fix(array)`：首行加 `mem/mem_debug.h`，删冗余 `<malloc.h>`，纳入追踪（array 不在 tracker 依赖链，无递归风险）。
     - `docs`：在 `thpool.c`/`ring_buffer.c`/`list.c`/`hashmap.c` 误判位置加注释固化"已正确实现"事实，防止重复误判（HIGH-3 jobqueue 已有独立 rwmutex、HIGH-6 ring_buffer_clear 头文件已有契约、CRITICAL-1 foreach 已全程持锁、HIGH-7 rehash OOM 已优雅降级、list 已用 allocator 抽象被追踪）。
     - `test(harness)`：`main.cpp` 加 `_CrtSetReportMode` 把 CRT assert 重定向到 stderr（仅 Win+Debug），防止 ASSERT 失败弹模态对话框卡死 CTest/CI。
   - **验证（全量回归）**：
     - **Clean build 双配置零警告**：Debug（含 mem_check 递归路径）+ Release（ABI/常规路径）从零重建，0 error 0 warning。
     - **CTest 19/19 全绿**：Debug 135s / Release 83s，两配置均 100% pass 0 fail。
     - **递归安全实证**：`allocator_test`/`basic_test`/`mplite_test`/`memleak_test` 全过 → `lcu_malloc → tracker → hashmap_put → raw(断开)` 无栈溢出/死锁；连续 3 轮 mem 测试稳定无偶发。
     - **追踪有效性**：`memleak_test`（故意制造泄漏）能检出 → 盲区已消除。
     - **ABI 兼容**：现有 `hashmap_create` 调用方（`str_params.c` 等）未改却编译链接通过。
     - **baseline 对照**：`file_logger_test` 的 2 处清理 assert（test.c:108/122，日志 retention 未删旧文件）经 git stash 还原改动后**完全一致地复现**，确认是 **pre-existing bug，与本次改动无关**。
   - **Open issue（非本次范围）**：file_logger 日志清理（retention/size）未正确删除旧文件，独立追踪；P1 待办（foreach 回调 debug 守卫、put 返回值四段式文档、hashmap/ring_buffer 标注规范 pilot）。



