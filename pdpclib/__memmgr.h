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
    size_t filler; /* we add this so that *(p - size_t) is writable */
} MEMMGRN;

typedef struct {
    MEMMGRN *start;
} MEMMGR;

#define MEMMGR_ALIGN 16

#define MEMMGRN_SZ \
  ((sizeof(MEMMGRN) % MEMMGR_ALIGN == 0) ? \
   sizeof(MEMMGRN) : \
   ((sizeof(MEMMGRN) / MEMMGR_ALIGN + 1) * 16))

#define memmgrDefaults __mmDef
#define memmgrInit __mmInit
#define memmgrTerm __mmTerm
#define memmgrSupply __mmSupply
#define memmgrAllocate __mmAlloc
#define memmgrFree __mmFree
#define memmgrFreeId __mmFId
#define memmgrMaxSize __mmMaxSize
#define memmgrRealloc __mmRealloc

void memmgrDefaults(MEMMGR *memmgr);
void memmgrInit(MEMMGR *memmgr);
void memmgrTerm(MEMMGR *memmgr);
void memmgrSupply(MEMMGR *memmgr, void *buffer, size_t szbuf);
void *memmgrAllocate(MEMMGR *memmgr, size_t bytes, int id);
void memmgrFree(MEMMGR *memmgr, void *ptr);
void memmgrFreeId(MEMMGR *memmgr, int id);
size_t memmgrMaxSize(MEMMGR *memmgr);
int memmgrRealloc(MEMMGR *memmgr, void *ptr, size_t newsize);

extern MEMMGR __memmgr;

#endif
