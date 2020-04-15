#include "mplite.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common_macro.h"

/*
 ** A minimum allocation is an instance of the following structure.
 ** Larger allocations are an array of these structures where the
 ** size of the array is a power of 2.
 **
 ** The size of this object must be a power of two.  That fact is
 ** verified in mplite_init().
 */
typedef struct mplite_link mplite_link_t;

struct mplite_link {
    int next; /* Index of next free chunk */
    int prev; /* Index of previous free chunk */
};

/*
 ** Masks used for mplite_t.aCtrl[] elements.
 */
#define MPLITE_CTRL_LOGSIZE  0x1f    /* Log2 Size of this block */
#define MPLITE_CTRL_FREE     0x20    /* True if not checked out */

#if defined(_WIN32) && !defined(snprintf)
#define snprintf(buf, buf_size, format, ...) \
        _snprintf_s(buf, buf_size, (buf_size) - 1, format, ## __VA_ARGS__)
#endif /* #ifdef _WIN32 */

/*
 ** Assuming mplite_t.zPool is divided up into an array of mplite_link_t
 ** structures, return a pointer to the idx-th such link.
 */
#define mplite_getlink(handle, idx) ((mplite_link_t *)    \
        (&handle->zPool[(idx) * handle->szAtom]))

#define mplite_enter(handle)    if((handle != NULL) &&        \
        ((handle)->lock.acquire != NULL))                    \
        { (handle)->lock.acquire((handle)->lock.arg); }
#define mplite_leave(handle)    if((handle != NULL) &&        \
        ((handle)->lock.release != NULL))                    \
        { (handle)->lock.release((handle)->lock.arg); }

static int mplite_logarithm(const int iValue);
static int mplite_size(const mplite_t *handle, const void *p);
static void mplite_link(mplite_t *handle, const int i, const int iLogsize);
static void mplite_unlink(mplite_t *handle, const int i, const int iLogsize);
static void *mplite_malloc_unsafe(mplite_t *handle, const int nByte);
static void mplite_free_unsafe(mplite_t *handle, const void *pOld);

MPLITE_API int mplite_init(mplite_t *handle, const void *buf,
                           const int buf_size, const int min_alloc,
                           const mplite_lock_t *lock)
{
    int ii; /* Loop counter */
    int nByte; /* Number of bytes of memory available to this allocator */
    uint8_t *zByte; /* Memory usable by this allocator */
    int nMinLog; /* Log base 2 of minimum allocation size in bytes */
    int iOffset; /* An offset into handle->aCtrl[] */

    /* Check the parameters */
    if ((NULL == handle) || (NULL == buf) || (buf_size <= 0) ||
        (min_alloc <= 0)) {
        return MPLITE_ERR_INVPAR;
    }

    /* Initialize the mplite_t object */
    memset(handle, 0, sizeof (*handle));

    /* Copy the lock if it is not NULL */
    if (lock != NULL) {
        memcpy(&handle->lock, lock, sizeof (handle->lock));
    }

    /* The size of a mplite_link_t object must be a power of two.  Verify that
     ** this is case.
     */
    ASSERT_RET_VALUE((sizeof (mplite_link_t)&(sizeof (mplite_link_t) - 1)) == 0, -1);

    nByte = buf_size;
    zByte = (uint8_t*) buf;

    nMinLog = mplite_logarithm(min_alloc);
    handle->szAtom = (1 << nMinLog);
    while ((int) sizeof (mplite_link_t) > handle->szAtom) {
        handle->szAtom = handle->szAtom << 1;
    }

    handle->nBlock = (nByte / (handle->szAtom + sizeof (uint8_t)));
    handle->zPool = zByte;
    handle->aCtrl = (uint8_t *) & handle->zPool[handle->nBlock * handle->szAtom];

    for (ii = 0; ii <= MPLITE_LOGMAX; ii++) {
        handle->aiFreelist[ii] = -1;
    }

    iOffset = 0;
    for (ii = MPLITE_LOGMAX; ii >= 0; ii--) {
        int nAlloc = (1 << ii);
        if ((iOffset + nAlloc) <= handle->nBlock) {
            handle->aCtrl[iOffset] = (uint8_t) (ii | MPLITE_CTRL_FREE);
            mplite_link(handle, iOffset, ii);
            iOffset += nAlloc;
        }
        ASSERT_RET_VALUE((iOffset + nAlloc) > handle->nBlock, -2);
    }

    return MPLITE_OK;
}

MPLITE_API void *mplite_malloc(mplite_t *handle, const int nBytes)
{
    int64_t *p = 0;

    /* Check the parameters */
    if ((NULL == handle) || (nBytes <= 0)) {
        return NULL;
    }

    mplite_enter(handle);
    p = mplite_malloc_unsafe(handle, nBytes);
    mplite_leave(handle);

    return (void*) p;
}

