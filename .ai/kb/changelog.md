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

7. **pthread_win_simple `pthread_cond_init` 计数器未初始化（Release 50% 死锁修复）** — 2026-06-08
   - **症状**：`thpool_test` 在 Release/x64 下偶发挂死（30 次冷启动测约 50% 命中），同测试 Debug 下 0/20 死锁。挂死现场 100% 集中在 `thpool_wait`（5/6 样本）或 `thpool_destroy`（1/6 样本），表现为 worker 线程未被 `pthread_cond_signal` / `pthread_cond_broadcast` 唤醒。
   - **根因**：`src/thread/pthread_win_simple/pthread_win_simple.c::pthread_cond_init` 仅初始化 `mSemaphore` 与 `mLock`，未初始化 `mWaiting` / `mWake` / `mGeneration` 三个 size_t 计数器。`thpool.c::jobqueue_init` 用 `malloc`（非 `calloc`）分配 `bsem`，其内嵌 `pthread_cond_t` 三计数器为未初始化值。
     - **Release 行为**：MSVC `/MT` Release 堆分配返回真实垃圾内容，`mWaiting` 任意值 → `pthread_cond_signal/broadcast` 中 `if (cond->mWaiting > cond->mWake)` 决策错误（要么发不该发的信号、要么不发该发的信号），50% 概率出现 lost wakeup 死锁。
     - **Debug 行为**：MS CRT Debug 堆把新分配填 `0xCDCDCDCD`，三个 size_t 相等，`mWaiting > mWake` 永远 false，`pthread_cond_*` 不会做错决策（虽然语义已破，但巧合不挂死）。
   - **修复**：`pthread_cond_init` 显式置零三个计数器（`src/thread/pthread_win_simple/pthread_win_simple.c:182`）。该 API 的 POSIX 契约是「初始化条件变量到可用状态」——之前的实现未达成契约，是真 bug，不是 caller 责任。
   - **验证**：
     - 修复后 Release `thpool_test` 直接执行 50/50 通过（修复前 50% 挂死）；
     - `ctest -C Debug` 20/20 PASS（87s）、`ctest -C Release` 20/20 PASS（83s），零回归；
     - `hashmap_test` / `posix_thread_test` / `file_logger_test` / `msg_queue_handler_test` 各连续 10 次 Release 跑 0 挂 0 fail。
   - **影响面**：所有依赖 `pthread_cond_t` 且分配方式非 `calloc`/`memset` 清零的调用链（`thpool` 的 bsem、`fixed_msg_queue`、`msg_queue` 等）。本修复在初始化层根治，无需逐处加防御性 memset。
   - **测试覆盖建议**（未在本次提交）：在 thpool_test 中显式触发多次 init/destroy 循环，提升对此类 lost wakeup 的捕获率。

8. **0527~0608 全量改动深度验证测试** — 2026-06-08
   - **目的**：针对 0527 以来全部功能改动（19 commits）做深度边界/压力/并发测试，验证接口输入输出符合预期。
   - **环境**：Windows 11 / MSVC 19.44 (VS 2022 Enterprise) / Debug x64 / PRJ_WIN_PTHREAD_MODE=0。
   - **新增测试文件**：
     - `src_demo/deep_validation_test.c` — 16 个子测试覆盖：
       - allocation_tracker 递归安全（1000 alloc/free 无栈溢出、50 realloc canary 完整、strdup/strndup 追踪正确）
       - hashmap allocator 注入（raw allocator 3 路径、10000 条目压力 + 全量 rehash、foreach NULL guard、early-exit 行为、NULL map/allocator 防御）
       - file_logger 路径溢出保护（MAX_LOG_FOLDER_PATH_SIZE-1 拒绝、无效配置拒绝、正常生命周期）
       - strings 边界（strlcpy 截断/零 size、strlcat 截断、strreplace 多次替换/空 pattern 返回 NULL/反斜杠替换）
       - calloc 乘法溢出保护
     - `src_demo/deep_validation2_test.c` — 11 个子测试覆盖：
       - thpool 多轮创建销毁（5 轮 ×200 jobs，sticky shutdown <500ms）、8 线程 ×1000 jobs 并发、空池 wait 即时返回
       - ring_buffer 基本读写、满/环绕 wrap-around 数据完整性、peek + discard 不移动读指针
       - ring_buffer SPSC 并发（5000 items，producer/consumer 双线程，严格顺序验证 P1-5 原子可见性）
       - ring_buffer create_with_mem 外部内存模式
       - list 空操作防御（NULL front/back/remove/foreach）、1000 元素压力 + 连续 remove front
   - **验证结果**：
     - `ctest -C Debug -L lcu`：**22/22 全部通过**，0 failed，耗时 150s
     - 全部 27 个深度子测试 PASS，零 assertion 警告
     - 既有 20 个原始测试零退化
   - **发现并修复的实际 bug**：
     - `hashmap_foreach` 提前退出失效：回调返回 false 时只退出内层 while(当前 bucket 链)，外层 for 继续扫后续 bucket。修复：`break` → `goto foreach_done` 真正退出双层循环。验证：修复前 visited=93(stop_after=5)，修复后 visited=5(精确)。

