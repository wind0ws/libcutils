# 简介
  这是一个通用C工具库，支持 unix(android)/windows 等平台
  > PS: 当前api接口可能经常改动，详情请见commit提交日志

----
## 组件
* **log**
  > 支持同时打印日志到 file/console/logcat(Android), 也支持配置日志输出格式、等级.
  
  *  **logger** : logger门面. xlog and slog 实现它定义的宏
  *  **xlog** : 支持打印 level/tag/tid/time 这些log头和日志信息
  *  **slog** : 简易log实现，实际上是对printf的二次包装, 只支持打印level/TAG和日志信息

* **thread**
  > 支持Windows上使用posix风格的线程/信号量（thread/semaphore）接口(感谢 [pthread-win32](https://sourceforge.net/projects/pthreads4w/)). 
    unix(linux/android)本身就是posix风格。

* **data**
  > 常用数据结构和操作接口: 包括 ***array***, ***hashmap***, ***list***, ***base64*** ...

* **file**
  > 常用文件操作: 包括 **file_util_read/write**, **ini_reader/parser** ...

* **memory**
   *  **string** : 包括 ***asprintf***, ***stringbuilder***, ***str_params***, ***strlcpy***, ***strlcat***, ***strreplace***, ***strsplit***, ***strtrim***, ***strutf8len*** ...
   *  **allocator** : 可根据heap内存，帮助你提早发现内存泄露和内存破坏等问题.
   *  **mplite** : 内存池实现 [SQLite's memsys5 memory subsystem](https://github.com/hannes/sqlite-simplified/blob/master/mem5.c)

* **ring_buffer**
  > 环形队列: 包括 ***ringbuffer***, ***ring_msg_queue***, ***msg_queue_handler***, ***autocover_buffer*** ...

* **time**
  > 常用时间操作: 包括 ***current_milliseconds***, ***fast_second2date*** ...

----
## 编译
  
  * ### 常见平台 (windows/linux/android)
    > 进入 ***build***  文件夹, 编辑环境路径脚本（**setup_env.bat/setup_env.sh**）
	(**为了确定 cmake/ninja/ndk(for Android) 等工具的路径位置**), 
	然后执行脚本来编译, 编译完后会将产物拷贝到指定的位置
    
    > 简要示例:
    
    |平台         | 一键部署编译                             | 手动编译                                         |
    | --------    | :-----                                   | :----                                            |
    | **windows** | `deploy_for_windows.bat`                 | ` make_windows.bat Win32 Release 0 `             | 
    | **linux**   | `chmod +x *.sh && ./deploy_for_linux.sh` | ` chmod +x *.sh && ./make_linux.sh m64 Release ` |
    | **android** | `deploy_for_android.bat`                 | ` make_android.bat armeabi-v7a Release `         |
  
  * ### 其他平台 (交叉编译)
    1. #### 首先，在 **build/cmake/** 文件夹下写cmake交叉编译规则文件:
      > 定义 c/cxx 编译器位置和平台编译参数.
      
      > 示例: 在 **build/cmake/toolchains** 文件夹下创建 **hisi.toolchain.cmake** 文件, 
	        然后写一些类似下面的交叉编译配置信息:
      ```cmake
      SET(UNIX TRUE CACHE BOOL "")
	  # Tell the cmake script what the platform name is, must setup this for cross compile
      SET(CMAKE_SYSTEM_NAME Linux) # this one is important
      SET(CMAKE_SYSTEM_VERSION 1)  # this one not so much
	  SET(PLATFORM Hisi)           # important: tell script the platform name
      
      SET(CROSS_TOOLCHAIN_PATH_PREFIX "/root/toolchains/hisi-linux/x86-arm/arm-himix100-linux/bin/arm-himix100-linux-")
      message(STATUS "current CROSS_TOOLCHAIN_PATH_PREFIX is => ${CROSS_TOOLCHAIN_PATH_PREFIX}")

      #set compiler location
      SET(CMAKE_C_COMPILER "${CROSS_TOOLCHAIN_PATH_PREFIX}gcc")
      SET(CMAKE_CXX_COMPILER "${CROSS_TOOLCHAIN_PATH_PREFIX}g++")
      SET(CMAKE_AR "${CROSS_TOOLCHAIN_PATH_PREFIX}ar")
      SET(CMAKE_LINKER "${CROSS_TOOLCHAIN_PATH_PREFIX}ld")
      SET(CMAKE_RANLIB "${CROSS_TOOLCHAIN_PATH_PREFIX}ranlib")
      SET(CMAKE_STRIP "${CROSS_TOOLCHAIN_PATH_PREFIX}strip")
      
	  SET(PLATFORM_COMMON_FLAGS " -fPIC")
      string(APPEND CMAKE_C_FLAGS          "${PLATFORM_COMMON_FLAGS}")
      string(APPEND CMAKE_CXX_FLAGS        "${PLATFORM_COMMON_FLAGS}")
      string(APPEND CMAKE_EXE_LINKER_FLAGS "${PLATFORM_COMMON_FLAGS} -fPIE")
      add_definitions(-D_LCU_NOT_SUPPORT_PTHREAD_SETNAME) # custom compile definitions
      ```
  
    2. #### 执行 cmake 命令编译:
      > 生成makefile并编译
      
      > 示例: 进入 ***build***  文件夹里，执行命令:
      ```shell
      cmake -H. -B./build_hisi -DCMAKE_TOOLCHAIN_FILE=./cmake/toolchains/hisi.toolchain.cmake 
      cmake --build ./build_hisi --config Release
      ```
	  若平台(编译链)仅支持编译静态库，可以这么来编译:
	  ```shell
	  cmake -H. -B./build_abcd -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY -DPLATFORM=abcd -DCMAKE_TOOLCHAIN_FILE=./cmake/toolchains/abcd.toolchain.cmake
      cmake --build ./build_abcd --config Release --target lcu_static
	  ```

## 使用
  >  拷贝头文件和库（动态或静态）文件到你的项目中，并链接他们，或者直接拷贝源码到你的项目中。

## 示例
  > 请看 **src_demo** 目录，每个模块有对应的测试用例.

----
## 发布日志

* **1.5.6**
  > 1. 修复：time_util在命中时间cache时，多算了fmt_len的问题
  > 2. 更新：inih同步更新，重命名inih方法“ini_”到“ini_reader_”, 防止与其他项目冲突
  > 3. 新增：cmake脚本支持定义PLATFORM和PLATFORM_ABI, 统一化输出文件夹
  > 4. 新增: xlog支持锁，在多target打印上保持一致的顺序
  > 5. 新增: 中文文档

* **1.5.5**
  > 1. 新增：simple win32 pthread添加了更多的支持方法 (编译传参 PRJ_WIN_PTHREAD_MODE=1).
  > 2. 修复：xlog 宏调用接口错误.
  > 3. 新增：windows编译脚本支持更多版本的 visual studio.

* **1.5.3**
  > 1. 新增：logger_facade,  xlog/slog 实现这个log门面.
  > 2. 特性：提高xlog 性能: 使用自定义的打印方法名和行数函数.
  > 3. 修复：windows dll 导出符号问题 (借助cmake功能实现).

* **1.5.2**
  > 1. 特性：提高 time_util_get_current_time_str 性能.
  > 2. 特性：提高 xlog 性能.
  > 3. 新增：xlog添加flush模式，支持自动flush（默认）和每次print后flush.

* **1.4.1**
  > 1. 新增：添加 msg_queue/msg_queue_handler: 支持各种msg类型
  > 2. 重构：重命名 msg_queue/msg_queue_handler 为 fixed_msg_queue/fixed_msg_queue_handler
  > 3. 重构：提取 msg_queue_errno 到头文件.

* **1.0.0 ~ 1.4.0**
  > 这里没有记录详细日志, 详情请见提交日志. BTW: 建议使用最新版本
  >    示例：1.4.0 版本的URL是 https://github.com/wind0ws/libcutils/commits/1.4.0
  