MPLITE_API void mplite_free(mplite_t *handle, const void *pPrior)
{
    /* Check the parameters */
    if ((NULL == handle) || (NULL == pPrior)) {
        return;
    }

    mplite_enter(handle);
    mplite_free_unsafe(handle, pPrior);
    mplite_leave(handle);
}

MPLITE_API void *mplite_realloc(mplite_t *handle, const void *pPrior,
                                const int nBytes)
{
    int nOld;
    void *p;

    /* Check the parameters */
    if ((NULL == handle) || (NULL == pPrior) || (nBytes <= 0) ||
        (nBytes & (nBytes - 1))) {
        return NULL;
    }

    nOld = mplite_size(handle, pPrior);
    if (nBytes <= nOld) {
        return (void *) pPrior;
    }
    mplite_enter(handle);
    p = mplite_malloc_unsafe(handle, nBytes);
    if (p) {
        memcpy(p, pPrior, nOld);
        mplite_free_unsafe(handle, pPrior);
    }
    mplite_leave(handle);

    return p;
}

/* 
   like realloc but will not resize.
   for compatibility with MM::SetPtrSize
*/
MPLITE_API int mplite_resize(mplite_t *handle, const void *pPrior,
                             const int nBytes)
{
    int nOld;

    /* Check the parameters */
    if ((NULL == handle) || (NULL == pPrior) || (nBytes <= 0) ||
        (nBytes & (nBytes - 1))) {
        return MPLITE_ERR_INVPAR;
    }

    nOld = mplite_size(handle, pPrior);
    if (nBytes <= nOld) {
        return MPLITE_OK;
    }

    return MPLITE_ERR_INVPAR; /* not really, but ok. */
}

/* return the largest available block size */
MPLITE_API int mplite_maxmem(mplite_t *handle)
{
    unsigned i;

    if (NULL == handle) return 0;

    mplite_enter(handle);
    for (i = MPLITE_LOGMAX + 1; i; --i)
    {
        if (handle->aiFreelist[i - 1] != -1) break;
    }
    mplite_leave(handle);
    if (i) return (1 << (i - 1)) * handle->szAtom;
    else return 0;
}

/* return the total available memory */
MPLITE_API int mplite_freemem(mplite_t *handle)
{
    int total;
    int blocksize;
    int i;

    if (NULL == handle) return 0;

    total = 0;
    mplite_enter(handle);
    for (i = 0, blocksize = 1; i <= MPLITE_LOGMAX; ++i, blocksize <<= 1)
    {
        int index = handle->aiFreelist[i];

        while (index != -1)
        {
            mplite_link_t *link = mplite_getlink(handle, index);
            total += blocksize;

            index = link->next;
        }
    }
    mplite_leave(handle);

    return total * handle->szAtom;

    /* return handle->szAtom * handle->nBlock - handle->currentOut; */
}


MPLITE_API int mplite_roundup(mplite_t *handle, const int n)
{
    int iFullSz;

    /* Check the parameters */
    if ((NULL == handle) || (n > MPLITE_MAX_ALLOC_SIZE)) {
        return 0;
    }

    for (iFullSz = handle->szAtom; iFullSz < n; iFullSz *= 2);

    return iFullSz;
}

MPLITE_API void mplite_print_stats(const mplite_t * const handle,
                                   const mplite_putsfunc_t putsfunc)
{
    if ((handle == NULL) || (putsfunc == NULL)) {
        return;
    }
	char zStats[256];
	snprintf(zStats, sizeof(zStats), "Total number of calls to malloc: %u",
		(unsigned)handle->nAlloc);
	putsfunc(zStats);

	snprintf(zStats, sizeof(zStats), "Total of all malloc calls - includes "
		"internal fragmentation: %u", (unsigned)handle->totalAlloc);
	putsfunc(zStats);

	snprintf(zStats, sizeof(zStats), "Total internal fragmentation: %u",
		(unsigned)handle->totalExcess);
	putsfunc(zStats);

	snprintf(zStats, sizeof(zStats), "Current checkout, including internal "
		"fragmentation: %u", handle->currentOut);
	putsfunc(zStats);

	snprintf(zStats, sizeof(zStats), "Current number of distinct checkouts: %u",
		handle->currentCount);
	putsfunc(zStats);

	snprintf(zStats, sizeof(zStats), "Maximum instantaneous currentOut: %u",
		handle->maxOut);
	putsfunc(zStats);

	snprintf(zStats, sizeof(zStats), "Maximum instantaneous currentCount: %u",
		handle->maxCount);
	putsfunc(zStats);

	snprintf(zStats, sizeof(zStats), "Largest allocation (exclusive of "
		"internal frag): %u", handle->maxRequest);
	putsfunc(zStats);
}

