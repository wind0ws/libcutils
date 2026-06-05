# file_logger Cleanup Bug 深度诊断任务

## 症状

file_logger_test.c 两处 ASSERT 失败(baseline 和当前版本均复现):

1. **Line 108**: `ASSERT(false == does_log_file_exist(old_file))` 
   - 场景:创建 `lcu_cleanup_old.log`(mtime = now - 3天) 和 `lcu_cleanup_recent.log`(mtime = now)
   - 调用 `file_logger_run_cleanup_now`,期望 old 被删、recent 保留
   - **实际**:old 文件仍然存在

2. **Line 122**: `ASSERT(false == does_log_file_exist("lcu_cleanup_size_a.log"))`
   - 场景:创建 a/b/c 三个文件(总和超过 30KB 限制),a 最旧(now-3s)
   - 调用 `file_logger_run_cleanup_now`,期望 a 被删
   - **实际**:a 文件仍然存在

## 配置

```c
#define TEST_LOG_RETENTION_DAYS (2U)
#define TEST_LOG_TOTAL_LIMIT_BYTES (30U * 1024U)

g_logger_ctx.logger_cfg.max_log_retention_days = TEST_LOG_RETENTION_DAYS;
g_logger_ctx.logger_cfg.max_total_log_storage_bytes = TEST_LOG_TOTAL_LIMIT_BYTES;
g_logger_ctx.logger_cfg.log_file_name_prefix = "lcu_";
g_logger_ctx.logger_cfg.log_folder_path = "./log" (Windows) / "./log/" (POSIX)
```

## 关键代码路径

### 1. 测试代码创建 dummy 文件

**src_demo/log/file_logger_test.c:157-175**
```c
static void create_dummy_log_file(const char *file_name, size_t file_size, time_t modified_time)
{
    char full_path[TEST_LOG_PATH_BUFFER];
    int written = snprintf(full_path, sizeof(full_path), "%s%s", 
                          g_logger_ctx.logger_cfg.log_folder_path, file_name);
    ASSERT(written > 0 && written < (int)sizeof(full_path));
    FILE *fp = fopen(full_path, "wb");
    ASSERT(fp);
    // ... write pattern ...
    fflush(fp);
    fclose(fp);
    set_file_time(full_path, modified_time);  // ← 关键:设置 mtime
}

static void set_file_time(const char *full_path, time_t modified_time)
{
#ifdef _WIN32
    struct __utimbuf64 new_time = {0};
    new_time.actime = modified_time;
    new_time.modtime = modified_time;
    _utime64(full_path, &new_time);  // ← 无返回值检查!
#else
    struct utimbuf new_time;
    new_time.actime = modified_time;
    new_time.modtime = modified_time;
    utime(full_path, &new_time);  // ← 无返回值检查!
#endif
}
```

**可疑点A**: `_utime64`/`utime` 无返回值检查,可能静默失败(权限/路径/文件锁定)

### 2. cleanup 主逻辑

**src/log/file_logger.c:121-185**
```c
static void file_logger_cleanup_logs(file_logger_t *handle)
{
    if (file_logger_should_skip_cleanup(handle)) return;
    
    // 收集目录下所有匹配 "lcu_*.log" 的文件
    log_file_entry_t *entries = NULL;
    size_t entry_count = 0U;
    if (!file_logger_gather_entries(handle, &entries, &entry_count) || 0U == entry_count)
    {
        file_logger_free_entries(&entries);
        return;
    }
    
    const bool has_retention_policy = (handle->cfg.max_log_retention_days > 0U);
    const bool has_size_policy = (handle->cfg.max_total_log_storage_bytes > 0U);
    
    // === 按日期清理 ===
    time_t expire_before = 0;
    if (has_retention_policy)
    {
        time_t now = time(NULL);
        expire_before = now - (time_t)handle->cfg.max_log_retention_days * 24 * 3600;
    }
    size_t write_idx = 0U;
    for (size_t i = 0U; i < entry_count; ++i)
    {
        log_file_entry_t entry = entries[i];
        // 跳过当前正在写的文件
        if (handle->cur_file_path[0] != '\0' && 0 == strcmp(entry.path, handle->cur_file_path))
        {
            total_size += entry.size;
            entries[write_idx++] = entry;
            continue;
        }
        // 过期文件删除
        if (has_retention_policy && entry.modified_time < expire_before)
        {
            file_logger_remove_file(handle, entry.path);
            continue;  // ← 不放入剩余列表
        }
        total_size += entry.size;
        entries[write_idx++] = entry;
    }
    entry_count = write_idx;
    
    // === 按大小清理 ===
    if (has_size_policy && total_size > handle->cfg.max_total_log_storage_bytes)
    {
        qsort(entries, entry_count, sizeof(log_file_entry_t), log_file_entry_compare);
        for (size_t i = 0U; i < entry_count && total_size > handle->cfg.max_total_log_storage_bytes; ++i)
        {
            if (handle->cur_file_path[0] != '\0' && 0 == strcmp(entries[i].path, handle->cur_file_path))
            {
                continue;  // 跳过当前写入文件
            }
            if (file_logger_remove_file(handle, entries[i].path))
            {
                if (total_size > entries[i].size)
                    total_size -= entries[i].size;
                else
                    total_size = 0U;
            }
        }
    }
    file_logger_free_entries(&entries);
}
```

