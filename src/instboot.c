/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  instboot - install boot sector code                              */
/*                                                                   */
/*  This program allows you to overwrite the boot sector             */
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
        printf("usage: instboot <drive num> <sect #> <bootsec>\n");
        printf("example: instboot 81 0 pbootsec.com\n");
        printf("81 = D drive\n");
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
    printf("rc is %d\n", rc);
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
