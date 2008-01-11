/*********************************************************************/
/*                                                                   */
/*  This Program Written By Paul Edwards.                            */
/*  Released to the public domain.                                   */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  memmgr - manage memory                                           */
/*                                                                   */
/*********************************************************************/

#include "__memmgr.h"

#ifdef __MEMMGR_INTEGRITY
#include <stdlib.h>
#endif

MEMMGR __memmgr;

void memmgrDefaults(MEMMGR *memmgr)
{
    return;
}

void memmgrInit(MEMMGR *memmgr)
{
    memmgr->start = NULL;
    memmgr->startf = NULL;
    return;
}

void memmgrTerm(MEMMGR *memmgr)
{
    return;
}

/* Supply a block of memory. We make sure this is inserted in
   the right spot logically in memory, and since it will be a
   free block, we stick it at the front of the list */
void memmgrSupply(MEMMGR *memmgr, void *buffer, size_t szbuf)
{
    MEMMGRN *p, *l, *b;

    if (((int)buffer % MEMMGR_ALIGN) != 0)
    {
        szbuf -= (MEMMGR_ALIGN - (int)buffer % MEMMGR_ALIGN);
        buffer = (char *)buffer + (MEMMGR_ALIGN - (int)buffer % MEMMGR_ALIGN);
    }

    if ((szbuf % MEMMGR_ALIGN) != 0)
    {
        szbuf -= szbuf % MEMMGR_ALIGN;
    }

    p = memmgr->start;
    l = NULL;
    while ((p != NULL) && ((MEMMGRN *)buffer >= p))
    {
        l = p;
        p = p->next;
    }

    b = (MEMMGRN *)buffer;

    b->prev = l;
    b->next = p;

    if (l != NULL)
    {
        l->next = b;
    }
    else
    {
        memmgr->start = b;
    }

    if (p != NULL)
    {
        p->prev = b;
    }

    b->fixed = 1;
    b->size = szbuf;
    b->allocated = 0;
    
    /* add this to the front of the list */
    b->nextf = memmgr->startf;
    if (b->nextf != NULL)
    {
        b->nextf->prevf = b;
    }
    b->prevf = NULL;
    memmgr->startf = b;
#ifdef __MEMMGR_INTEGRITY
    b->eyecheck1 = b->eyecheck2 = 0xa5a5a5a5;
    memmgrIntegrity(memmgr);
#endif
    return;
}

void *memmgrAllocate(MEMMGR *memmgr, size_t bytes, int id)
{
    MEMMGRN *p, *n;
    size_t oldbytes = bytes;

#ifdef __MEMMGR_INTEGRITY
    memmgrIntegrity(memmgr);
#endif
    if ((bytes % MEMMGR_ALIGN) != 0)
    {
        bytes += (MEMMGR_ALIGN - bytes % MEMMGR_ALIGN);
    }

    /* now that it is aligned, add the node size and make sure
    /* that it is a multiple of the minimum size */
    bytes += (MEMMGRN_SZ + MEMMGR_MINFREE - 1);

    /* if they have exceeded the limits of the data type,
       bail out now. */
    if (bytes < oldbytes)
    {
        return (NULL);
    }
    
    bytes &= ~(MEMMGRN_SZ + MEMMGR_MINFREE - 1);
    /* should have decent alignment now */

    p = memmgr->startf;

    while (p != NULL)
    {
        if (p->size >= bytes)
        {
            /* The free chain should never have something allocated.
               If it does, let's just crash so the user can get a
               call stack rather than have their data randomly
               corrupted. */
            if (p->allocated)
            {
#if MEMMGR_CRASH
                *(char *)0 = 0;
#endif
                return (NULL);
            }
            if ((p->size - bytes) >= (MEMMGRN_SZ + MEMMGR_MINFREE))
            {
                n = (MEMMGRN *)((char *)p + bytes);
                n->next = p->next;
                n->prev = p;
                p->next = n;
                n->fixed = 0;
                n->size = p->size - bytes;
                n->allocated = 0;
#ifdef __MEMMGR_INTEGRITY
                n->eyecheck1 = n->eyecheck2 = 0xa5a5a5a5;
#endif
                p->size = bytes;
                
                /* remove p this from the free chain and
                   replace with n */
                n->nextf = p->nextf;
                n->prevf = p->prevf;
                if (n->nextf != NULL)
                {
                    n->nextf->prevf = n;
                }
                if (n->prevf != NULL)
                {
                    n->prevf->nextf = n;
                }
                else
                {
                    memmgr->startf = n;
                }
            }
            /* otherwise we're not creating a new node, so just
               remove this entry from the free chain */
            else
            {
                if (p->nextf != NULL)
                {
                    p->nextf->prevf = p->prevf;
                }
                if (p->prevf != NULL)
                {
                    p->prevf->nextf = p->nextf;
                }
                /* if the previous entry is NULL, then we must be
                   the first in the queue. If we're not, crash */
                else if (memmgr->startf != p)
                {
#if MEMMGR_CRASH
                    *(char *)0 = 0;
#endif
                }
                else
                {
                    memmgr->startf = p->nextf;
                }
            }
            /* for safety, don't keep the old free pointer chain
               hanging around */
            p->nextf = NULL;
            p->prevf = NULL;

            p->allocated = 0x5a5a;
            p->id = id;
            break;
        }
        p = p->next;
    }
    if (p == NULL)
    {
        return (p);
    }
    else
    {
        size_t *q;

        q = (size_t *)((char *)p + MEMMGRN_SZ);
        *(q - 1) = oldbytes;
#ifdef __MEMMGR_INTEGRITY
        memmgrIntegrity(memmgr);
#endif
        return ((char *)p + MEMMGRN_SZ);
    }
}

