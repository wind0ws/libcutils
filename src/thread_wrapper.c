//
// Created by Admin on 2020/2/12.
//
#ifdef _WIN32

#ifdef __cplusplus
extern "C"
{
#endif// __cplusplus

#include "thread_wrapper.h"

    unsigned int pthread_id(pthread_t *pth) 
    {
#ifdef WIN32
        return (unsigned int)(pth->mThreadID);
#else
        return (unsigned int)(*pth);
#endif // WIN32
    }

#ifdef __cplusplus
};
#endif // __cplusplus

#endif /* #ifdef _WIN32 */
