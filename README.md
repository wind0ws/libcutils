# Common C Utils
  What's this?  This is a common c utils library (works for unix/android/windows).

## Components
* **log**
  > print log to file/console/logcat(if on Android) at the same time, of course you can config it.

* **thread**
  > posix style thread/semaphore api on windows(thanks for [pthread-win32](https://sourceforge.net/projects/pthreads4w/)). also works on unix(linux/Android).

* **memory**
   *  **string** : include ***stringbuilder***, ***strlcpy**, ***strlcat***, ***strreplace***, ***strsplit***, ***strtrim***, ***strutf8len***, ***strnutf8len***
   *  **allocator** : can trace heap memory, help you find memory leak or memory corruption.
   *  **mplite** : A zero-malloc memory pool based on SQLite's memsys5 memory subsystem

* **data**
  > data structure : include ***array***, ***hashmap***, ***list***, ***base64*** ...

* **ring**
  > ringbuffer: include ***ringbuffer***, ***ring_msg_queue***, ***msg_queue_handler***, ***autocover_buffer*** ...

* **time**
  > time_util: include ***current_milliseconds***, ***fast_second2date*** ...

## How to build
  > go to ***build***  folder, and edit bat or sh first(to make sure cmake/NDK(if build for Android) location), then double click bat or sh to build it, it will copy the compiled result to the specified location

## How to use
  > just copy header and lib(static or shared) to your project, and link it.
  