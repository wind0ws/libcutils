# Common C Utils
 What this?  This is a common c utils library (works for unix/android/windows).

## Components
* **log**
> print log to file, console, logcat(if on Android) at the same time, of course you can config it.

*  **thread**
> posix style thread/semaphore api on windows. both work on windows/unix(linux).

* **memory**
>*  **string** : include **stringbuilder**, **strlcpy**, **strlcat**, **strreplace**, **strsplit**.
 *   **allocator** : can trace heap memory, help you find memory leak.
 *  **mplite** : A zero-malloc memory pool based on SQLite's memsys5 memory subsystem

* **data**
> data structure : include **array**, **hashmap**, **list**

* **ring**
> ringbuffer: include **ringbuffer**, **ring_msg_queue**

## How to build
> go to ***build***  folder, and edit bat or sh first(to make sure cmake and NDK location), then double click bat or sh to build it.

## How to use
> just copy header and lib to your project, and link it.