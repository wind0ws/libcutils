# mem_debug 内存调试与所有权契约

> **最佳实践(TL;DR)**:LCU 只发 **Release 静态库**(不开 `_LCU_MEM_CHECK_FEATURE_ENABLE`),
> 客户每个 `.c/.cpp` 首行 include `mem_debug.h`,客户 **Debug / Release 两种模式都能直接链接、
> free 不崩**,且 Debug 下自动获得 MSVC CRT 内存检测。
> 原理:静态库 `.lib` 不含 CRT,malloc/free 在客户链 EXE 时才绑定 → Release 库在 Debug EXE 里
> 自动走 debug 堆(单堆)。**前提**:静态库 + 不开 flag + 客户静态 CRT 同系(`/MT`·`/MTd`)+ 单 EXE。
> 非 MSVC 平台查内存优先用 GCC/Clang **ASAN**。

`mem/mem_debug.h` 是 LCU 的内存调试入口,**按约定应在每个 `.c/.cpp` 源文件第一行 include**。
它是**纯文本宏**,在每个翻译单元(TU)编译时按宏选择形态,不是运行时开关。

## 一、三种形态(互斥,由编译期宏决定)

| 形态 | 触发条件 | malloc/free 去向 | 检测机制 |
|------|---------|-----------------|---------|
| **A · CRT 堆** | Windows + `_DEBUG` + **未**定义 `_LCU_MEM_CHECK_FEATURE_ENABLE` | 保持 CRT malloc/free,仅改写 `new` | MS CRT 调试堆(`_CrtDumpMemoryLeaks`) |
| **B · LCU tracker** | 定义 `_LCU_MEM_CHECK_FEATURE_ENABLE=1`(任意平台) | 改写为 `lcu_malloc_trace`/`lcu_free` | LCU canary + 文件行号追踪 |
| **C · 透传** | 其余(Release 无 flag / Linux 无 flag) | 纯 libc,不改写 | 无 |

形态 A 与 B 在头文件里互斥。**flag 必须库与客户两端完全一致(全开或全关)**:
若只有客户开 flag 而库没开,C++ 会因 placement `operator new` 符号未定义而 **LNK 链接错误**
(该 operator 只定义在 `mem_debug.cpp` 且 gated 在库自身的 flag 上)。

## 二、最佳实践

### 默认发版:形态 A / C(库不开 flag)

**LCU Release 发版库不编译 `_LCU_MEM_CHECK_FEATURE_ENABLE`。** 客户只需:

1. 每个 `.c/.cpp` 首行 `#include "mem/mem_debug.h"`;
2. 客户自己用 **Debug 构建** → Windows 下自动获得 CRT 堆泄漏检测(形态 A);Release 零开销透传(形态 C)。

> 注意范围:CRT 检测只覆盖**客户自己代码**的分配;Release DLL **内部**的泄漏它看不到
> (不同堆、未插桩)。这是查"客户侧"内存问题,不是查 LCU 内部。

### 非 MSVC 环境

1. **首选 GCC/Clang ASAN**(`-fsanitize=address`)。比 LCU tracker 更强:红区、
   use-after-free、栈溢出、全局越界都能抓,且无需改 LCU 编译方式。
2. **环境无 ASAN 且坚持用 LCU 检测** → 形态 B:**客户必须重新编译 LCU 库并开启**
   `_LCU_MEM_CHECK_FEATURE_ENABLE=1`,且**客户工程同样开启该 flag**(两端一致)。

## 三、内存所有权契约(最重要,任何形态都适用)

LCU 内部有三条分配路径:

- **tracked**(`lcu_malloc`/`lcu_calloc`/`lcu_strdup` 及 trace 变体):带 canary,**绝不跨边界返回**;
- **raw**(`lcu_malloc_raw`):纯 libc malloc,**用于所有"返回堆内存给客户"的 API**;
- 全局 `operator new/delete`(仅形态 B 替换)。

**释放规则取决于 LCU 是静态库还是 DLL**(详见第四节):

- **静态库**:EXE 内单一 CRT 单堆,客户直接 `free(ptr)` 即可,**不会崩**;
- **DLL(`/MT`)**:DLL 有私有堆,客户用自己 CRT 的 `free()` 释放 = 跨堆崩溃,
  **必须走 LCU 的 free**(已 include `mem_debug.h` 时 `free` 已被改写/回退;否则显式
  `lcu_free_raw`)。

