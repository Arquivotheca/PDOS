/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  physmem.c - Physical Memory Management                           */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "physmem.h"

#define BITMAP_OFFSET(addr) ((addr) / PAGE_FRAME_SIZE / 8)

void physmemmgrInit(PHYSMEMMGR *physmemmgr)
{
    /* Marks all physical memory as unavailable. */
    memset(physmemmgr->pages, 0, 512);
}

void physmemmgrSupply(PHYSMEMMGR *physmemmgr,
                      unsigned long addr,
                      unsigned long size)
{
    unsigned long new_addr;
    
    if (addr & 0xfff)
    {
        /* Aligns the address on the 4 KB boundary
         * and decreases size accordingly. */
        new_addr = (addr & 0xfffff000UL) + PAGE_FRAME_SIZE;
        size -= new_addr - addr;
        addr = new_addr;
    }
    if (addr > 512 * 8 * PAGE_FRAME_SIZE)
    {
        printf("PHYSMEMMGR does not support more than %u bytes of memory\n",
               512 * 8 * PAGE_FRAME_SIZE);
        return;
    }
    if ((addr + size) / (PAGE_FRAME_SIZE * 8) > 512)
    {
        /* Reduces size provided so physmmemgr can handle it. */
        size = 512 * 8 * PAGE_FRAME_SIZE - addr;
    }
    if (addr & 0x7000)
    {
        /* The address is not on 8 * PAGE_FRAME_SIZE,
         * so only a part of a byte should be set. */
        physmemmgr->pages[BITMAP_OFFSET(addr)] |= ((0xff
                                                    << ((addr & 0x7000) >> 12))
                                                   & 0xff);
        new_addr = (addr & 0xffff8000UL) + 8 * PAGE_FRAME_SIZE;
        size -= new_addr - addr;
        addr = new_addr;
    }
    memset(physmemmgr->pages + BITMAP_OFFSET(addr),
           0xff,
           BITMAP_OFFSET(size));
    if (size & 0x7000)
    {
        /* Some memory remains that is less than 8 page frames. */
        addr = addr + (size & 0xffff8000);
        physmemmgr->pages[BITMAP_OFFSET(addr)] |= ((0xff
                                                    >> (8 - ((size & 0x7000)
                                                             >> 12))));
    }
}

void *physmemmgrAllocPageFrame(PHYSMEMMGR *physmemmgr)
{
    int i;

    for (i = 0; i < 512; i++)
    {
        if (physmemmgr->pages[i])
        {
            int x;

            for (x = 0; !(physmemmgr->pages[i] & (1U << x)); x++);
            physmemmgr->pages[i] &= 0xff ^ (1U << x);

            return ((void *)((i * 8 + x) * PAGE_FRAME_SIZE));
        }
    }

    printf("(PHYSMEMMGR) Out of memory\n");
    return (0);
}

void physmemmgrFreePageFrame(PHYSMEMMGR *physmemmgr, void *addr)
{
    unsigned long offset = BITMAP_OFFSET((unsigned long)addr);

    physmemmgr->pages[offset] |= 1U << ((((unsigned long)addr) >> 12) & 0x7);
}
