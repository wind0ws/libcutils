# 修改记录

> 变更索引,不是工作日志。每条目硬上限 ~8 行,字段:**动机 / 改动 / 影响 / 关联**。
> 过程产物(评审博弈、bisect 路径、ctest 耗时数字)走 `~/.claude/plans/`,不进此处。
> 
> **归档触发**:本文件 > 200 行 **或** 版本发布,迁已发版条目到 `archive/`(见 [archive/README.md](archive/README.md))。

---

## v1.9.0(开发中,基线 1.8.0)

### 1. 测试框架重构:自动注册 + CTest 集成 — 2026-05-27
- **动机**:`lcu_demo` 数字索引调用难维护;新增测试要改 main.cpp/CMakeLists.txt
- **改动**:`*_test.c` 末尾 `LCU_TEST_REGISTER(name, "desc")` 自动注册;CLI 加 `--list/--all/--filter <glob>/--fail-fast/--junit`;`enable_testing()` 集成 CTest;耗时换高精度墙钟;注册节点静态分配
- **影响**:**Breaking** — `./lcu_demo 0 5 8` 不再支持,改 `./lcu_demo time_util_test`;删除 `inc/test/`(不外发)
- **文件**:新增 `src_demo/lcu_test_{registry,args,console,glob}.{h,c}`

### 2. 代码评审 v4 全量修复(23 项)+ 跨平台原子操作 — 2026-05-29
- **动机**:v2/v3 plan 未闭环的 5 P0 + 10 P1 + 8 P2
- **关键 P0**:`file_logger_init` 失败泄漏 → goto cleanup;`thpool` detach + 销毁原语 UB → joinable + sticky shutdown(O(N)→O(1) 唤醒);`file_util_append_slash` 缓冲不足静默覆盖 → 返回 -3;Win32 `pthread_mutex_unlock` 不识别 `RECURSIVE/ERRORCHECK` 静态初始化器 → `>=` 与 lock 对齐
- **ring_buffer 跨平台原子**:`common_macro.h` 4 层 fallback(C11 stdatomic / GCC __atomic / MSVC barrier / volatile)+ STATIC_ASSERT 兜底
- **跨平台验收**:Windows MSVC x64 / Linux GCC 5.4 m64+m32 / ASan / **ARMv8 真机** 全绿;反汇编确认 `ldar`/`stlr` 真生成,80s SPSC 压测 20 亿次 push/pop 严格相等
- **关联**:详细决策见 `~/.claude/plans/p1-3-usleep-producer-tingly-comet.md`

### 3. hashmap allocator 注入,根治追踪盲区 — 2026-06-04
- **动机**:`hashmap.c`/`array.c` 直连 libc 绕过 tracker;但 hashmap 历史不能走 `lcu_*alloc`(tracker 内部用 hashmap 会无限递归)
- **改动**:`hashmap_create_with_allocator(..., const allocator_t*)`,三条分配路径(struct/buckets/Entry)全走 allocator;新增 `allocator_{calloc,malloc}_raw` 走 libc 不经 tracker,tracker 内部 map 用 raw 断递归;`array.c` 首行 include `mem_debug.h` 纳入追踪
- **影响**:盲区消除(memleak_test 能检出 hashmap 泄漏);`hashmap_create` 转调新 API,**ABI 字节不变**
- **关联**:`hashmap.c`/`thpool.c`/`ring_buffer.c`/`list.c` 加 `NOTE(reviewed)` 注释固化误判位置防重复评审

### 4. pthread_cond_init Release 50% 死锁修复 — 2026-06-08
- **症状**:`thpool_test` Release/x64 偶发 50% 挂死(Debug 0/20),挂死现场集中在 `thpool_wait/destroy` 的 worker 未被唤醒
- **根因**:`pthread_cond_init` 漏初始化 `mWaiting/mWake/mGeneration` 三计数器;`thpool` 用 malloc(非 calloc)分配 bsem,Release 堆为真垃圾,`pthread_cond_signal` 中 `mWaiting > mWake` 决策错误 → lost wakeup
- **改动**:`pthread_cond_init` 显式置零三计数器(`pthread_win_simple.c:182`)
- **影响面**:所有依赖 `pthread_cond_t` 且非 calloc/memset 清零的调用链;在初始化层根治,无需逐处加防御性 memset

