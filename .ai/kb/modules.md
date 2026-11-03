# 模块速查

| 模块 | 头文件目录 | 说明 |
|------|-----------|------|
| **data** | `inc/data/` | 数据结构: array, list, hashmap, base64, bit_ops |
| **log** | `inc/log/` | 日志门面模式，支持 xlog(完整) 和 slog(精简) |
| **mem** | `inc/mem/` | 内存管理: allocator, mplite(零malloc内存池), stringbuilder |
| **thread** | `inc/thread/` | 线程抽象: pthread兼容层, portable_thread, thpool(线程池) |
| **ring** | `inc/ring/` | 环形缓冲: ring_buffer, msg_queue, auto_cover_buffer |
| **file** | `inc/file/` | 文件工具: file_util, ini_parser, ini_reader |
| **time** | `inc/time/` | 时间工具: time_util, RFC1123/2822 格式化 |
| **sys** | `inc/sys/` | 系统抽象: dirent(Windows), dlfcn(Windows) |
| **net** | `inc/net/` | 网络工具: URL编解码 |

## 日志使用

```c
#define _LOG_TAG "MyModule"
#include "log/logger_facade_xlog.h"  // 或 logger_facade_slog.h

LOGI("message: %s", str);
LOGE_TRACE("error code = %d", err_code);  // 带函数名称和代码行数位置信息
```

- X 族快捷宏 `LOGXV/D/I/W/E` 隐含脱敏(`LOG_FLAG_SANITIZE`)。其 flags 默认值由公开旋钮
  `LOGX_FLAGS` 控制：默认即脱敏，TU 级 opt-out 需在**任何 log 头首次 include 前** `#define LOGX_FLAGS 0`；
  单条细粒度控制用 `TLOGX(level, 0, ...)`(关脱敏) / `TLOGX(level, LOG_FLAG_SANITIZE, ...)`(开脱敏)。
  该值覆盖整个 flags(不止脱敏位)。

## 线程使用

```c
#include "thread/portable_thread.h"  // 平台无关 API
#include "thread/thpool.h"           // 线程池

// 线程池示例
threadpool pool = thpool_init(4);
thpool_add_work(pool, my_func, my_arg);
thpool_wait(pool);
thpool_destroy(pool);
```
