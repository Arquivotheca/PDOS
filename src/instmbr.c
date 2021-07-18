/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  instmbr - install an MBR (master boot record) at LBA sector 0    */
/*                                                                   */
/*********************************************************************/

/* Utility bin2hex in ozpd was used to generate the hex values
   from the "nasm mbr.asm" output */

#include <stdio.h>
#include <stdlib.h>

#include "bos.h"
#include "unused.h"

static int readLBA(void *buf, 
                   int sectors, 
                   int drive, 
                   unsigned long sector);

static int writeLBA(void *buf, 
                    int sectors, 
                    int drive, 
                    unsigned long sector);

int main(int argc, char **argv)
{
    static unsigned char mbr[512] = {
    "\xFA\x31\xC0\x8E\xD8\x8E\xC0\x8E\xD0\xBC\x00\x7C\xFB\xBE\x00\x7C"
    "\xBF\x00\x06\xB9\x00\x01\xF3\xA5\xEA\x1D\x06\x00\x00\xBE\xBE\x07"
    "\xB9\x04\x00\xF6\x04\x80\x75\x0A\x83\xC6\x10\xE2\xF6\xBD\x92\x06"
    "\xEB\x51\x80\x7C\x04\x00\xBD\xAD\x06\x74\x48\xB4\x41\xBB\xAA\x55"
    "\xCD\x13\xBD\xDA\x06\x72\x3C\x81\xFB\x55\xAA\x75\x36\x66\x8B\x44"
    "\x08\x66\xA3\x70\x07\xB9\x03\x00\x56\xB4\x42\xBE\x68\x07\xCD\x13"
    "\x73\x0B\xB4\x00\xCD\x13\xE2\xF1\xBD\x00\x07\xEB\x16\x5E\x81\x3E"
    "\xFE\x7D\x55\xAA\xBD\x23\x07\x75\x0A\x89\xF5\xEA\x00\x7C\x00\x00"
    "\xCD\x10\x45\xB4\x0E\x8A\x46\x00\x3C\x00\x75\xF4\x87\xDB\xFB\xF4"
    "\xEB\xFA\x4E\x6F\x20\x61\x63\x74\x69\x76\x65\x20\x70\x61\x72\x74"
    "\x69\x74\x69\x6F\x6E\x20\x66\x6F\x75\x6E\x64\x21\x00\x41\x63\x74"
    "\x69\x76\x65\x20\x70\x61\x72\x74\x69\x74\x69\x6F\x6E\x20\x68\x61"
    "\x73\x20\x69\x6E\x76\x61\x6C\x69\x64\x20\x70\x61\x72\x74\x69\x74"
    "\x69\x6F\x6E\x20\x74\x79\x70\x65\x21\x00\x42\x49\x4F\x53\x20\x64"
    "\x6F\x65\x73\x20\x6E\x6F\x74\x20\x73\x75\x70\x70\x6F\x72\x74\x20"
    "\x4C\x42\x41\x20\x65\x78\x74\x65\x6E\x73\x69\x6F\x6E\x73\x21\x00"
    "\x46\x61\x69\x6C\x65\x64\x20\x74\x6F\x20\x72\x65\x61\x64\x20\x76"
    "\x6F\x6C\x75\x6D\x65\x20\x62\x6F\x6F\x74\x20\x72\x65\x63\x6F\x72"
    "\x64\x21\x00\x56\x6F\x6C\x75\x6D\x65\x20\x62\x6F\x6F\x74\x20\x72"
    "\x65\x63\x6F\x72\x64\x20\x69\x73\x20\x6E\x6F\x74\x20\x62\x6F\x6F"
    "\x74\x61\x62\x6C\x65\x20\x28\x6D\x69\x73\x73\x69\x6E\x67\x20\x30"
    "\x78\x61\x61\x35\x35\x20\x62\x6F\x6F\x74\x20\x73\x69\x67\x6E\x61"
    "\x74\x75\x72\x65\x29\x21\x00\x00\x10\x00\x01\x00\x00\x7C\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00"
    };
    unsigned char buf[512];
    int drive;
    unsigned long sect = 0;
    int rc;
    
    if (argc <= 1)
    {
        printf("usage: instmbr <drive>\n");
        printf("example: instmbr 81\n");
        printf("which will install to the second hard disk, typically,\n");
        printf("but not necessarily, the D drive\n");
        printf("80 = first, 81 = second, 82 = third, 83 = fourth\n");
        return (EXIT_FAILURE);
    }
    
    drive = strtol(*(argv + 1), NULL, 16);
    rc = readLBA(buf, 1, drive, sect);

    if (rc != 0)
    {
        printf("read failed with rc %d\n", rc);
        return (EXIT_FAILURE);
    }

    memcpy(buf, mbr, sizeof mbr);

    rc = writeLBA(buf, 1, drive, sect);

    if (rc != 0)
    {
        printf("write failed with rc %d\n", rc);
        return (EXIT_FAILURE);
    }

    printf("MBR has been written to drive %x\n", drive);
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