9. **跨边界所有权转交内存契约修复（strreplace / file_util_read_all / asprintf / str_params_to_str）** — 2026-06-08
   - **背景**：深度验证中发现 strreplace 返回堆指针给调用方，文档约定"need free"，但内部用 `lcu_malloc_trace` 分配 → 当 allocation_tracker 开启时，返回值是 canary 偏移 + 追踪的特殊指针，外部集成方用标准 `free()` 释放 → **堆损坏崩溃 + 误报泄漏**。
   - **全面排查**：系统排查所有"分配后把所有权转交给调用方"的公共 API，确认隐患面：
     - `strreplace` (strings.c) — 显式 `lcu_malloc_trace`，文档说"need free"
     - `file_util_read_all` (file_util.c) — `malloc` 被 mem_debug.h 改写成 `lcu_malloc_trace`，via out_alloced_file_data 转交所有权
     - **`asprintf`/`vasprintf` (asprintf.c)** — **标准 POSIX 函数名**，全世界都知道"用 libc free"，但 Windows 实现里 `malloc` 被改写，**最危险的案例**
     - `str_params_to_str` (str_params.c) — 返回 asprintf buffer（后续改为 raw）OR `strdup("")`（追踪），**同一返回值两套释放语义**
   - **统一契约决策**：用户选择**方案 Y（统一裸 libc malloc）**：
     - 理由 1：asprintf 是标准名，强制裸 malloc，不如三者统一
     - 理由 2：习惯性 `free()` 恒正确，不留地雷（方案 B 的配对释放器无法阻止错误的 free 调用）
     - 代价：转交出去的 buffer 失去追踪（可接受——所有权已交出，该是调用方工具去管）
   - **修复（4 API + 8 内部调用点 + 2 测试）**：
     1. **基础设施**：新增 `lcu_malloc_raw` / `lcu_free_raw` (allocator.h/.c) — 显式裸 libc malloc/free，供跨边界内存的内部分配/释放点使用。
     2. **strreplace** (strings.c:156 + strings.h 契约)：改回裸 `malloc`（本就不含 mem_debug.h）；内部调用点 file_logger.c:390/441 → `lcu_free_raw`，加 allocator.h include。
     3. **asprintf** (asprintf.c)：整个文件就是 asprintf 实现，加 `#undef malloc` / `#undef free` 块（与 allocator.c 同模式）恢复为 libc；内部调用点 str_params.c:348/358 → `lcu_free_raw`；`str_params_to_str` 的空路径从 `strdup("")`（追踪）改 `lcu_malloc_raw` 统一契约；str_params_test.c:58 → `lcu_free_raw`；str_params.h 更新契约文档。
     4. **file_util_read_all** (file_util.c:263)：`malloc` → `lcu_malloc_raw`，错误路径 :272 → `lcu_free_raw`；file_util.h 更新契约文档；内部调用点 ini_parser.c:225 → `lcu_free_raw`，加 allocator.h include。
     5. **遗漏调用点补完**（回归测试抓出）：flag build (tracker ON) 初次全量跑出 2 个失败 — string_test.c:46 和 deep_validation_test.c 5 处 strreplace 结果用 `free`（=lcu_free）释放 raw 指针 → 崩溃。说明初审遗漏了两个测试文件。补修后全绿。
   - **回归测试（关键验证）**：
     - 新增 `src_demo/integration/ownership_contract_test.c` — **不含 mem_debug.h**（模拟外部 SDK 集成方，`malloc`/`free`=libc），4 个子测试各循环 50~100 次分配 + libc free。
     - 测试策略：tracker 状态由构建宏控制（不手动翻转，避免内部分配/释放状态错位）。两个构建分别验证：
       - 普通构建 (build_win64, tracker OFF)：23/23 PASS — 基础正确性
       - **Flag 构建 (tool/build, -D_LCU_MEM_CHECK_FEATURE_ENABLE=1, tracker ON 全局)：23/23 PASS** — **原 bug 触发条件，核心验证**
     - **反向验证**：临时把 strreplace 改回 `lcu_malloc_trace`，flag 构建立即崩在 `_CrtIsValidHeapPointer` assertion（libc free 作用在 canary 偏移指针）→ 证明测试真能抓到原 bug。
   - **影响面**：所有返回堆所有权给调用方的公共 API，现在统一约定"raw libc malloc + 文档明确 libc free 契约"。内部 lcu 代码（含 mem_debug.h）释放这些 buffer 时必须用 `lcu_free_raw`。
   - **最终验证**：两个构建配置 × 全量 CTest 23/23 = **100% PASS，零回归**。