void memmgrFree(MEMMGR *memmgr, void *ptr)
{
    MEMMGRN *p, *n, *l;
    int combprev = 0; /* did we combine with the previous? */
    int combnext = 0; /* are we combining with the next node? */

#ifdef __MEMMGR_INTEGRITY
    memmgrIntegrity(memmgr);
#endif
    p = (MEMMGRN *)((char *)ptr - MEMMGRN_SZ);

    /* If they try to free a bit of memory that isn't remotely
       what it's meant to be, just crash so that they get a
       call stack */
    if (p->allocated != 0x5a5a)
    {
#if MEMMGR_CRASH
        *(char *)0 = 0;
#endif
        return;
    }
    
    /* let's hope we're in the middle of a valid chain */
    l = p->prev;
    n = p->next;
    
    /* If the previous block is also free, just expand it's size 
       without any further fuss */
    if (!p->fixed && (l != NULL) && !l->allocated)
    {
        l->size += p->size;
        l->next = p->next;
        combprev = 1;
    }
    /* is the next one up combinable? */
    if ((n != NULL) && !n->allocated && !n->fixed)
    {
        combnext = 1;
    }
    
    /* We can have a fuss-free combination if the previous node
       was not combined */
    if (combnext && !combprev)
    {
        p->size += n->size;
        p->next = n->next;
        p->nextf = n->nextf;
        p->allocated = 0;
        if (p->nextf != NULL)
        {
            p->nextf->prevf = p;
        }
        p->prevf = n->prevf;
        if (p->prevf != NULL)
        {
            p->prevf->nextf = p;
        }
        else if (memmgr->startf != n)
        {
#if MEMMGR_CRASH
            *(char *)0 = 0;
#endif
            return;
        }
        else
        {
            memmgr->startf = p;
        }
    }
    
    /* this is the hairy situation. We're combining two existing
       free blocks into one. While the blocks themselves are
       contiguous, the two components are at random spots in the
       free memory chain, e.g. they might be B and E in
       A <-> B <-> C <-> D <-> E <-> F
       So what's the obvious thing to do? Give it up and become a
       Buddhist monk! The less obvious thing is to keep B in its
       spot, just with an enhanced size, then get D and F to link 
       together. The special case of the two nodes actually already
       being linked together by happy coincidence doesn't need
       special handling. If it does, that monastery looks more
       and more appealing every day. Do you reckon Buddhist monks
       talk about giving it all up and doing C programming? Once
       the node E is eliminated, B can be expanded. */

    else if (combnext && combprev)
    {
        if (n->nextf != NULL)
        {
            n->nextf->prevf = n->prevf;
        }
        if (n->prevf != NULL)
        {
            n->prevf->nextf = n->nextf;
        }
        else if (memmgr->startf != n)
        {
#if MEMMGR_CRASH
            *(char *)0 = 0;
#endif
            return;
        }
        else
        {
            memmgr->startf = n->nextf;
        }

        /* Ok, the free memory has been taken care of, now we go
           back to the newly combined node and combine it with
           this one. */        
        l->size += n->size;
        l->next = n->next;
        
        /* That wasn't so hairy after all */
    }

#ifdef __MEMMGR_INTEGRITY
    memmgrIntegrity(memmgr);
#endif
    return;
}

