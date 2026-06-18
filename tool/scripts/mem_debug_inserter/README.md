# mem_debug_inserter

自动确保 `#include "mem/mem_debug.h"` 是 C/C++ 源文件的**第一个 include**,使 LCU 内存调试生效。

## 功能

- **智能插入**:在首个 `#include` 之前插入,自动跳过文件头注释、空行、feature 宏(如 `_GNU_SOURCE`)
- **重定位检测**:若 `mem_debug.h` 已存在但不在首位,自动移至正确位置
- **条件编译处理**:首个 include 被 `#ifdef` 包裹时,插到条件块**之前**(顶层),带 `[WARN]` 提示
- **注释剥离**:不会被注释里的假 `#include` 误导
- **格式保留**:保持 CRLF/LF、UTF-8 BOM 不变
- **幂等性**:改完再跑 0 change
- **Per-file opt-out**:文件头部加 `lcu-mem-debug: skip` 注释可跳过

## 用法

```bash
cd /path/to/libcutils  # 必须从项目根目录运行(Python 模块路径)

# 1. 先 dry-run 看报告(默认)
python -m tool.scripts.mem_debug_inserter <路径...>

# 2. 确认无误后执行
python -m tool.scripts.mem_debug_inserter <路径...> --apply

# 常用选项
python -m tool.scripts.mem_debug_inserter src demo --apply --quiet  # 只列需改的
python -m tool.scripts.mem_debug_inserter . --ext c,cpp --exclude mydir  # 自定义扩展名和排除
python -m tool.scripts.mem_debug_inserter . --apply --backup  # 写 .bak 备份
python -m tool.scripts.mem_debug_inserter --help  # 完整帮助
```

## 参数

| 参数 | 说明 |
|------|------|
| `paths` | 文件或目录(默认当前目录) |
| `--apply` | 执行修改(默认 dry-run) |
| `--quiet` | 只输出需改动的文件 |
| `--backup` | 修改前写 `.bak`(配合 `--apply`) |
| `--ext EXTS` | 逗号分隔的扩展名(默认 `c,cpp,cc,cxx`) |
| `--exclude DIRS` | 额外排除目录(已内置排除 `build`、`.git`、`Debug`/`Release` 等) |
| `--help` | 显示帮助 |

## 状态码

- **OK**: `mem_debug.h` 已是首个 include
- **INSERT**: 不存在,将插入
- **MOVE**: 存在但不在首位,将移动
- **SKIP**: 文件带 `lcu-mem-debug: skip` 标记
- **NONE**: 文件无任何 `#include`,无锚点可插

## Per-file opt-out

某些文件**绝不能**包含 `mem_debug.h`(如 `allocation_tracker.c`,会导致 tracker 递归)。
在文件头部(前 40 行)任意注释写入:

```c
/* lcu-mem-debug: skip — this file must NOT include mem_debug.h */
```

## 典型工作流(客户集成)

```bash
# 1. 客户拿到 LCU Release 静态库 + 头文件
# 2. 在客户源码树运行脚本
cd /path/to/customer_project
python -m tool.scripts.mem_debug_inserter src --quiet  # 先看报告
python -m tool.scripts.mem_debug_inserter src --apply  # 确认后执行
# 3. 客户编译 Debug → 自动拿 MSVC CRT 内存检测;Release 零开销
```

## 注意

- **`WARN: first include sits inside #if block`**: 脚本会把 `mem_debug.h` 提到 `#if` 之前(顶层)。
  人工确认这几个文件提前包含无副作用。
- **扫 LCU 源码自身**: `src/mem/allocation_tracker.c` 会被误报 INSERT(它绝不能包含此头)。
  已为其加 skip 标记。扫**客户工程**无此问题。
