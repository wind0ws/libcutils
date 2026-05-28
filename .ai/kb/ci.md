# CI 集成指南

## 概述

`lcu_demo` 内置 JUnit XML 输出与 CTest 集成，可直接接入 CI/CD 管道。

## CTest 标签

- `lcu`：默认用例（18 条），CI 必跑
- `lcu_optional`：opt-in 用例（如 `memleak_test` 故意 leak），CI 选跑

## GitHub Actions 示例

```yaml
name: lcu-ci
on: [push, pull_request]

jobs:
  test-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Configure
        run: cd tool && cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
      - name: Build
        run: cmake --build tool/build -j
      - name: Test (default suite)
        run: cd tool/build && ctest -L "^lcu$" --output-on-failure -j 4
      - name: Run with JUnit
        if: always()
        run: tool/deploy/debug/linux_x64/lcu_demo --junit junit.xml --all
      - uses: actions/upload-artifact@v4
        if: always()
        with:
          name: junit-xml
          path: junit.xml

  test-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
      - name: Configure
        run: cd tool; cmake -B build -S .
      - name: Build
        run: cmake --build tool/build --config Debug -j
      - name: Test
        run: cd tool/build; ctest -L "^lcu$" --output-on-failure -j 4 -C Debug
```

## Jenkins Pipeline 示例

```groovy
pipeline {
    agent any
    stages {
        stage('Configure') {
            steps {
                sh 'cd tool && cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug'
            }
        }
        stage('Build') {
            steps {
                sh 'cmake --build tool/build -j'
            }
        }
        stage('Test') {
            steps {
                sh 'cd tool/build && ctest -L "^lcu$" --output-on-failure -j 4 --output-junit ctest-results.xml'
            }
            post {
                always {
                    junit 'tool/build/ctest-results.xml'
                }
            }
        }
    }
}
```

## 失败诊断

CTest 失败时按顺序排查：

1. `ctest --rerun-failed --output-on-failure -V` — 看详细输出
2. `lcu_demo <case_name>` — 单独跑失败用例（带完整日志）
3. `tool/build/Testing/Temporary/LastTest.log` — CTest 日志归档
4. 看 `tool/deploy/<config>/<plat>/memleak.log`（Windows MSVC Debug）

## 退出码语义

| 退出码 | 含义 |
|---|---|
| `0` | 全过 |
| `1..254` | 失败用例数 |
| `255` | CLI 参数错误 |

JUnit 报告由 `lcu_demo --junit <file>` 直接输出（与 `--all` / `--filter` / 名字配合）。可与 CTest 的 `--output-junit` 并存。

## CTest 并行说明

- `ctest -j N` 是**进程级并行**，每个 case 独立进程
- 单进程内不并行（由 `lcu_demo` 内部串行执行选定用例）
- `posix_thread_test` / `thpool_test` / `msg_queue_handler_test` 用 `RUN_SERIAL TRUE`（多线程 + xlog 全局上下文）
- `file_logger_test` 用 `RESOURCE_LOCK "log_dir"`（写共享日志目录）
- 慢用例（`time_util_test` / `file_logger_test`）单独 `TIMEOUT 300`

## 已知限制

1. **新增/删除测试需 reconfigure**：CMake 3.10 不支持 `CONFIGURE_DEPENDS`，新增 `*_test.c` 后必须 `cmake .` 重 configure，否则 `add_test` 不会更新。
2. **`#if 0` 包裹注册宏**：CMake 仍会发现并 add_test，但运行时报 unknown test。**对策**：用 `//` 注释整行或删除。
3. **同一行多个 `LCU_TEST_REGISTER`**：CMake 只匹配第一个。**对策**：每行只写一个注册宏。
4. **mem_debug 验证须 Debug build**：Windows MSVC Debug 下 CRT 全局跟踪自动生效；Linux 默认无追踪，需 `-D_LCU_MEM_CHECK_FEATURE_ENABLE=1`。
