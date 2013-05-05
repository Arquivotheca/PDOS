/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  raw2file - copy data from a specific place on the hard disk      */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "bos.h"

/* start sector is set to 0x800 - 1 MB */
/* this shows up as 0x7c1 relative to the partition start */
#define STARTSECT 0x800

static int readLBA(void *buf, 
                   int sectors, 
                   int drive, 
                   unsigned long sector);

int main(int argc, char **argv)
{
    long sect;
    FILE *fq;
    static char buf[512];
    long numsect;
    int drive;

    if (argc <= 3)
    {
        printf("usage: raw2file <drive> <numsect> <output file>\n");
        printf("e.g. raw2file 82 5000 temp.zip\n");
        return (EXIT_FAILURE);
    }
    fq = fopen(*(argv + 3), "wb");
    if (fq == NULL)
    {
        printf("failed to open %s for writing\n", *(argv + 3));
        return (EXIT_FAILURE);
    }
    printf("opened %s for writing\n", *(argv + 3));
    numsect = atol(*(argv + 2));
    drive = strtol(*(argv + 1), NULL, 16);
    for (sect = STARTSECT; sect < (STARTSECT + numsect); sect++)
    {
        printf("reading sector %ld\n", sect);
        printf("%d\n", readLBA(buf, 1, drive, sect));
        fwrite(buf, sizeof buf, 1, fq);
    }
    printf("finished writing %ld bytes\n", numsect * 512L);
    fclose(fq);
    return (0);
}

static int readLBA(void *buf, 
                   int sectors, 
                   int drive, 
                   unsigned long sector)
{
    int rc;
    int ret = -1;
    int tries;
#ifdef __32BIT__
    void *readbuf = transferbuf;
#else
    void *readbuf = buf;
#endif

    tries = 0;
    while (tries < 5)
    {
        rc = BosDiskSectorRLBA(readbuf, 1, drive, sector, 0);
        if (rc == 0)
        {
#ifdef __32BIT__    
            memcpy(buf, transferbuf, 512);
#endif        
            ret = 0;
            break;
        }
        BosDiskReset(drive);
        tries++;
    }
    return (ret);
}

