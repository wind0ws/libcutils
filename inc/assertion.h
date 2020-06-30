#pragma once
#ifndef __LCU_ASSERTION_H__
#define __LCU_ASSERTION_H__
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ASSERT_RET_VOID
#define ASSERT_RET_VOID(condition) if((condition) == false) { assert(false); return; }
#endif // !ASSERT_VOID
#ifndef ASSERT_RET_VALUE
#define ASSERT_RET_VALUE(condition, ret) if((condition) == false) { assert(false); return ret; }
#endif // !ASSERT_RETURN

#ifdef __cplusplus
};
#endif

#endif //__LCU_ASSERTION_H__