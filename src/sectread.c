/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  sectread - read an arbitrary sector of the disk                  */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "bos.h"
#include "unused.h"

static void dodump(unsigned char *fp, long start, long count);
static int readLBA(void *buf, 
                   int sectors, 
                   int drive, 
                   unsigned long sector);

int main(int argc, char **argv)
{
    char buf[512];
    int drive;
    long sect;
    FILE *fq;
    int rc;
    
    if (argc <= 3)
    {
        printf("usage: sectread <drive> <sect #> <outfile>\n");
        printf("example: sectread 81 63 boot.com\n");
        printf("A drive = 0, D drive = 81, etc\n");
        return (EXIT_FAILURE);
    }
    
    drive = strtol(*(argv + 1), NULL, 16);
    sect = atol(*(argv + 2));
    fq = fopen(*(argv + 3), "wb");
    if (fq == NULL)
    {
        printf("failed to open %s for writing\n", *(argv + 3));
        return (EXIT_FAILURE);
    }
    
    rc = readLBA(buf, 1, drive, sect);

    if (rc != 0)
    {
        printf("read failed with rc %d\n", rc);
        return (EXIT_FAILURE);
    }
    fwrite(buf, 1, sizeof buf, fq);
    printf("sector dumped successfully\n");
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

    unused(sectors);
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
