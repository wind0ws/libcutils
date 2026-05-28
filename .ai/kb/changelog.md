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