10. **0608 评审落地（文档/注释批次）+ 闭环验证 + 开放项移交** — 2026-06-09
   - **背景**：0608 双人独立评审（Claude Opus 4.8 + Codex gpt-5.5，方法论"先读头文件契约再看实现"）纠偏上一版误报，重新核出 2 C + 4 H + 4 M + 3 L + 1 caller-side + 3 文档项。本次仅落地**零逻辑风险**的文档/注释子集，逻辑修复留待后续按优先级执行。
   - **本次已完成（8 文件，纯注释/文档，零可执行代码改动）**：
     - **H-3（取最廉价方案）**：`inc/lcu.h` / `inc/time/time_util.h` 补线程安全契约 `@warning`——`*_global_init/cleanup` 的 `g_init_times` 非原子，要求"启动时单线程调用一次"。落点是头文件契约而非加 once 保护（根因仍在，见开放项）。
     - **D-1**：`inc/data/hashmap.h` 重写 `hashmap_put` `@return` 文档——旧文档"caller 释放 old_value"与实现（map 自动释放）矛盾，所有现存 caller 已按实现行为运行，判为文档过期；改文档对齐实现（替换路径返回值仅作"是否替换"指示，不可 deref/free）。
     - **误判位置固化注释** `NOTE(reviewed 2026-06-08)`：`src/lcu.c`、`src/time/time_util.c`（g_init_times 非原子是契约非 bug）、`src/data/hashmap.c`（replace 分支 key 所有权归 caller per 头文件 `if new entry`；`hashmap_size` 无 NULL 守卫是 caller 前置条件）、`src/data/array.c`（容量乘法溢出是加固缺口非活跃 bug，无契约无 caller，记 D-2）、`src/data/base64.c`（size helper INT_MAX 截断是加固缺口，记 D-3）。防后续重复误报。
   - **闭环验证**（原 `.ai/closed-loop-verification-2026-06-09.md`，删前转录）：
     - 改动全为注释/doc-block，`git diff` 过滤后无可执行行变更。
     - Windows MSVC 19.44 x64：Release + Debug 双配置 clean rebuild，0 新增警告（仅 pre-existing `C4996 fopen` in ini_reader.c）。
     - CTest `-L lcu`：**Release 23/23 PASS (83.49s) + Debug 23/23 PASS (137.17s)**，对基线（#7/#8 的 20/20）零回归（harness 新纳入 2 core + 1 optional）。
     - 关键路径覆盖：hashmap_test / str_params_test（put replace key 所有权）/ time_util_test / allocator_test / deep_validation{,2}_test / ownership_contract_test 全 PASS。
   - **⚠️ 开放项移交（0608 评审发现，本次未修；原 `review-2026-06-08.md` 删前转录，后续按序处理）**：
     - **C-1 CRITICAL**：`allocation_tracker_resize_for_canary`（allocation_tracker.c:230）`size + 2*canary` 无上限校验，`lcu_malloc(SIZE_MAX-8)` → 环绕小块 + 尾 canary 越界写。修：入口 `size > SIZE_MAX - 2*canary ? 0 : ...`。
     - **C-2 CRITICAL**：`file_logger_log`（file_logger.c:501/513）`size_t msg_size` 截断为 uint32_t 后却用原始 size_t memcpy，传 >4GB → 巨型堆溢出。修：入口拒 `msg_size > INT_MAX` 或头文件文档化上限。
     - **H-1 HIGH**：`ini_parser_dump`（ini_parser.c:625）用 `strdup`（经 mem_debug.h 改写为 tracked 指针），外部 libc free 崩——与 b2f71fe 已修的跨边界所有权同类，遗漏此文件。修：改 `lcu_malloc_raw`+strcpy，头文件补 libc free 契约。
     - **H-2 HIGH**：`msg_queue_handler_push`（msg_queue_handler.c:271）未校验负 `obj_len`（头文件 errno 已预期校验），负值经 uint32_t 提升成巨值。修：入口 `obj_len < 0` 拒。
     - **H-4 HIGH**：`file_util_mkdirs`（file_util.c:81）`> MAX_FOLDER_PATH_LEN` 应为 `>=`，等长时填满无 NUL 位 → 栈外读。修：改 `>=`。
     - **M-1~M-4 MEDIUM**：thpool `jobqueue.len` 跨锁 race（M-1）/ thpool `volatile int` 当同步原语（M-2）/ `pthread_rwlock_init` Windows simple 读未初始化 `*rwlock`（M-3，pthread_win_simple.c:281）/ `file_util` 读写 `int` 进度累加溢出（M-4）。
     - **L-1~L-3 LOW**：`lcu_realloc_trace` tracker 未 INIT 时静默丢数据（建议加 ASSERT）/ `hashmap_size(NULL)` 段错误（实现不一致，已加注释，待决定加守卫或文档化）/ `pthread_cond_init` 忽略 `CreateSemaphoreW` 失败（pthread_win_simple.c:194，资源耗尽应返 ENOMEM）。
     - **Caller-1（非库 bug）**：`str_params_create_str`（str_params.c:140）重复 key 时新 key 泄漏，应照 `add_str` 模式在 cleanup 释放。
   - **行动优先级**：下一发布前修 C-1/C-2；1.9.0 前修 H-1/H-2/H-4；M/L/Caller-1 中期。

