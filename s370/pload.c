/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pload.c - Load PDOS/370                                          */
/*                                                                   */
/*  When this (main) program is entered, interrupts are disabled,    */
/*  and this program needs to find the PDOS program and load it into */
/*  memory, and transfer control, again with interrupts disabled.    */
/*                                                                   */
/*  The way it does this is that when doing an I/O, it only          */
/*  enables the I/O then and there, and waits on it to complete,     */
/*  at which point it is disabled again.                             */
/*                                                                   */
/*  The operating system itself won't be able to get away with this  */
/*  simplistic logic because it needs to deal with timer interrupts  */
/*  etc.                                                             */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static char buf[40000];

#define CHUNKSZ 18452

#define PDOS_CODE 0x200000
#define PDOS_HEAP 0x300000

int main(int argc, char **argv)
{
    int dev;
    char *load;
    char *start = (char *)PDOS_CODE;
    void (*entry)(void *) = (void (*)(void *))0x200800;
    int i;
    int j;
    struct { int dum;
             int len;
             char *heap; } pblock = { 0, 4, (char *)PDOS_HEAP };
    void (*fun)(void *);

    printf("PDOS should reside on cylinder 1, head 0 of IPL device\n");
    dev = initsys();
    printf("IPL device is %x\n", dev);
    load = start;
    for (i = 0; i < 10; i++)
    {
        for (j = 1; j < 4; j++)
        {
#if 0
            printf("loading to %p from 1, %d, %d\n", load, i, j);
#endif
            rdblock(dev, 1, i, j, load, CHUNKSZ);
            load += CHUNKSZ;
        }
    }
    entry(&pblock);
#if 0
    printf("about to read first block of PDOS\n");
    rdblock(dev, 1, 0, 1, buf, sizeof buf);
    printf("got %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3]);
    printf("and %x %x %x %x\n", buf[4], buf[5], buf[6], buf[7]);
#endif
    return (0);
}