### 5. 0527-0608 改动深度验证测试 — 2026-06-08
- **改动**:新增 `src_demo/deep_validation{,2}_test.c` 共 27 子测试,覆盖 allocation_tracker 递归安全 / hashmap allocator 注入 / file_logger 路径溢出 / strings 边界 / thpool 多轮 + 8×1000 并发 / ring_buffer SPSC 并发 / list 空操作
- **抓出真 bug**:`hashmap_foreach` 提前退出失效——回调返 false 只退出当前 bucket 链,外层 for 继续扫;修为 `goto foreach_done` 真双层退出
- **影响**:验证 #3/#4 修复无回归

### 6. 跨边界所有权统一裸 libc — 2026-06-08
- **动机**:`strreplace`/`file_util_read_all`/`asprintf`/`str_params_to_str` 返回堆指针给调用方,内部用 `lcu_malloc_trace` → tracker 开启时外部 `free()` 堆损坏 + 误报泄漏
- **改动**:基础设施新增 `lcu_malloc_raw`/`lcu_free_raw`;4 API 统一改裸 libc malloc;内部 8 个释放点改 `lcu_free_raw`;头文件契约统一为"libc free"
- **影响**:外部集成方习惯性 `free()` 恒正确;失去追踪是设计取舍(所有权已交)
- **关联**:新增 `src_demo/integration/ownership_contract_test.c`(不含 mem_debug.h,模拟外部 SDK)

### 7. 0608 评审文档化批次(零逻辑改动) — 2026-06-09
- **改动**:`*_global_init` 单线程契约 `@warning`(H-3);`hashmap_put` `@return` 文档重写对齐实现行为(D-1,旧文档"caller 释放 old_value"与实现矛盾);6 处 `NOTE(reviewed 2026-06-08)` 固化误判位置(g_init_times 非原子是契约 / replace 分支 key 所有权 / hashmap_size NULL 守卫 / array 容量乘法溢出 / base64 size INT_MAX 截断)
- **影响**:防后续重复误报,无可执行代码改动

