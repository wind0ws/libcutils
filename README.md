# Brief
  What's this: This is a common c utils library (works for unix(android)/windows).
  > note: api may change frequently at this stage. see commits log for more details

----
## Components
* **log**
  > print log to file/console/logcat(if on Android) at the same time, of course you can config it.
  
  *  **logger** : a logger_facade. xlog and slog implement it
  *  **xlog** : support print level/tag/tid/time with log message
  *  **slog**  : simple log, only print level/tag with log message

* **thread**
  > posix style thread/semaphore api on windows(thanks for [pthread-win32](https://sourceforge.net/projects/pthreads4w/)). 
  also works on unix(linux/android).

* **data**
  > data structure : include ***array***, ***hashmap***, ***list***, ***base64*** ...

* **file**
  > file util: include ** file_util_read/write **, **ini_reader/parser** ...

* **memory**
   *  **string** : include ***stringbuilder***, ***strlcpy***, ***strlcat***, ***strreplace***, ***strsplit***, ***strtrim***, ***strutf8len***, ***strnutf8len***
   *  **allocator** : can trace heap memory, help you find memory leak or memory corruption.
   *  **mplite** : A zero-malloc memory pool based on [SQLite's memsys5 memory subsystem](https://github.com/hannes/sqlite-simplified/blob/master/mem5.c)

* **ring**
  > ringbuffer: include ***ringbuffer***, ***ring_msg_queue***, ***msg_queue_handler***, ***autocover_buffer*** ...

* **time**
  > time_util: include ***current_milliseconds***, ***fast_second2date*** ...

----
## How to build
  > go to ***build***  folder, and edit build script first( ** to make sure cmake/NDK(for Android) location **), then execute script to build it, it will copy the compiled result to the specified location
  
  for example:
  
  |platform     | onekey deploy build                      | manual build                                     |
  | --------    | :-----                                   | :----                                            |
  | **windows** | `deploy_for_windows.bat`                 | ` make_windows.bat Win32 Release 0 `             | 
  | **linux**   | `chmod +x *.sh && ./deploy_for_linux.sh` | ` chmod +x *.sh && ./make_linux.sh m64 Release ` |
  | **android** | `deploy_for_android.bat`                 | ` make_android.bat armeabi-v7a Release `         |

## How to use
  >  copy header and lib(static or shared) to your project, and link it,
  or just copy source and header file to your project.

## Want demo
  > see test folder, it demonstrate to you simple use case.

----
## Release Log

* **1.5.3**
  > 1. add logger_facade, and xlog/slog implement it
  > 2. improve xlog performance: use our method to print function name and line number
  > 3. windows dll truly export all symbols( this feature supported by cmake )

* **1.5.2**
  > 1. improve time_util_get_current_time_str performance
  > 2. improve xlog performance
  > 3. add flush mode on xlog

* **1.4.1**
  > 1. add msg_queue/msg_queue_handler: support various msg
  > 2. rename old msg_queue/msg_queue_handler to fixed_msg_queue/fixed_msg_queue_handler
  > 3. extract msg_queue_errno to header.

* **1.0.0 ~ 1.4.0**
  > Release log not write here, see commit log for more details. btw: suggest use latest code
  >    for example, ver 1.4.0 commit log url is https://github.com/wind0ws/libcutils/commits/1.4.0
  