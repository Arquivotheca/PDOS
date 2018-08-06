/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pdos - public domain operating system                            */
/*                                                                   */
/*  This is the main entry point of the operating system proper.     */
/*  There is some startup code that is called before this though,    */
/*  but after that main is called like any other pos format a.out.   */
/*                                                                   */
/*********************************************************************/

#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "pdos.h"
#include "protint.h"
#include "pos.h"
#include "bos.h"
#include "fat.h"
#include "support.h"
#include "lldos.h"
#include "memmgr.h"
#include "patmat.h"
#include "unused.h"
#include "dow.h"

#define MAX_PATH 260 /* With trailing '\0' included. */
#define DEFAULT_DOS_VERSION 0x0004 /* 0004 = DOS 4.0, 2206 = DOS 6.22, etc. */
#define NUM_SPECIAL_FILES 5
    /* stdin, stdout, stderr, stdaux, stdprn */
#define STACKSZ32 0x40000 /* stack size for 32-bit version */

typedef struct {
    int env;
    unsigned char *cmdtail;
    char *fcb1;
    char *fcb2;
} PARMBLOCK;

typedef struct {
    FAT fat;
    BPB bpb;
    char cwd[MAX_PATH];
    int accessed;
    unsigned long sectors_per_cylinder;
    unsigned int num_heads;
    unsigned int sectors_per_track;
    unsigned long hidden;
    int drive;
    int lba;
} DISKINFO;

void pdosRun(void);
static void initdisks(void);
static void scanPartition(int drive);
static void processPartition(int drive, unsigned char *prm);
static void processExtended(int drive, unsigned char *prm);

static void initfiles(void);

static void int21handler(union REGS *regsin,
                        union REGS *regsout,
                        struct SREGS *sregs);

static void make_ff(char *pat);
static void scrunchf(char *dest, char *new);
static int ff_search(void);

#ifdef __32BIT__
int int20(unsigned int *regs);
int int21(unsigned int *regs);
/* INT 25 - Absolute Disk Read */
int int25(unsigned int *regs);
/* INT 26 - Absolute Disk Write */
int int26(unsigned int *regs);
/**/
#endif
static void loadConfig(void);
static void loadPcomm(void);
static void loadExe(char *prog, PARMBLOCK *parmblock);
static int fixexe32(unsigned char *psp, unsigned long entry, unsigned int sp,
                    int doing_pcomm);
static int bios2driv(int bios);
static int fileCreat(const char *fnm, int attrib, int *handle);
static int dirCreat(const char *dnm, int attrib);
static int newFileCreat(const char *fnm, int attrib, int *handle);
static int fileOpen(const char *fnm, int *handle);
static int fileWrite(int fno, const void *buf, size_t szbuf,
                     size_t *writtenbytes);
static int fileDelete(const char *fnm);
static int dirDelete(const char *dnm);
static int fileClose(int fno);
static int fileRead(int fno, void *buf, size_t szbuf, size_t *readbytes);
static void accessDisk(int drive);
static void upper_str(char *str);
int bcd2int(unsigned int bcd);
void dumplong(unsigned long x);
void dumpbuf(unsigned char *buf, int len);
static void readLogical(void *diskptr, unsigned long sector, void *buf);
static void writeLogical(void *diskptr, unsigned long sector, void *buf);
static int readAbs(void *buf,
                int sectors,
                int drive,
                int track,
                int head,
                int sect);
static int readLBA(void *buf,
                int sectors,
                int drive,
                unsigned long sector);
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
static void analyseBpb(DISKINFO *diskinfo, unsigned char *bpb);
int pdosstrt(void);
static int formatcwd(const char *input,char *output);

static MEMMGR memmgr;
#ifdef __32BIT__
static MEMMGR btlmem;
#endif

/* we implement special versions of allocate and free */
#ifndef __32BIT__
#define PDOS16_MEMSTART 0x3000
#define memmgrAllocate(m,b,i) pdos16MemmgrAllocate(m,b,i)
#define memmgrFree(m,p) pdos16MemmgrFree(m,p)
static void pdos16MemmgrFree(MEMMGR *memmgr, void *ptr);
static void *pdos16MemmgrAllocate(MEMMGR *memmgr, size_t bytes, int id);
static void *pdos16MemmgrAllocPages(MEMMGR *memmgr, size_t pages, int id);
static void pdos16MemmgrFreePages(MEMMGR *memmgr, size_t page);
static int pdos16MemmgrReallocPages(MEMMGR *memmgr,
                                    void *ptr,
                                    size_t newpages);
#endif

#define MAXDISKS 20
static DISKINFO disks[MAXDISKS];
static DISKINFO bootinfo;
static unsigned int currentDrive;
static unsigned int tempDrive;
static unsigned int bootDrivePhysical;
static unsigned int bootDriveLogical;
static unsigned int lastDrive;
static unsigned char *bootBPB;
static char *cwd;
static int lastrc;
static int lba;
static unsigned long psector; /* partition sector offset */
static int attr;
static unsigned char *curProc; /* currently running process */

#define MAXFILES 40
static struct {
    FATFILE fatfile;
    FAT *fatptr;
    int inuse;
    int special;
} fhandle[MAXFILES];

static char ff_path[FILENAME_MAX];
static char ff_pat[FILENAME_MAX];

static char shell[100] = "";
static int ff_handle;
static DTA origdta;
static DTA *dta =&origdta;
static int debugcnt = 0;
static void *exec_pstart;
static void *exec_c_api;
#ifdef __32BIT__
static int exec_subcor;
#endif

/* DOS version reported to programs.
 * Can be changed via PosSetDosVersion() call. */
static unsigned int reportedDosVersion = DEFAULT_DOS_VERSION;

/* Log unimplemented flag. If set, unimplemented INT 21 calls are logged. */
static int logUnimplementedFlag = 0;

/* Extended Control+Break checking flag.
 * NOTE: We track flag value but actually changing Ctrl+Break checking
 * behaviour based on flag value isn't currently implemented.
 */
static int breakFlag = 0;

/* Verify after write flag.
 * NOTE: We track flag value but actually doing the verification isn't done
 * by PDOS API itself. But PCOMM COPY command will respond to flag.
 * Implementing VERIFY inside the command interpreter COPY command(s) is
 * basically how Windows NT handles it, see here:
 * https://blogs.msdn.microsoft.com/oldnewthing/20160121-00/?p=92911
 * (MS-DOS instead implements it inside the device drivers, but PDOS
 * doesn't really have such a thing yet.)
 */
static int verifyFlag = 0;

/* Log unimplemented call, but only if logUnimplementedFlag=1. */
static void logUnimplementedCall(int intNum, int ah, int al);

/* If > 0, program is a TSR, don't deallocate its memory on exit. */
static int tsrFlag;

#ifdef __32BIT__
extern char *__vidptr;
/* SUBADDRFIX - given a pointer from the subprogram, convert
it into an address usable by us */
#define SUBADDRFIX(x) ((void *)((char *)(x) + subcor - __abscor))
#define ADDRFIXSUB(x) ((void *)((char *)(x) - subcor + __abscor))
#define SUB2ABS(x) ((void *)((char *)(x) + subcor))

extern int __abscor;
unsigned long __userparm;
int subcor;
static void *gdt;
static char *transferbuf;
static long freem_start; /* start of free memory below 1 MiB,
                            absolute address */
static unsigned long doreboot;
static char *sbrk_start = NULL;
static char *sbrk_end;
#else
#define ABSADDR(x) ((void *)(x))
#endif

#ifndef USING_EXE
int main(void)
{
    pdosRun();
    dumpbuf("\r\nNo command processor - system halted\r\n", 40);
    return (0);
}
#endif

void pdosRun(void)
{
    int drivnum;
#ifdef USING_EXE
    char *m;
    int mc;
#endif
#ifdef __32BIT__
    long memavail;
#endif

#ifdef __32BIT__
    __vidptr = ABSADDR(0xb8000);
#endif
#if (!defined(USING_EXE) && !defined(__32BIT__))
    instint();
#endif
    printf("welcome to PDOS-"
#ifdef __32BIT__
        "32"
#else
        "16"
#endif
        "\n");

#ifndef __32BIT__
    bootBPB = (unsigned char *)ABSADDR(0x7c00 + 11);
#endif
    bootDrivePhysical = bootBPB[25];
    bootDriveLogical = 0;
    disks[0].accessed = 0;
    disks[1].accessed = 0;
    analyseBpb(&bootinfo, bootBPB);
    bootinfo.lba = 0;
    initdisks();
#ifdef EXE32
    bootDriveLogical = 3;
#endif
    if (bootDriveLogical == 0)
    {
        /* if we are booting from floppy, use the in-memory BPB */
        if (bootDrivePhysical < 2)
        {
            bootDriveLogical = bootDrivePhysical;
        }
        /* otherwise some unknown device, tack it on the end and use */
        else
        {
            bootDriveLogical = lastDrive;
            lastDrive++;
        }
        disks[bootDriveLogical] = bootinfo;
        fatDefaults(&disks[bootDriveLogical].fat);
        fatInit(&disks[bootDriveLogical].fat,
                bootBPB,
                readLogical,
                writeLogical,
                &disks[bootDriveLogical]);
        strcpy(disks[bootDriveLogical].cwd, "");
        disks[bootDriveLogical].accessed = 1;
    }
    currentDrive = bootDriveLogical;
    cwd = disks[currentDrive].cwd;
    initfiles();
    loadConfig();
    memmgrDefaults(&memmgr);
    memmgrInit(&memmgr);
#if defined(USING_EXE)
    for (mc = 0; mc < 7; mc++)
    {
        m = malloc(64000);
        memmgrSupply(&memmgr, m, 64000);
    }
#elif defined(__32BIT__)
    /* We don't want to use the region 0x100000 to 0x110000 in case
    someone else wants to use it for 16-bit device drivers etc.
    And while we're at it, we'll skip 0x110000 to 0x200000 in case
    the A20 line is disabled, so this way they get at least 1 meg
    they can use. */
    memavail = BosExtendedMemorySize();
    if (memavail < 3000000)
    {
        memavail = 3000000; /* assume they've got 4 meg installed */
    }
    memavail -= 0x100000; /* subtract a meg to be unused */
#ifdef EXE32
    memavail -= 0x500000; /* room for disk cache */
    memmgrSupply(&memmgr, ABSADDR(0x700000), memavail);
#else
    memmgrSupply(&memmgr, ABSADDR(0x200000), memavail);
#endif
    memmgrDefaults(&btlmem);
    memmgrInit(&btlmem);
    memmgrSupply(&btlmem, ABSADDR(freem_start),
                 0xa0000 - freem_start);
    {
        char *below = transferbuf;
        volatile char *above = below + 0x100000;

        *above = '\0';
        *below = 'X';
        if (*above == 'X')
        {
            printf("A20 line not enabled - random results\n");
        }
    }
#else
    /* Ok, time for some heavy hacking.  Because we want to
    be able to supply a full 64k to DOS apps, we can't
    have the normal control buffers polluting the space.
    So we redefine everything by dividing by 16.  So in
    order to supply 0x30000 to 0x90000, ie a size of
    0x60000, we divide by 16 and only supply 0x6000.
    We then waste the full 0x30000 to 0x40000 for use
    by this dummy memory block.  So we tell memmgr
    that we are supplying 0x30000 for a length of 0x6000,
    and it will happily manage the control blocks within
    that, but then before returning to the app, we multiply
    the offset by 16 and add 0x10000.  When freeing
    memory, we do the reverse, ie substract 0x10000 and
    then divide by 16.  Oh, and because we took away so
    much memory, we only end up supplying 0x5000U. */
    memmgrSupply(&memmgr, (char *)MK_FP(PDOS16_MEMSTART,0x0000), 0x5000U);
#endif
#ifndef USING_EXE
    loadPcomm();
    memmgrTerm(&memmgr);
#ifdef __32BIT__
    memmgrTerm(&btlmem);
#endif
#endif
    return;
}

/* for each physical disk,
for each partition
for each extended partion
gather bpb info, plus (real) drive number */

static void initdisks(void)
{
    int rc;
    int x;

    lastDrive = 2;
    for (x = 0x80; x < 0x84; x++)
    {
        rc = BosFixedDiskStatus(x);
        if (1) /* (rc == 0) */
        {
            scanPartition(x);
        }
    }
    return;
}

/* Partition table values */
#define PT_OFFSET 0x1be /* offset of PT in sector */
#define PT_LEN 16 /* length of each entry */
#define PT_ENT 4 /* number of partition table entries */
#define PTO_SYSID 4 /* offset of system id */
#define PTS_FAT12 1 /* DOS 12-bit FAT */
#define PTS_FAT16S 4 /* DOS 16-bit FAT < 32MB */
#define PTS_DOSE 5 /* DOS extended partition */
#define PTS_FAT16B 6 /* DOS 16-bit FAT >= 32MB */
#define PTS_FAT32 0x0b /* Win95 32-bit FAT */
#define PTS_FAT32L 0x0c /* Win95 32-bit FAT LBA */
#define PTS_FAT16L 0x0e /* Win95 16-bit FAT LBA */
#define PTS_W95EL 0x0f /* Win95 Extended LBA */

static void scanPartition(int drive)
{
    unsigned char buf[512];
    int rc;
    int sectors = 1;
    int track = 0;
    int head = 0;
    int sector = 1;
    int x;
    int systemId;

    /* read partition table */
    rc = readAbs(buf, sectors, drive, track, head, sector);
    if (rc == 0)
    {
        psector = 0;
        /* for each partition */
        for (x = 0; x < PT_ENT; x++)
        {
            lba = 0;
            systemId = buf[PT_OFFSET + x * PT_LEN + PTO_SYSID];
            /* Currently supported systems. */
            /* +++Add support for all systems and test. */
            if ((systemId == PTS_FAT12)
                || (systemId == PTS_FAT16S)
                || (systemId == PTS_FAT16B)
                || (systemId == PTS_FAT32)
                || (systemId == PTS_FAT32L))
            {
                processPartition(drive, &buf[PT_OFFSET + x * PT_LEN]);
            }
        }
        for (x = 0; x < PT_ENT; x++)
        {
            lba = 0;
            systemId = buf[PT_OFFSET + x * PT_LEN + PTO_SYSID];
            if ((systemId == PTS_DOSE)
                || (systemId == PTS_W95EL))
            {
                processExtended(drive, &buf[PT_OFFSET + x * PT_LEN]);
            }
        }
    }
    return;
}

static void processPartition(int drive, unsigned char *prm)
{
    unsigned char buf[512];
    int rc;
    int sectors = 1;
    int track;
    int head;
    int sect;
    int x;
    unsigned char *bpb;
    unsigned int spt;
    unsigned int hpc;
    unsigned long sector;

    head = prm[1];
    sect = prm[2] & 0x1f;
    track = (((unsigned int)prm[2] & 0xc0) << 2) | prm[3];
    sector = prm[8]
            | ((unsigned long)prm[9] << 8)
            | ((unsigned long)prm[10] << 16)
            | ((unsigned long)prm[11] << 24);
    sector += psector;
    if (lba)
    {
        rc = readLBA(buf,
                    sectors,
                    drive,
                    sector);
    }
    else
    {
        rc = readAbs(buf,
                    sectors,
                    drive,
                    track,
                    head,
                    sect);
    }
    if (rc != 0)
    {
        return;
    }
    bpb = buf + 11;
    analyseBpb(&disks[lastDrive], bpb);

    /* we set the lba to whatever mode we are currently in */
    disks[lastDrive].lba = lba;

    /* the number of hidden sectors doesn't appear to be properly
    filled in for extended partitions when formatted with MSDOS,
    so we just use the value computed already */
    disks[lastDrive].hidden = sector;

    /* if physical disks and hidden sectors match, this is the boot drive */
    if ((drive == bootDrivePhysical)
        && (disks[lastDrive].hidden == bootinfo.hidden))
    {
        bootDriveLogical = lastDrive;
    }
    disks[lastDrive].drive = drive;
    fatDefaults(&disks[lastDrive].fat);
    fatInit(&disks[lastDrive].fat, bpb, readLogical,
        writeLogical, &disks[lastDrive]);
    strcpy(disks[lastDrive].cwd, "");
    disks[lastDrive].accessed = 1;
    lastDrive++;
    return;
}