### 8. 0608 评审开放项全量落地(C/H/M/L 13 项)— 2026-06-10
- **改动**:
  - **C-1** allocation_tracker resize 入口加 `SIZE_MAX - 2*canary` 溢出守卫
  - **C-2** `file_logger_log` 拒 `msg_size > INT_MAX`(一次堵两处截断)
  - **H-1** `ini_parser_dump` strdup→`lcu_malloc_raw`(补 #6 漏修同源 bug)
  - **H-2/H-4** `msg_queue_handler_push` 拒负 obj_len;`file_util_mkdirs` `>` 改 `>=` 边界栈外读
  - **M-3/M-4/4.5** `pthread_rwlock_init` 移除 init 前读 `*rwlock` 的 UB;`pri_internal_rw_file` int→size_t 防 >2GB UB;`ini_parser_save` 原子写(Windows MoveFileExA / POSIX rename)
  - **L-1/L-3/Caller-1** realloc tracker-uninit ASSERT;`pthread_cond_init` semaphore 失败返 ENOMEM;`str_params_create_str` 重复 key 不泄漏
  - **M-1/M-2/4.2/4.3 文档化**:thpool 跨锁 race / `volatile int` 当同步原语,改 atomic 留 1.9.0+
- **ABI 审查**(0527→HEAD):**零破坏**。仅 slog hex 内部符号重命名(`__slog_internal_hex_print` → 单下划线),持有 0527 前头文件且用 `SLOGx_HEX` 宏的集成方需重编(响亮链接错误,非静默损坏)

### 9. Windows 无弹窗统一诊断 — 2026-06-15
- **动机**:Agent/CTest 遇 ASSERT 或 CRT 报告被模态对话框阻塞;`/MTd` 下 EXE/DLL 各自独立 CRT 状态
- **改动**:新增 `diagnostics` 模块统一接管 ASSERT/CRT warn-error-leak/tracker leak;`MEM_CHECK_INIT/DEINIT` 通过当前模块 CRT 函数表登记去重(覆盖 EXE+DLL);单实例写 `lcu_diagnostics.log`,并发进程写 `lcu_diagnostics.<pid>.log`,自动保留最近 32 个;CMake 修复错放进编译 flags 的 `/DEBUG`(被解析为 `/D EBUG` 致 `_DEBUG` 未定义),Debug targets 显式定义 `_DEBUG`
- **影响**:`lcu_demo/main.cpp` 局部 `_CrtSetReportMode` 撤销

### 10. mem 模块审计:5 项硬伤修复 — 2026-06-16
- **动机**:客户在源文件首行 include `mem_debug.h`(宏重写 new/malloc/free/realloc)的交叉污染审计
- **改动**:
  - **#1** `lcu_realloc` 对未追踪指针对称回退 libc realloc(原走 `ASSERT_ABORT` 进程崩,客户 realloc 公共 API 返回的 raw 指针时会死)
  - **#2** operator new/delete 移出头文件 → `src/mem/mem_debug.cpp`,消除 ODR(LNK1169 复现+修复)
  - **M3** 补 placement delete,堵 `new T` 构造抛异常时 raw 块泄漏
  - **#4** 移除 realloc 的 `+4096` 过分配,恢复尾 canary 越界检测
  - **#3** uninit 后释放 tracked 指针的堆损坏:canary 改编译期常量 + uninit 后 free 发一次性 loud warn
  - **M4** `pthread_mutex_init` 失败 ASSERT_ABORT
- **文档化项**:#5(raw double-free 不报警)/ M1(uninit 与 notify_* 无锁竞争 = test-only)
- **关联**:新增 `src_demo/integration/{mem_realloc_contract_test.c,opnew_odr_test.cpp}`

### 11. ini_parser 缩进 key 被多行续值吞掉 — 2026-06-17
- **症状**:SDK 用户反馈 `   port=8080` 解析不到
- **根因**:`INI_ALLOW_MULTILINE=1`(Python configparser 多行值模式)时,缩进行被当作上一行 value 续行,缩进 key 从未建立
- **改动**:`INI_ALLOW_MULTILINE` 默认 1→0,保 `#ifndef` 守卫(调用方可 `#define` 恢复)
- **影响**:依赖多行续值的调用方需在 include 前自定义宏;常规 `key=value` 不受影响
- **文件**:`inc/file/ini_reader.h`;新增 `test_ini_parser_indented_keys`

### 12. ini_reader 同步上游 inih 改进 — 2026-06-17
- **动机**:跟进 inih 上游(2009-2025),引入更安全/性能更好的 API
- **改动**:新增 `ini_reader_parse_string_length(string, length, ...)` 支持带长度字符串(免 strlen,适用网络数据/string_view);`ini_rstrip` 接预算 `end` 指针避免重复扫描;超长行 `abyss[16]` 缓冲消费剩余字节防解析器卡死;入口 `assert` 校验
- **影响**:新增 API 向后兼容;保留本地配置(`ini_reader_*` 前缀、`INI_ALLOW_MULTILINE=0`、`STOP_ON_FIRST_ERROR=1`、`ALLOW_NO_VALUE=1`)

### 13. mem_debug 所有权文档 + 首行 include 自动化脚本 — 2026-06-18
- **动机**:`mem_debug.h` 三形态/静态库 vs DLL 的内存安全模型散落注释难查;客户易忘"首行 include"
- **改动**:新增 `.ai/kb/mem_debug.md`(三形态、最佳实践=Release 静态库+两端 include、所有权契约、静态库单堆 vs DLL 跨堆);新增 `tool/scripts/mem_debug_inserter/`(模块化,dry-run 默认/`--apply`/`--help`,智能识别注释头/feature 宏/`#ifdef` 包裹/已存在位置,幂等);`allocation_tracker.c` 加 `lcu-mem-debug: skip` 防误伤
- **影响**:文档纯新增;脚本不动现有代码;`CLAUDE.md` 索引表登记 mem_debug.md
- **关联**:`inc/mem/mem_debug.h`、`inc/mem/allocator.h` 注释为依据

### 14. file_util_read_txt 动态缓冲支持任意长度行 — 2026-06-18
- **症状**:固定 `buf[1024]` 遇超长行时截断,残留片段被当新行,导致数据丢失 + 行号错位
- **根因**:`fgets(buf, 1024, fp)` 单次最多读 1023 字节,无 `\n` 时未检测截断,下次读入同一行剩余部分误判为新行
- **改动**:初始 1024 字节堆分配,检测未读完(无 `\n` 且未 EOF)时翻倍 `realloc` 直到完整行
- **影响**:兼容任意长度行,向后兼容;短行(<1024)零额外开销;API 文档注释新增"动态扩展"说明
- **文件**:`src/file/file_util.c`,`inc/file/file_util.h`

---

## 遗留路线(1.9.0+,非阻塞发版)

> 详细 bug 台账见 **[open-items.md](open-items.md)**

- **thpool atomic 迁移**(P1) — `volatile int` → `lcu_atomic_uint32_t`,消除 ARM 弱模型残留风险(条目 8 M-1/M-2/4.2/4.3)
- **file_util API 现代化**(P2) — 返回类型改 `ssize_t`,支持 >2GB 不截断(条目 8 M-4 已防 UB,这是 API 限制)
- **加固缺口**(P2) — D-2 array 容量乘法溢出 / D-3 base64 size INT_MAX 截断
- **设计取舍/文档化** — M1 uninit 并发安全(test-only);#5 raw double-free 检测盲区

### 15. 一键发版脚本 — 2026-06-22
- **动机**:缺少统一发版入口,各平台脚本需手动逐个调用
- **改动**:新增 `tool/deploy_release.ps1`(PowerShell 编排引擎) + `tool/deploy_release.bat`(cmd 薄壳);四平台均复用既有子脚本;顺带修复既有脚本 `deploy_for_linux.sh:20` 未引用括号语法错误(`echo ...($_build_type)...` → 加双引号);`build.md` 新增一键发版文档
- **影响**:单条命令 `deploy_release.bat` 发全平台 Release;任一平台失败阻断打包并非零退出;产物归档到 `deploy/__archive__/lcu_<ver>_release_<date>.tar.gz`
- **关联**:`tool/deploy_release.ps1`、`tool/deploy_release.bat`、`tool/deploy_for_linux.sh`、`.ai/kb/build.md`

### 16. mem_debug.h CRT 头文件顺序修复 — 2026-06-22
- **动机**:`diagnostics.h` 在 Windows Debug 下先包含 `<stdlib.h>`,导致 `mem_debug.h` 后定义的 `_CRTDBG_MAP_ALLOC` 失效(CRT malloc/free 宏替换必须在首次包含 stdlib.h 前定义),内存泄漏无法报告文件名/行号
- **改动**:`mem_debug.h` 在需要时先定义 `_CRTDBG_MAP_ALLOC` 并包含 `<stdlib.h>/<crtdbg.h>`,再包含 `diagnostics.h`(仅一处,L28);`diagnostics.h` 顶层不再包含标准库,在 CRT API 区域按需包含并通过 `_CRTDBG_H_` 守卫避免重复;`diagnostics.c:554` 补齐 `#endif` 注释
- **影响**:Windows Debug 下 MSVC CRT 内存泄漏检测现可正确报告位置;包含结构清晰(单点 include),其他平台不受影响
- **关联**:`inc/mem/mem_debug.h`、`inc/debug/diagnostics.h`、`src/debug/diagnostics.c`

### 17. mem_debug.h 移除 diagnostics.h 依赖 — 2026-06-22
- **动机**:Debug 客户端包含 `mem_debug.h` 链接 Release lcu 库时 LNK2001(`lcu_diagnostics_register_current_crt` 未定义);根因:`mem_debug.h` 包含 `diagnostics.h` 导致必须匹配库编译配置,违背轻量头文件设计
- **改动**:移除 `diagnostics.h` include;Windows Debug 路径的 `MEM_CHECK_INIT()` 改为**头文件内联实现**(直接调用 `_CrtSetReportMode/_CrtSetReportFile` 等 CRT API,无链接依赖);`_LCU_MEM_CHECK_FEATURE_ENABLE` 和 fallback 路径添加前向声明避免警告
- **影响**:Windows Debug 客户端可链接**任意配置**的 lcu 库(Release/Debug 均可),完整保留内存泄漏检测能力;头文件自洽,无编译配置耦合
- **关联**:`inc/mem/mem_debug.h:28-64,88-96,156-165`

### 18. ini 长行解析与诊断增强 — 2026-06-23
- **动机**:`sample.ini` 长中文注释超过旧 `INI_MAX_LINE=200`, `file_util_read_all` 成功但 `ini_parser_parse_str` 返回 NULL, 调用方只能看到模糊失败。
- **改动**:`ini_reader` 默认改为 heap+realloc、`INI_MAX_LINE` 提升到 64KiB, 超长注释行丢弃继续、超长配置行失败; `ini_parser` value 改为内联 256B + 超长堆分配; 新增 `*_with_diagnostics` API。
- **影响**:支持长注释/长 value, 避免 value 静默截断; 旧 parse API 保持兼容, 新 API 可返回行号/reader_code/message。
- **关联**:`inc/file/ini_reader.h`,`src/file/ini_reader.c`,`inc/file/ini_parser.h`,`src/file/ini_parser.c`,`src_demo/file/ini_test.c`