**可疑点B**: 日期清理逻辑中 `entry.modified_time < expire_before` 判断是否正确?

### 3. 文件扫描

**src/log/file_logger.c:187-242**
```c
static bool file_logger_gather_entries(file_logger_t *handle, log_file_entry_t **entries_p, size_t *entry_count_p)
{
    DIR *dir = opendir(handle->cfg.log_folder_path);
    if (!dir) return false;
    
    struct dirent *dir_entry = NULL;
    while ((dir_entry = readdir(dir)) != NULL)
    {
        if ('.' == dir_entry->d_name[0]) continue;
        if (!file_logger_is_log_file(handle, dir_entry->d_name)) continue;  // ← 前缀+扩展名过滤
        
        char full_path[MAX_FULL_PATH_SIZE];
        int written = snprintf(full_path, sizeof(full_path), "%s%s", 
                              handle->cfg.log_folder_path, dir_entry->d_name);
        
        FILE_LOGGER_STAT_STRUCT file_stat;
        if (0 != FILE_LOGGER_STAT(full_path, &file_stat)) continue;
        if (!S_ISREG(file_stat.st_mode)) continue;
        
        log_file_entry_t entry;
        memset(&entry, 0, sizeof(entry));
        strlcpy(entry.path, full_path, sizeof(entry.path));
        entry.modified_time = file_stat.st_mtime;  // ← 关键:读取 mtime
        entry.size = (uint64_t)file_stat.st_size;
        // ... push to entries ...
    }
    closedir(dir);
    return true;
}

static bool file_logger_is_log_file(const file_logger_t *handle, const char *file_name)
{
    const size_t prefix_len = strlen(handle->cfg.log_file_name_prefix);
    if (prefix_len > 0U && strncmp(file_name, handle->cfg.log_file_name_prefix, prefix_len) != 0)
        return false;
    const char *extension = strrchr(file_name, '.');
    if (!extension || 0 != strcmp(extension, ".log"))
        return false;
    return true;
}
```

**可疑点C**: Windows 下 `FILE_LOGGER_STAT` 宏定义,`st_mtime` 是否正确读取?

### 4. 文件删除

**src/log/file_logger.c:294-305**
```c
static bool file_logger_remove_file(file_logger_t *handle, const char *path)
{
    if (!path || '\0' == path[0]) return false;
    
    if (0 != remove(path))
    {
        LOG_LIB_IMPL("Failed to remove log file %s (errno=%d)\n", path, errno);
        return false;
    }
    LOG_LIB_IMPL("Removed log file: %s\n", path);
    return true;
}
```

**可疑点D**: `remove` 失败是否正确报告? 有无权限/锁定问题?

### 5. Windows/POSIX stat 宏定义

需要在 file_logger.c 头部查看:
```c
#ifdef _WIN32
#define FILE_LOGGER_STAT_STRUCT struct _stat64
#define FILE_LOGGER_STAT _stat64
#else
#define FILE_LOGGER_STAT_STRUCT struct stat
#define FILE_LOGGER_STAT stat
#endif
```

## 诊断目标

workflow 需回答:

1. **root cause 层**:为什么 old 文件没被删?
   - `set_file_time` 失败导致 mtime 未设置?
   - `gather_entries` 读到的 mtime 不对?
   - cleanup 逻辑判断条件错误?
   - `remove` 调用失败?

2. **多假设并行验证**:
   - 假设1:Windows `_utime64` 失败(路径/权限/文件句柄未释放)
   - 假设2:路径拼接问题(Windows "./log" vs POSIX "./log/",导致 "./loglcu_cleanup_old.log"?)
   - 假设3:`expire_before` 计算或比较逻辑错误
   - 假设4:`FILE_LOGGER_STAT` 在 Windows 下读 mtime 有问题
   - 假设5:`remove` 失败但未阻止测试继续(assert 应该捕获这个,但 CRT 重定向后 assert 不 abort)

3. **修复方案对抗**:生成 2-3 个候选修复,做 pros/cons 分析,选最稳妥的

## 文件位置

- **src/log/file_logger.c** - cleanup 主逻辑
- **src/log/file_logger.h** - API 声明
- **src_demo/log/file_logger_test.c** - 测试代码
- **inc/common_macro.h** - ASSERT 宏定义
- **src/file/file_util.c** - mkdirs 等工具

## 平台信息

- OS: Windows 11 Pro 10.0.26100
- Compiler: MSVC (Visual Studio 17 2022)
- Build: Debug 配置
- 测试失败位置: test.c:108, 122

## 要求

1. **读取关键源码**确认 5 个可疑点的实际代码
2. **构建假设树**,找到 root cause
3. **生成 2-3 个修复方案**,做对抗评审(权衡 risk/complexity/compatibility)
4. **输出修复计划** markdown,包含:
   - Root cause 明确结论
   - 推荐修复(patch diff + 理由)
   - 备选方案(若推荐方案有 edge case)
   - 验证步骤(怎么跑测试确认修复有效)
