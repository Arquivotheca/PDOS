/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  dumpsec - dump a logical sector                                  */
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
    int rc;
    
    if (argc <= 2)
    {
        printf("usage: dumpsec <drive> <sect #>\n");
        printf("example: dumpsec 81 0\n");
        return (EXIT_FAILURE);
    }
    
    drive = strtol(*(argv + 1), NULL, 16);
    sect = atol(*(argv + 2));
    
    rc = readLBA(buf, 1, drive, sect);
    /* rc = BosDiskSectorRLBA(buf, 1, drive, sect, 0); */
    
    printf("rc is %d\n", rc);
    dodump(buf, 0, 512);
    return (0);
}


static void dodump(unsigned char *fp, long start, long count)
{
    int c, pos1, pos2;
    long x = 0L;
    char prtln[100];

    while (((c = fp[x]) != EOF) && (x != count))
    {
        if (x % 16 == 0)
        {
            memset(prtln, ' ', sizeof prtln);
            sprintf(prtln, "%0.6lX   ", start + x);
            pos1 = 8;
            pos2 = 45;
        }
        sprintf(prtln + pos1, "%0.2X", c);
        if (isprint(c))
        {
            sprintf(prtln + pos2, "%c", c);
        }
        else
        {
            sprintf(prtln + pos2, ".");
        }
        pos1 += 2;
        *(prtln + pos1) = ' ';
        pos2++;
        if (x % 4 == 3)
        {
            *(prtln + pos1++) = ' ';
        }
        if (x % 16 == 15)
        {
            printf("%s\n", prtln);
        }
        x++;
    }
    if (x % 16 != 0)
    {
        printf("%s\n", prtln);
    }
    return;
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
