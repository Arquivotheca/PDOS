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

/* pload is a standalone loader, and thus is normally loaded
   starting at location 0. */
#define PLOAD_START 0x0

/* when pload is directly IPLed, and thus starts from the
   address in location 0, it knows to create its own stack,
   which it does at 0.5 MB in (thus creating a restriction
   of only being 0.5 MB in size unless this is changed) */
/* This is governed by the code in sapstart, so these constants
   need to match that */
#define PLOAD_STACK (PLOAD_START + 0x080000)

/* the heap - for the equivalent of getmains - is located
   another 0.5 MB in, ie at the 1 MB location */
#define PLOAD_HEAP (PLOAD_STACK + 0x080000)

/* PDOS is loaded another 1 MB above the PLOAD heap - ie the 2 MB location */
#define PDOS_CODE (PLOAD_HEAP + 0x100000)

/* standard offset for a standalone PDPCLIB program that doesn't need
   to load itself, since it wasn't directly IPLed */
#define PDOS_ENTRY (PDOS_CODE + 0x800)

/* The heap starts 1 MB after the code (ie the 3 MB location).
   So PDOS can't be more than 1 MB in size unless this is changed */
#define PDOS_HEAP (PDOS_CODE + 0x100000)

static int ipldev;

static int findFile(char *dsn, int *c, int *h, int *r);

int main(int argc, char **argv)
{
    char *load;
    char *start = (char *)PDOS_CODE;
    void (*entry)(void *) = (void (*)(void *))PDOS_ENTRY;
    int cyl;
    int head;
    int rec;
    /* programs using the standalone version of PDPCLIB need
       to be informed where they can allocate a heap and
       potentially other parameters (that's why there is a
       length indicator as well. dum should be 0 to indicate
       no normal MVS-style parameters */
    struct { int dum;
             int len;
             char *heap; } pblock = { 0, 4, (char *)PDOS_HEAP };
    void (*fun)(void *);
    int x;
    int cnt;

    ipldev = initsys();
#if 0
    printf("IPL device is %x\n", ipldev);
#endif
    if (findFile("PDOS.SYS", &cyl, &head, &rec) != 0)
    {
        printf("PDOS.SYS not found\n");
        return (12);
    }
#if 0
    printf("PDOS resides on cylinder %d, head %d, rec %d of IPL device\n",
        cyl, head, rec);
#endif
    load = start;
    for (x = 0; x < 30; x++)
    {
#if 0
        printf("loading to %p from %d, %d, %d\n", load, cyl, head, rec);
#endif
        cnt = rdblock(ipldev, cyl, head, rec, load, CHUNKSZ);
        load += CHUNKSZ;
        rec++;
        if (rec >= 4)
        {
            rec = 1;
            head++;
        }
        if (head >= 15)
        {
            head = 0;
            cyl++;
        }
        if (cnt == 0) break; /* reached EOF */
    }
    entry(&pblock);
#if 0
    printf("about to read first block of PDOS\n");
    rdblock(ipldev, cyl, head, rec, buf, sizeof buf);
    printf("got %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3]);
    printf("and %x %x %x %x\n", buf[4], buf[5], buf[6], buf[7]);
#endif
    return (0);
}


/* find a file on disk */

#define MAXBLKSZ 32767

static int findFile(char *dsn, int *c, int *h, int *r)
{
    char *raw;
    char *initial;
    char *load;
    /* Standard C programs can start at a predictable offset */
    int (*entry)(void *);
    int cyl;
    int head;
    int rec;
    static int savearea[20]; /* needs to be in user space */
    static char *pptrs[1];
    char tbuf[MAXBLKSZ];
    char srchdsn[FILENAME_MAX+10]; /* give an extra space */
    int cnt = -1;
    int lastcnt = 0;
    int ret = 0;
    struct {
        char ds1dsnam[44];
        char ds1fmtid;
        char unused1[60];
        char unused2;
        char unused3;
        char startcchh[4];
        char endcchh[4];
    } dscb1;
    int len;

    if (memchr(dsn, '\0', FILENAME_MAX) == NULL)
    {
        len = FILENAME_MAX;
    }
    else
    {
        len = strlen(dsn);
    }
    memcpy(srchdsn, dsn, len);
    strcpy(srchdsn + len, " ");
    len++; /* force a search for the blank */
    
    /* read VOL1 record which starts on cylinder 0, head 0, record 3 */
    cnt = rdblock(ipldev, 0, 0, 3, tbuf, MAXBLKSZ);
    if (cnt >= 20)
    {
        cyl = head = rec = 0;
        /* +++ probably time to create some macros for this */
        memcpy((char *)&cyl + sizeof(int) - 2, tbuf + 15, 2);
        memcpy((char *)&head + sizeof(int) - 2, tbuf + 17, 2);
        memcpy((char *)&rec + sizeof(int) - 1, tbuf + 19, 1);
        
        while ((cnt =
               rdblock(ipldev, cyl, head, rec, &dscb1, sizeof dscb1))
               > 0)
        {
            if (cnt >= sizeof dscb1)
            {
                if (dscb1.ds1fmtid == '1')
                {
                    dscb1.ds1fmtid = ' '; /* for easy comparison */
                    if (memcmp(dscb1.ds1dsnam,
                               srchdsn,
                               len) == 0)
                    {
                        cyl = head = 0;
                        rec = 1;
                        /* +++ more macros needed here */
                        memcpy((char *)&cyl + sizeof(int) - 2, 
                               dscb1.startcchh, 2);
                        memcpy((char *)&head + sizeof(int) - 2,
                               dscb1.startcchh + 2, 2);
                        break;
                    }
                }
            }
            rec++;
        }        
    }
    
    if (cnt <= 0)
    {
        /* not found */
        return (-1);
    }
    *c = cyl;
    *h = head;
    *r = rec;
    return (0);
}