static void processExtended(int drive, unsigned char *prm)
{
    unsigned char buf[512];
    int rc;
    int sectors = 1;
    int track;
    int head;
    int sect;
    int x;
    int systemId;
    unsigned long sector;
    unsigned long extsector;

    if (prm[PTO_SYSID] == PTS_W95EL)
    {
        lba = 1;
    }
    head = prm[1];
    sect = prm[2] & 0x1f;
    track = (((unsigned int)prm[2] & 0xc0) << 2) | prm[3];
    sector = prm[8]
            | ((unsigned long)prm[9] << 8)
            | ((unsigned long)prm[10] << 16)
            | ((unsigned long)prm[11] << 24);
    extsector = sector;
    while (sect != 0)
    {
        if (lba)
        {
            readLBA(buf,
                    sectors,
                    drive,
                    sector);
        }
        else
        {
            readAbs(buf,
                    sectors,
                    drive,
                    track,
                    head,
                    sect);
        }
        systemId = buf[PT_OFFSET + 0 * PT_LEN + PTO_SYSID];
        if ((systemId == PTS_FAT12)
            || (systemId == PTS_FAT16S)
            || (systemId == PTS_FAT16B))
        {
            psector = sector;
            processPartition(drive, buf + PT_OFFSET);
        }
        head = buf[PT_OFFSET + 1 * PT_LEN + 1];
        sect = buf[PT_OFFSET + 1 * PT_LEN + 2] & 0x1f;
        track = (((unsigned int)buf[PT_OFFSET + 1 * PT_LEN + 2]
                & 0xc0) << 2)
                | buf[PT_OFFSET + 1 * PT_LEN + 3];
        sector = buf[PT_OFFSET + 1 * PT_LEN + 8]
                | ((unsigned long)buf[PT_OFFSET + 1 * PT_LEN + 9] << 8)
                | ((unsigned long)buf[PT_OFFSET + 1 * PT_LEN + 10] << 16)
                | ((unsigned long)buf[PT_OFFSET + 1 * PT_LEN + 11] << 24);
        sector += extsector;
    }
    return;
}

static void initfiles(void)
{
    int x;

    for (x = 0; x < NUM_SPECIAL_FILES; x++)
    {
        fhandle[x].inuse = 1;
        fhandle[x].special = 1;
    }
    for (x = NUM_SPECIAL_FILES; x < MAXFILES; x++)
    {
        fhandle[x].inuse = 0;
        fhandle[x].special = 0;
    }
    return;
}

