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

#define MAX_PATH 120

typedef struct {
    int env;
    unsigned char *cmdtail;
    char *fcb1;
    char *fcb2;
} PARMBLOCK;

typedef struct {
    FAT fat;
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
#endif
static void loadConfig(void);
static void loadPcomm(void);
static void loadExe(char *prog, PARMBLOCK *parmblock);
static int fixexe32(unsigned char *psp, unsigned long entry, unsigned int sp);
static int bios2driv(int bios);
static int fileCreat(const char *fnm, int attrib);
static int fileOpen(const char *fnm);
static int fileClose(int fno);
static int fileRead(int fno, void *buf, size_t szbuf);
static void accessDisk(int drive);
static void upper_str(char *str);
void dumplong(unsigned long x);
void dumpbuf(unsigned char *buf, int len);
static void readLogical(void *diskptr, long sector, void *buf);
static void writeLogical(void *diskptr, long sector, void *buf);
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

static MEMMGR memmgr;

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
static int currentDrive;
static int bootDrivePhysical;
static int bootDriveLogical;
static int lastDrive;
static unsigned char *bootBPB;
static char *cwd;
static int lastrc;
static int lba;
static unsigned long psector; /* partition sector offset */
static int attr;

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
static char origdta[128];
static char *dta = origdta;
static int debugcnt = 0;
static void *exec_pstart;
static void *exec_c_api;
#ifdef __32BIT__
static int exec_subcor;
#endif

#ifdef __32BIT__
char *__vidptr;
/* SUBADDRFIX - given a pointer from the subprogram, convert
   it into an address usable by us */
#define SUBADDRFIX(x) ((void *)((char *)(x) + subcor - __abscor))
#define ADDRFIXSUB(x) ((void *)((char *)(x) - subcor + __abscor))
#define SUB2ABS(x) ((void *)((char *)(x) + subcor))

int __abscor;
unsigned long __userparm;
int subcor;
static void *gdt;
static char *transferbuf;
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
            if ((systemId == PTS_FAT12)
                || (systemId == PTS_FAT16S)
                || (systemId == PTS_FAT16B))
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
    
    fhandle[0].inuse = 1;
    fhandle[1].inuse = 1;
    fhandle[2].inuse = 1;
    fhandle[0].special = 1;
    fhandle[1].special = 1;
    fhandle[2].special = 1;
    for (x = 3; x < MAXFILES; x++)
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
    long readbytes;
    int ret;
    void *tempdta;
    PARMBLOCK *pb;

#if 0
    if (debugcnt < 50)
    {
        printf("ZZZ %d %04x YYY\n",debugcnt, regsin->x.ax);    
        /*dumplong(regsin->x.ax);*/
        debugcnt++;
    }
#endif    
    switch (regsin->h.ah)
    {
        case 0x02:
            PosDisplayOutput(regsin->h.dl);
            break;

        case 0x07:
            regsout->h.al = PosDirectCharInputNoEcho();
            break;			

        case 0x08:
            regsout->h.al = PosGetCharInputNoEcho();
            break;

        case 0x09:
            PosDisplayString(MK_FP(sregs->ds, regsin->x.dx));
            break;

        case 0x0e:
            regsout->h.al = PosSelectDisk(regsin->h.dl);
            break;
            
        case 0x19:
            regsout->h.al = PosGetDefaultDrive();
            break;

        case 0x1a:
#ifdef __32BIT__
            tempdta = SUBADDRFIX(regsin->d.edx);
#else                        
            tempdta = MK_FP(sregs->ds, regsin->x.dx);
#endif            
            PosSetDTA(tempdta);
            break;

        case 0x25:
#ifdef __32BIT__
            p = SUBADDRFIX(regsin->d.edx);
#else                        
            p = MK_FP(sregs->ds, regsin->x.dx);
#endif            
            PosSetInterruptVector(regsin->h.al, p);
            break;
            
        case 0x2a:
            {
                int year, month, day, dow;
                
                PosGetSystemDate(&year, &month, &day, &dow);
                regsout->x.cx = year;
                regsout->h.dh = month;
                regsout->h.dl = day;
                regsout->h.al = dow;
            }
            break;

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

        case 0x2f:
            tempdta = PosGetDTA();
#ifdef __32BIT__
            regsout->d.ebx = (int)ADDRFIXSUB(tempdta);
#else                        
            regsout->x.bx = FP_OFF(tempdta);
            sregs->es = FP_SEG(tempdta);
#endif            
            break;

        case 0x30:
            regsout->x.ax = PosGetDosVersion();
            regsout->x.bx = 0;
            regsout->x.cx = 0;
            break;
        
        case 0x35:
            p = PosGetInterruptVector(regsin->h.al);
#ifdef __32BIT__
            regsout->d.ebx = (int)ADDRFIXSUB(p);
#else                        
            regsout->x.bx = FP_OFF(p);
            sregs->es = FP_SEG(p);
#endif
            break;
            
        case 0x3b:
#ifdef __32BIT__        
            regsout->d.eax = PosChangeDir(SUBADDRFIX(regsin->d.edx));
#else
            regsout->x.ax = PosChangeDir(MK_FP(sregs->ds, regsin->x.dx));
#endif            
            break;
            
        case 0x3c:
#ifdef __32BIT__
            p = SUBADDRFIX(regsin->d.edx);
            ret = PosCreatFile(p, regsin->x.cx, &regsout->d.eax);
#else            
            p = MK_FP(sregs->ds, regsin->x.dx);
            ret = PosCreatFile(p, regsin->x.cx, &regsout->x.ax);
#endif            
            if (ret < 0)
            {
                regsout->x.cflag = 1;
                regsout->x.ax = -ret;
            }
            break;
            
        case 0x3d:
#ifdef __32BIT__
            p = SUBADDRFIX(regsin->d.edx);
            ret = PosOpenFile(p, regsin->h.al, &regsout->d.eax);
#else            
            p = MK_FP(sregs->ds, regsin->x.dx);
            ret = PosOpenFile(p, regsin->h.al, &regsout->x.ax);
#endif            
            if (ret < 0)
            {
                regsout->x.cflag = 1;
                regsout->x.ax = 0x02;
            }
            break;
            
        case 0x3e:
#ifdef __32BIT__        
            regsout->d.eax = PosCloseFile(regsin->d.ebx);
#else
            regsout->x.ax = PosCloseFile(regsin->x.bx);
#endif            
            break;
            
        case 0x3f:
#ifdef __32BIT__
            p = SUBADDRFIX(regsin->d.edx);
            PosReadFile(regsin->d.ebx,
                        p,
                        regsin->d.ecx,
                        &readbytes);
            regsout->d.eax = readbytes;
#else            
            p = MK_FP(sregs->ds, regsin->x.dx);
            PosReadFile(regsin->x.bx,
                        p,
                        regsin->x.cx,
                        &readbytes);
            regsout->x.ax = readbytes;
#endif
            break;
            
        case 0x40:
#ifdef __32BIT__
            p = SUBADDRFIX(regsin->d.edx);
            regsout->d.eax = PosWriteFile(regsin->d.ebx,
                                          p,
                                          regsin->d.ecx);
#else
            p = MK_FP(sregs->ds, regsin->x.dx);
            regsout->x.ax = PosWriteFile(regsin->x.bx,
                                         p,
                                         regsin->x.cx);
#endif
            break;

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

        case 0x42:
#ifdef __32BIT__
            regsout->d.eax = PosMoveFilePointer(regsin->d.ebx,
                                                regsin->d.ecx,
                                                regsin->h.al);
#else
            readbytes = ((unsigned long)regsin->x.cx << 16) | regsin->x.dx;
            readbytes = PosMoveFilePointer(regsin->x.bx,
                                           readbytes,
                                           regsin->h.al);
            regsout->x.cx = readbytes >> 16;
            regsout->x.dx = readbytes & 0xffff;
#endif                                                
            break;
            
        case 0x43:
            regsout->x.cflag = 1;
            break;

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
              int x;
              int y;
              p = MK_FP(sregs->ds, regsin->x.dx);
              
               for (x = 0; x < 5; x++) 
                {
                 for (y = 0; y < 16; y++)
                  {
                   printf("%02X", *((unsigned char *)p + 16 * x + y));
                  }
                  printf("\n");
                }
               for(;;);
            } 
            break;
                                    
        case 0x47:
#ifdef __32BIT__
            p = SUBADDRFIX(regsin->d.esi);
            regsout->d.eax = PosGetCurDir(regsin->h.dl, p);
#else
            p = MK_FP(sregs->ds, regsin->x.si);
            regsout->x.ax = PosGetCurDir(regsin->h.dl, p);
#endif            
            break;
            
        case 0x48:
#ifdef __32BIT__
            regsout->d.eax = (int)PosAllocMem(regsin->d.ebx);
            regsout->d.eax = (int)ADDRFIXSUB(regsout->d.eax);
#else
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
            
        case 0x4a:
#ifdef __32BIT__
            regsout->d.eax = PosReallocPages(SUBADDRFIX(regsin->d.ecx),
                                             regsin->d.ebx,
                                             &regsout->d.ebx);
#else
            regsout->x.ax = PosReallocPages(MK_FP(sregs->es, 0),
                                            regsin->x.bx,
                                            &regsout->x.bx);
#endif
            if (regsout->x.ax != 0)
            {
                regsout->x.cflag = 1;
            }
            break;
            
        case 0x4b:
#ifdef __32BIT__        
             pb = SUBADDRFIX(regsin->d.ebx);
             if (pb != NULL)
             {
                 pb->cmdtail = SUBADDRFIX(pb->cmdtail);
             }
             PosExec(SUBADDRFIX(regsin->d.edx), pb);
#else
             PosExec(MK_FP(sregs->ds, regsin->x.dx),
                     MK_FP(sregs->es, regsin->x.bx));
#endif                     
             break;
             
        case 0x4c:
#if (!defined(USING_EXE))
             PosTerminate(regsin->h.al);
#endif
             break;

        case 0x4d:
             regsout->x.ax = PosGetReturnCode();
             break;
             
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
             
        case 0x4f:
             regsout->x.ax = PosFindNext();
             if (regsout->x.ax != 0)
             {
                 regsout->x.cflag = 1;
             }
             break;
             
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
          
        /* emx calls are 0x7f */
#ifdef __32BIT__
        case 0x7f:
            /* sbrk */
            if (regsin->h.al == 0)
            {
                if (sbrk_start == NULL)
                {
                    sbrk_start = PosAllocMem(0x100000);
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
            break;
#endif

        case 0xf3:
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
            break;

        default:
             /*printf("unimplemented dos call %x\n", regsin->x.ax);*/
             break;
             
    }
    return;
}


/* !!! START OF POS FUNCTIONS !!! */

void PosTermNoRC(void)
{
    PosTerminate(0);
}

void PosDisplayOutput(int ch)
{
    unsigned char buf[1];

    buf[0] = ch;
    PosWriteFile(1, buf, 1);
    return;
}

/*Get Character Input from DOS*/
int PosDirectCharInputNoEcho(void)
{
    int scan;
    int ascii;


    BosReadKeyboardCharacter(&scan, &ascii);

    return ascii;
}
/**/


/* Written By NECDET COKYAZICI, Public Domain */

int PosGetCharInputNoEcho(void)
{
    int scan;
    int ascii;


    BosReadKeyboardCharacter(&scan, &ascii);

    return ascii;
}
            
void PosDisplayString(const char *buf)
{
    const char *p;

    p = memchr(buf, '$', (size_t)-1);
    if (p == NULL) p = buf;
    PosWriteFile(1, buf, p - buf);
    return;
}


int PosSelectDisk(int drive)
{
    currentDrive = drive;
    if (drive < 2)
    {
        accessDisk(drive);
    }
    cwd = disks[drive].cwd;
    return (lastDrive);
}

int PosGetDefaultDrive(void)
{
    return (currentDrive);
}

void PosGetSystemDate(int *year, int *month, int *day, int *dow)
{
    return;
}

void PosGetSystemTime(int *hour, int *minute, int *second, int *hundredths)
{
    return;
}

void *PosGetDTA(void)
{
    return (dta);
}

void PosSetDTA(void *p)
{
    dta = p;    
    return;
}

void PosSetInterruptVector(int intnum, void *handler)
{
    disable();
    *((void **)0 + intnum) = handler;
    enable();
}

unsigned int PosGetDosVersion(void)
{
    int version;
    
    /* set to 4.00 */
    version = (0x00 << 8) | 0x04;
    return (version);
}

void *PosGetInterruptVector(int intnum)
{
    return *((void **)0 + intnum);
}

int PosChangeDir(char *to)
{
    char *p;
    
    if (strcmp(to, "..") == 0)
    {
        p = strrchr(cwd, '\\');
        if (p != NULL)
        {
            *p = '\0';
        }
        else
        {
            strcpy(cwd, "");
        }
    }
    else if (to[0] == '\\')
    {
        strcpy(cwd, to + 1);
    }
    else
    {
        if (strcmp(cwd, "") != 0)
        {
            strcat(cwd, "\\");
        }
        strcat(cwd, to);
    }
    upper_str(cwd);
    return (0);
}

int PosCreatFile(const char *name, int attrib, int *handle)
{
    char filename[MAX_PATH];
    int fno;

    if (name[1] == ':')
    {
        name += 2;
    }
    if ((name[0] == '\\') || (name[0] == '/'))
    {
        fno = fileCreat(name, attrib);
    }
    else
    {
        strcpy(filename, cwd);
        strcat(filename, "\\");
        strcat(filename, name);
        fno = fileCreat(filename, attrib);
    }
    if (fno < 0)
    {
        *handle = -fno;
    }
    else
    {
        *handle = fno;
    }
    return (fno);
}

int PosOpenFile(const char *name, int mode, int *handle)
{
    char filename[MAX_PATH];
    int fno;

    if (name[1] == ':')
    {
        name += 2;
    }
    if ((name[0] == '\\') || (name[0] == '/'))
    {
        fno = fileOpen(name);
    }
    else
    {
        strcpy(filename, cwd);
        strcat(filename, "\\");
        strcat(filename, name);
        fno = fileOpen(filename);
    }
    if (fno < 0)
    {
        *handle = -fno;
    }
    else
    {
        *handle = fno;
    }
    return (fno);
}

int PosCloseFile(int fno)
{
    int ret;
    
    ret = fileClose(fno);
    return (ret);
}

void PosReadFile(int fh, void *data, size_t bytes, size_t *readbytes)
{
    unsigned char *p;
    size_t x = 0;

    if (fh < 3)
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
    }
    else
    {
        x = fileRead(fh, data, bytes);
    }
    *readbytes = x;
    return;
}

