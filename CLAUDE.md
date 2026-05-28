# CLAUDE.md

本文档为 Agent 提供项目指导，帮助理解和操作本代码库。

## 项目概述

libcutils (lcu) 是一个跨平台 C 工具库，支持 Windows、Linux 和 Android。提供常用数据结构、日志、线程、内存管理和文件工具。

## 编码规范

@.ai/kb/conventions.md

## 知识库索引

以下知识文件按需读取，执行相关任务前用 Read 工具加载对应文件：

| 场景 | 文件 | 何时读取 |
|------|------|----------|
| 构建、部署、CI、工具链 | `.ai/kb/build.md` | 执行构建命令、配置工具链、排查编译问题时 |
| 模块 API、用法示例 | `.ai/kb/modules.md` | 新增/修改模块代码、查看 API 用法时 |
| 修改记录 | `.ai/kb/changelog.md` | 会话结束需要记录修改时 |

## 注意事项

- **hashmap 非线程安全** - 需自行加锁或创建时提供锁函数
- **ring_buffer_clear 非线程安全** - 确保无并发读写时调用
- Windows 下 `PRJ_WIN_PTHREAD_MODE` 决定 pthread 实现方式

## 修改规范

会话中有重要修改时，将总结追加到 `.ai/kb/changelog.md` 底部，形成有序列表。