static void int21handler(union REGS *regsin,
                        union REGS *regsout,
                        struct SREGS *sregs)
{
    void *p;
    void *q;
    int handle;
    size_t readbytes;
    size_t writtenbytes;
    long newpos;
    int ret;
    int attr;
    void *tempdta;
    PARMBLOCK *pb;

#if 0
    if (debugcnt < 200)
    {
        printf("ZZZ %d %04x YYY\n",debugcnt, regsin->x.ax);
        /*dumplong(regsin->x.ax);*/
        debugcnt++;
    }
#endif
    switch (regsin->h.ah)
    {
        /* INT 21,00 - Program terminate (no return code) */
        case 0x00:
            PosTerminate(0);
            break;

        /* INT 21,01 - Keyboard input (with echo) */
        /* INT 21,02 - Display output */
        case 0x02:
            regsout->h.al = PosDisplayOutput(regsin->h.dl);
            break;

        /* INT 21,03 - Wait for STDAUX input */
        /* INT 21,04 - Output character to STDAUX */
        /* INT 21,05 - Output character to STDPRN */
        /* INT 21,06 - Direct console IO */
        case 0x06:
            regsout->h.al = PosDirectConsoleOutput(regsin->h.dl);
            break;

        /* INT 21,07 - Direct console input without echo */
        case 0x07:
            regsout->h.al = PosDirectCharInputNoEcho();
            break;

        /* INT 21,08 - Console input without echo */
        case 0x08:
            regsout->h.al = PosGetCharInputNoEcho();
            break;

        /* INT 21,09 - Print $-terminated string */
        case 0x09:
            regsout->h.al = PosDisplayString(MK_FP(sregs->ds, regsin->x.dx));
            break;

        /* INT 21,0A - Buffered Input */
        case 0x0a:
#ifdef __32BIT__
            PosReadBufferedInput(SUBADDRFIX(regsin->d.edx));
#else
            PosReadBufferedInput(MK_FP(sregs->ds, regsin->x.dx));
#endif
            break;

        /* INT 21,0B - Check STDIN status */
        /* INT 21,0C - Clear Keyboard Buffer then Invoke Function */
        /* INT 21,0D - Disk Reset */
        /* INT 21,0E - Select Disk */
        case 0x0e:
            regsout->h.al = PosSelectDisk(regsin->h.dl);
            break;

        /* INT 21,0F - FCB Open File */
        /* INT 21,10 - FCB Close File */
        /* INT 21,11 - FCB Find First */
        /* INT 21,12 - FCB Find Next */
        /* INT 21,13 - FCB Delete File */
        /* INT 21,14 - FCB Sequential Read */
        /* INT 21,15 - FCB Sequential Write */
        /* INT 21,16 - FCB Create File */
        /* INT 21,17 - FCB Rename File */
        /* INT 21,18 - DOS NULL FUNCTION
         * CP/M BDOS call DRV_LOGINVEC - Return bitmap of logged-in drives */

        /* INT 21,19 - Get current default drive */
        case 0x19:
            regsout->h.al = PosGetDefaultDrive();
            break;

        /* INT 21,1A - Set Disk Transfer Address (DTA) */
        case 0x1a:
#ifdef __32BIT__
            tempdta = SUBADDRFIX(regsin->d.edx);
#else
            tempdta = MK_FP(sregs->ds, regsin->x.dx);
#endif
            PosSetDTA(tempdta);
            break;

        /* INT 21,1A - Set disk transfer address */
        /* INT 21,1B - Get allocation table info */
        /* INT 21,1C - Get allocation table info  */
        /* INT 21,1D - DOS NULL FUNCTION
         * CP/M BDOS call DRV_ROVEC - Return bitmap of read-only drives */
        /* INT 21,1E - DOS NULL FUNCTION
         * CP/M BDOS call F_ATTRIB - Set CP/M file attributes */
        /* INT 21,1F - Get Drive Parameter Table (DPT) (DOS 1.0+) */
        /* INT 21,20 - DOS NULL FUNCTION
         * CP/M BDOS call F_USERNUM - Get/Set CP/M user number */
        /* INT 21,21 - FCB Random Read */
        /* INT 21,22 - FCB Random Write */
        /* INT 21,23 - FCB Get File Size */
        /* INT 21,24 - FCB Set Relative Record Field */
        /* INT 21,25 - Set Interrupt Vector */
        case 0x25:
#ifdef __32BIT__
            p = SUBADDRFIX(regsin->d.edx);
#else
            p = MK_FP(sregs->ds, regsin->x.dx);
#endif
            PosSetInterruptVector(regsin->h.al, p);
            break;

        /* INT 21,26 - Create New PSP */
        /* INT 21,27 - FCB Random Block Read */
        /* INT 21,28 - FCB Random Block Write */
        /* INT 21,29 - FCB Parse Filename */
        /* INT 21,2A - Get System Date */
        case 0x2a:
            {
                int year, month, day, dw;

                PosGetSystemDate(&year, &month, &day, &dw);
                regsout->x.cx = year;
                regsout->h.dh = month;
                regsout->h.dl = day;
                regsout->h.al = dw;
            }
            break;

        /* INT 21,2B - Set System Date */
        case 0x2b:
            regsout->h.al = PosSetSystemDate(regsin->x.cx,
                                             regsin->h.dh,
                                             regsin->h.dl);
            break;

        /* INT 21,2C - Get System Time */
        case 0x2c:
            {
                int hour, minute, second, hundredths;

                PosGetSystemTime(&hour, &minute, &second, &hundredths);
                regsout->h.ch = hour;
                regsout->h.cl = minute;
                regsout->h.dh = second;
                regsout->h.dl = hundredths;
            }
            break;

        /* INT 21,2D - Set System Time */

        /* INT 21,2E - Set Verify Flag */
        case 0x2e:
            PosSetVerifyFlag(regsin->h.al);
            break;

        /* INT 21,2F - Get Disk Transfer Address (DTA) */
        case 0x2f:
            tempdta = PosGetDTA();
#ifdef __32BIT__
            regsout->d.ebx = (int)ADDRFIXSUB(tempdta);
#else
            regsout->x.bx = FP_OFF(tempdta);
            sregs->es = FP_SEG(tempdta);
#endif
            break;

        /* INT 21,30 - Get DOS Version */
        case 0x30:
            regsout->x.ax = PosGetDosVersion();
            regsout->x.bx = 0;
            regsout->x.cx = 0;
            break;

        /* INT 21,31 - Terminate and Stay Resident */
        case 0x31:
            PosTerminateAndStayResident(regsin->h.al, regsin->x.dx);
            break;

        /* INT 21,32 - Get Drive Parameter Table (DPT) (DOS 2.0+) */

        /* INT 21,33 - primary function is managing breakFlag.
         * Also some other subfunctions around DOS version and boot drive.
         */
        case 0x33:
            /* AL=0: Get break flag in DL register */
            if (regsin->h.al == 0)
            {
                regsout->h.dl = PosGetBreakFlag();
            }

            /* AL=1: Set break flag in DL register */
            else if (regsin->h.al == 1)
            {
                PosSetBreakFlag(regsin->h.dl);
            }

            /* AL=2: Set break flag, return old value.
             * This just combines AL=0 and AL=1 in one operation.
             */
            else if (regsin->h.al == 2)
            {
                regsout->h.dl = PosGetBreakFlag();
                PosSetBreakFlag(regsin->h.dl);
            }

            /* AL=3/AL=4: These functions managed CPSW
             * (Code Page SWitching) state. In released versions of MS-DOS,
             * these have always been NO-OPS. The CPSW functions were
             * implemented during the development of DOS 4.0, but removed
             * before it was releaseed.
             */

            /* AL=5: Get DOS boot drive (PosGetBootDrive) */
            else if (regsin->h.al == 5)
            {
                regsout->h.dl = PosGetBootDrive();
            }

            /* AL=6: Get DOS version number.
             * Unlike INT 21,30, which can be faked with SETVER (in MS-DOS 5+),
             * 3306 is supposed to return the real DOS version. However, in
             * PDOS we will just return the fake version here too.
             */
            else if (regsin->h.al == 6)
            {
                /* BL = Major Version, BH = Minor version */
                regsout->x.bx = PosGetDosVersion();

                /* DL = Revision, DH = Version Flags */
                regsout->x.dx = 0;
            }

            else
            {
                logUnimplementedCall(0x21, regsin->h.ah, regsin->h.al);
            }
            break;

        /* INT 21,34 - Get INDOS Flag Address */
        /* INT 21,35 - Get Interrupt Vector */
        case 0x35:
            p = PosGetInterruptVector(regsin->h.al);
#ifdef __32BIT__
            regsout->d.ebx = (int)ADDRFIXSUB(p);
#else
            regsout->x.bx = FP_OFF(p);
            sregs->es = FP_SEG(p);
#endif
            break;

        /* INT 21,36 - Get Free Space */
        case 0x36:
#ifdef __32BIT__
            PosGetFreeSpace(regsin->h.dl,
                            &regsout->d.eax,
                            &regsout->d.ebx,
                            &regsout->d.ecx,
                            &regsout->d.edx);
#else
            PosGetFreeSpace(regsin->h.dl,
                            &regsout->x.ax,
                            &regsout->x.bx,
                            &regsout->x.cx,
                            &regsout->x.dx);
#endif
            break;

        /* INT 21,37 - Get/Set Switch Character */
        /* INT 21,38 - Get/Set Country Information */
        /* INT 21,39 - Make Directory */
        case 0x39:
#ifdef __32BIT__
            regsout->d.eax = PosMakeDir(SUBADDRFIX(regsin->d.edx));
            if (regsout->d.eax) regsout->x.cflag = 1;
#else
            regsout->x.ax = PosMakeDir(MK_FP(sregs->ds, regsin->x.dx));
            if (regsout->x.ax) regsout->x.cflag = 1;
#endif
            break;

        /* INT 21,3A - Remove Directory */
        case 0x3a:
#ifdef __32BIT__
            regsout->d.eax = PosRemoveDir(SUBADDRFIX(regsin->d.edx));
            if (regsout->d.eax) regsout->x.cflag = 1;
#else
            regsout->x.ax = PosRemoveDir(MK_FP(sregs->ds, regsin->x.dx));
            if (regsout->x.ax) regsout->x.cflag = 1;
#endif
            break;

        /* INT 21,3B - Change Directory */
        case 0x3b:
#ifdef __32BIT__
            regsout->d.eax = PosChangeDir(SUBADDRFIX(regsin->d.edx));
            if (regsout->d.eax) regsout->x.cflag = 1;
#else
            regsout->x.ax = PosChangeDir(MK_FP(sregs->ds, regsin->x.dx));
            if (regsout->x.ax) regsout->x.cflag = 1;
#endif
            break;

        /* INT 21,3C - Create File */
        case 0x3c:
#ifdef __32BIT__
            p = SUBADDRFIX(regsin->d.edx);
            regsout->d.eax = PosCreatFile(p, regsin->x.cx, &handle);
            if (regsout->d.eax) regsout->x.cflag = 1;
            else regsout->d.eax = handle;
#else
            p = MK_FP(sregs->ds, regsin->x.dx);
            regsout->x.ax = PosCreatFile(p, regsin->x.cx, &handle);
            if (regsout->x.ax) regsout->x.cflag = 1;
            else regsout->x.ax = handle;
#endif
            break;

        /* INT 21,3D - Open File */
        case 0x3d:
#ifdef __32BIT__
            p = SUBADDRFIX(regsin->d.edx);
            regsout->d.eax = PosOpenFile(p, regsin->h.al, &handle);
            if (regsout->d.eax) regsout->x.cflag = 1;
            else regsout->d.eax = handle;
#else
            p = MK_FP(sregs->ds, regsin->x.dx);
            regsout->x.ax = PosOpenFile(p, regsin->h.al, &handle);
            if (regsout->x.ax) regsout->x.cflag = 1;
            else regsout->x.ax = handle;
#endif
            break;

        /* INT 21,3E - Close File */
        case 0x3e:
#ifdef __32BIT__
            regsout->d.eax = PosCloseFile(regsin->d.ebx);
#else
            regsout->x.ax = PosCloseFile(regsin->x.bx);
#endif
            break;

        /* INT 21,3F - Read File */
        case 0x3f:
#ifdef __32BIT__
            p = SUBADDRFIX(regsin->d.edx);
            regsout->d.eax = PosReadFile(regsin->d.ebx,
                                         p,
                                         regsin->d.ecx,
                                         &readbytes);
            if (regsout->d.eax) regsout->x.cflag = 1;
            else regsout->d.eax = readbytes;
#else
            p = MK_FP(sregs->ds, regsin->x.dx);
            regsout->x.ax = PosReadFile(regsin->x.bx,
                                        p,
                                        regsin->x.cx,
                                        &readbytes);
            if (regsout->x.ax) regsout->x.cflag = 1;
            else regsout->x.ax = readbytes;
#endif
            break;

        /* INT 21,40 - Write File */
        case 0x40:
#ifdef __32BIT__
            p = SUBADDRFIX(regsin->d.edx);
            regsout->d.eax = PosWriteFile(regsin->d.ebx,
                                          p,
                                          regsin->d.ecx,
                                          &writtenbytes);
            if (regsout->d.eax) regsout->x.cflag = 1;
            else regsout->d.eax = writtenbytes;
#else
            p = MK_FP(sregs->ds, regsin->x.dx);
            regsout->x.ax = PosWriteFile(regsin->x.bx,
                                         p,
                                         regsin->x.cx,
                                         &writtenbytes);
            if (regsout->x.ax) regsout->x.cflag = 1;
            else regsout->x.ax = writtenbytes;
#endif
            break;

        /* INT 21,41 - Delete File */
        case 0x41:
#ifdef __32BIT__
            p = SUBADDRFIX(regsin->d.edx);
#else
            p = MK_FP(sregs->ds, regsin->x.dx);
#endif
            regsout->x.ax = PosDeleteFile(p);
            if (regsout->x.ax != 0)
            {
                regsout->x.cflag = 1;
            }
            break;

        /* INT 21,42 - Move File Pointer */
        case 0x42:
#ifdef __32BIT__
            regsout->d.eax = PosMoveFilePointer(regsin->d.ebx,
                                                regsin->d.ecx,
                                                regsin->h.al,
                                                &newpos);
            if (regsout->d.eax)
            {
                regsout->x.cflag = 1;
            }
            else regsout->d.eax = newpos;
#else
            newpos = ((unsigned long)regsin->x.cx << 16) | regsin->x.dx;
            regsout->x.ax = PosMoveFilePointer(regsin->x.bx,
                                               newpos,
                                               regsin->h.al,
                                               &newpos);
            if (regsout->x.ax)
            {
                regsout->x.cflag = 1;
            }
            else
            {
                regsout->x.dx = newpos >> 16;
                regsout->x.ax = newpos & 0xffff;
            }
#endif
            break;

        /* INT 21,43 - Get File Attributes */
        case 0x43:
            if (regsin->h.al == 0x00)
            {
#ifdef __32BIT__
                p = SUBADDRFIX(regsin->d.edx);
#else
                p = MK_FP(sregs->ds, regsin->x.dx);
#endif

#ifdef __32BIT__
                regsout->d.eax = PosGetFileAttributes(p,&attr);
                regsout->d.ecx=attr;
#else
                regsout->x.ax = PosGetFileAttributes(p,&attr);
                regsout->x.cx=attr;
#endif

#ifdef __32BIT__
                if (regsout->d.eax != 0)
                {
                    regsout->x.cflag = 1;
                }
                else
                {
                    regsout->x.cflag=0;
                }
#else
                if (regsout->x.ax != 0)
                {
                    regsout->x.cflag = 1;
                }
                else
                {
                    regsout->x.cflag=0;
                }
#endif
            }

            else if (regsin->h.al == 0x01)
            {
#ifdef __32BIT__
                p = SUBADDRFIX(regsin->d.edx);
#else
                p = MK_FP(sregs->ds, regsin->x.dx);
#endif

#ifdef __32BIT__
                regsout->d.eax = PosSetFileAttributes(p,regsin->d.ecx);
#else
                regsout->x.ax = PosSetFileAttributes(p,regsin->x.cx);
#endif

#ifdef __32BIT__
                if (regsout->d.eax != 0)
                {
                    regsout->x.cflag = 1;
                }
                else
                {
                    regsout->x.cflag=0;
                }

#else
                if (regsout->x.ax != 0)
                {
                    regsout->x.cflag = 1;
                }
                else
                {
                    regsout->x.cflag=0;
                }
#endif

            }
            else
            {
                logUnimplementedCall(0x21, regsin->h.ah, regsin->h.al);
            }
            break;

        /* INT 21,44 - IOCTL functions */
        case 0x44:
            if (regsin->h.al == 0x00)
            {
#ifdef __32BIT__
                regsout->x.ax = PosGetDeviceInformation(regsin->x.bx,
                                                        &regsout->d.edx);
#else
                regsout->x.ax = PosGetDeviceInformation(regsin->x.bx,
                                                        &regsout->x.dx);
#endif
                if (regsout->x.ax != 0)
                {
                    regsout->x.cflag = 1;
                }
            }
            else if (regsin->h.al == 0x08)
            {
#ifdef __32BIT__
                regsout->d.eax = PosBlockDeviceRemovable(regsin->h.bl);
                if ((int)regsout->d.eax < 0)
                {
                    regsout->x.cflag = 1;
                    regsout->d.eax = -regsout->d.eax;
                }
#else
                regsout->x.ax = PosBlockDeviceRemovable(regsin->h.bl);
                if ((int)regsout->x.ax < 0)
                {
                    regsout->x.cflag = 1;
                    regsout->x.ax = -regsout->x.ax;
                }
#endif
            }


            /* Implementing the Int 21h/AX=4409 call*/
            else if (regsin->h.al == 0x09)
            {
#ifdef __32BIT__
                regsout->d.eax = PosBlockDeviceRemote(regsin->h.bl,
                                                    &regsout->d.edx);
                if ((int)regsout->d.eax < 0)
                {
                    regsout->x.cflag = 1;
                    regsout->d.eax = -regsout->d.eax;
                }
#else
                regsout->x.ax = PosBlockDeviceRemote(regsin->h.bl,
                                                    &regsout->x.dx);
                if ((int)regsout->x.ax < 0)
                {
                    regsout->x.cflag = 1;
                    regsout->x.ax = -regsout->x.ax;
                }
#endif
            }

/**/
/*Function to dump the contents of DS,DX register*/
            else if (regsin->h.al == 0x0D)
            {
#ifdef __32BIT__
                p=SUBADDRFIX(regsin->d.edx);
                regsout->d.eax =PosGenericBlockDeviceRequest(regsin->h.bl,
                                                            regsin->h.ch,
                                                            regsin->h.cl,p);
#else
                p = MK_FP(sregs->ds, regsin->x.dx);
                regsout->x.ax =PosGenericBlockDeviceRequest(regsin->h.bl,
                                                            regsin->h.ch,
                                                            regsin->h.cl,p);
#endif
            }
            else
            {
                logUnimplementedCall(0x21, regsin->h.ah, regsin->h.al);
            }
            break;

        /* INT 21,45 - Duplicate File Handle */
        case 0x45:
#ifdef __32BIT__
            ret = PosDuplicateFileHandle(regsin->d.ebx);
            if (ret < 0)
            {
                regsout->x.cflag = 1;
                ret = -ret;
            }
            regsout->d.eax = ret;
#else
            ret = PosDuplicateFileHandle(regsin->x.bx);
            if (ret < 0)
            {
                regsout->x.cflag = 1;
                ret = -ret;
            }
            regsout->x.ax = ret;
#endif
            break;

        /* INT 21,46 - Force Duplicate File Handle */
        case 0x46:
#ifdef __32BIT__
            regsout->d.eax = PosForceDuplicateFileHandle(regsin->d.ebx,
                                                         regsin->d.ecx);
            if (regsout->d.eax) regsout->x.cflag = 1;
#else
            regsout->x.ax = PosForceDuplicateFileHandle(regsin->x.bx,
                                                        regsin->x.cx);
            if (regsout->x.ax) regsout->x.cflag = 1;
#endif
            break;

        /* INT 21,47 - Get Current Directory */
        case 0x47:
#ifdef __32BIT__
            p = SUBADDRFIX(regsin->d.esi);
            regsout->d.eax = PosGetCurDir(regsin->h.dl, p);
#else
            p = MK_FP(sregs->ds, regsin->x.si);
            regsout->x.ax = PosGetCurDir(regsin->h.dl, p);
#endif
            break;

        /* INT 21,48 - Allocate Memory */
        case 0x48:
#ifndef __32BIT__
            /* This function is only useful in 16-bit PDOS */
            /* for some bizarre reason, MSC allocates 32 bytes,
            then attempts to turn it into 16k - do this in
            advance! */
            if (regsin->x.bx == 2)
            {
                regsin->x.bx = 0x400;
            }
            p = PosAllocMemPages(regsin->x.bx, &regsout->x.bx);
            if (p == NULL)
            {
                regsout->x.cflag = 1;
                regsout->x.ax = 0x08;
            }
            else
            {
                regsout->x.ax = FP_SEG(p) + (FP_OFF(p) >> 4);
            }
#endif
            break;

        /* INT 21,49 - Free Memory */
        case 0x49:
#ifdef __32BIT__
            regsout->d.eax = PosFreeMem(SUBADDRFIX(regsin->d.ebx));
#else
            regsout->x.ax = PosFreeMem(MK_FP(sregs->es, 0));
#endif
            if (regsout->x.ax != 0)
            {
                regsout->x.cflag = 1;
            }
            break;

        /* INT 21,4A - Resize Memory Block */
        case 0x4a:
#ifdef __32BIT__
            regsout->d.eax = PosReallocPages(SUBADDRFIX(regsin->d.ecx),
                                            regsin->d.ebx,
                                            &regsout->d.ebx);
#else
            regsout->x.ax = PosReallocPages(MK_FP(sregs->es, 0),
                                            regsin->x.bx,
                                            &regsout->x.bx);
            /* regsout->x.ax = 0; */
#endif
            if (0) /* regsout->x.ax != 0) */ /* always return success */
            {
                regsout->x.cflag = 1;
            }
            else
            {
                regsout->x.cflag = 0;
            }
            break;

        /* INT 21,4B - Load and Execute Program */
        case 0x4b:
            /* AL=00: Load and Execute */
            if (regsin->h.al == 0)
            {
#ifdef __32BIT__
                pb = SUBADDRFIX(regsin->d.ebx);
                if (pb != NULL)
                {
                    pb->cmdtail = SUBADDRFIX(pb->cmdtail);
                }
                PosExec(SUBADDRFIX(regsin->d.edx), pb);
                if (pb != NULL)
                {
                    pb->cmdtail = ADDRFIXSUB(pb->cmdtail);
                }
#else
                PosExec(MK_FP(sregs->ds, regsin->x.dx),
                        MK_FP(sregs->es, regsin->x.bx));
#endif
            }
            /* AL=01: Load but don't execute (for use by debuggers) */
            /* AL=03: Load overlay */
            /* AL=04: Execute in background.
             *        ("European" multitasking MS-DOS 4.0 only) */
            else
            {
                logUnimplementedCall(0x21, regsin->h.ah, regsin->h.al);
            }
            break;

        /* INT 21,4C - Terminate (with return code) */
        case 0x4c:
#if (!defined(USING_EXE))
            PosTerminate(regsin->h.al);
#endif
            break;

        /* INT 21,4D - Get Subprocess Return Code */
        case 0x4d:
            regsout->x.ax = PosGetReturnCode();
            break;

        /* INT 21,4E - Find First File */
        case 0x4e:
#ifdef __32BIT__
            regsout->d.eax = PosFindFirst(SUBADDRFIX(regsin->d.edx),
                                        regsin->x.cx);
            if (regsout->d.eax != 0)
            {
                regsout->x.cflag = 1;
            }
#else
            regsout->x.ax = PosFindFirst(MK_FP(sregs->ds, regsin->x.dx),
                                        regsin->x.cx);
            if (regsout->x.ax != 0)
            {
                regsout->x.cflag = 1;
            }
#endif
            break;

        /* INT 21,4F - Find Next File */
        case 0x4f:
            regsout->x.ax = PosFindNext();
            if (regsout->x.ax != 0)
            {
                regsout->x.cflag = 1;
            }
            break;

        /* INT 21,50 - Set Current Process ID */
        /* INT 21,51 - Get Current Process ID */
        case 0x51:
#ifdef __32BIT__
            regsout->d.ebx = PosGetCurrentProcessId();
#else
            regsout->x.bx = PosGetCurrentProcessId();
#endif
            break;

        /* INT 21,52 - Get INVARS Pointer */
        /* INT 21,53 - Generate Drive Parameter Table */

        /* INT 21,54 - Get Verify Flag */
        case 0x54:
            regsout->h.al = PosGetVerifyFlag();
            break;

        /* INT 21,55 - Create New PSP */
        /* INT 21,56 - Rename File */
        case 0x56:
#ifdef __32BIT__
            p = SUBADDRFIX(regsin->d.edx);
            q = SUBADDRFIX(regsin->d.edi);
#else
            p = MK_FP(sregs->ds, regsin->x.dx);
            q = MK_FP(sregs->es, regsin->x.di);
#endif
            regsout->x.ax = PosRenameFile(p, q);
            if (regsout->x.ax != 0)
            {
                regsout->x.cflag = 1;
            }
            break;

        /* INT 21,57 - Get/Set File Date/Time */
        case 0x57:
            if (regsin->h.al == 0x00)
            {
#ifdef __32BIT__
                regsout->d.eax=PosGetFileLastWrittenDateAndTime(regsin->d.ebx,
                                                              &regsout->d.edx,
                                                              &regsout->d.ecx);
#else
                regsout->x.ax=PosGetFileLastWrittenDateAndTime(regsin->x.bx,
                                                              &regsout->x.dx,
                                                              &regsout->x.cx);
#endif

#ifdef __32BIT__

                if (regsout->d.eax != 0)
                {
                    regsout->x.cflag = 1;
                }
                else
                {
                    regsout->x.cflag=0;
                }
#else
                if (regsout->x.ax != 0)
                {
                    regsout->x.cflag = 1;
                }
                else
                {
                    regsout->x.cflag=0;
                }
#endif
            }
            else if(regsin->h.al==0x01)
            {
#ifdef __32BIT__
                regsout->d.eax=PosSetFileLastWrittenDateAndTime(regsin->d.ebx,
                                                               regsout->d.edx,
                                                               regsout->d.ecx);
#else
                regsout->x.ax=PosSetFileLastWrittenDateAndTime(regsin->x.bx,
                                                              regsout->x.dx,
                                                              regsout->x.cx);
#endif

#ifdef __32BIT__

                if (regsout->d.eax != 0)
                {
                    regsout->x.cflag = 1;
                }
                else
                {
                    regsout->x.cflag=0;
                }
#else
                if (regsout->x.ax != 0)
                {
                    regsout->x.cflag = 1;
                }
                else
                {
                    regsout->x.cflag=0;
                }
#endif
            }
            else
            {
                logUnimplementedCall(0x21, regsin->h.ah, regsin->h.al);
            }

            break;

        /* INT 21,58 - Get/Set Memory Allocation Strategy */
        /* INT 21,59 - Get Extended Error Information */
        /* INT 21,5A - Create Temporary File */
        /* INT 21,5B - Create New File */
        case 0x5b:
#ifdef __32BIT__
            p = SUBADDRFIX(regsin->d.edx);
            regsout->d.eax = PosCreatNewFile(p, regsin->x.cx, &handle);
            if (regsout->d.eax) regsout->x.cflag = 1;
            else regsout->d.eax = handle;
#else
            p = MK_FP(sregs->ds, regsin->x.dx);
            regsout->x.ax = PosCreatNewFile(p, regsin->x.cx, &handle);
            if (regsout->x.ax) regsout->x.cflag = 1;
            else regsout->x.ax = handle;
#endif
            break;

        /* INT 21,5C - Lock/Unlock File */
        /* INT 21,5D - Miscellaneous functions (networking, SHARE.EXE) */
        /* INT 21,5E - Miscellaneous functions (networking, printing)  */
        /* INT 21,5F - Device redirection */
        /* INT 21,60 - Get True File Name */
        case 0x60:
#ifdef __32BIT__
            p = SUBADDRFIX(regsin->d.esi);
            q = SUBADDRFIX(regsin->d.edi);
#else
            p = MK_FP(sregs->ds, regsin->x.si);
            q = MK_FP(sregs->es, regsin->x.di);
#endif
            ret=PosTruename(p,q);

            if(ret < 0)
            {
                regsout->x.cflag=1;
#ifdef __32BIT__
                regsout->d.eax=-ret;
#else
                regsout->x.ax=-ret;
#endif
            }
            break;
        /* INT 21,61 - Unused (reserved for networking use) */
        /* INT 21,62 - Get PSP address */
        case 0x62:
#ifdef __32BIT__
            regsout->d.ebx = PosGetCurrentProcessId();
#else
            regsout->x.bx = PosGetCurrentProcessId();
#endif
            break;
        /* INT 21,63 - Double Byte Character Functions (East Asian) */
        /* INT 21,64 - Set device driver lookahead (undocumented) */
        /* INT 21,65 - Get Extended Country Information */
        /* INT 21,66 - Get/Set Code Page Table */
        /* INT 21,67 - Set Handle Count (resize Job File Table (JFT)) */
        /* INT 21,68 - Flush Handle */
        /* INT 21,69 - Get/Set Disk Serial Number */
        /* INT 21,6A - same as function 68 */
        /* INT 21,6B - legacy IFS function (DOS 4.x only) */
        /* INT 21,6C - Extended Open/Create */
        /* INT 21,6D - various (ROM DOS or OS/2 only) */
        /* INT 21,6E - various (ROM DOS or OS/2 only) */
        /* INT 21,6F - various (ROM DOS or OS/2 only) */
        /* INT 21,70 - Windows 95 internationalization */
        /* INT 21,71 - Windows 95 Long File Name API */
        /* INT 21,73 - Windows 95 Drive Lock/Flush, FAT32 functions */
        /*Function call AX=7303h*/
        case 0x73:
            if(regsin->h.al==0x03)
            {
                regsout->x.cflag=0;
#ifdef __32BIT__
                regsout->d.eax=0;
#else
                regsout->h.al=0;
#endif
            }
            else
            {
                logUnimplementedCall(0x21, regsin->h.ah, regsin->h.al);
            }
            break;
        /**/
        /* emx calls are 0x7f */
#ifdef __32BIT__
        case 0x7f:
            /* sbrk */
            if (regsin->h.al == 0)
            {
                if (sbrk_start == NULL)
                {
                    sbrk_start = PosAllocMem(0x100000, LOC32);
                    sbrk_end = sbrk_start + 0x10000;
                }
                regsout->d.eax = (unsigned int)sbrk_end;
                sbrk_end += regsin->d.edx;
            }
            /* version - return infinite! */
            else if (regsin->h.al == 0x0a)
            {
                regsout->d.eax = 0xffffffffU;
            }
            else
            {
                logUnimplementedCall(0x21, regsin->h.ah, regsin->h.al);
            }
            break;
#endif

        /* INT 21,F6 - PDOS extensions */
        case 0xf6:
            if (regsin->h.al == 0)
            {
#ifdef __32BIT__
                PosDisplayInteger(regsin->d.ecx);
#else
                PosDisplayInteger(regsin->x.cx);
#endif
            }
            else if (regsin->h.al == 1)
            {
                PosReboot();
            }
            else if (regsin->h.al == 2)
            {
#ifdef __32BIT__
                PosSetRunTime(SUBADDRFIX(regsin->d.ebx),
                            SUBADDRFIX(regsin->d.ecx));
#endif
            }
            else if (regsin->h.al == 3)
            {
                PosSetDosVersion(regsin->x.bx);
            }
            else if (regsin->h.al == 4)
            {
                regsout->x.ax = PosGetLogUnimplemented();
            }
            else if (regsin->h.al == 5)
            {
                PosSetLogUnimplemented(!!(regsin->x.bx));
            }
            else if (regsin->h.al == 6)
            {
                regsout->x.ax = PosGetMagic();
            }
            else if (regsin->h.al == 7)
            {
#ifdef __32BIT__
                p = SUBADDRFIX(regsin->d.edx);
#else
                p = MK_FP(sregs->ds, regsin->x.dx);
#endif
                PosGetMemoryManagementStats(p);
            }
#ifdef __32BIT__
            /* this function is currently only implemented
               for 32-bit, but it could potentially be
               implemented in 16-bit too. */
            else if (regsin->h.al == 8)
            {
                regsout->d.eax = (int)PosAllocMem(regsin->d.ebx, regsin->d.ecx);
                regsout->d.eax = (int)ADDRFIXSUB(regsout->d.eax);
            }
#endif
            else if (regsin->h.al == 9)
            {
                p = PosGetErrorMessageString(regsin->x.bx);
#ifdef __32BIT__
                regsout->d.edx = (int)ADDRFIXSUB(p);
#else
                regsout->x.dx = FP_OFF(p);
                sregs->es = FP_SEG(p);
#endif
            }
            else
            {
                logUnimplementedCall(0x21, regsin->h.ah, regsin->h.al);
            }
            break;

        default:
            logUnimplementedCall(0x21, regsin->h.ah, regsin->h.al);
            break;
    }
    return;
}


