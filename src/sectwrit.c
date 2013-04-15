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
    
    if (argc <= 3)
    {
        printf("usage: sectwrit <drive num> <sect #> <file>\n");
        printf("example: sectwrit 81 63 pbootsec.com\n");
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
    sect = atol(*(argv + 2));
    rc = writeLBA(buf, 1, drive, sect);
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
