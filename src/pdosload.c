/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pdosload - load PDOS into memory and then run it                 */
/*                                                                   */
/*********************************************************************/

#include "protint.h"
#include "fat.h"
#include "bos.h"
#include "lldos.h"
#include "support.h"
#include "pdos.h"
#include "unused.h"
#include "dump.h"

typedef struct {
    unsigned long sectors_per_cylinder;
    unsigned int num_heads;
    unsigned int sectors_per_track;
    unsigned long hidden;
    int drive;
} DISKINFO;

static DISKINFO diskinfo;
FAT gfat;

int readAbs(void *buf, int sectors, int drive, int track, int head, int sect);

#ifndef PDOS32
static void fixexe(unsigned long laddr, 
                   unsigned int seg, 
                   unsigned int *progentry);
#endif
static void readLogical(void *diskptr, unsigned long sector, void *buf);
static void analyseBpb(DISKINFO *diskinfo, unsigned char *bpb);
static unsigned long doreboot(unsigned long parm);
static void dopoweroff(void);

void pdosload(void)
{
    unsigned long psp;
    unsigned long load;
    unsigned long loads;
    unsigned char *bpb;
#ifdef PDOS32
    /* the transfer buffer gives us 512 bytes for
       a sector plus 16 bytes for the parameter
       block, required by BosDiskSectorRLBA */
    static unsigned char transferbuf[512+16];
    pdos_parms pp;
#else
    FATFILE fatfile;
    static char buf[512];
    unsigned long start;
    size_t readbytes;
    unsigned int progentry;
    int x;
#endif        

    /* start loading PDOS straight after PLOAD, ie 0x600 + 64k */
    psp = 0x10600UL;
    loads = (unsigned long)psp + 0x100;
    load = loads;
    bpb = (unsigned char *)(0x7c00 - 0x600 + 11);
    analyseBpb(&diskinfo, bpb);
    fatDefaults(&gfat);
    fatInit(&gfat, bpb, readLogical, 0, &diskinfo);
#ifdef PDOS32    
    a20e(); /* enable a20 line */
    pp.transferbuf = ADDR2ABS(transferbuf);
    pp.doreboot = (unsigned long)(void (far *)())doreboot;
    pp.dopoweroff = (unsigned long)(void (far *)())dopoweroff;
    pp.bpb = ADDR2ABS(bpb);
    runaout("MSDOS.SYS", load, ADDR2ABS(&pp));
#else    
    fatOpenFile(&gfat, "MSDOS.SYS", &fatfile);
    do 
    {
        fatReadFile(&gfat, &fatfile, buf, 0x200, &readbytes);
        for (x = 0; x < 512; x++)
        {
            putabs(load + x, buf[x]);
        }
        load += 0x200;
    } while (readbytes == 0x200);
    start = (unsigned long)(void far *)psp;
    start >>= 4;
    fixexe(loads, (unsigned int)start, &progentry);
    start <<= 16;
    start += progentry;
    callfar(start);
#endif    
    fatTerm(&gfat);
    
    /* If the OS has returned to us, do an INT 19H to
       move on to the next bootable disk, as some BIOSes
       have as a feature.  E.g. on a formatted disk that
       has no OS, the boot sector code does an INT 19H
       after saying "press any key", and the BIOS knows
       that if the floppy hasn't been changed, boot off
       the hard disk instead. */
    BosSystemWarmBoot();
    return;
}

#ifndef PDOS32
/* *progentry is set to the program's entry point, suitable for use
   as an offset */

static void fixexe(unsigned long laddr, 
                   unsigned int seg, 
                   unsigned int *progentry)
{
    unsigned int relseg;
    int numentries;
    unsigned long entstart;
    unsigned long progstart; /* where the code actually starts */
    unsigned long fix;
    int x;
    unsigned int headerLen;
    unsigned short val;
    unsigned short fixs;
    unsigned short fixo;

    relseg = seg;    
    headerLen = getabs(laddr + 0x08) 
                | ((unsigned int)getabs(laddr + 0x09) << 8);
    headerLen *= 16;
    entstart = (unsigned long)(laddr + (getabs(laddr + 0x18) 
                       | ((unsigned int)getabs(laddr + 0x19) << 8)));
    numentries = getabs(laddr + 0x06) 
            | ((unsigned int)getabs(laddr + 0x07) << 8);
    progstart = (unsigned long)(laddr + headerLen);
    *progentry = headerLen + (getabs(laddr + 0x14) 
               | ((unsigned int)getabs(laddr + 0x15) << 8));
    *progentry += 0x100;
    relseg += (headerLen + 0x100) / 16;
    for (x = 0; x < numentries; x++)
    {
        fixs = getabs(entstart + x * 4 + 3);
        fixs = (fixs << 8) | getabs(entstart + x * 4 + 2);
        fixo = getabs(entstart + x * 4 + 1);
        fixo = (fixo << 8) | getabs(entstart + x * 4);
        fix = (unsigned long)(progstart + fixo + fixs * 16);
        val = getabs(fix + 1);
        val = (val << 8) | getabs(fix);
        val += relseg;
        putabs(fix, val & 0xff);
        putabs(fix + 1, val >> 8);
    }
    return;
}
#endif

static void readLogical(void *diskptr, unsigned long sector, void *buf)
{
    int track;
    int head;
    int sect;
    DISKINFO *diskinfo;
    int ret;

    diskinfo = (DISKINFO *)diskptr;
    sector += diskinfo->hidden;
    track = (int)(sector / diskinfo->sectors_per_cylinder);
    head = (int)(sector % diskinfo->sectors_per_cylinder);
    sect = head % diskinfo->sectors_per_track + 1;
    head = head / diskinfo->sectors_per_track;
    ret = readAbs(buf, 1, diskinfo->drive, track, head, sect);
    if (ret != 0)
    {
        dumpbuf("halt0001", 8);
        for (;;) ;
    }
    return;
}

static void analyseBpb(DISKINFO *diskinfo, unsigned char *bpb)
{
    diskinfo->drive = bpb[25];
    diskinfo->num_heads = bpb[15];
    diskinfo->hidden = bpb[17]
                       | ((unsigned long)bpb[18] << 8)
                       | ((unsigned long)bpb[19] << 16)
                       | ((unsigned long)bpb[20] << 24);
    diskinfo->sectors_per_track = (bpb[13] | ((unsigned int)bpb[14] << 8));
    diskinfo->sectors_per_cylinder = diskinfo->sectors_per_track 
                                     * diskinfo->num_heads;
    return;
}

static unsigned long doreboot(unsigned long parm)
{
    unused(parm);
    reboot();
    return (0);
}

static void dopoweroff(void)
{
    poweroff();
}

#ifdef NEED_DUMP
void dumpbuf(unsigned char *buf, int len)
{
    int x;

    for (x = 0; x < len; x++)
    {
        BosWriteText(0, buf[x], 0);
    }
    return;
}

void dumplong(unsigned long x)
{
    int y;
    char *z = "0123456789abcdef";
    char buf[9];
    
    for (y = 0; y < 8; y++)
    {
        buf[7 - y] = z[x & 0x0f];
        x /= 16;
    }
    buf[8] = '\0';
    dumpbuf(buf, 8);
    return;
}
#endif