/* !!! START OF POS FUNCTIONS !!! */

/* INT 20 */
void PosTermNoRC(void)
{
    PosTerminate(0);
}

/* INT 21/AH=02h */
unsigned int PosDisplayOutput(unsigned int ch)
{
    unsigned char buf[1];
    size_t writtenbytes;

    buf[0] = ch;
    PosWriteFile(1, buf, 1, &writtenbytes);

    return ch;
}

/* INT 21/AH=06h */
unsigned int PosDirectConsoleOutput(unsigned int ch)
{
    unsigned char buf[1];
    size_t writtenbytes;

    buf[0] = ch;
    PosWriteFile(1, buf, 1, &writtenbytes);

    return ch;
}

/* INT 21/AH=07h */
unsigned int PosDirectCharInputNoEcho(void)
{
    int scan;
    int ascii;
    static int gotextend = 0;

    if (gotextend != 0)
    {
        ascii = gotextend;
        gotextend = 0;
    }
    else
    {
        BosReadKeyboardCharacter(&scan, &ascii);
        if (ascii == 0)
        {
            gotextend = scan;
        }
    }

    return ascii;
}

/* Written By NECDET COKYAZICI, Public Domain */

/* INT 21/AH=08h */
unsigned int PosGetCharInputNoEcho(void)
{
    int scan;
    int ascii;

    BosReadKeyboardCharacter(&scan, &ascii);

    return ascii;
}

/* INT 21/AH=09h */
unsigned int PosDisplayString(const char *buf)
{
    const char *p;
    size_t writtenbytes;

    p = memchr(buf, '$', (size_t)-1);
    if (p == NULL) p = buf;
    PosWriteFile(1, buf, p - buf, &writtenbytes);
    return ('$');
}

/* INT 21/AH=0Eh */
unsigned int PosSelectDisk(unsigned int drive)
{
    unsigned int ret;

    currentDrive = drive;

    if (drive < 2)
    {
        accessDisk(drive);
    }

    cwd = disks[drive].cwd;

    ret = lastDrive;

    if (ret < 5)
    {
        ret = 5;
    }

    return (ret);
}

/* INT 21/AH=19h */
unsigned int PosGetDefaultDrive(void)
{
    return (currentDrive);
}

/* INT 21/AH=1Ah */
void PosSetDTA(void *p)
{
    dta = p;
    return;
}

/* INT 21/AH=25h */
void PosSetInterruptVector(int intnum, void *handler)
{
    disable();
    *((void **)0 + intnum) = handler;
    enable();
}

/* INT 21/AH=2Ah */
void PosGetSystemDate(int *year, int *month, int *day, int *dw)
{
    int c,y,m,d;
    int retval;

    retval = BosGetSystemDate(&c,&y,&m,&d);

    if(retval == 0)
    {
        *year = bcd2int(c) * 100 + bcd2int(y);
        *month = bcd2int(m);
        *day = bcd2int(d);
        *dw = dow(*year,*month,*day);
    }

    return;
}

/* INT 21/AH=2Bh */
unsigned int PosSetSystemDate(int year, int month, int day)
{
    BosSetSystemDate(year / 100,year % 100,month,day);

    return 0;
}

/* INT 21/AH=2Ch */
void PosGetSystemTime(unsigned int *hour, unsigned int *minute,
                      unsigned int *second, unsigned int *hundredths)
{
    unsigned long ticks,t;
    unsigned int midnight;

    BosGetSystemTime(&ticks,&midnight);

    t=(ticks*1000)/182;
    *hundredths=(unsigned int)t%100;
    t/=100;
    *second=(unsigned int)t%60;
    t/=60;
    *minute=(unsigned int)t%60;
    t/=60;
    *hour=(unsigned int)t;
    return;
}

void *PosGetDTA(void)
{
    return (dta);
}

unsigned int PosGetDosVersion(void)
{
    return (reportedDosVersion);
}

void PosSetDosVersion(unsigned int version)
{
    reportedDosVersion = version;
}

void PosSetLogUnimplemented(int flag)
{
    logUnimplementedFlag = !!flag;
}

int PosGetLogUnimplemented(void)
{
    return logUnimplementedFlag;
}

void PosSetBreakFlag(int flag)
{
    breakFlag = !!flag;
}

int PosGetBreakFlag(void)
{
    return breakFlag;
}

void PosSetVerifyFlag(int flag)
{
    verifyFlag = !!flag;
}

int PosGetVerifyFlag(void)
{
    return verifyFlag;
}

int PosGetMagic(void)
{
    return PDOS_MAGIC;
}

void *PosGetInterruptVector(int intnum)
{
    return *((void **)0 + intnum);
}

/*To find out the free space in hard disk given by drive*/
void PosGetFreeSpace(int drive,
                     unsigned int *secperclu,
                     unsigned int *numfreeclu,
                     unsigned int *bytpersec,
                     unsigned int *totclus)
{
    if (drive==0)
    {
        drive=currentDrive;
    }
    else
    {
        drive--;
    }
    if ((drive >= 0) && (drive < lastDrive))
    {
        *secperclu=4;
        *numfreeclu=25335;
        *bytpersec=512;
        *totclus=25378;
    }
    else if (drive >=lastDrive)
    {
        *secperclu=0xFFFF;
    }
    return;
}
/**/

int PosMakeDir(const char *dname)
{
    int ret;
    char dirname[MAX_PATH];

    ret = formatcwd(dname, dirname);
    if (ret) return (ret);
    ret = dirCreat(dirname, DIRENT_SUBDIR);

    return (ret);
}

int PosRemoveDir(const char *dname)
{
    int ret;
    char dirname[MAX_PATH];

    ret = formatcwd(dname, dirname);
    if (ret) return (ret);
    ret = dirDelete(dirname);

    return (ret);
}

int PosChangeDir(const char *to)
{
    char newcwd[MAX_PATH];
    int attr;
    int ret;

    ret = formatcwd(to, newcwd);
    if (ret) return (ret);
    /* formatcwd gave us "[drive]:\[rest]" but we want only the [rest],
     * so we use to to point at the [rest]. */
    to = newcwd + 3;

    /* formatcwd also provided us with tempDrive from the path we gave it. */
    ret = fatGetFileAttributes(&disks[tempDrive].fat, to, &attr);
    if (ret || !(attr & DIRENT_SUBDIR)) return (POS_ERR_PATH_NOT_FOUND);

    /* If to is "", we should just change to root
     * by copying newcwd into cwd. */
    if (strcmp(to, "") == 0) strcpy(disks[tempDrive].cwd, to);
    /* fatPosition provides us with corrected path with LFNs
     * where possible and correct case, so we use it as cwd. */
    else strcpy(disks[tempDrive].cwd, disks[tempDrive].fat.corrected_path);

    return (0);
}

int PosCreatFile(const char *name, int attrib, int *handle)
{
    char filename[MAX_PATH];
    int ret;

    ret = formatcwd(name, filename);
    if (ret) return (ret);
    return (fileCreat(filename, attrib, handle));
}

int PosOpenFile(const char *name, int mode, int *handle)
{
    char filename[MAX_PATH];
    int ret;

    ret = formatcwd(name, filename);
    if (ret) return (ret);
    return (fileOpen(filename, handle));
}

int PosCloseFile(int fno)
{
    int ret;

    ret = fileClose(fno);
    return (ret);
}

int PosReadFile(int fh, void *data, size_t bytes, size_t *readbytes)
{
    unsigned char *p;
    size_t x = 0;
    int ret;

    if (fh < NUM_SPECIAL_FILES)
    {
        p = (unsigned char *)data;
        for (; x < bytes; x++)
        {
            int scan;
            int ascii;

            BosReadKeyboardCharacter(&scan, &ascii);
            p[x] = ascii;
            BosWriteText(0, ascii, 0);
            if (ascii == '\r')
            {
                BosWriteText(0, '\n', 0);
                /* NB: this will need to be fixed, potential
                buffer overflow - bummer! */
                x++;
                p[x] = '\n';
                x++;
                break;
            }
        }
        *readbytes = x;
        ret = 0;
    }
    else
    {
        ret = fileRead(fh, data, bytes, readbytes);
    }
    return (ret);
}

int PosWriteFile(int fh, const void *data, size_t len, size_t *writtenbytes)
{
    unsigned char *p;
    size_t x;
    int ret;

    if (fh < NUM_SPECIAL_FILES)
    {
        p = (unsigned char *)data;
        for (x = 0; x < len; x++)
        {
            BosWriteText(0, p[x], 0);
        }
        *writtenbytes = len;
        ret = 0;
    }
    else
    {
        ret = fileWrite(fh, data, len, writtenbytes);
    }
    return (ret);
}

/* To delete a given file with fname as its filename */
int PosDeleteFile(const char *name)
{
    char filename[MAX_PATH];
    int ret;

    ret = formatcwd(name, filename);
    if (ret) return (ret);
    ret = fileDelete(filename);
    if (ret < 0)
    {
        return(ret);
    }
    return (ret);
}
/**/

int PosMoveFilePointer(int handle, long offset, int whence, long *newpos)
{
    /* Checks if a valid handle was provided. */
    if (handle < NUM_SPECIAL_FILES || handle >= MAXFILES
        || !(fhandle[handle].inuse))
    {
        return (POS_ERR_INVALID_HANDLE);
    }
    /* Checks if whence value is valid. */
    if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
    {
        /* MSDOS returns 0x1 (function number invalid). */
        return (POS_ERR_FUNCTION_NUMBER_INVALID);
    }
    *newpos = fatSeek(fhandle[handle].fatptr, &(fhandle[handle].fatfile),
                    offset, whence);
    return (0);
}

/*To get the attributes of a given file*/
int PosGetFileAttributes(const char *fnm,int *attr)
{
    int rc;
    char tempf[FILENAME_MAX];

    rc = formatcwd(fnm, tempf);
    if (rc) return (rc);
    fnm = tempf;
    rc = fatGetFileAttributes(&disks[tempDrive].fat,tempf + 2,attr);
    return (rc);
}
/**/

