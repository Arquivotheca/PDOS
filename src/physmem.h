/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  physmem.h - header file for Physical Memory Management           */
/*                                                                   */
/*********************************************************************/

#ifndef PHYSMEM_INCLUDED
#define PHYSMEM_INCLUDED

#define PAGE_FRAME_SIZE 0x1000

typedef struct {
    unsigned char pages[512]; /* Bitmap for the first 16 MB. 1 = free. */
} PHYSMEMMGR;

void physmemmgrInit(PHYSMEMMGR *physmemmgr);
void physmemmgrSupply(PHYSMEMMGR *physmemmgr,
                      unsigned long addr,
                      unsigned long size);
void *physmemmgrAllocPageFrame(PHYSMEMMGR *physmemmgr);
void physmemmgrFreePageFrame(PHYSMEMMGR *physmemmgr, void *addr);

#endif /* PHYSMEM_INCLUDED */
