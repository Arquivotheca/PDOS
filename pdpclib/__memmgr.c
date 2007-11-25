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

MEMMGR __memmgr;

void memmgrDefaults(MEMMGR *memmgr)
{
    return;
}

void memmgrInit(MEMMGR *memmgr)
{
    memmgr->start = NULL;
    return;
}

void memmgrTerm(MEMMGR *memmgr)
{
    return;
}

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
    b->size = szbuf - MEMMGRN_SZ;
    b->allocated = 0;
    return;
}

void *memmgrAllocate(MEMMGR *memmgr, size_t bytes, int id)
{
    MEMMGRN *p, *n;
    size_t oldbytes = bytes;
    
    if ((bytes % MEMMGR_ALIGN) != 0)
    {
        bytes += (MEMMGR_ALIGN - bytes % MEMMGR_ALIGN);
    }

    /* if they have exceeded the limits of the data type,
       bail out now. */    
    if (bytes < oldbytes)
    {
        return (NULL);
    }
    
    p = memmgr->start;
    
    while (p != NULL)
    {
        if ((p->size >= bytes) && !p->allocated)
        {
            if ((p->size - bytes) > MEMMGRN_SZ)
            {
                n = (MEMMGRN *)((char *)p + MEMMGRN_SZ + bytes);
                n->next = p->next;
                n->prev = p;
                p->next = n;
                n->fixed = 0;
                n->size = p->size - bytes - MEMMGRN_SZ;
                n->allocated = 0;
                p->size = bytes;
            }
            p->allocated = 1;
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
        return ((char *)p + MEMMGRN_SZ);
    }
}

void memmgrFree(MEMMGR *memmgr, void *ptr)
{
    MEMMGRN *p, *n, *l;

    ptr = (char *)ptr - MEMMGRN_SZ;
    
    p = memmgr->start;
    l = NULL;
    
    while (p != NULL)
    {
        if (p == ptr)
        {
            p->allocated = 0;
            
            n = p->next;
            if ((n != NULL) && !n->allocated && !n->fixed)
            {
                p->size += n->size + MEMMGRN_SZ;
                p->next = n->next;
            }
            
            if (!p->fixed && (l != NULL) && !l->allocated)
            {
                l->size += p->size + MEMMGRN_SZ;
                l->next = p->next;
            }
            break;
        }
        l = p;
        p = p->next;
    }
    return;
}

void memmgrFreeId(MEMMGR *memmgr, int id)
{
    MEMMGRN *p, *n, *l;

    p = memmgr->start;
    l = NULL;
    
    while (p != NULL)
    {
        if ((p->id == id) && p->allocated)
        {
            p->allocated = 0;
            
            n = p->next;
            if ((n != NULL) && !n->allocated && !n->fixed)
            {
                p->size += n->size + MEMMGRN_SZ;
                p->next = n->next;
            }
            
            if (!p->fixed && (l != NULL) && !l->allocated)
            {
                l->size += p->size + MEMMGRN_SZ;
                l->next = p->next;
            }
        }
        l = p;
        p = p->next;
    }
    return;
}

/* find the largest block of memory available */
size_t memmgrMaxSize(MEMMGR *memmgr)
{
    MEMMGRN *p, *n;
    size_t max = 0;
    
    p = memmgr->start;
    
    while (p != NULL)
    {
        if ((p->size > max) && !p->allocated)
        {
            max = p->size;
        }
        p = p->next;
    }
    return (max);
}

/* resize a memory block */
/* note that the size in the control block is the
   size available for data */
int memmgrRealloc(MEMMGR *memmgr, void *ptr, size_t newsize)
{
    MEMMGRN *p, *n, *z;
    size_t oldbytes = newsize;
    
    if ((newsize % MEMMGR_ALIGN) != 0)
    {
        newsize += (MEMMGR_ALIGN - newsize % MEMMGR_ALIGN);
    }

    /* if they have exceeded the limits of the data type,
       bail out now. */    
    if (newsize < oldbytes)
    {
        return (-1);
    }
    

    ptr = (char *)ptr - MEMMGRN_SZ;
    
    p = memmgr->start;
    
    while (p != NULL)
    {
        if (p == ptr)
        {
            /* don't allow reduction in size unless
               it is by a large enough amount */
            if (p->size < (newsize + MEMMGRN_SZ))
            {
                return (-1);
            }
            
            /* handle overflow condition */
            if ((newsize + MEMMGRN_SZ) < newsize)
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
            p->size = newsize;
            
            /* combine with next block if possible */
            z = n->next;
            if ((z != NULL) && !z->allocated && !z->fixed)
            {
                n->size += z->size + MEMMGRN_SZ;
                n->next = z->next;
            }
            
            break;
        }
        p = p->next;
    }
    
    /* if we exhausted list, they passed a dud pointer */
    if (p == NULL)
    {
        return (-2);
    }
    return (0);
}