/*To set the attributes of a given file*/
int PosSetFileAttributes(const char *fnm,int attr)
{
    int rc;
    char tempf[FILENAME_MAX];

    rc = formatcwd(fnm, tempf);
    if (rc) return (rc);
    fnm = tempf;
    rc = fatSetFileAttributes(&disks[tempDrive].fat,tempf + 2,attr);
    return (rc);
}
/**/

int PosGetDeviceInformation(int handle, unsigned int *devinfo)
{
    *devinfo = 0;
    if (handle < NUM_SPECIAL_FILES)
    {
        if (handle == 0)
        {
            *devinfo |= (1 << 0); /* standard input device */
        }
        else if ((handle == 1) || (handle == 2))
        {
            *devinfo |= (1 << 1); /* standard output device */
        }
        *devinfo |= (1 << 7); /* character device */
    }
    else
    {
        /* block device (disk file) */
    }
    return (0);
}

int PosBlockDeviceRemovable(int drive)
{
    if (drive < 2)
    {
        return (0);
    }
    else
    {
        return (1);
    }
}
/*Gets the status of remote device if error returns the error code*/
int PosBlockDeviceRemote(int drive,int *da)
{
    *da=0;
    return (0);
}
/**/

/*Implementation of the function call 440D*/
int PosGenericBlockDeviceRequest(int drive,int catcode,int function,
                                void *parm_block)
{
    PB_1560 *pb;

    if (drive == 0)
    {
        drive = currentDrive;
    }
    else
    {
        drive--;
    }
    pb=parm_block;
    memcpy(&pb->bpb, &disks[drive].bpb, sizeof pb->bpb);
    return (0);
}
/**/

int PosDuplicateFileHandle(int fh)
{
    int x;

    if (fh >= MAXFILES || fh < 0)
    {
        return (-POS_ERR_INVALID_HANDLE);
    }

    for (x = NUM_SPECIAL_FILES; x < MAXFILES; x++)
    {
        if (!fhandle[x].inuse)
        {
            break;
        }
    }
    if (x == MAXFILES) return (-POS_ERR_MANY_OPEN_FILES);

    fhandle[x] = fhandle[fh];

    return (x);
}

int PosForceDuplicateFileHandle(int fh, int newfh)
{
    if (fh >= MAXFILES || fh < 0 || newfh >= MAXFILES || newfh < 0)
    {
        return (POS_ERR_INVALID_HANDLE);
    }

    if (fhandle[newfh].inuse) fileClose(newfh);

    fhandle[newfh] = fhandle[fh];

    return (0);
}

int PosGetCurDir(int drive, char *buf)
{
    if (drive == 0)
    {
        drive = currentDrive;
    }
    else
    {
        drive--;
    }
    strcpy(buf, disks[drive].cwd);
    return (0);
}

#ifdef __32BIT__
void *PosAllocMem(unsigned int size, unsigned int flags)
{
    unsigned int subpool;

    subpool = flags & 0xff;
    if ((flags & LOC_MASK) == LOC20)
    {
        return (memmgrAllocate(&btlmem, size, subpool));
    }
    return (memmgrAllocate(&memmgr, size, subpool));
}
#endif

#ifdef __32BIT__
void *PosAllocMemPages(unsigned int pages, unsigned int *maxpages)
{
    void *p;

    p = memmgrAllocate(&memmgr, pages * 16, 0);
    if (p == NULL)
    {
        *maxpages = memmgrMaxSize(&memmgr) / 16;
    }
    return (p);
}
#else
void *PosAllocMemPages(unsigned int pages, unsigned int *maxpages)
{
    void *p;

    p = pdos16MemmgrAllocPages(&memmgr, pages, 0);
    if (p == NULL)
    {
        *maxpages = memmgrMaxSize(&memmgr);
    }
    return (p);
}
#endif

int PosFreeMem(void *ptr)
{
#ifdef __32BIT__
    if ((unsigned long)ptr < 0x0100000)
    {
        memmgrFree(&btlmem, ptr);
        return (0);
    }
#endif
    memmgrFree(&memmgr, ptr);
    return (0);
}

#ifdef __32BIT__
int PosReallocPages(void *ptr, unsigned int newpages, unsigned int *maxp)
{
    int ret;

    ret = memmgrRealloc(&memmgr, ptr, newpages * 16);
    if (ret != 0)
    {
        *maxp = memmgrMaxSize(&memmgr) / 16;
        ret = 8;
    }
    return (ret);
}
#else
int PosReallocPages(void *ptr, unsigned int newpages, unsigned int *maxp)
{
    int ret;

    ret = pdos16MemmgrReallocPages(&memmgr, ptr, newpages);
    if (ret != 0)
    {
        *maxp = memmgrMaxSize(&memmgr);
        ret = 8;
    }
    return (ret);
}
#endif

void PosExec(char *prog, void *parmblock)
{
    PARMBLOCK *parms = parmblock;
    char tempp[FILENAME_MAX];

    if (formatcwd(prog, tempp)) return;
    /* +++Add a way to let user know it failed on formatcwd. */
    prog = tempp;
    loadExe(prog, parms);
    return;
}

void PosTerminate(int rc)
{
    callwithbypass(rc);
    return;
}

#ifndef __32BIT__
void exit(int rc)
{
    PosTerminate(rc);
}
#endif

int PosGetReturnCode(void)
{
    return (lastrc);
}

int PosFindFirst(char *pat, int attrib)
{
    int ret;
    FFBLK *ffblk;

    attr = attrib;
    memset(dta, '\0', 0x15); /* clear out reserved area */
    make_ff(pat);
    ret = fileOpen(ff_path, &ff_handle);
    if (ret) return (3);
    ret = ff_search();
    if (ret == 0x12)
    {
        ret = 2;
    }

    /*On multiple FindFirst calls the contents in the DTA are erased,
      to avoid this the ff_pat formed by the current FindFirst call
      is stored in a temporary buffer and set in DTA so that proper
      data gets populated DTA*/
    ffblk = (FFBLK *) dta;
    ffblk->handle = ff_handle;
    strcpy(ffblk->pat, ff_pat);

    return (ret);
}

int PosFindNext(void)
{
    FFBLK *ffblk;

    ffblk = (FFBLK *) dta;
    ff_handle = ffblk->handle;
    strcpy(ff_pat, ffblk->pat);

    return (ff_search());
}

/* To rename a given file */
int PosRenameFile(const char *old, const char *new)
{
    int rc;
    char tempf1[FILENAME_MAX];
    char tempf2[FILENAME_MAX];
    /* +++Add support for moving files using rename. */

    rc = formatcwd(old,tempf1);
    if (rc) return (rc);
    strcpy(tempf2, new);

    old = tempf1;
    new = tempf2;

    rc = fatRenameFile(&disks[tempDrive].fat, tempf1 + 2,new);
    return (rc);
}
/**/

/*Get last written date and time*/
int PosGetFileLastWrittenDateAndTime(int handle,
                                     unsigned int *fdate,
                                     unsigned int *ftime)
{
    FATFILE *fatfile;

    fatfile=&fhandle[handle].fatfile;
    *fdate=fatfile->fdate;
    *ftime=fatfile->ftime;
    return 0;
}
/**/

/*Set the last written date and time for a given file*/
int PosSetFileLastWrittenDateAndTime(int handle,
                                     unsigned int fdate,
                                     unsigned int ftime)
{
    FATFILE *fatfile;
    FAT *fat;

    fatfile=&fhandle[handle].fatfile;
    fat=fhandle[handle].fatptr;
    fatfile->fdate=fdate;
    fatfile->ftime=ftime;
    fatUpdateDateAndTime(fat,fatfile);
    return 0;
}
/**/

int PosCreatNewFile(const char *name, int attrib, int *handle)
{
    int ret;
    char filename[MAX_PATH];

    ret = formatcwd(name, filename);
    if (ret) return (ret);
    return (newFileCreat(filename, attrib, handle));
}

int PosTruename(char *prename,char *postname)
{
    formatcwd(prename,postname);
    return(0);
}

void PosDisplayInteger(int x)
{
    printf("integer is %x\n", x);
#ifdef __32BIT__
    printf("normalized is %x\n", SUBADDRFIX(x));
    printf("absolute is %x\n", SUB2ABS(x));
#endif
    return;
}

#ifdef __32BIT__
void PosReboot(void)
{
    unsigned short newregs[11];

    runreal_p(doreboot, 0);
    return;
}
#else
void PosReboot(void)
{
    int *posttype = (int *)0x472;
    void (*postfunc)(void) = (void (*)(void))MK_FP(0xffff, 0x0000);

    *posttype = 0x1234;
    postfunc();
    return;
}
#endif

#ifdef __32BIT__
void PosSetRunTime(void *pstart, void *c_api)
{
    exec_pstart = pstart;
    exec_c_api = c_api;
#ifdef __32BIT__
    exec_subcor = subcor;
#endif
    return;
}
#endif

/* !!! END OF POS FUNCTIONS !!! */


static void make_ff(char *pat)
{
    char *p;

    strcpy(ff_pat, pat);
    upper_str(ff_pat);
    scrunchf(ff_path, ff_pat);
    p = strrchr(ff_path, '\\');
    if (p != NULL)
    {
        strcpy(ff_pat, p + 1);
        *p = '\0';
    }
    else
    {
        p = strrchr(ff_path, ':');
        if (p != NULL)
        {
            strcpy(ff_pat, p + 1);
            *(p + 1) = '\0';
        }
        else
        {
            strcpy(ff_pat, ff_path);
            ff_path[0] = '\0';
        }
    }
    if (strcmp(ff_pat, "") == 0)
    {
        strcpy(ff_pat, "*.*");
    }
    return;
}

static void scrunchf(char *dest, char *new)
{
    char *mycwd;
    char *p;
    int drive;

    strcpy(dest, "");
    mycwd = cwd;
    if (strchr(new, ':') != NULL)
    {
        drive = *new - 'A';
        if (drive < 2)
        {
            accessDisk(drive);
        }
        mycwd = disks[drive].cwd;
        memcpy(dest, new, 2);
        dest[2] = '\0';
        new += 2;
    }
    if ((*new == '/') || (*new == '\\'))
    {
        strcat(dest, new);
    }
    else if ((strchr(new, '\\') != NULL) || (strchr(new, '/') != NULL))
    {
        strcat(dest, "\\");
        strcat(dest, new);
    }
    else
    {
        strcat(dest, mycwd);
        while (memcmp(new, "..", 2) == 0)
        {
            p = strrchr(dest, '\\');
            if (p != NULL)
            {
                *p = '\0';
            }
            else
            {
                strcpy(dest, "\\");
            }
            new += 3;
        }
        strcat(dest, "\\");
        strcat(dest, new);
    }
    return;
}

static int ff_search(void)
{
    size_t readbytes;
    char file[13];
    char *p;
    DIRENT dirent;
    unsigned char lfn[255]; /*+++Add UCS-2 support. */
    unsigned int lfn_len = 0;
    unsigned char checksum;
    /* Upper lfn which is a string. */
    unsigned char testlfn[256];

    fileRead(ff_handle, &dirent, sizeof dirent, &readbytes);
    while ((readbytes == sizeof dirent) && (dirent.file_name[0] != '\0'))
    {
        if (dirent.file_name[0] != DIRENT_DEL)
        {
            if (dirent.file_attr == DIRENT_LFN)
            {
                /* Reads LFN entry and stores the checksum. */
                checksum = readLFNEntry(&dirent, lfn, &lfn_len);
            }
            else
            {
                /* If LFN was found before, checks
                 * if it belongs to this 8.3 entry. */
                if (lfn_len)
                {
                    /* Checks if the checksum is correct for this entry. */
                    if (checksum == generateChecksum(dirent.file_name))
                    {
                        /* If it is correct, copies LFN, makes it string
                         * and uppers it so it is compatible with patmat. */
                        memcpy(testlfn, lfn, lfn_len);
                        testlfn[lfn_len] = '\0';
                        /* +++Make UCS-2 compatible, because upper_str
                         * uses only char, not unsigned int. Also patmat
                         * is not yet UCS-2 compatible. */
                        upper_str(testlfn);
                    }
                    /* If it is not, makes sure that
                     * this LFN will not be used again. */
                    else lfn_len = 0;
                }
                memcpy(file, dirent.file_name, sizeof(dirent.file_name));
                file[sizeof(dirent.file_name)] = '\0';
                p = strchr(file, ' ');
                if (p != NULL)
                {
                    *p = '\0';
                }
                else
                {
                    p = file + strlen(file);
                }
                *p++ = '.';
                memcpy(p, dirent.file_ext,sizeof(dirent.file_ext));
                p[3] = '\0';
                p = strchr(file, ' ');
                if (p != NULL)
                {
                    *p-- = '\0';
                }
                if ((patmat(file, ff_pat) ||
                    /* If it has LFN, it is enough if only one of them fits. */
                     (lfn_len && patmat(testlfn, ff_pat)))

                    /* if it is not a directory, or they asked for
                    directories, then that is OK */
                    && (((dirent.file_attr & DIRENT_SUBDIR) == 0)
                        || ((attr & DIRENT_SUBDIR) != 0))

                    /* if it is not a volume label, or they asked
                    for volume labels, then that is OK */
                    && (((dirent.file_attr & DIRENT_EXTRAB3) == 0)
                        || ((attr & DIRENT_EXTRAB3) != 0))

                )
                {
                    if ((p != NULL) && (*p == '.')) *p = '\0';
                    dta->attrib = dirent.file_attr; /* attribute */

                    dta->file_time = dirent.last_modtime[0]   /*time*/
                    | ((unsigned int)dirent.last_modtime[1] << 8);

                    dta->file_date = dirent.last_moddate[0]   /*date*/
                    | ((unsigned int)dirent.last_moddate[1] << 8);

                    dta->file_size = dirent.file_size[0]      /*size*/
                    | ((unsigned long)dirent.file_size[1] << 8)
                    | ((unsigned long)dirent.file_size[2] << 16)
                    | ((unsigned long)dirent.file_size[3] << 24);

                    dta->startcluster=dirent.start_cluster[0]
                    |(dirent.start_cluster[1]<<8);

                    memset(dta->file_name, '\0', sizeof(dta->file_name));
                    strcpy(dta->file_name, file);

                    /* Checks if this file has LFN associated
                     * and stores it in the DTA. This check
                     * is sufficient, because the check before
                     * sets lfn_len to 0 when it fails. */
                    if (lfn_len)
                    {
                        memcpy(dta->lfn, lfn, lfn_len);
                        /* Adds null terminator at the end of LFN. */
                        dta->lfn[lfn_len] = '\0';
                    }
                    else dta->lfn[0] = '\0';
                    return (0);
                }
            }
        }
        fileRead(ff_handle, &dirent, sizeof dirent, &readbytes);
    }
    fileClose(ff_handle);
    return (0x12);
}

#ifdef __32BIT__
/* registers come in as eax, ebx, ecx, edx, esi, edi, cflag */
int int20(unsigned int *regs)
{
    static union REGS regsin;
    static union REGS regsout;
    static struct SREGS sregs;

    regsin.d.eax = regs[0];
    regsin.d.ebx = regs[1];
    regsin.d.ecx = regs[2];
    regsin.d.edx = regs[3];
    regsin.d.esi = regs[4];
    regsin.d.edi = regs[5];
    regsin.d.cflag = 0;
    memcpy(&regsout, &regsin, sizeof regsout);
    PosTermNoRC();
    regs[0] = regsout.d.eax;
    regs[1] = regsout.d.ebx;
    regs[2] = regsout.d.ecx;
    regs[3] = regsout.d.edx;
    regs[4] = regsout.d.esi;
    regs[5] = regsout.d.edi;
    regs[6] = regsout.d.cflag;
    return (0);
}