int PosWriteFile(int fh, const void *data, size_t len)
{
    unsigned char *p;
    size_t x;

    if (fh < 3)
    {
        p = (unsigned char *)data;
        for (x = 0; x < len; x++)
        {
            BosWriteText(0, p[x], 0);
        }
    }
    else
    {
        len = fileWrite(fh, data, len);
    }
    return (len);
}

/* unimplemented */
int PosDeleteFile(const char *fname)
{
    return (0);
}

/* unimplemented */
long PosMoveFilePointer(int handle, long offset, int whence)
{
    return (0);
}

int PosGetDeviceInformation(int handle, unsigned int *devinfo)
{
    *devinfo = 0;
    if (handle < 3)
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

void *PosAllocMem(unsigned int size)
{
    return (memmgrAllocate(&memmgr, size, 0)); 
}

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
    
    if ((strchr(prog, '\\') == NULL)
        && (strchr(prog, '/') == NULL))
    {
        strcpy(tempp, cwd);
        strcat(tempp, "\\");
        strcat(tempp, prog);
    }
    else
    {
        strcpy(tempp, prog);
    }
    upper_str(tempp);
    prog = tempp;
    loadExe(prog, parms);
    return;
}

void PosTerminate(int rc)
{
    callwithbypass(rc);
    return;
}
             