/*
 ** Return the ceiling of the logarithm base 2 of iValue.
 **
 ** Examples:   mplite_logarithm(1) -> 0
 **             mplite_logarithm(2) -> 1
 **             mplite_logarithm(4) -> 2
 **             mplite_logarithm(5) -> 3
 **             mplite_logarithm(8) -> 3
 **             mplite_logarithm(9) -> 4
 */
static int mplite_logarithm(const int iValue)
{
    int iLog;
    for (iLog = 0; (1 << iLog) < iValue; iLog++);
    return iLog;
}

/*
 ** Return the size of an outstanding allocation, in bytes.
 ** This only works for chunks that are currently checked out.
 */
static int mplite_size(const mplite_t *handle, const void *p)
{
    int iSize, i;
    ASSERT_RET_VALUE( p!=0 , -1);
    i = (int)((uint8_t *) p - handle->zPool) / handle->szAtom;
    ASSERT_RET_VALUE(i >= 0 && i < handle->nBlock, -2);
    iSize = handle->szAtom * (1 << (handle->aCtrl[i] & MPLITE_CTRL_LOGSIZE));
    return iSize;
}

/*
 ** Link the chunk at handle->aPool[i] so that is on the iLogsize
 ** free list.
 */
static void mplite_link(mplite_t *handle, const int i, const int iLogsize)
{
    int x;
    ASSERT_RET_VOID(i >= 0 && i < handle->nBlock);
    ASSERT_RET_VOID(iLogsize >= 0 && iLogsize <= MPLITE_LOGMAX);
    ASSERT_RET_VOID((handle->aCtrl[i] & MPLITE_CTRL_LOGSIZE) == iLogsize);

    x = mplite_getlink(handle, i)->next = handle->aiFreelist[iLogsize];
    mplite_getlink(handle, i)->prev = -1;
    if (x >= 0) {
        ASSERT_RET_VOID(x < handle->nBlock);
        mplite_getlink(handle, x)->prev = i;
    }
    handle->aiFreelist[iLogsize] = i;
}

/*
 ** Unlink the chunk at handle->aPool[i] from list it is currently
 ** on.  It should be found on handle->aiFreelist[iLogsize].
 */
static void mplite_unlink(mplite_t *handle, const int i, const int iLogsize)
{
    int next, prev;
    ASSERT_RET_VOID(i >= 0 && i < handle->nBlock);
    ASSERT_RET_VOID(iLogsize >= 0 && iLogsize <= MPLITE_LOGMAX);
    ASSERT_RET_VOID((handle->aCtrl[i] & MPLITE_CTRL_LOGSIZE) == iLogsize);

    next = mplite_getlink(handle, i)->next;
    prev = mplite_getlink(handle, i)->prev;
    if (prev < 0) {
        handle->aiFreelist[iLogsize] = next;
    }
    else {
        mplite_getlink(handle, prev)->next = next;
    }
    if (next >= 0) {
        mplite_getlink(handle, next)->prev = prev;
    }
}


#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:26451)
#endif // _WIN32
/*
 ** Return a block of memory of at least nBytes in size.
 ** Return NULL if unable.  Return NULL if nBytes==0.
 **
 ** The caller guarantees that nByte is positive.
 **
 ** The caller has obtained a lock prior to invoking this
 ** routine so there is never any chance that two or more
 ** threads can be in this routine at the same time.
 */