int int21(unsigned int *regs)
{
    static union REGS regsin;
    static union REGS regsout;
    static struct SREGS sregs;

    regsin.d.eax = regs[0];
    regsin.d.ebx = regs[1];
    regsin.d.ecx = regs[2];
    regsin.d.edx = regs[3];
    regsin.d.esi = regs[4];
    regsin.d.edi = regs[5];
    regsin.d.cflag = 0;
    memcpy(&regsout, &regsin, sizeof regsout);
    int21handler(&regsin, &regsout, &sregs);
    regs[0] = regsout.d.eax;
    regs[1] = regsout.d.ebx;
    regs[2] = regsout.d.ecx;
    regs[3] = regsout.d.edx;
    regs[4] = regsout.d.esi;
    regs[5] = regsout.d.edi;
    regs[6] = regsout.d.cflag;
    return (0);
}

/* INT 25 - Absolute Disk Read */
int int25(unsigned int *regs)
{
    static union REGS regsin;
    static union REGS regsout;
    static struct SREGS sregs;
    DP *dp;
    void *p;

    regsin.d.eax = regs[0];
    regsin.d.ebx = regs[1];
    regsin.d.ecx = regs[2];
    regsin.d.edx = regs[3];
    regsin.d.esi = regs[4];
    regsin.d.edi = regs[5];
    regsin.d.cflag = 0;
    memcpy(&regsout, &regsin, sizeof regsout);
    dp=SUBADDRFIX(regsin.d.ebx);
    p = SUBADDRFIX(dp->transferaddress);
    PosAbsoluteDiskRead(regsin.h.al,dp->sectornumber,dp->numberofsectors,p);
    regs[0] = regsout.d.eax;
    regs[1] = regsout.d.ebx;
    regs[2] = regsout.d.ecx;
    regs[3] = regsout.d.edx;
    regs[4] = regsout.d.esi;
    regs[5] = regsout.d.edi;
    regs[6] = regsout.d.cflag;
    return (0);
}
/* INT 26 - Absolute Disk Write */
int int26(unsigned int *regs)
{
    static union REGS regsin;
    static union REGS regsout;
    static struct SREGS sregs;
    DP *dp;
    void *p;

    regsin.d.eax = regs[0];
    regsin.d.ebx = regs[1];
    regsin.d.ecx = regs[2];
    regsin.d.edx = regs[3];
    regsin.d.esi = regs[4];
    regsin.d.edi = regs[5];
    regsin.d.cflag = 0;
    memcpy(&regsout, &regsin, sizeof regsout);
    dp=SUBADDRFIX(regsin.d.ebx);
    p = SUBADDRFIX(dp->transferaddress);
    PosAbsoluteDiskWrite(regsin.h.al,dp->sectornumber,dp->numberofsectors,p);
    regs[0] = regsout.d.eax;
    regs[1] = regsout.d.ebx;
    regs[2] = regsout.d.ecx;
    regs[3] = regsout.d.edx;
    regs[4] = regsout.d.esi;
    regs[5] = regsout.d.edi;
    regs[6] = regsout.d.cflag;
    return (0);
}
/**/
#else
void int20(unsigned int *regptrs,
        unsigned int es,
        unsigned int ds,
        unsigned int di,
        unsigned int si,
        unsigned int dx,
        unsigned int cx,
        unsigned int bx,
        unsigned int cflag,
        unsigned int ax)
{
    static union REGS regsin;
    static union REGS regsout;
    static struct SREGS sregs;

    regsin.x.ax = ax;
    regsin.x.bx = bx;
    regsin.x.cx = cx;
    regsin.x.dx = dx;
    regsin.x.si = si;
    regsin.x.di = di;
    regsin.x.cflag = 0;
    sregs.ds = ds;
    sregs.es = es;
    memcpy(&regsout, &regsin, sizeof regsout);
    PosTermNoRC();
    regptrs[0] = sregs.es;
    regptrs[1] = sregs.ds;
    regptrs[2] = regsout.x.di;
    regptrs[3] = regsout.x.si;
    regptrs[4] = regsout.x.dx;
    regptrs[5] = regsout.x.cx;
    regptrs[6] = regsout.x.bx;
    regptrs[7] = regsout.x.cflag;
    regptrs[8] = regsout.x.ax;
    return;
}

void int21(unsigned int *regptrs,
        unsigned int es,
        unsigned int ds,
        unsigned int di,
        unsigned int si,
        unsigned int dx,
        unsigned int cx,
        unsigned int bx,
        unsigned int cflag,
        unsigned int ax)
{
    static union REGS regsin;
    static union REGS regsout;
    static struct SREGS sregs;

    regsin.x.ax = ax;
    regsin.x.bx = bx;
    regsin.x.cx = cx;
    regsin.x.dx = dx;
    regsin.x.si = si;
    regsin.x.di = di;
    regsin.x.cflag = 0;
    sregs.ds = ds;
    sregs.es = es;
    memcpy(&regsout, &regsin, sizeof regsout);
    int21handler(&regsin, &regsout, &sregs);
    regptrs[0] = sregs.es;
    regptrs[1] = sregs.ds;
    regptrs[2] = regsout.x.di;
    regptrs[3] = regsout.x.si;
    regptrs[4] = regsout.x.dx;
    regptrs[5] = regsout.x.cx;
    regptrs[6] = regsout.x.bx;
    regptrs[7] = regsout.x.cflag;
    regptrs[8] = regsout.x.ax;
    return;
}

/* INT 25 - Absolute Disk Read */
void int25(unsigned int *regptrs,
        unsigned int es,
        unsigned int ds,
        unsigned int di,
        unsigned int si,
        unsigned int dx,
        unsigned int cx,
        unsigned int bx,
        unsigned int cflag,
        unsigned int ax)
{
    static union REGS regsin;
    static union REGS regsout;
    static struct SREGS sregs;
    DP *dp;
    void *p;

    regsin.x.ax = ax;
    regsin.x.bx = bx;
    regsin.x.cx = cx;
    regsin.x.dx = dx;
    regsin.x.si = si;
    regsin.x.di = di;
    regsin.x.cflag = 0;
    sregs.ds = ds;
    sregs.es = es;
    memcpy(&regsout, &regsin, sizeof regsout);
    dp=MK_FP(sregs.ds,regsin.x.bx);
    p=dp->transferaddress;
    PosAbsoluteDiskRead(regsin.h.al,dp->sectornumber,dp->numberofsectors,p);
    regptrs[0] = sregs.es;
    regptrs[1] = sregs.ds;
    regptrs[2] = regsout.x.di;
    regptrs[3] = regsout.x.si;
    regptrs[4] = regsout.x.dx;
    regptrs[5] = regsout.x.cx;
    regptrs[6] = regsout.x.bx;
    regptrs[7] = regsout.x.cflag;
    regptrs[8] = regsout.x.ax;
    return;
}
/**/
/* INT 26 - Absolute Disk Write */
void int26(unsigned int *regptrs,
        unsigned int es,
        unsigned int ds,
        unsigned int di,
        unsigned int si,
        unsigned int dx,
        unsigned int cx,
        unsigned int bx,
        unsigned int cflag,
        unsigned int ax)
{
    static union REGS regsin;
    static union REGS regsout;
    static struct SREGS sregs;
    DP *dp;
    void *p;

    regsin.x.ax = ax;
    regsin.x.bx = bx;
    regsin.x.cx = cx;
    regsin.x.dx = dx;
    regsin.x.si = si;
    regsin.x.di = di;
    regsin.x.cflag = 0;
    sregs.ds = ds;
    sregs.es = es;
    memcpy(&regsout, &regsin, sizeof regsout);
    dp=MK_FP(sregs.ds,regsin.x.bx);
    p=dp->transferaddress;
    PosAbsoluteDiskWrite(regsin.h.al,dp->sectornumber,dp->numberofsectors,p);
    regptrs[0] = sregs.es;
    regptrs[1] = sregs.ds;
    regptrs[2] = regsout.x.di;
    regptrs[3] = regsout.x.si;
    regptrs[4] = regsout.x.dx;
    regptrs[5] = regsout.x.cx;
    regptrs[6] = regsout.x.bx;
    regptrs[7] = regsout.x.cflag;
    regptrs[8] = regsout.x.ax;
    return;
}
/**/
#endif

static void loadConfig(void)
{
    size_t readbytes;
    unsigned char buf[512];
    int x;
    int fh;
    int ret;

    ret = fileOpen("CONFIG.SYS", &fh);
    if (!ret)
    {
        do
        {
            fileRead(fh, buf, sizeof buf, &readbytes);
            for (x = 0; x < readbytes; x++)
            {
                if (memcmp(buf, "SHELL=", 6) == 0)
                {
                    char *p;

                    memcpy(shell, buf + 6, sizeof shell);
                    shell[sizeof shell - 1] = '\0';
                    p = strchr(shell, '\r');
                    if (p != NULL)
                    {
                        *p = '\0';
                    }
                }
                BosWriteText(0, buf[x], 0);
            }
        } while (readbytes == 0x200);
        fileClose(fh);
    }
    return;
}

static void loadPcomm(void)
{
    static PARMBLOCK p = { 0, "\x2/p\r", NULL, NULL };
    static PARMBLOCK altp = { 0, "\x0\r", NULL, NULL };

    if (strcmp(shell, "") == 0)
    {
        loadExe("COMMAND.COM", &p);
    }
    else
    {
        loadExe(shell, &altp);
    }
    return;
}


/* loadExe - load an executable into memory */
/* 1. read first 10 bytes of header to get header len */
/* 2. allocate room for header */
/* 3. load rest of header */
/* 4. allocate room for environment */
/* 5. allocate room for executable + psp */
/* 6. load executable */
/* 7. fix up executable using relocation table */
/* 8. extract SS + SP from header */
/* 9. call executable */

/* for 32-bit we make the following mods: */
/* 1. header is larger */
/* 6. executable needs 2-stage load */

#ifdef __32BIT__
#include "a_out.h"
#endif

static void loadExe(char *prog, PARMBLOCK *parmblock)
{
    static int first = 1;
    int doing_pcomm = 0;
#ifdef __32BIT__
    struct exec firstbit;
#else
    unsigned char firstbit[10];
#endif
    unsigned int headerLen;
    unsigned char *header = NULL;
    unsigned char *envptr;
    unsigned long exeLen;
    unsigned char *psp;
    unsigned char *origpsp;
    unsigned char *origenv;
    unsigned char *exeStart;
    unsigned int numReloc;
    unsigned long *relocStart;
    unsigned int relocI;
    unsigned int *fixSeg;
    unsigned int addSeg;
    unsigned int ss;
    unsigned int sp;
    unsigned char *exeEntry;
    unsigned int maxPages;
    int fno;
    size_t readbytes;
    int ret;
    unsigned char *bss;
    int y;
    int isexe = 0;
    char *olddta;
    unsigned char *saveCurProc;

    if (fileOpen(prog, &fno)) return;
#ifdef __32BIT__
    fileRead(fno, &firstbit, sizeof firstbit, &readbytes);
    if (first)
    {
        first = 0;
        doing_pcomm = 0;
    }
    if (doing_pcomm)
    {
        /* this logic only applies to executables built with EMX,
           ie only PCOMM. All other cross-compiled apps do not
           have NULs after the firstbit that need to be skipped */
        headerLen = N_TXTOFF(firstbit);
        header = memmgrAllocate(&memmgr, headerLen, 0);
        memcpy(header, &firstbit, sizeof firstbit);
        fileRead(fno, header + sizeof firstbit, headerLen - sizeof firstbit,
                 &readbytes);
    }
#else
    fileRead(fno, firstbit, sizeof firstbit, &readbytes);
    if (memcmp(firstbit, "MZ", 2) == 0)
    {
        isexe = 1;
        headerLen = *(unsigned int *)&firstbit[8];
        headerLen *= 16;
        header = memmgrAllocate(&memmgr, headerLen, 0);
        memcpy(header, firstbit, sizeof firstbit);
        fileRead(fno, header + sizeof firstbit, headerLen - sizeof firstbit,
                 &readbytes);
    }
#endif

    envptr = memmgrAllocate(&memmgr,
                            strlen(prog) + 7 + sizeof(unsigned short), 0);
#ifndef __32BIT__
    origenv = envptr;
    envptr = (unsigned char *)FP_NORM(envptr);
#endif
    memcpy(envptr, "A=B\0\0", 5);
    *((unsigned short *)(envptr + 5)) = 1; /* who knows why */
    memcpy(envptr + 5 + sizeof(unsigned short), prog, strlen(prog) + 1);

#ifdef __32BIT__
    if (doing_pcomm)
    {
        exeLen = N_BSSADDR(firstbit) - N_TXTADDR(firstbit) + firstbit.a_bss;
    }
    else
    {
        exeLen = firstbit.a_text + firstbit.a_data + firstbit.a_bss;
    }
    /* allocate exeLen + 0x100 (psp) + stack + extra (safety margin) */
    psp = memmgrAllocate(&memmgr, exeLen + 0x100 + STACKSZ32 + 0x100, 0);
#else
    if (isexe)
    {
        exeLen = *(unsigned int *)&header[4];
        exeLen = (exeLen + 1) * 512 - headerLen;
    }
    else
    {
        exeLen = 0x10000;
    }
    maxPages = memmgrMaxSize(&memmgr);
    if ((long)maxPages * 16 < exeLen)
    {
        printf("insufficient memory to load program\n");
        printf("required %ld, available %ld\n",
            exeLen, (long)maxPages * 16);
        memmgrFree(&memmgr, header);
        memmgrFree(&memmgr, envptr);
        return;
    }
    psp = pdos16MemmgrAllocPages(&memmgr, maxPages, 0);
    origpsp = psp;
    psp = (unsigned char *)FP_NORM(psp);
#endif
    if (psp == NULL)
    {
        printf("insufficient memory to load program\n");
        if (header != NULL) memmgrFree(&memmgr, header);
        memmgrFree(&memmgr, envptr);
        return;
    }

    /* set various values in the psp */
    psp[0] = 0xcd;
    psp[1] = 0x20;
#ifndef __32BIT__
    *(unsigned int *)&psp[0x2c] = FP_SEG(envptr);
#endif
    /* set to say 640k in use */
    *(unsigned int *)(psp + 2) = FP_SEG(psp) + maxPages;
    if (parmblock != NULL)
    {
        /* 1 for the count and 1 for the return */
        memcpy(psp + 0x80, parmblock->cmdtail, parmblock->cmdtail[0] + 1 + 1);
    }

    exeStart = psp + 0x100;
#ifndef __32BIT__
    exeStart = (unsigned char *)FP_NORM(exeStart);
#endif

#ifdef __32BIT__
    fileRead(fno, exeStart, firstbit.a_text, &readbytes);
    if (doing_pcomm)
    {
        fileRead(fno,
                exeStart + N_DATADDR(firstbit) - N_TXTADDR(firstbit),
                firstbit.a_data, &readbytes);
    }
    else
    {
        fileRead(fno, exeStart + firstbit.a_text, firstbit.a_data, &readbytes);
    }
#else
    if (isexe)
    {
        unsigned int maxread = 32768;
        unsigned long totalRead = 0;
        char *upto;

        while (totalRead < exeLen)
        {
            if ((exeLen - totalRead) < maxread)
            {
                maxread = exeLen - totalRead;
            }
            upto = ABS2ADDR(ADDR2ABS(exeStart) + totalRead);
            upto = FP_NORM(upto);
            fileRead(fno, upto, maxread, &readbytes);
            totalRead += maxread;
        }
    }
    else
    {
        memcpy(exeStart, firstbit, sizeof firstbit);
        fileRead(fno, exeStart + sizeof firstbit, 32768, &readbytes);
        fileRead(fno, FP_NORM(exeStart + sizeof firstbit + 32768), 32768,
                 &readbytes);
    }
#endif
    fileClose(fno);

#ifndef __32BIT__
    if (isexe)
    {
        numReloc = *(unsigned int *)&header[6];
        relocStart = (unsigned long *)(header +
                *(unsigned int *)&header[0x18]);
        addSeg = FP_SEG(exeStart);
        for (relocI = 0; relocI < numReloc; relocI++)
        {
            /* This 16:16 arithmetic will work because the exeStart
            offset is 0. */
            fixSeg = (unsigned int *)
                    ((unsigned long)exeStart + relocStart[relocI]);
            *fixSeg = *fixSeg + addSeg;
        }

        ss = *(unsigned int *)&header[0xe];
        ss += addSeg;
        sp = *(unsigned int *)&header[0x10];

        /* This 16:16 arithmetic will work because the exeStart
        offset is 0 */
        exeEntry = (unsigned char *)((unsigned long)exeStart
                                    + *(unsigned long *)&header[0x14]);
    }
    else
    {
        ss = FP_SEG(psp);
        sp = 0xfffe;
        *(unsigned int *)MK_FP(ss, sp) = 0;
        exeEntry = psp + 0x100;
    }

    /* printf("exeEntry: %lx, psp: %lx, ss: %x, sp: %x\n",
        exeEntry, psp, ss, sp); */
#else
    /* initialise BSS */
    if (doing_pcomm)
    {
        bss = exeStart + N_BSSADDR(firstbit);
    }
    else
    {
        bss = exeStart + firstbit.a_text + firstbit.a_data;
    }
    for (y = 0; y < firstbit.a_bss; y++)
    {
        bss[y] = '\0';
    }
    if (doing_pcomm)
    {
        sp = N_BSSADDR(firstbit) + firstbit.a_bss + 0x8000;
    }
    else
    {
        sp = (unsigned int)bss + firstbit.a_bss + STACKSZ32;
    }
    if (!doing_pcomm)
    {
        unsigned int *corrections;
        unsigned int i;
        unsigned int offs;
        unsigned int type;
        unsigned int zapdata;
        unsigned char *zap;

        zap = psp + 0x100;
        zapdata = (unsigned int)ADDR2ABS(zap);
        if (firstbit.a_trsize != 0)
        {
            corrections = memmgrAllocate(&memmgr, firstbit.a_trsize, 0);
            if (corrections == NULL)
            {
                printf("insufficient memory %d\n", firstbit.a_trsize);
                memmgrFree(&memmgr, envptr);
                memmgrFree(&memmgr, psp);
                return;
            }
            fileRead(fno, corrections, firstbit.a_trsize, &readbytes);
            for (i = 0; i < firstbit.a_trsize / 4; i += 2)
            {
                offs = corrections[i];
                type = corrections[i + 1];
                if (((type >> 24) & 0xff) != 0x04)
                {
                    continue;
                }
                *(unsigned int *)(zap + offs) += zapdata;
            }
            memmgrFree(&memmgr, corrections);
        }
        if (firstbit.a_drsize != 0)
        {
            corrections = memmgrAllocate(&memmgr, firstbit.a_drsize, 0);
            if (corrections == NULL)
            {
                printf("insufficient memory %d\n", firstbit.a_drsize);
                memmgrFree(&memmgr, envptr);
                memmgrFree(&memmgr, psp);
                return;
            }
            fileRead(fno, corrections, firstbit.a_drsize, &readbytes);
            zap = psp + 0x100 + firstbit.a_text;
            for (i = 0; i < firstbit.a_drsize / 4; i += 2)
            {
                offs = corrections[i];
                type = corrections[i + 1];
                if (((type >> 24) & 0xff) != 0x04)
                {
                    continue;
                }
                *(unsigned int *)(zap + offs) += zapdata;
            }
            memmgrFree(&memmgr, corrections);
        }
        firstbit.a_entry += (unsigned long)ADDR2ABS(psp + 0x100);
    }
#endif

    /* Before executing program, set tsrFlag = 0.
     * If program is a TSR, it will change this to non-zero.
     */
    tsrFlag = 0;

#if (!defined(USING_EXE) && !defined(__32BIT__))
    olddta = dta;
    dta = psp + 0x80;
    saveCurProc = curProc;
    curProc = psp;
    ret = callwithpsp(exeEntry, psp, ss, sp);
    curProc = saveCurProc;
    dta = olddta;
    psp = origpsp;
    envptr = origenv;
#endif
#ifdef __32BIT__
    saveCurProc = curProc;
    curProc = psp;
    ret = fixexe32(psp, firstbit.a_entry, sp, doing_pcomm);
    curProc = saveCurProc;
#endif
    lastrc = ret;

    /*
     * Don't free all of program's memory if it is a TSR. Only the
     * "un-reserved" portion.
     */
    if (tsrFlag == 0)
    {
        if (header != NULL) memmgrFree(&memmgr, header);
        memmgrFree(&memmgr, psp);
        memmgrFree(&memmgr, envptr);
    }
    else
    {
        PosReallocPages(psp, tsrFlag, &maxPages);
    }

    /* Set TSR flag back to 0 */
    tsrFlag = 0;

    return;
}

