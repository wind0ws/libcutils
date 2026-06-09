#pragma once
#ifndef _LCU_H
#define _LCU_H

#ifdef __cplusplus
extern "C" {
#endif	

	/**
	 * get version of lcu
	 * note: do not free returned string 
	 */
	char* lcu_get_version();

	/**
	 * global init lcu
	 *
	 * call at the beginning of your app.
	 *
	 * @warning NOT thread-safe. Call exactly once at application startup,
	 *          from a single thread, before spawning any worker threads.
	 *          The internal refcount (g_init_times) uses a non-atomic
	 *          increment; concurrent calls can double-initialize subsystems
	 *          (e.g. time_util) and the 8-bit counter wraps after 255 calls.
	 *          If you need nested/concurrent init, serialize it yourself.
	 * @return 0 on success, non-zero if any subsystem init failed.
	 */
	int lcu_global_init();

	/**
	 * global cleanup lcu
	 *
	 * call at ending of your app,
	 * otherwise maybe some resource not released
	 *
	 * @warning NOT thread-safe, and must be paired 1:1 with lcu_global_init().
	 *          Call once at shutdown from a single thread, after all worker
	 *          threads that use lcu have stopped. See lcu_global_init() for
	 *          the refcount caveat.
	 */
	int lcu_global_cleanup();
	
#ifdef __cplusplus	
}
#endif

#endif // !_LCU_H
