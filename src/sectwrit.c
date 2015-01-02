/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  sectwrit - write to a sector                                     */
/*                                                                   */
/*  This program allows you to write to any sector                   */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "bos.h"
#include "unused.h"

static int writeAbs(void *buf,
                    int sectors,
                    int drive,
                    int track,
                    int head,
                    int sect);
static int writeLBA(void *buf, 
                    int sectors, 
                    int drive, 
                    unsigned long sector);

int main(int argc, char **argv)
{
    FILE *fp;
    char buf[512];
    int drive;
    long sect;
    int rc;
    int trk;
    int hd;
    int sec;
    int lba = 0;
    
    if (argc <= 3)
    {
        printf("usage: sectwrit <drive num> <sect #> <file>\n");
        printf("example: sectwrit 81 63 pbootsec.com\n");
        printf("example: sectwrit 0 0/0/1 pbootsec.com\n");
        printf("0 = A drive, 81 = D drive etc\n");
        return (EXIT_FAILURE);
    }
    fp = fopen(*(argv + 3), "rb");
    if (fp == NULL)
    {
        printf("failed to open %s\n", *(argv + 2));
        return (EXIT_FAILURE);
    }
    fread(buf, 1, sizeof buf, fp);
    drive = strtol(*(argv + 1), NULL, 16);
    if (strchr(*(argv + 2), '/') != NULL)
    {
        sscanf(*(argv +2), "%d/%d/%d", &trk, &hd, &sec);
    }
    else
    {
        sect = atol(*(argv + 2));
        lba = 1;
    }
    if (lba)
    {
        rc = writeLBA(buf, 1, drive, sect);
    }
    else
    {
        rc = writeAbs(buf, 1, drive, trk, hd, sec);
    }
    if (rc == 0)
    {
        printf("successful\n");
    }
    else
    {
        printf("failed with rc %d\n", rc);
        return (EXIT_FAILURE);
    }
    return (0);
}


static int writeAbs(void *buf,
                    int sectors,
                    int drive,
                    int track,
                    int head,
                    int sect)
{
    int rc;
    int ret = -1;
    int tries;
#ifdef __32BIT__
    void *writebuf = transferbuf;
#else
    void *writebuf = buf;
#endif

    unused(sectors);
#ifdef __32BIT__
    memcpy(transferbuf, buf, 512);
#endif
    tries = 0;
    while (tries < 5)
    {
        rc = BosDiskSectorWrite(writebuf, 1, drive, track, head, sect);
        if (rc == 0)
        {
            ret = 0;
            break;
        }
        BosDiskReset(drive);
        tries++;
    }
    return (ret);
}


static int writeLBA(void *buf, 
                    int sectors, 
                    int drive, 
                    unsigned long sector)
{
    int rc;
    int ret = -1;
    int tries;
#ifdef __32BIT__
    void *writebuf = transferbuf;
#else
    void *writebuf = buf;
#endif

    unused(sectors);
#ifdef __32BIT__    
    memcpy(transferbuf, buf, 512);
#endif        
    tries = 0;
    while (tries < 5)
    {
        rc = BosDiskSectorWLBA(writebuf, 1, drive, sector, 0);
        if (rc == 0)
        {
            ret = 0;
            break;
        }
        BosDiskReset(drive);
        tries++;
    }
    return (ret);
}
