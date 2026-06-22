# 构建命令

所有构建脚本位于 `tool/` 目录，需在该目录下执行。

## 一键发版（windows / linux / android / linaro7.5.0）

`deploy_release.bat`（cmd 薄壳）+ `deploy_release.ps1`（PowerShell 编排引擎）一键发版多平台。
默认全部 Release；windows 用 pthread_mode=1（pthreads-win32 静态库，真正的 posix pthread 实现）；
linux / linaro7.5.0 经 WSL 交叉编译（默认发行版 `ubuntu_16.04`）。

```bash
# 一键全平台发版（Release + tar.gz 归档）
deploy_release.bat

# 指定构建类型 / 平台子集（平台用逗号分隔，无空格）
deploy_release.bat Debug
deploy_release.bat Release "windows,linux"

# 进阶参数直接调 PowerShell
powershell -ExecutionPolicy Bypass -File deploy_release.ps1 -Platforms windows,linux -BuildType Release -Distro ubuntu_22.04 -NoPackage
```

- 各平台复用既有子脚本（`deploy_for_windows.bat` / `deploy_for_android.bat` / `deploy_for_linux.sh` / `make_cross_platform.sh`），不重写构建逻辑。
- 任一平台失败则跳过打包并以非零码退出；汇总表显示每平台 OK/FAIL 与耗时。
- 归档产物：`tool/deploy/__archive__/lcu_<git描述>_<类型>_<日期>.tar.gz`（含本次平台目录 + `inc/`），`-NoPackage` 可跳过。
- 依赖：WSL（linux/linaro）、VS（windows，自动探测）、NDK r16b+ 与 ninja（android）、host `tar`。

## 输出目录

构建产物位于 `tool/deploy/<构建类型>/<平台>_<abi>/`:

- `lcu.dll` / `liblcu.so` - 动态库
- `lcu_a.a` / `liblcu_a.a` - 静态库
- `inc/` - 头文件

## Windows

```bash
# 一键构建（Win32 + Win64）
deploy_for_windows.bat Release

# 单独构建指定架构
make_windows.bat Win64 Release 1    # 参数: 架构, 构建类型, pthread模式
# pthread模式: 0=原生实现, 1=pthread静态库, 2=pthread动态库
```

## Linux

```bash
chmod +x *.sh
./deploy_for_linux.sh Release       # 一键构建 (m64 + m32)
./make_cross_platform.sh linux m64 Release  # 单独构建指定架构
```

## Android

```bash
deploy_for_android.bat Release c++_static   # 一键构建（所有 ABI）
make_android.bat armeabi-v7a Release        # 单独构建指定ABI
# ABI: armeabi-v7a, arm64-v8a, x86, x86_64
```

## 交叉编译

1. 创建工具链文件: `tool/cmake/toolchains/<平台>.toolchain.cmake`
2. 执行: `./make_cross_platform.sh <平台> <abi> Release`

仅构建静态库:

```bash
cmake -H. -B./build_<平台> -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_DEMO=OFF -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY -DCMAKE_TOOLCHAIN_FILE=./cmake/toolchains/<平台>.toolchain.cmake
cmake --build ./build_<平台> --config Release --target lcu_static
```
