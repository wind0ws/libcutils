# 开放项台账

> 登记"评审发现但未修"的真实 bug/债务。每项包含:**位置 / 根因 / 修复方向 / 优先级 / 状态**。
> 修复落地时,在 changelog.md 写一行,本文件标 `[closed]`。

---

## 活跃项(未结案)

### D-2: array 容量乘法溢出(加固缺口,非活跃 bug)
- **位置**: `src/data/array.c`
- **根因**: `capacity * element_size` 无上限校验,乘法溢出时分配小块后越界写
- **修复方向**: 入口加 `capacity > SIZE_MAX / element_size` 守卫
- **优先级**: P2(文档无契约,无现存 caller 传巨值,触发需刻意构造)
- **状态**: `[open]` 中期加固
- **关联**: 条目 7 已加 `NOTE(reviewed 2026-06-08)` 固化"加固缺口非活跃 bug"

### D-3: base64 size helper INT_MAX 截断(加固缺口)
- **位置**: `src/data/base64.c`
- **根因**: size helper 返回 `int`,输入 >INT_MAX 截断后调用方分配小块越界
- **修复方向**: 改返回 `size_t` 或入口拒 >INT_MAX
- **优先级**: P2(无现存 caller 传巨输入,ABI 影响需评估)
- **状态**: `[open]` 中期加固
- **关联**: 条目 7 固化注释

### M1: allocation_tracker uninit 并发安全
- **位置**: `src/mem/allocation_tracker.c`
- **根因**: `uninit` 与 `notify_alloc/free` 无锁竞争 = UAF
- **修复方向**: 改 atomic/RCU 或加 quiesce 机制
- **优先级**: P2(test-only API,生产不调用 uninit)
- **状态**: `[open]` 文档化决策 — `allocation_tracker.h` 已标注"test-only,调用前须 quiesce 所有 alloc/free 线程"
- **关联**: 条目 10 / 13

### #5: raw 指针 double-free/野指针检测盲区
- **位置**: `src/mem/allocator.c` `lcu_free`
- **根因**: `lcu_free` 对未命中指针回退 libc free,raw 指针 double-free 走 libc 不报警
- **修复方向**: 无(设计取舍——要对称回退 raw 就必然放弃 raw 检测;封死回退则破坏条目 6/10 的跨边界 libc free 契约)
- **优先级**: P2(设计决策,非 bug)
- **状态**: `[open]` 文档化 — `allocator.h` 已标注"raw 指针释放不享越界/double-free 检测"
- **关联**: 条目 10 #1 / 条目 13

### thpool volatile→atomic 迁移(ARM 严格正确性)
- **位置**: `src/thread/thpool.c`
- **根因**: `jobqueue.len` 跨锁读(M-1)/ `volatile int` 当同步原语(M-2)/ `add_work` 与 `destroy` 并发孤儿 job(4.3)
- **修复方向**: 迁移 `lcu_atomic_uint32_t`,消除 ARM 弱模型残留风险
- **优先级**: P1(ARMv8 真机 80s 压测未触发,但理论风险存在)
- **状态**: `[open]` 1.9.0+ 路线 — `thpool.h` 已加 `@warning` 文档化 ARM 弱模型限制与并发契约
- **关联**: 条目 8 M-1/M-2/4.2/4.3

### file_util API 返回类型现代化
- **位置**: `src/file/file_util.c` 读写函数
- **根因**: 返回 `int` 截断,>2GB 文件累加 UB(M-4 已加固内部为 size_t,但返回仍截断)
- **修复方向**: 改返回 `ssize_t`,头文件加 `@since 1.10.0` ABI 警告
- **优先级**: P2(M-4 已防 UB,截断是 API 限制非安全问题)
- **状态**: `[open]` 1.9.0+ 路线
- **关联**: 条目 8 M-4

---

## 已结案项(历史参考)

### C-1: allocation_tracker_resize_for_canary 溢出 `[closed 2026-06-10]`
- **症状**: `lcu_malloc(SIZE_MAX-8)` 环绕小块 + 尾 canary 越界写
- **修复**: 入口 `size > SIZE_MAX - 2*canary` 守卫
- **关联**: 条目 8 Phase 1 / commit 25bcfa8

