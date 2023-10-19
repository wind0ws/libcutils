#include "config/lcu_build_config.h"

#if(defined(_WIN32) && _LCU_CFG_WIN_PTHREAD_MODE == LCU_WIN_PTHREAD_IMPLEMENT_MODE_SIMPLE)

#include "thread/pthread_win_simple/ptw32_implement.h"
#include "thread/pthread_win_simple/ptw32_mcs_lock.h"


int ptw32_concurrency = 0;

/* What features have been auto-detected */
int ptw32_features = 0;

/*
 * Global [process wide] thread sequence Number
 */
unsigned __int64 ptw32_threadSeqNumber = 0;

/*
 * Function pointer to QueueUserAPCEx if it exists, otherwise
 * it will be set at runtime to a substitute routine which cannot unblock
 * blocked threads.
 */
DWORD (*ptw32_register_cancellation) (PAPCFUNC, HANDLE, DWORD) = NULL;

/*
 * Global lock for managing pthread_t struct reuse.
 */
ptw32_mcs_lock_t ptw32_thread_reuse_lock = 0;

/*
 * Global lock for testing internal state of statically declared mutexes.
 */
ptw32_mcs_lock_t ptw32_mutex_test_init_lock = 0;

/*
 * Global lock for testing internal state of PTHREAD_COND_INITIALIZER
 * created condition variables.
 */
ptw32_mcs_lock_t ptw32_cond_test_init_lock = 0;

/*
 * Global lock for testing internal state of PTHREAD_RWLOCK_INITIALIZER
 * created read/write locks.
 */
ptw32_mcs_lock_t ptw32_rwlock_test_init_lock = 0;

/*
 * Global lock for testing internal state of PTHREAD_SPINLOCK_INITIALIZER
 * created spin locks.
 */
ptw32_mcs_lock_t ptw32_spinlock_test_init_lock = 0;

/*
 * Global lock for condition variable linked list. The list exists
 * to wake up CVs when a WM_TIMECHANGE message arrives. See
 * w32_TimeChangeHandler.c.
 */
ptw32_mcs_lock_t ptw32_cond_list_lock = 0;

#endif // _WIN32  && _LCU_CFG_WIN_PTHREAD_MODE == LCU_WIN_PTHREAD_IMPLEMENT_MODE_SIMPLE