#ifdef __32BIT__
static int fixexe32(unsigned char *psp, unsigned long entry, unsigned int sp,
                    int doing_pcomm)
{
    char *source;
    char *dest;
    unsigned long dataStart;
    unsigned int *header;
    void *abs;
    struct {
        unsigned short limit_15_0;
        unsigned short base_15_0;
        unsigned char base_23_16;
        unsigned char access;
        unsigned char gran_limit;
        unsigned char base_31_24;
    } *realcode, savecode, *realdata, savedata;
    int ret;
    int oldsubcor;
    unsigned char *commandLine;
    POS_EPARMS exeparms;
    unsigned long exeStart;

    commandLine = psp + 0x80;

    if (doing_pcomm)
    {
        exeStart = (unsigned long)psp + 0x100 - 0x10000;
    }
    else
    {
        exeStart = 0;
        sp = (unsigned int)ADDR2ABS(sp);
    }

    dataStart = exeStart;
    if (doing_pcomm)
    {
        dataStart = (unsigned long)ADDR2ABS(dataStart);
    }

    /* now we need to record the subroutine's absolute offset fix */
    oldsubcor = subcor;
    subcor = dataStart;

    if (doing_pcomm)
    {
        exeStart = (unsigned long)ADDR2ABS(exeStart);
    }

    exeparms.len = sizeof exeparms;
    exeparms.abscor = subcor;
    exeparms.psp = ADDRFIXSUB(psp);
    exeparms.cl = ADDRFIXSUB(commandLine);
    exeparms.callback = 0;
    exeparms.pstart = ADDRFIXSUB(exec_pstart);
    exeparms.crt = exec_c_api;
    exeparms.subcor = exec_subcor;

    /* now we set the GDT entry directly */
    /* after first of all saving the old values */
    realcode = (void *)((char *)gdt + 0x28);
    savecode = *realcode;
    realcode->base_15_0 = exeStart & 0xffff;
    realcode->base_23_16 = (exeStart >> 16) & 0xff;
    realcode->base_31_24 = (exeStart >> 24) & 0xff;

    /* now set the data values */
    realdata = (void *)((char *)gdt + 0x30);
    savedata = *realdata;
    realdata->base_15_0 = dataStart & 0xffff;
    realdata->base_23_16 = (dataStart >> 16) & 0xff;
    realdata->base_31_24 = (dataStart >> 24) & 0xff;

    ret = call32(entry, ADDRFIXSUB(&exeparms), sp);
    subcor = oldsubcor;
    *realcode = savecode;
    *realdata = savedata;
    return (ret);
}
#endif

static int bios2driv(int bios)
{
    int drive;

    if (bios >= 0x80)
    {
        drive = bios - 0x80 + 2;
    }
    else
    {
        drive = bios;
    }
    return (drive);
}

static int fileCreat(const char *fnm, int attrib, int *handle)
{
    int x;
    const char *p;
    int drive;
    int rc;

    p = strchr(fnm, ':');
    if (p == NULL)
    {
        p = fnm;
        drive = currentDrive;
    }
    else
    {
        drive = *(p - 1);
        drive = toupper(drive) - 'A';
        p++;
    }
    for (x = NUM_SPECIAL_FILES; x < MAXFILES; x++)
    {
        if (!fhandle[x].inuse)
        {
            break;
        }
    }
    if (x == MAXFILES) return (-4); /* 4 = too many open files */
    rc = fatCreatFile(&disks[drive].fat, p, &fhandle[x].fatfile, attrib);
    if (rc) return (rc);
    fhandle[x].inuse = 1;
    fhandle[x].fatptr = &disks[drive].fat;
    *handle = x;
    return (0);
}

static int dirCreat(const char *dnm, int attrib)
{
    const char *p;
    int drive;
    int rc;
    char parentname[MAX_PATH];
    char *end;
    char *temp;

    p = strchr(dnm, ':');
    if (p == NULL)
    {
        p = dnm;
        drive = currentDrive;
    }
    else
    {
        drive = *(p - 1);
        drive = toupper(drive) - 'A';
        p++;
    }

    if ((p[0] == '\\') || (p[0] == '/'))
    {
        p++;
    }

    memset(parentname,'\0',sizeof(parentname));
    end = strrchr(p, '\\');
    temp = strrchr(p, '/');

    if(!end || (temp > end))
    {
        end = temp;
    }
    if(end)
    {
        strncpy(parentname,p,(end-p));
    }

    rc = fatCreatDir(&disks[drive].fat, p, parentname, attrib);
    return (rc);
}

static int newFileCreat(const char *fnm, int attrib, int *handle)
{
    int x;
    const char *p;
    int drive;
    int rc;

    p = strchr(fnm, ':');
    if (p == NULL)
    {
        p = fnm;
        drive = currentDrive;
    }
    else
    {
        drive = *(p - 1);
        drive = toupper(drive) - 'A';
        p++;
    }
    for (x = NUM_SPECIAL_FILES; x < MAXFILES; x++)
    {
        if (!fhandle[x].inuse)
        {
            break;
        }
    }
    if (x == MAXFILES) return (-POS_ERR_MANY_OPEN_FILES);
    rc = fatCreatNewFile(&disks[drive].fat, p, &fhandle[x].fatfile, attrib);
    if (rc) return (rc);
    fhandle[x].inuse = 1;
    fhandle[x].fatptr = &disks[drive].fat;
    *handle = x;
    return (0);
}

static int fileOpen(const char *fnm, int *handle)
{
    int x;
    const char *p;
    int drive;
    int rc;

    p = strchr(fnm, ':');
    if (p == NULL)
    {
        p = fnm;
        drive = currentDrive;
    }
    else
    {
        drive = *(p - 1);
        drive = toupper(drive) - 'A';
        p++;
    }
    for (x = NUM_SPECIAL_FILES; x < MAXFILES; x++)
    {
        if (!fhandle[x].inuse)
        {
            break;
        }
    }
    if (x == MAXFILES) return (-POS_ERR_MANY_OPEN_FILES);
    rc = fatOpenFile(&disks[drive].fat, p, &fhandle[x].fatfile);
    if (rc) return (rc);
    fhandle[x].inuse = 1;
    fhandle[x].fatptr = &disks[drive].fat;
    *handle = x;
    return (0);
}

static int fileClose(int fno)
{
    if (!fhandle[fno].special)
    {
        fhandle[fno].inuse = 0;
    }
    return (0);
}

static int fileRead(int fno, void *buf, size_t szbuf, size_t *readbytes)
{
    return (fatReadFile(fhandle[fno].fatptr, &fhandle[fno].fatfile, buf,
                        szbuf, readbytes));
}


static int fileWrite(int fno, const void *buf, size_t szbuf,
                     size_t *writtenbytes)
{
    return (fatWriteFile(fhandle[fno].fatptr, &fhandle[fno].fatfile, buf,
                         szbuf, writtenbytes));
}

/*Fatdelete function to delete a file when fnm is given as filename*/
static int fileDelete(const char *fnm)
{
    int x;
    const char *p;
    int drive;
    int rc;
    int attr;

    p = strchr(fnm, ':');
    if (p == NULL)
    {
        p = fnm;
        drive = currentDrive;
    }
    else
    {
        drive = *(p - 1);
        drive = toupper(drive) - 'A';
        p++;
    }

    /* Checks if the file that should be deleted is not a directory.
     * Directories must be deleted using dirDelete instead. */
    rc = fatGetFileAttributes(&disks[drive].fat, p, &attr);
    if (rc || (attr & DIRENT_SUBDIR)) return (POS_ERR_FILE_NOT_FOUND);

    rc = fatDeleteFile(&disks[drive].fat, p);
    if (rc != 0) return (-1);
    return (rc);
}

static int dirDelete(const char *dnm)
{
    int x;
    const char *p;
    int drive;
    int rc;
    int attr;
    int dotcount = 0;
    int fh;
    size_t readbytes;
    DIRENT dirent;

    p = strchr(dnm, ':');
    if (p == NULL)
    {
        p = dnm;
        drive = currentDrive;
    }
    else
    {
        drive = *(p - 1);
        drive = toupper(drive) - 'A';
        p++;
    }

    rc = fatGetFileAttributes(&disks[drive].fat, p, &attr);
    if (rc || !(attr & DIRENT_SUBDIR)) return (POS_ERR_PATH_NOT_FOUND);

    if (drive == currentDrive)
    {
        if (strcmp(disks[drive].fat.corrected_path,cwd) == 0)
        {
            return (POS_ERR_ATTEMPTED_TO_REMOVE_CURRENT_DIRECTORY);
        }
    }

    fileOpen(dnm, &fh);

    fileRead(fh, &dirent, sizeof dirent, &readbytes);
    while ((readbytes == sizeof dirent) && (dirent.file_name[0] != '\0'))
    {
        /* LFNs should be ignored when checking if the directory is empty. */
        if (dirent.file_name[0] != DIRENT_DEL &&
            dirent.file_attr != DIRENT_LFN)
        {
            if (dirent.file_name[0] == DIRENT_DOT) dotcount++;
            if (dirent.file_name[0] != DIRENT_DOT || dotcount > 2)
            {
                fileClose(fh);
                return (POS_ERR_PATH_NOT_FOUND);
            }
        }
        fileRead(fh, &dirent, sizeof dirent, &readbytes);
    }
    fileClose(fh);

    rc = fatDeleteFile(&disks[drive].fat, p);
    if (rc == POS_ERR_FILE_NOT_FOUND) return (POS_ERR_PATH_NOT_FOUND);
    return (rc);
}
/**/

/**/
static void accessDisk(int drive)
{
    unsigned char buf[512];
    int rc;
    int sectors = 1;
    int track = 0;
    int head = 0;
    int sector = 1;
    unsigned char *bpb;

    rc = readAbs(buf,
                sectors,
                drive,
                track,
                head,
                sector);
    if (rc != 0)
    {
        return;
    }
    bpb = buf + 11;
    if (disks[drive].accessed)
    {
        fatTerm(&disks[drive].fat);
    }
    analyseBpb(&disks[drive], bpb);
    disks[drive].lba = 0;
    fatDefaults(&disks[drive].fat);
    fatInit(&disks[drive].fat, bpb, readLogical, writeLogical, &disks[drive]);
    strcpy(disks[drive].cwd, "");
    disks[drive].accessed = 1;
    return;
}