void memmgrFreeId(MEMMGR *memmgr, int id)
{
    MEMMGRN *p, *l;

    p = memmgr->start;
    l = NULL;

#ifdef __MEMMGR_INTEGRITY
    memmgrIntegrity(memmgr);
#endif
    while (p != NULL)
    {
        if ((p->id == id) && p->allocated)
        {
            /* skip past the MEMMGRN */
            memmgrFree(memmgr, p + 1);

            /* It is possible that the p node has been invalidated
               now, because of combination with the previous node.
               So we go back to the previous pointer and try again.
               This time it shouldn't find the node allocated. */
            if (l != NULL)
            {
                p = l;
            }
        }
        l = p;
        p = p->next;
    }
#ifdef __MEMMGR_INTEGRITY
    memmgrIntegrity(memmgr);
#endif
    return;
}

/* find the largest block of memory available */
size_t memmgrMaxSize(MEMMGR *memmgr)
{
    MEMMGRN *p;
    size_t max = 0;

    p = memmgr->startf;

    while (p != NULL)
    {
        if (p->size > max)
        {
            max = p->size;
        }
        p = p->next;
    }
    if (max != 0)
    {
        max -= MEMMGRN_SZ;
    }
    return (max);
}


#ifdef __MEMMGR_INTEGRITY
/* do an integrity check */
void memmgrIntegrity(MEMMGR *memmgr)
{
    MEMMGRN *p;
    size_t max = 0;

    p = memmgr->start;

    while (p != NULL)
    {
        if ((p->eyecheck1 != 0xa5a5a5a5) || (p->eyecheck2 != 0xa5a5a5a5))
        {
            *(char *)0 = '\0'; /* try to invoke crash */
            exit(EXIT_FAILURE);
        }
        p = p->next;
    }
    return;
}
#endif

/* resize a memory block */
/* note that the size in the control block is the
   size of available data plus the control block */
/* note that we don't currently see if the current buffer actually
   has enough room for their new data size, nor do we see if the next node
   is free. We should, but we don't yet. */
int memmgrRealloc(MEMMGR *memmgr, void *ptr, size_t newsize)
{
    MEMMGRN *p, *n, *z;
    size_t oldbytes = newsize;

#ifdef __MEMMGR_INTEGRITY
    memmgrIntegrity(memmgr);
#endif
    if ((newsize % MEMMGR_ALIGN) != 0)
    {
        newsize += (MEMMGR_ALIGN - newsize % MEMMGR_ALIGN);
    }
    newsize += MEMMGRN_SZ;

    /* if they have exceeded the limits of the data type,
       bail out now. */
    if (newsize < oldbytes)
    {
        return (-1);
    }


    p = (MEMMGRN *)((char *)ptr - MEMMGRN_SZ);

    /* If they try to manipulate a bit of memory that isn't remotely
       what it's meant to be, just crash so that they get a
       call stack */
    if (p->allocated != 0x5a5a)
    {
#if MEMMGR_CRASH
        *(char *)0 = 0;
#endif
        return (-1);
    }
    
    /* let's hope we're in the middle of a valid chain */

    /* don't allow reduction in size unless
       it is by a large enough amount */
    if (p->size < (newsize + MEMMGRN_SZ + MEMMGR_MINFREE))
    {
        return (-1);
    }

    /* insert new control block */
    n = (MEMMGRN *)((char *)p + MEMMGRN_SZ + newsize);
    n->next = p->next;
    n->prev = p;
    p->next = n;
    n->fixed = 0;
    n->size = p->size - newsize - MEMMGRN_SZ;
    n->allocated = 0;
#ifdef __MEMMGR_INTEGRITY
    n->eyecheck1 = n->eyecheck2 = 0xa5a5a5a5;
#endif
    p->size = newsize;

    /* combine with next block if possible */
    z = n->next;
    if ((z != NULL) && !z->allocated && !z->fixed)
    {
        n->size += z->size;
        n->next = z->next;
        n->nextf = z->nextf;
        if (n->nextf != NULL)
        {
            n->nextf->prevf = n;
        }
        n->prevf = z->prevf;
        if (n->prevf != NULL)
        {
            n->prevf->nextf = n;
        }
        else if (memmgr->startf != z)
        {
#if MEMMGR_CRASH
            *(char *)0 = 0;
#endif
            return (-1);
        }
        else
        {
            memmgr->startf = n;
        }
    }


#ifdef __MEMMGR_INTEGRITY
    memmgrIntegrity(memmgr);
#endif

    /* Keep track of the new size */
    {
        size_t *q;

        q = (size_t *)((char *)p + MEMMGRN_SZ);
        *(q - 1) = oldbytes;
    }
    return (0);
}
