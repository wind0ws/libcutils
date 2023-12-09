#pragma once
#ifndef PTW32_MCS_LOCK_H
#define PTW32_MCS_LOCK_H

#include "config/lcu_build_config.h"

//do not include this header directly, you should include "posix_thread.h" instead!
#if(defined(_WIN32) && _LCU_CFG_WIN_PTHREAD_MODE == LCU_WIN_PTHREAD_IMPLEMENT_MODE_SIMPLE)
#pragma warning(push)
#pragma warning(disable: 5105)
#include <windows.h>
#pragma warning(pop)

/* MSVC 7.1 doesn't like complex #if expressions */
#define INLINE
#if defined(PTW32_BUILD_INLINED)
#  if defined(HAVE_C_INLINE) || defined(__cplusplus)
#    undef INLINE
#    define INLINE inline
#  endif
#endif

/*
 * MCS lock queue node - see ptw32_MCS_lock.c
 */
struct ptw32_mcs_node_t_
{
  struct ptw32_mcs_node_t_ **lock;        /* ptr to tail of queue */
  struct ptw32_mcs_node_t_  *next;        /* ptr to successor in queue */
  HANDLE                     readyFlag;   /* set after lock is released by
                                             predecessor */
  HANDLE                     nextFlag;    /* set after 'next' ptr is set by
                                             successor */
};

typedef struct ptw32_mcs_node_t_     ptw32_mcs_local_node_t;
typedef struct ptw32_mcs_node_t_*    ptw32_mcs_lock_t;

#ifdef __cplusplus
extern "C" {
#endif

void ptw32_mcs_flag_set (HANDLE * flag);

void ptw32_mcs_flag_wait (HANDLE * flag);

void ptw32_mcs_lock_acquire (ptw32_mcs_lock_t * lock, ptw32_mcs_local_node_t * node);

void ptw32_mcs_lock_release (ptw32_mcs_local_node_t * node);

int ptw32_mcs_lock_try_acquire (ptw32_mcs_lock_t * lock, ptw32_mcs_local_node_t * node);

void ptw32_mcs_node_transfer (ptw32_mcs_local_node_t * new_node, ptw32_mcs_local_node_t * old_node);

#ifdef __cplusplus
};
#endif

#endif // _WIN32 && LCU_WIN_PTHREAD_IMPLEMENT_MODE_SIMPLE

#endif // !PTW32_MCS_LOCK_H