static void upper_str(char *str)
{
    int x;

    for (x = 0; str[x] != '\0'; x++)
    {
        str[x] = toupper(str[x]);
    }
    return;
}

/*Function Converting the Date received in BCD format to int format*/
int bcd2int(unsigned int bcd)
{
    return (bcd & 0x0f) + 10 * ((bcd >> 4) & 0x0f);
}
/**/

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

void dumpbuf(unsigned char *buf, int len)
{
    int x;

    for (x = 0; x < len; x++)
    {
        BosWriteText(0, buf[x], 0);
    }
    return;
}

static void readLogical(void *diskptr, unsigned long sector, void *buf)
{
    int track;
    int head;
    int sect;
    DISKINFO *diskinfo;
    int ret;

    diskinfo = (DISKINFO *)diskptr;
    sector += diskinfo->hidden;
    track = sector / diskinfo->sectors_per_cylinder;
    head = sector % diskinfo->sectors_per_cylinder;
    sect = head % diskinfo->sectors_per_track + 1;
    head = head / diskinfo->sectors_per_track;
    if (diskinfo->lba)
    {
        ret = readLBA(buf, 1, diskinfo->drive, sector);
    }
    else
    {
        ret = readAbs(buf, 1, diskinfo->drive, track, head, sect);
    }
    if (ret != 0)
    {
        printf("failed to read sector %ld - halting\n", sector);
        for (;;) ;
    }
    return;
}

static void writeLogical(void *diskptr, unsigned long sector, void *buf)
{
    int track;
    int head;
    int sect;
    DISKINFO *diskinfo;
    int ret;

    diskinfo = (DISKINFO *)diskptr;
    sector += diskinfo->hidden;
    track = sector / diskinfo->sectors_per_cylinder;
    head = sector % diskinfo->sectors_per_cylinder;
    sect = head % diskinfo->sectors_per_track + 1;
    head = head / diskinfo->sectors_per_track;
    if (diskinfo->lba)
    {
        ret = writeLBA(buf, 1, diskinfo->drive, sector);
    }
    else
    {
        ret = writeAbs(buf, 1, diskinfo->drive, track, head, sect);
    }
    if (ret != 0)
    {
        printf("failed to write sector %ld - halting\n", sector);
        for (;;) ;
    }
    return;
}

static int readAbs(void *buf,
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
    void *readbuf = transferbuf;
#else
    void *readbuf = buf;
#endif

    unused(sectors);
    tries = 0;
    while (tries < 5)
    {
        rc = BosDiskSectorRead(readbuf, 1, drive, track, head, sect);
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
    memcpy(&diskinfo->bpb, bpb, sizeof diskinfo->bpb);
    return;
}

#ifdef __32BIT__
int pdosstrt(void)
{
    pdos_parms *pp;
    POS_EPARMS eparms;
    char psp[256];

    __abscor = rp_parms->dsbase;
    gdt = ABSADDR(rp_parms->gdt);
    freem_start = rp_parms->freem_start;
    pp = ABSADDR(__userparm);
    transferbuf = ABSADDR(pp->transferbuf);
    doreboot = pp->doreboot;
    bootBPB = ABSADDR(pp->bpb);
    protintHandler(0x20, int20);
    protintHandler(0x21, int21);
    protintHandler(0x25, int25);
    protintHandler(0x26, int26);
    psp[0x80] = 0;
    psp[0x81] = 0;
    eparms.psp = psp;
    eparms.abscor = __abscor;
    eparms.callback = 0;
    return (__start(NULL, NULL, NULL, &eparms));
    return;
}

int displayc(void)
{
    *__vidptr = 'C';
    return (0);
}

int displayd(void)
{
    *__vidptr = 'D';
    return (0);
}

#endif

#ifndef __32BIT__
static void *pdos16MemmgrAllocate(MEMMGR *memmgr, size_t bytes, int id)
{
    size_t pages;

    /* we need to round up bytes to nearest page */
    pages = bytes / 16 + ((bytes & 0x0f) != 0 ? 1 : 0);
    return (pdos16MemmgrAllocPages(memmgr, pages, id));
}

static void pdos16MemmgrFree(MEMMGR *memmgr, void *ptr)
{
    unsigned long abs;

    abs = ADDR2ABS(ptr);
    abs -= 0x10000UL;
    abs -= (unsigned long)PDOS16_MEMSTART * 16;
    abs /= 16;
    /* ignore strange free requests */
    if (abs > 0x6000U)
    {
        return;
    }
    abs += (unsigned long)PDOS16_MEMSTART * 16;
    ptr = ABS2ADDR(abs);
    (memmgrFree)(memmgr, ptr);
    return;
}

static void *pdos16MemmgrAllocPages(MEMMGR *memmgr, size_t pages, int id)
{
    void *ptr;
    unsigned long abs;

    /* I don't know why some apps request 0 pages, but we'd better
    give them a decent pointer. */
    if (pages == 0)
    {
        pages = 1;
    }
    ptr = (memmgrAllocate)(memmgr, pages, id);
    if (ptr == NULL)
    {
        return (ptr);
    }
    abs = ADDR2ABS(ptr);

    /* and because we wasted 0x10000 for control blocks, we
    skip that, and the bit above 3000 we multiply by 16. */
    abs -= (unsigned long)PDOS16_MEMSTART * 16;
    abs *= 16;
    abs += (unsigned long)PDOS16_MEMSTART * 16;
    abs += 0x10000UL;
    ptr = ABS2ADDR(abs);
    ptr = FP_NORM(ptr);
    return (ptr);
}

static void pdos16MemmgrFreePages(MEMMGR *memmgr, size_t page)
{
    void *ptr;

    ptr = MK_FP(page, 0);
    pdos16MemmgrFree(memmgr, ptr);
    return;
}

static int pdos16MemmgrReallocPages(MEMMGR *memmgr,
                                    void *ptr,
                                    size_t newpages)
{
    unsigned long abs;
    int ret;

    abs = ADDR2ABS(ptr);
    abs -= 0x10000UL;
    abs -= (unsigned long)PDOS16_MEMSTART * 16;
    abs /= 16;
    abs += (unsigned long)PDOS16_MEMSTART * 16;
    ptr = ABS2ADDR(abs);
    ret = (memmgrRealloc)(memmgr, ptr, newpages);
    return (ret);
}

#endif

/*int 25 function call*/
unsigned int PosAbsoluteDiskRead(int drive, unsigned long start_sector,
                                unsigned int sectors, void *buf)
{
    long x;
    for(x=0;x<sectors;x++)
    {
    readLogical(&disks[drive],x,(char *)buf+x*512);
    }
    return(0);
}
/**/

/*int 26 function call*/
unsigned int PosAbsoluteDiskWrite(int drive, unsigned long start_sector,
                                unsigned int sectors, void *buf)
{
    long x;
    for(x=0;x<sectors;x++)
    {
    writeLogical(&disks[drive],x,(char *)buf+x*512);
    }
    return(0);
}
/**/

/*
 Different cases for cwd. The user can input
 directory name, file name in a format convenient to
 user. In each case the cwd must prepended or appended
 with the appropriate directory name, drive name and current
 working directory, ending '\' or '/' must be removed
 and dotdots ("..") must be resolved.
*/
static int formatcwd(const char *input,char *output)
{
    char *p;
    char *temp;
    char *read;
    char *write;
   /*
     The user only provides the <folder-name>
     e.g. \from\1.txt the function corrects it to
     e.g. c:\from\1.txt.
     */
    if (input[0] == '\\')
    {
        output[0] = 'A' + currentDrive;
        strcpy(output + 1,":");
        strcat(output,input);
    }

    /*
     The user provides the file name in full format
     e.g. c:\from\1.txt
     */
    else if((strlen(input) >= 3)
            && (memcmp(input + 1, ":\\", 2) == 0)
           )
    {
        strcpy(output,input);
    }

    /*
     The user misses the '\' e.g. c:1.txt.
     */
    else if (strlen(input) >= 3 && input[1] == ':'
             && input[2] != '\\')
    {
        char *cwd;

        memcpy(output, input, 2);
        memcpy(output + 2, "\\", 2);
        /*
         The current working directory must be fetched from
         the disk array so that the working directory does
         not change on drive switch.
         */
        cwd = disks[toupper(input[0])-'A'].cwd;
        strcat(output,cwd);
        if(strcmp(cwd,"")!= 0)
        {
            strcat(output,"\\");
        }
        strcat(output,input+2);
    }

    /*
     The user provides only the <file-name>
     e.g. 1.txt in that case the drive name,'\'
     and currect working directory needs to be
     prepended e.g. c:\from\1.txt.
     */
    else
    {
        output[0] = 'A' + currentDrive;
        strcpy(output + 1,":");
        strcat(output,"\\");
        strcat(output,cwd);
        if(strcmp(cwd,"")!= 0)
        {
            strcat(output,"\\");
        }
        strcat(output,input);
    }
    tempDrive=toupper(output[0]) - 'A';
    /* Checks for '\' or '/' before the null terminator and removes it. */
    p = strchr(output, '\0') - 1;
    if (p[0] == '\\' || p[0] == '/') p[0] = '\0';
    /* Checks the output for ".."s and changes the output accordingly. */
    read = output + 2;
    write = read;
    while (1)
    {
        read++;
        p = strchr(read, '\\');
        temp = strchr(read, '/');
        if (temp && (temp < p || !p))
        {
            p = temp;
            p[0] = '\\';
        }
        /* Checks if we are still inside the string. */
        if (p)
        {
            if (p - read == 2 && strncmp(read, "..", 2) == 0)
            {
                write[0] = '\0';
                /* If we would ascend above root with "..", return error. */
                if (write == output + 2) return(POS_ERR_PATH_NOT_FOUND);
                /* Finds last '\' and positions write on it. */
                write = strrchr(output + 2, '\\');
            }
            else
            {
                /* Write is positioned on '\' so we move it further. */
                write++;
                /* Copies this part of processed string onto write. */
                memcpy(write, read, p - read);
                /* Increments write for it to be after the copied part. */
                write += p - read;
                /* Adds '\' at the end of copied part. */
                write[0] = '\\';
            }
            /* Moves read further. */
            read = p;
            continue;
        }
        /* We are at the last part of string, so we set p to the end. */
        p = strchr(read, '\0');
        if (strcmp(read, "..") == 0)
        {
            write[0] = '\0';
            /* If we would ascend above root with "..", return error. */
            if (write == output + 2) return(POS_ERR_PATH_NOT_FOUND);
            /* Finds last '\' and positions write on it. */
            write = strrchr(output + 2, '\\');
            /* If we are at the first backslash (the one in "c:\"),
             * we add null terminator after it. */
            if (write == output + 2) write++;
        }
        else
        {
            /* Write is positioned on '\' so we move it further. */
            write++;
            /* Copies this part of processed string onto write. */
            memcpy(write, read, p - read);
            /* Increments write for it to be after the copied part. */
            write += p - read;
        }
        /* Adds null terminator at the end of the output. */
        write[0] = '\0';
        break;
    }
    return (0);
}
/**/

static void logUnimplementedCall(int intNum, int ah, int al)
{
    if (logUnimplementedFlag)
    {
        printf("UNIMPLEMENTED: INT %02X,AH=%02X,AL=%02X\n", intNum, ah, al);
        return;
    }
}

/* INT 21,AH=0A: Read Buffered Input */
void PosReadBufferedInput(pos_input_buffer *buf)
{
    size_t readBytes;
    unsigned char cbuf[3]; /* Only 1 byte needed, but protect against any
                              buffer overflow bug in PosReadFile */

    /* Reset actual chars read to 0 */
    buf->actualChars = 0;

    /* If no chars asked to be read, return immediately */
    if (buf->maxChars == 0)
        return;

    /* Loop until we read either max chars or CR */
    for (;;)
    {
        /* Read enough already, exit loop */
        if (buf->actualChars == buf->maxChars)
            break;

        /* Read one byte */
        readBytes = 0;
        PosReadFile(1, &cbuf, 1, &readBytes);
        if (readBytes > 0)
        {
            /* Handle backspace */
            if (cbuf[0] == '\b')
            {
                if (buf->actualChars == 0)
                {
                    /* Nothing to backspace, ring bell */
                    PosDisplayOutput('\a');
                }
                else
                {
                    /* Something to backspace, remove from buffer and from
                     * screen */
                    buf->actualChars--;
                    PosDisplayOutput(' ');
                    PosDisplayOutput('\b');
                }

            }
            /* Read CR, we exit loop */
            else if (cbuf[0] == '\r')
            {
                /* Don't increment actualChars
                 * since CR not included in count */
                buf->data[buf->actualChars] = cbuf[0];
                break;
            }
            else
            {
                /* Add the byte we read to the buffer */
                buf->data[buf->actualChars++] = cbuf[0];
            }
        }
        else
        {
            /* Read nothing? Probably an IO error. Exit loop then */
            break;
        }
    }

    /* Okay, we are done, return to caller. */
    return;
}

/* INT 21,31 - PosTerminateAndStayResident */
void PosTerminateAndStayResident(int exitCode, int paragraphs)
{
    tsrFlag = paragraphs;
    PosTerminate(exitCode);
}

void PosGetMemoryManagementStats(void *stats)
{
    MEMMGRSTATS *s = (MEMMGRSTATS*)stats;
    memmgrGetStats(&memmgr, stats);
#ifdef __32BIT__
    /* In 32-bit, divide bytes by 16 before returning.
     * This is so we use consistent units of paragraphs (16 bytes)
     * for both PDOS-16 and PDOS-32.
     */
    s->maxFree = s->maxFree >> 4;
    s->maxAllocated = s->maxAllocated >> 4;
    s->totalFree = s->totalFree >> 4;
    s->totalAllocated = s->totalAllocated >> 4;
#endif
}

/* INT 21, function 33, subfunc 05 - get boot drive */
int PosGetBootDrive(void)
{
    return bootDriveLogical + 1;
}

/* INT 21, function 51/62 - get current process ID */
int PosGetCurrentProcessId(void)
{
#ifdef __32BIT__
    return (int) curProc;
#else
    return FP_SEG(curProc);
#endif
}

#define ERR2STR(n) case POS_ERR_##n: return #n

char *PosGetErrorMessageString(unsigned int errorCode) /* func f6.09 */
{
    switch (errorCode)
    {
        ERR2STR(NO_ERROR);
        ERR2STR(FUNCTION_NUMBER_INVALID);
        ERR2STR(FILE_NOT_FOUND);
        ERR2STR(PATH_NOT_FOUND);
        ERR2STR(MANY_OPEN_FILES);
        ERR2STR(ACCESS_DENIED);
        ERR2STR(INVALID_HANDLE);
        ERR2STR(MEM_CONTROL_BLOCK_DESTROYED);
        ERR2STR(INSUFFICIENT_MEMORY);
        ERR2STR(MEMORY_BLOCK_ADDRESS_INVALID);
        ERR2STR(ENVIRONMENT_INVALID);
        ERR2STR(FORMAT_INVALID);
        ERR2STR(ACCESS_CODE_INVALID);
        ERR2STR(DATA_INVALID);
        ERR2STR(FIXUP_OVERFLOW);
        ERR2STR(INVALID_DRIVE);
        ERR2STR(ATTEMPTED_TO_REMOVE_CURRENT_DIRECTORY);
        ERR2STR(NOT_SAME_DEVICE);
        ERR2STR(NO_MORE_FILES);
        ERR2STR(FILE_EXISTS);
    }
    return NULL;
}