涉及返回堆内存的 API:`file_util_read_all`、`ini_parser_dump`、
`asprintf`/`vasprintf`、`str_params_to_str`、`strreplace` 等。

| LCU 形式 + 客户 TU | 释放方式 |
|-------------------|---------|
| 静态库(任意) | 直接 `free(ptr)`(单堆,安全) |
| DLL,客户 TU 已 include `mem_debug.h` | `free(ptr)` —— 宏已改写/回退到 LCU 内部释放 |
| DLL,客户 TU 未 include `mem_debug.h` | **显式 `lcu_free_raw(ptr)`** |

> 建议:即使用静态库,也优先按"走 LCU 的 free / `lcu_free_raw`"的习惯写,
> 这样将来若切成 DLL 不必再改释放代码。

## 四、构建一致性约束(静态库 vs DLL,模型完全不同)

**先分清静态库和 DLL —— 二者的内存安全模型本质不同:**

- **静态库(`.lib`,LCU 主要发版形式)**:`.lib` 内部**不含 CRT**,malloc/free 只是未绑定符号,
  CRT 在**最终链接 EXE 时**才拉入。链接成功的 EXE **全程只有一份 CRT、一个堆**,
  LCU 分配 + 客户释放都绑定同一堆 → **不存在跨堆,free 不会崩**。
  - 反证:若真出现两份 CRT,MSVC 会先报 `LNK2005`(malloc 重复定义)**直接链不过**;
    能链成功就证明已收敛成单堆。
- **DLL(`/MT`)**:DLL 把 CRT **静态打进自身**,拥有**私有堆**。LCU.dll 分配、客户 `free` 释放
  = **跨堆 → 崩溃 / 堆损坏**。这才是第三节"必须用 LCU 的 free"的真正适用场景。

### CRT 配置(`/MT` vs `/MTd`)

- **DLL**:库与客户 CRT 必须严格一致,否则跨堆崩溃。
- **静态库**:`Release 库(/MT) + Debug 客户(/MTd)` 实测**能链能跑、free 不崩**
  (可能有 `LNK4098` 警告;最终单一 debug 堆,raw 缓冲与客户 `free` 都是 `_NORMAL_BLOCK`,匹配)。
  - 代价:`LNK4098` 依赖链接器容忍,换 MSVC 版本 / 库多用 CRT 符号时可能升级为 `LNK2005` 错误;
    且 LCU 内部泄漏会报在 `allocator.c` 行而非客户行(能查到,定位偏粗)。
  - 想要干净 + 精确归属:给 Debug 客户额外配一份 **Debug 静态库**(同样不开 flag)。**非强制**。

### flag 一致性(任何形式都适用)

- `_LCU_MEM_CHECK_FEATURE_ENABLE` 要么 LCU 库 + 客户全开(形态 B),要么全不开(A/C),
  不能半开 → C++ 链接错误 / new-delete 语义分裂。

### 形态 B + 多模块

- tracker 状态 `allocations` 是 `static` 单实例。若把 LCU 当**静态库**链进 EXE + 其它 DLL,
  会产生**多份 tracker** → A 模块分配、B 模块释放时按 untracked 处理,直接 libc free 一个
  canary 偏移指针 → 堆损坏。**多模块查漏场景应让 LCU 作为 DLL**(单实例)。

### 生命周期(形态 B)

- 所有 LCU 内存必须在 `MEM_CHECK_DEINIT()` 之前释放;之后再释放曾被 track 的指针无法安全
  还原 canary 偏移(代码会告警但不能修复)。

## 五、速记

| 场景 | 推荐 |
|------|------|
| 常规发版 | LCU 不开 flag,发 Release 静态库; 客户 include `mem_debug.h`; 客户 Debug 拿 CRT 查漏(形态 A) |
| 静态库 + Release 库给 Debug 客户 | ✅ 能链能跑、free 不崩(单堆);可能有 `LNK4098` 警告,可选配 Debug 库求干净 |
| 非 MSVC 查内存 | 优先 GCC/Clang ASAN |
| 无 ASAN 必须用 LCU tracker | 重编 LCU + 客户均开 flag(形态 B);多模块用同一个 LCU DLL |
| 释放 LCU 返回的缓冲 | **静态库**:直接 `free` 即可(单堆);**DLL**:已包 `mem_debug.h` 用 `free`,否则用 `lcu_free_raw` |
| 跨堆崩溃风险 | 仅 **DLL(`/MT`)** 才有;静态库单堆无此问题 |
