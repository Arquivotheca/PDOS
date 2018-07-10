/*********************************************************************/
/*                                                                   */
/*  This Program Written By Paul Edwards.                            */
/*  Released to the public domain.                                   */
/*                                                                   */
/*********************************************************************/

#ifndef MEMMGR_INCLUDED
#define MEMMGR_INCLUDED

#include <stddef.h>

typedef struct memmgrn {
    struct memmgrn *next;
    struct memmgrn *prev;
    int fixed;
    size_t size;
    int allocated;
    int id;
} MEMMGRN;

typedef struct {
    MEMMGRN *start;
} MEMMGR;

/* Used to report memory manage statistics */
typedef struct {
    /* Size of largest free block */
    size_t maxFree;
    /* Size of largest allocated block */
    size_t maxAllocated;
    /* Sum of sizes of all free blocks */
    size_t totalFree;
    /* Sum of sizes of all allocated blocks */
    size_t totalAllocated;
    /* Number of free blocks */
    long countFree;
    /* Number of allocated blocks */
    long countAllocated;
} MEMMGRSTATS;

#define MEMMGR_ALIGN 16

#define MEMMGRN_SZ \
  ((sizeof(MEMMGRN) % MEMMGR_ALIGN == 0) ? \
   sizeof(MEMMGRN) : \
   ((sizeof(MEMMGRN) / MEMMGR_ALIGN + 1) * 16))
   
void memmgrDefaults(MEMMGR *memmgr);
void memmgrInit(MEMMGR *memmgr);
void memmgrTerm(MEMMGR *memmgr);
void memmgrSupply(MEMMGR *memmgr, void *buffer, size_t szbuf);
void *memmgrAllocate(MEMMGR *memmgr, size_t bytes, int id);
void memmgrFree(MEMMGR *memmgr, void *ptr);
void memmgrFreeId(MEMMGR *memmgr, int id);
size_t memmgrMaxSize(MEMMGR *memmgr);
size_t memmgrGetSize(MEMMGR *memmgr, void *ptr);
int memmgrRealloc(MEMMGR *memmgr, void *ptr, size_t newsize);
void memmgrGetStats(MEMMGR *memmgr, MEMMGRSTATS *stats);

#endif
