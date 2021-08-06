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
#ifdef PDOS32
static void ivtCopyEntries(int dest, int orig, int count);
static void picRemap(int master_offset, int slave_offset);
#endif

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

#ifdef PDOS32
    /* Copies BIOS interrupt vectors and remaps IRQs
     * so they do not conflict with protected mode exceptions. */
    {
        disable();
        /* 16 vectors from 0x10 to 0xA0. (BIOS calls) */
        ivtCopyEntries(0xA0, 0x10, 16);
        /* 8 vectors from 0x08 to 0xB0. (BIOS IRQ handlers 0 - 7) */
        ivtCopyEntries(0xB0, 0x08, 8);
        /* 8 vectors from 0x70 to 0xB8. (BIOS IRQ handlers 8 - 15) */
        ivtCopyEntries(0xB8, 0x70, 8);
        picRemap(0xB0, 0xB8);
        enable();
    }
#endif

    /* start loading PDOS straight after PLOAD, ie 0x600 + 64k */
    psp = 0x10600UL;
    loads = (unsigned long)psp + 0x100;
    load = loads;
    bpb = (unsigned char *)(0x7c00 - 0x600 + 11);
    analyseBpb(&diskinfo, bpb);
    fatDefaults(&gfat);
    fatInit(&gfat, bpb, readLogical, 0, &diskinfo, 0);
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
    start = psp;
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

#ifdef PDOS32
static void ivtCopyEntries(int dest, int orig, int count)
{
    /* To access the IVT far pointers must be used. */
    unsigned long far *new_ivt;
    unsigned long far *old_ivt;

    new_ivt = (unsigned long far *) (long)(dest * sizeof(unsigned long));
    old_ivt = (unsigned long far *) (long)(orig * sizeof(unsigned long));
    for (; count > 0; count--)
    {
        *new_ivt = *old_ivt;
        new_ivt++;
        old_ivt++;
    }
}

/* Code for remapping Programmable Interrupt Controller (PIC). */

/* This may be useful to aid in understanding:
   https://wiki.osdev.org/8259_PIC */

#define PIC1         0x20
#define PIC2         0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA    (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA    (PIC2 + 1)

#define ICW1_ICW4_NEEDED 0x01
#define ICW1_INIT        0x10

#define ICW4_8086_MODE 0x1

static void picRemap(int master_offset, int slave_offset)
{
    unsigned char master_mask, slave_mask;

    /* Saves the masks so they can be restored after remapping. */
    master_mask = rportb(PIC1_DATA);
    slave_mask = rportb(PIC2_DATA);

    /* Starts the initialization (ICW1). */
    wportb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4_NEEDED);
    wportb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4_NEEDED);

    /* Sends the offsets to PICs (ICW2). */
    wportb(PIC1_DATA, master_offset);
    wportb(PIC2_DATA, slave_offset);

    /* Tells the master that the slave is at IRQ2 (ICW3). */
    wportb(PIC1_DATA, 0x1U << 2);
    /* Tells the slave that it is at IRQ2 (ICW3). */
    wportb(PIC2_DATA, 2);

    /* Sets the 8086 mode (ICW4). */
    wportb(PIC1_DATA, ICW4_8086_MODE);
    wportb(PIC2_DATA, ICW4_8086_MODE);

    /* Restores the masks. */
    wportb(PIC1_DATA, master_mask);
    wportb(PIC2_DATA, slave_mask);
}
#endif