### C-2: file_logger_log >4GB 堆溢出 `[closed 2026-06-10]`
- **症状**: `size_t msg_size` 截断为 uint32_t 后用原始 size_t memcpy
- **修复**: 入口拒 `msg_size > INT_MAX`
- **关联**: 条目 8 C-2 + 4.1 / commit 25bcfa8 + 6d22e35(GCC 补 `<limits.h>`)

### H-1: ini_parser_dump 跨边界所有权损坏 `[closed 2026-06-10]`
- **症状**: `strdup`(tracked)返回给调用方,外部 libc free 崩
- **修复**: 改 `lcu_malloc_raw`,补 ownership_contract_test 第 5 API
- **关联**: 条目 8 Phase 1 / 条目 6 漏修同源 bug

### H-2: msg_queue_handler_push 负长度巨值提升 `[closed 2026-06-10]`
- **修复**: 入口拒 `obj_len < 0`
- **关联**: 条目 8 Phase 2 / commit d19c3b2

### H-4: file_util_mkdirs 等长路径栈外读 `[closed 2026-06-10]`
- **修复**: `> MAX_FOLDER_PATH_LEN` 改 `>=`
- **关联**: 条目 8 Phase 2

### M-3: pthread_rwlock_init 读未初始化 `*rwlock` UB `[closed 2026-06-10]`
- **修复**: 移除 `|| NULL == *rwlock` 分支
- **关联**: 条目 8 Phase 2 / commit d19c3b2

### M-4: pri_internal_rw_file >2GB 累加 UB `[closed 2026-06-10]`
- **修复**: 累加器 int→size_t,返回前 >INT_MAX 截断
- **关联**: 条目 8 Phase 2

### 4.5: ini_parser_save 非原子写断电丢配置 `[closed 2026-06-10]`
- **修复**: Windows `MoveFileExA(REPLACE_EXISTING|WRITE_THROUGH)`,POSIX `rename`
- **关联**: 条目 8 Phase 2

### L-1: lcu_realloc_trace tracker 未 INIT 丢数据 `[closed 2026-06-10]`
- **修复**: 加 `ASSERT(cur_ptr_size > 0)`
- **关联**: 条目 8 Phase 3 / commit 5fac23b / 条目 13 #1 顺带消除

### L-3: pthread_cond_init CreateSemaphoreW 失败无返回码 `[closed 2026-06-10]`
- **修复**: 失败返 `ENOMEM`
- **关联**: 条目 8 Phase 3

### Caller-1: str_params_create_str 重复 key 新 key 泄漏 `[closed 2026-06-10]`
- **修复**: 检查 `hashmap_put` 返回,替换路径 `free(key)`
- **关联**: 条目 8 Phase 1

### #1: lcu_realloc 未追踪指针 ASSERT_ABORT 不对称 `[closed 2026-06-16]`
- **修复**: 新增 `allocation_tracker_try_ptr_size`,realloc 对未命中回退 libc realloc
- **关联**: 条目 13 #1(P0)

### #2: operator new/delete ODR 违反 LNK1169 `[closed 2026-06-16]`
- **修复**: 移到 `src/mem/mem_debug.cpp`,头文件只留声明
- **关联**: 条目 13 #2(P0)

### M3: placement delete 缺失,构造抛异常泄漏 `[closed 2026-06-16]`
- **修复**: 补 `operator delete(void*,const char*,const char*,int)`
- **关联**: 条目 13 M3(P1)

### #4: realloc +4096 过分配削弱越界检测 `[closed 2026-06-16]`
- **修复**: 删 `_REALLOC_MORE_SIZE`,按精确 size 放 canary
- **关联**: 条目 13 #4(P1)

### #3: uninit 后释放 tracked 指针堆损坏 `[closed 2026-06-16]`
- **修复**: canary 改编译期常量;uninit 残留 fatal-log;`tracker_was_torn_down` 一次性 loud warn
- **关联**: 条目 13 #3(P1)

### M4: allocation_tracker_init 未校验 pthread_mutex_init `[closed 2026-06-16]`
- **修复**: 失败 `ASSERT_ABORT`
- **关联**: 条目 13 M4(P2)