static void *mplite_malloc_unsafe(mplite_t *handle, const int nByte)
{
    int i; /* Index of a handle->aPool[] slot */
    int iBin; /* Index into handle->aiFreelist[] */
    int iFullSz; /* Size of allocation rounded up to power of 2 */
    int iLogsize; /* Log2 of iFullSz/POW2_MIN */

    /* nByte must be a positive */
    ASSERT_RET_VALUE(nByte > 0, NULL);

    /* No more than 1GiB per allocation */
    if( nByte > MPLITE_MAX_ALLOC_SIZE ) return 0;

    /* Keep track of the maximum allocation request.  Even unfulfilled
     ** requests are counted */
    if ((uint32_t) nByte > handle->maxRequest) {
        handle->maxRequest = nByte;
    }

    /* Round nByte up to the next valid power of two */
    for (iFullSz = handle->szAtom, iLogsize = 0; iFullSz < nByte; iFullSz *= 2,
        iLogsize++) {
    }

    /* Make sure handle->aiFreelist[iLogsize] contains at least one free
     ** block.  If not, then split a block of the next larger power of
     ** two in order to create a new free block of size iLogsize.
     */
    for (iBin = iLogsize; iBin <= MPLITE_LOGMAX && handle->aiFreelist[iBin] < 0;
        iBin++) {
    }
    if (iBin > MPLITE_LOGMAX) {
        return NULL;
    }

    i = handle->aiFreelist[iBin];
    mplite_unlink(handle, i, iBin);

    while (iBin > iLogsize) {
        int newSize;

        iBin--;
        newSize = 1 << iBin;
        handle->aCtrl[i + newSize] = (uint8_t) (MPLITE_CTRL_FREE | iBin);
        mplite_link(handle, i + newSize, iBin);
    }
    handle->aCtrl[i] = (uint8_t) iLogsize;

    /* Update allocator performance statistics. */
    handle->nAlloc++;
    handle->totalAlloc += iFullSz;
    handle->totalExcess += iFullSz - nByte;
    handle->currentCount++;
    handle->currentOut += iFullSz;
    if (handle->maxCount < handle->currentCount) {
        handle->maxCount = handle->currentCount;
    }
    if (handle->maxOut < handle->currentOut) {
        handle->maxOut = handle->currentOut;
    }

#ifdef MPLITE_DEBUG
  /* Make sure the allocated memory does not assume that it is set to zero
  ** or retains a value from a previous allocation */
  memset(&handle->zPool[i * handle->szAtom], 0xAA, iFullSz);
#endif

    /* Return a pointer to the allocated memory. */
    return (void*) &handle->zPool[i * handle->szAtom];
}
#ifdef _WIN32
#pragma warning(pop)  
#endif // _WIN32

/*
 ** Free an outstanding memory allocation.
 */
static void mplite_free_unsafe(mplite_t *handle, const void *pOld)
{
    uint32_t size, iLogsize;
    int iBlock;

    /* Set iBlock to the index of the block pointed to by pOld in
     ** the array of handle->szAtom byte blocks pointed to by handle->zPool.
     */
    iBlock = (int)((uint8_t *) pOld - handle->zPool) / handle->szAtom;

    /* Check that the pointer pOld points to a valid, non-free block. */
    ASSERT_RET_VOID(iBlock >= 0 && iBlock < handle->nBlock);
    ASSERT_RET_VOID(((uint8_t *) pOld - handle->zPool) % handle->szAtom == 0);
    ASSERT_RET_VOID((handle->aCtrl[iBlock] & MPLITE_CTRL_FREE) == 0);

    iLogsize = handle->aCtrl[iBlock] & MPLITE_CTRL_LOGSIZE;
    size = 1 << iLogsize;
    ASSERT_RET_VOID(iBlock + size - 1 < (uint32_t) handle->nBlock);

    handle->aCtrl[iBlock] |= MPLITE_CTRL_FREE;
    handle->aCtrl[iBlock + size - 1] |= MPLITE_CTRL_FREE;
    ASSERT_RET_VOID(handle->currentCount > 0);
    ASSERT_RET_VOID(handle->currentOut >= (size * handle->szAtom));
    handle->currentCount--;
    handle->currentOut -= size * handle->szAtom;
    ASSERT_RET_VOID(handle->currentOut > 0 || handle->currentCount == 0);
    ASSERT_RET_VOID(handle->currentCount > 0 || handle->currentOut == 0);

    handle->aCtrl[iBlock] = (uint8_t) (MPLITE_CTRL_FREE | iLogsize);
    while (iLogsize < MPLITE_LOGMAX) {
        int iBuddy;
        if ((iBlock >> iLogsize) & 1) {
            iBuddy = iBlock - size;
            ASSERT_RET_VOID(iBuddy >= 0);
        }
        else {
            iBuddy = iBlock + size;
            if (iBuddy >= handle->nBlock) break;
        }
        if (handle->aCtrl[iBuddy] != (MPLITE_CTRL_FREE | iLogsize)) break;
        mplite_unlink(handle, iBuddy, iLogsize);
        iLogsize++;
        if (iBuddy < iBlock) {
            handle->aCtrl[iBuddy] = (uint8_t) (MPLITE_CTRL_FREE | iLogsize);
            handle->aCtrl[iBlock] = 0;
            iBlock = iBuddy;
        }
        else {
            handle->aCtrl[iBlock] = (uint8_t) (MPLITE_CTRL_FREE | iLogsize);
            handle->aCtrl[iBuddy] = 0;
        }
        size *= 2;
    }

#ifdef MPLITE_DEBUG
  /* Overwrite freed memory with the 0x55 bit pattern to verify that it is
  ** not used after being freed */
  memset(&handle->zPool[iBlock * handle->szAtom], 0x55, size);
#endif

    mplite_link(handle, iBlock, iLogsize);
}