int PosGetReturnCode(void)
{
    return (lastrc);
}

int PosFindFirst(char *pat, int attrib)
{
    int ret;
    
    attr = attrib;
    memset(dta, '\0', 0x15); /* clear out reserved area */
    make_ff(pat);
    ff_handle = fileOpen(ff_path);
    if (ff_handle < 0)
    {
        return (3);
    }
    ret = ff_search();
    if (ret == 0x12)
    {
        ret = 2;
    }
    return (ret);
}

int PosFindNext(void)
{
    return (ff_search());
}

/* unimplemented */
int PosRenameFile(const char *old, const char *new)
{
    return (0);
}

int PosTruename(char *prename,char *postname)
{
    strcpy(postname,prename);
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
    int ret;
    unsigned char buf[32];
    char file[13];
    char *p;
    
    ret = fileRead(ff_handle, buf, sizeof buf);
    while ((ret == sizeof buf) && (buf[0] != '\0'))
    {
        if (buf[0] != 0xe5)
        {
            memcpy(file, buf, 8);
            file[8] = '\0';
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
            memcpy(p, buf + 8, 3);
            p[3] = '\0';
            p = strchr(file, ' ');
            if (p != NULL)
            {
                *p-- = '\0';
            }
            if (patmat(file, ff_pat) 
            
                /* if it is not a directory, or they asked for
                   directories, then that is OK */
                && (((buf[0x0b] & 0x10) == 0)
                    || ((attr & 0x10) != 0))
                    
                /* if it is not a volume label, or they asked
                   for volume labels, then that is OK */
                && (((buf[0x0b] & 0x08) == 0)
                    || ((attr & 0x08) != 0))
                    
               )
            {
                if ((p != NULL) && (*p == '.')) *p = '\0';
                dta[0x15] = buf[0x0b]; /* attribute */
                memcpy(dta + 0x16, buf + 0x16, 2); /* time */
                memcpy(dta + 0x18, buf + 0x18, 2); /* time */
                memcpy(dta + 0x1a, buf + 0x1c, 4); /* file size */
                /*memcpy(dta + 0x1e, buf, 11);
                dta[0x1e + 11] = '\0';*/
                memset(dta + 0x1e, '\0', 13);
                strcpy(dta + 0x1e, file);
                return (0);
            }
        }
        ret = fileRead(ff_handle, buf, sizeof buf);
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
#endif

static void loadConfig(void)
{
    size_t ret;
    unsigned char buf[512];
    int x;
    int fh;

    fh = fileOpen("CONFIG.SYS");
    if (fh >= 0)
    {
        do
        {
            ret = fileRead(fh, buf, sizeof buf);
            for (x = 0; x < ret; x++)
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
        } while (ret == 0x200);
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
#ifdef __32BIT__
    struct exec firstbit;
#else    
    unsigned char firstbit[10];
#endif    
    unsigned int headerLen;
    unsigned char *header;
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
    int ret;
    unsigned char *bss;
    int y;
    int isexe = 0;
    char *olddta;

    fno = fileOpen(prog);
    if (fno < 0) return;
#ifdef __32BIT__
    fileRead(fno, &firstbit, sizeof firstbit);
    headerLen = N_TXTOFF(firstbit);
    header = memmgrAllocate(&memmgr, headerLen, 0);
    memcpy(header, &firstbit, sizeof firstbit);
    fileRead(fno, header + sizeof firstbit, headerLen - sizeof firstbit);
#else
    fileRead(fno, firstbit, sizeof firstbit);
    if (memcmp(firstbit, "MZ", 2) == 0)
    {
        isexe = 1;
        headerLen = *(unsigned int *)&firstbit[8];
        headerLen *= 16;
        header = memmgrAllocate(&memmgr, headerLen, 0);
        memcpy(header, firstbit, sizeof firstbit);    
        fileRead(fno, header + sizeof firstbit, headerLen - sizeof firstbit);
    }
#endif    

    envptr = memmgrAllocate(&memmgr, strlen(prog) + 9, 0);
#ifndef __32BIT__
    origenv = envptr;
    envptr = (unsigned char *)FP_NORM(envptr);
#endif    
    memcpy(envptr, "A=B\0\0", 5);
    *((unsigned int *)(envptr + 5)) = 1; /* who knows why */
    memcpy(envptr + 7, prog, strlen(prog) + 1);

#ifdef __32BIT__
    exeLen = N_BSSADDR(firstbit) - N_TXTADDR(firstbit) + firstbit.a_bss;
    /* allocate exeLen + 0x100 (psp) + stack + extra (safety margin) */
    psp = memmgrAllocate(&memmgr, exeLen + 0x100 + 0x8000 + 0x100, 0);
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
        memmgrFree(&memmgr, header);
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
    fileRead(fno, exeStart, firstbit.a_text);
    fileRead(fno, 
             exeStart + N_DATADDR(firstbit) - N_TXTADDR(firstbit),
             firstbit.a_data);
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
            fileRead(fno, upto, maxread);
            totalRead += maxread;
        }
    }
    else
    {
        memcpy(exeStart, firstbit, sizeof firstbit);
        fileRead(fno, exeStart + sizeof firstbit, 32768);
        fileRead(fno, FP_NORM(exeStart + sizeof firstbit + 32768), 32768);
    }
#endif
    fileClose(fno);

#ifndef __32BIT__
    if (isexe)
    {
        numReloc = *(unsigned int *)&header[6];
        relocStart = (unsigned long *)(header + *(unsigned int *)&header[0x18]);
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
    bss = exeStart + N_BSSADDR(firstbit);
    for (y = 0; y < firstbit.a_bss; y++)
    {
        bss[y] = '\0';
    }
    sp = N_BSSADDR(firstbit) + firstbit.a_bss + 0x8000;
#endif           
#if (!defined(USING_EXE) && !defined(__32BIT__))
    olddta = dta;
    dta = psp + 0x80;
    ret = callwithpsp(exeEntry, psp, ss, sp);
    dta = olddta;
    psp = origpsp;
    envptr = origenv;
#endif
#ifdef __32BIT__
    ret = fixexe32(psp, firstbit.a_entry, sp);
#else
    memmgrFree(&memmgr, header);
#endif
    lastrc = ret;
    memmgrFree(&memmgr, psp);
    memmgrFree(&memmgr, envptr);
    return;
}

#ifdef __32BIT__
static int fixexe32(unsigned char *psp, unsigned long entry, unsigned int sp)
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

    exeStart = (unsigned long)psp + 0x100 - 0x10000;
    
    dataStart = exeStart;
    dataStart = (unsigned long)ADDR2ABS(dataStart);
    
    /* now we need to record the subroutine's absolute offset fix */
    oldsubcor = subcor;
    subcor = dataStart;

    exeStart = (unsigned long)ADDR2ABS(exeStart);

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

static int fileCreat(const char *fnm, int attrib)
{
    int x;
    const char *p;
    int drive;
    int rc;
    char tempf[FILENAME_MAX];

    strcpy(tempf, fnm);
    upper_str(tempf);
    fnm = tempf;
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
    for (x = 0; x < MAXFILES; x++)
    {
        if (!fhandle[x].inuse)
        {
            fhandle[x].inuse = 1;
            fhandle[x].special = 0;
            break;
        }
    }
    if (x == MAXFILES) return (-4); /* 4 = too many open files */
    rc = fatCreatFile(&disks[drive].fat, p, &fhandle[x].fatfile, attrib);
    if (rc < 0) return (rc);
    fhandle[x].fatptr = &disks[drive].fat;
    return (x);
}

static int fileOpen(const char *fnm)
{
    int x;
    const char *p;
    int drive;
    int rc;
    char tempf[FILENAME_MAX];

    strcpy(tempf, fnm);
    upper_str(tempf);
    fnm = tempf;
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
    for (x = 0; x < MAXFILES; x++)
    {
        if (!fhandle[x].inuse)
        {
            fhandle[x].inuse = 1;
            fhandle[x].special = 0;
            break;
        }
    }
    if (x == MAXFILES) return (-1);
    rc = fatOpenFile(&disks[drive].fat, p, &fhandle[x].fatfile);
    if (rc != 0) return (-1);
    fhandle[x].fatptr = &disks[drive].fat;
    return (x);
}

static int fileClose(int fno)
{
    if (!fhandle[fno].special)
    {
        fhandle[fno].inuse = 0;
    }
    return (0);
}

static int fileRead(int fno, void *buf, size_t szbuf)
{
    size_t ret;

    ret = fatReadFile(fhandle[fno].fatptr, &fhandle[fno].fatfile, buf, szbuf);
    return (ret);
}


static int fileWrite(int fno, void *buf, size_t szbuf)
{
    size_t ret;

    ret = fatWriteFile(fhandle[fno].fatptr, &fhandle[fno].fatfile, buf, szbuf);
    return (ret);
}

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

static void readLogical(void *diskptr, long sector, void *buf)
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

static void writeLogical(void *diskptr, long sector, void *buf)
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
    pp = ABSADDR(__userparm);
    transferbuf = ABSADDR(pp->transferbuf);
    doreboot = pp->doreboot;
    bootBPB = ABSADDR(pp->bpb);
    protintHandler(0x20, int20);
    protintHandler(0x21, int21);
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
