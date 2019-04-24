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
    int valid;
} DISKINFO;

/* ===== BEGINS PROCESS MANAGEMENT DATA STRUCTURES ===== */

/* PDOS_PROCESS: data structure with info about a process */
typedef struct _PDOS_PROCESS {
    char magic[4]; /* 'PCB\0' magic, to distinguish PCB (Process Control Block)
                      from MCB (Memory Control Block) */
    char exeName[PDOS_PROCESS_EXENAMESZ]; /* ASCIIZ short name of executable */
    unsigned long pid; /* The process ID.
                        * Under PDOS-32, this is pointer to PSP.
                        * Under PDOS-16, this is segment of PSP.
                        * Should match PosGetCurrentProcessId() if this is the
                        * current process.
                        */
    PDOS_PROCSTATUS status;
    void *envBlock; /* Environment block of this process */
    struct _PDOS_PROCESS *parent; /* NULL for root process */
    struct _PDOS_PROCESS *prev; /* Previous process */
    struct _PDOS_PROCESS *next; /* Next process */
} PDOS_PROCESS;

/* What boundary we want the process control block to be a multiple of */
#define PDOS_PROCESS_ALIGN 16

#define PDOS_PROCESS_SIZE \
  ((sizeof(PDOS_PROCESS) % PDOS_PROCESS_ALIGN == 0) ? \
   sizeof(PDOS_PROCESS) : \
   ((sizeof(PDOS_PROCESS) / PDOS_PROCESS_ALIGN + 1) * PDOS_PROCESS_ALIGN))

#define PDOS_PROCESS_SIZE_PARAS ((PDOS_PROCESS_SIZE)/16)

/* ===== ENDING PROCESS MANAGEMENT DATA STRUCTURES ===== */

void pdosRun(void);
static void initdisks(void);
static void scanPartition(int drive);
static void processPartition(int drive, unsigned char *prm);
static void processExtended(int drive, unsigned char *prm);

static void initfiles(void);

static void int21handler(union REGS *regsin,
                        union REGS *regsout,
                        struct SREGS *sregs);

static void int80handler(union REGS *regsin,
                        union REGS *regsout,
                        struct SREGS *sregs);

static void make_ff(char *pat);
static void scrunchf(char *dest, char *new);
static int ff_search(void);

#ifdef __32BIT__
int int0E(unsigned int *regs);
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
#ifdef __32BIT__
static int loadExe32(char *prog, PARMBLOCK *parmblock);
static int fixexe32(unsigned char *psp, unsigned long entry, unsigned int sp,
                    unsigned long exeStart, unsigned long dataStart);
#endif
static int bios2driv(int bios);
static int fileCreat(const char *fnm, int attrib, int *handle);
static int dirCreat(const char *dnm, int attrib);
static int newFileCreat(const char *fnm, int attrib, int *handle);
static int fileOpen(const char *fnm, int *handle);
static int fileWrite(int fno, const void *buf, size_t szbuf,
                     size_t *writtenbytes);
static int fileDelete(const char *fnm);
static int dirDelete(const char *dnm);
static int fileSeek(int fno, long offset, int whence, long *newpos);
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
static int pdosWriteText(int ch);
static size_t envBlockSize(char *envptr);
static char *envBlockTail(char *envptr);
static char *envCopy(char *previous, char *progName);
static char *envAllocateEmpty(char *progName);
static char *envModify(char *envPtr, char *name, char *value);
static void getDateTime(FAT_DATETIME *ptr);
static int isDriveValid(int drive);

static MEMMGR memmgr;
#ifdef __32BIT__
static MEMMGR btlmem;
#endif

/* we implement special versions of allocate and free */
#ifndef __32BIT__
#define PDOS16_MEMSTART 0x3000
#define memmgrAllocate(m,b,i) pdos16MemmgrAllocate(m,b,i)
#define memmgrFree(m,p) pdos16MemmgrFree(m,p)
#define memmgrSetOwner(m,p,o) pdos16MemmgrSetOwner(m,p,o)
static void pdos16MemmgrFree(MEMMGR *memmgr, void *ptr);
static void *pdos16MemmgrAllocate(MEMMGR *memmgr, size_t bytes, int id);
static void *pdos16MemmgrAllocPages(MEMMGR *memmgr, size_t pages, int id);
static void pdos16MemmgrFreePages(MEMMGR *memmgr, size_t page);
static int pdos16MemmgrReallocPages(MEMMGR *memmgr,
                                    void *ptr,
                                    size_t newpages);
static void pdos16MemmgrSetOwner(MEMMGR *memmgr, void *ptr,
                                 unsigned long owner);
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
static int memId = 0; /* give each program a unique ID for their memory */
static unsigned long psector; /* partition sector offset */
static int attr;
static PDOS_PROCESS *initProc = NULL; /* initial process; beginning of process
                                         control block chain */
static PDOS_PROCESS *curPCB = NULL; /* PCB of process running right now */

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

/* Tick count at boot time. Can be used to compute system uptime. */
static unsigned long bootedAt;

/* Current color to use */
static unsigned int currentAttrib = 7;

/* Current video page */
static unsigned int currentPage = 0;

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
static unsigned long dopoweroff;
static char *sbrk_start = NULL;
static char *sbrk_end;
#else
#define ABSADDR(x) ((void *)(x))
#endif

#ifdef __32BIT__
#include "physmem.h"
#include "vmm.h"
#include "liballoc.h"

PHYSMEMMGR physmemmgr;
VMM kernel_vmm;

#define KERNEL_HEAP_ADDR 0xE0000000
#define KERNEL_HEAP_SIZE 0x10000000

/* Liballoc hooks. */
/* Locking functions disable and enable interrupts
 * as a better way is not yet implemented. */
int liballoc_lock()
{
    disable();
    return (0);
}

int liballoc_unlock()
{
    enable();
    return (0);
}

void *liballoc_alloc(size_t num_pages)
{
    return (vmmAllocPages(&kernel_vmm, num_pages));
}

int liballoc_free(void *addr, size_t num_pages)
{
    vmmFreePages(&kernel_vmm, addr, num_pages);

    return (0);
}
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
    unsigned long memstart;
    unsigned long memavail;
#endif

#ifdef __32BIT__
    __vidptr = ABSADDR(0xb8000);
#endif
#if (!defined(USING_EXE) && !defined(__32BIT__))
    instint();
#endif
    PosSetVideoAttribute(0x1E);
    printf("Welcome to PDOS-"
#ifdef __32BIT__
        "32"
#else
        "16"
#endif
        "\n");
    PosSetVideoAttribute(0x7);

    /* Initialise BIOS tick count at startup. */
    bootedAt = BosGetClockTickCount();

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
                &disks[bootDriveLogical],
                getDateTime);
        strcpy(disks[bootDriveLogical].cwd, "");
        disks[bootDriveLogical].accessed = 1;
        disks[bootDriveLogical].valid = 1;
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
    if (memavail < 4*1024*1024)
    {
        printf("less than 4 MiB available - system halting\n");
        for (;;) ;
    }
#ifdef EXE32
    memavail -= 0x500000; /* room for disk cache */
    memmgrSupply(&memmgr, ABSADDR(0x700000), memavail);
#else
    /* skip the first 2 MiB */
    memstart = 0x200000;
    memavail -= 0x200000;
    /* Provides memory to physical memory manager. */
    physmemmgrInit(&physmemmgr);
    physmemmgrSupply(&physmemmgr, memstart, memavail);
    /* Sets up paging. */
    {
        unsigned long *page_directory = physmemmgrAllocPageFrame(&physmemmgr);
        unsigned long *page_table = physmemmgrAllocPageFrame(&physmemmgr);
        unsigned long *pt_entry;
        unsigned long paddr;

        /* Marks all Page Directory entries as not present. */
        memset(page_directory, 0, PAGE_FRAME_SIZE);
        /* Identity paging for the first 4 MiB. */
        for (paddr = PAGE_RW | PAGE_PRESENT, pt_entry = page_table;
             paddr < 0x400000;
             paddr += PAGE_FRAME_SIZE, pt_entry++)
        {
            *pt_entry = paddr;
        }
        /* Puts the first Page Table into the Page Directory. */
        page_directory[0] = (((unsigned long)page_table)
                             | PAGE_RW | PAGE_PRESENT);
        /* Maps the Page Directory into itself
         * so all Page Tables can be accessed at 0xffc00000.
         * The Page Directory itself is at 0xfffff000. */
        page_directory[1023] = (((unsigned long)page_directory)
                                | PAGE_RW | PAGE_PRESENT);
        loadPageDirectory(page_directory);
        enablePaging();
    }
    /* Sets up the kernel VMM. */
    vmmInit(&kernel_vmm, &physmemmgr);
    vmmBootstrap(&kernel_vmm, (void *)KERNEL_HEAP_ADDR, KERNEL_HEAP_SIZE);
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
#ifndef __32BIT__
    memmgrTerm(&memmgr);
#endif
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
        writeLogical, &disks[lastDrive], getDateTime);
    strcpy(disks[lastDrive].cwd, "");
    disks[lastDrive].accessed = 1;
    disks[lastDrive].valid = 1;
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
            || (systemId == PTS_FAT16B)
            || (systemId == PTS_FAT32)
            || (systemId == PTS_FAT32L))
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
    unsigned long pid;
    size_t sz;

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
                memmgrSetOwner(&memmgr, p, FP_SEG(curPCB));
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
                    sbrk_start = PosAllocMem(0x100000, POS_LOC32);
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
            else if (regsin->h.al == 0xA)
            {
                PosPowerOff();
            }
            else if (regsin->h.al == 0xC)
            {
#ifdef __32BIT__
                p = SUBADDRFIX(regsin->d.edx);
                sz = regsin->d.ecx;
                pid = regsin->d.ebx;
#else
                p = MK_FP(sregs->ds, regsin->x.dx);
                sz = regsin->x.cx;
                pid = regsin->x.bx;
#endif
                regsout->x.ax = PosProcessGetInfo(pid, p, sz);
            }
            else if (regsin->h.al == 0xD)
            {
#ifdef __32BIT__
                p = SUBADDRFIX(regsin->d.edx);
                pid = regsin->d.ecx;
#else
                p = MK_FP(sregs->ds, regsin->x.dx);
                pid = regsin->x.cx;
#endif
                PosProcessGetMemoryStats(pid, p);
            }
            else if (regsin->h.al == 0x30)
            {
                PosClearScreen();
            }
            else if (regsin->h.al == 0x31)
            {
                PosMoveCursor(regsin->h.bh, regsin->h.bl);
            }
            else if (regsin->h.al == 0x32)
            {
#ifdef __32BIT__
                p = (void *) regsin->d.edx;
                regsin->d.eax = PosGetVideoInfo(p, regsin->d.ecx);
#else
                p = MK_FP(sregs->ds, regsin->x.dx);
                regsin->x.ax = PosGetVideoInfo(p, regsin->x.cx);
#endif
            }
            else if (regsin->h.al == 0x33)
            {
                regsout->x.ax = PosKeyboardHit();
            }
            else if (regsin->h.al == 0x34)
            {
                PosYield();
            }
            else if (regsin->h.al == 0x35)
            {
#ifdef __32BIT__
                unsigned long seconds = regsin->d.edx;
#else
                unsigned long seconds = (regsin->x.dx) << 16UL;
                seconds |= regsin->x.bx;
#endif
                PosSleep(seconds);
            }
            else if (regsin->h.al == 0x36)
            {
                unsigned long ticks = PosGetClockTickCount();
#ifdef __32BIT__
                regsout->d.edx = ticks;
#else
                regsout->x.dx = (ticks >> 16UL);
                regsout->x.bx = ticks;
#endif
            }
            else if (regsin->h.al == 0x37)
            {
                PosSetVideoAttribute(regsin->x.bx);
            }
            else if (regsin->h.al == 0x38)
            {
                regsout->x.ax = PosSetVideoMode(regsin->x.bx);
            }
            else if (regsin->h.al == 0x39)
            {
                regsout->x.ax = PosSetVideoPage(regsin->x.bx);
            }
            else if (regsin->h.al == 0x3A)
            {
#ifdef __32BIT__
                regsout->d.eax = PosSetEnv(
                        (char*)regsin->d.ebx,
                        (char*)regsin->d.edx
                    );
#else
                regsout->x.ax = PosSetEnv(
                        (char*)MK_FP(sregs->ds,regsin->x.bx),
                        (char*)MK_FP(sregs->es,regsin->x.dx)
                    );
#endif
            }
            else if (regsin->h.al == 0x3B)
            {
                p = PosGetEnvBlock();
#ifdef __32BIT__
                regsout->d.eax = 0;
                regsout->d.ebx = (int)p;
#else
                regsout->x.ax = 0;
                regsout->x.bx = FP_OFF(p);
                sregs->ds = FP_SEG(p);
#endif
            }
            else if (regsin->h.al == 0x3C)
            {
#ifdef __32BIT__
                regsout->d.eax = PosSetNamedFont(
                        (char*)regsin->d.ebx
                    );
#else
                regsout->x.ax = PosSetNamedFont(
                        (char*)MK_FP(sregs->ds,regsin->x.bx)
                    );
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


#ifdef __32BIT__
static void int80handler(union REGS *regsin,
                        union REGS *regsout,
                        struct SREGS *sregs)
{
    void *p;
    size_t writtenbytes;

#if 0
    if (debugcnt < 200)
    {
        printf("ZZZ %d %04x YYY\n",debugcnt, regsin->x.ax);
        /*dumplong(regsin->x.ax);*/
        debugcnt++;
    }
#endif
    switch (regsin->d.eax)
    {
        /* Terminate (with return code) */
        case 0x1:
            PosTerminate(regsin->d.ebx);
            break;

        /* Write */
        case 0x4:
            p = SUBADDRFIX(regsin->d.ecx);
            regsout->d.eax = PosWriteFile(regsin->d.ebx,
                                          p,
                                          regsin->d.edx,
                                          &writtenbytes);
            if (regsout->d.eax) regsout->x.cflag = 1;
            else regsout->d.eax = writtenbytes;
            break;

        default:
            printf("got int 80H call %x\n", regsin->d.eax);
            break;
    }
    return;
}
#endif

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

    if (drive < 2)
    {
        accessDisk(drive);
    }
    if (isDriveValid(drive))
    {
        currentDrive = drive;

        cwd = disks[drive].cwd;
    }

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
void PosSetInterruptVector(unsigned int intnum, void *handler)
{
#ifdef __32BIT__
    protintHandler(intnum, handler);
#else
    disable();
    *((void **)0 + intnum) = handler;
    enable();
#endif
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
    ret = dirCreat(dirname, 0);

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
        while (x < bytes)
        {
            int scan;
            int ascii;

            BosReadKeyboardCharacter(&scan, &ascii);
            if ((ascii == '\b') && (x > 0))
            {
                x--;
                p[x] = '\0';
            }
            else
            {
                p[x] = ascii;
            }
            pdosWriteText(ascii);
            if (ascii == '\r')
            {
                pdosWriteText('\n');
                /* NB: this will need to be fixed, potential
                buffer overflow - bummer! */
                x++;
                p[x] = '\n';
                x++;
                break;
            }
            if (ascii != '\b')
            {
                x++;
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
            /* need this for Linux calls */
            if (p[x] == '\n')
            {
                pdosWriteText('\r');
            }
            pdosWriteText(p[x]);
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
    return (fileSeek(handle, offset, whence, newpos));
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
    if ((flags & POS_LOC_MASK) == POS_LOC20)
    {
        return (memmgrAllocate(&btlmem, size, memId + subpool));
    }
    return (kmalloc(size));
}
#endif

#ifdef __32BIT__
void *PosAllocMemPages(unsigned int pages, unsigned int *maxpages)
{
    void *p;

    p = kmalloc(pages * 16);
    if (p == NULL && maxpages != NULL)
    {
        *maxpages = 0;
    }
    return (p);
}
#else
void *PosAllocMemPages(unsigned int pages, unsigned int *maxpages)
{
    void *p;

    p = pdos16MemmgrAllocPages(&memmgr, pages, memId);
    if (p == NULL && maxpages != NULL)
    {
        *maxpages = memmgrMaxSize(&memmgr);
    }
    return (p);
}
#endif

/* Is there a process control block just before this pointer? */
static int isProcessPtr(void *ptr)
{
    unsigned long abs;
    PDOS_PROCESS *p;
    if (ptr == NULL)
        return 0;
#ifndef __32BIT__
    abs = ADDR2ABS(ptr);
    abs -= PDOS_PROCESS_SIZE;
    ptr = FP_NORM(ABS2ADDR(abs));
    p = (PDOS_PROCESS*)ptr;
#else
    p = (PDOS_PROCESS*)(((char *)ptr) - PDOS_PROCESS_SIZE);
#endif
    return p->magic[0] == 'P' &&
           p->magic[1] == 'C' &&
           p->magic[2] == 'B' &&
           p->magic[3] == 0;
}

static int pdosMemmgrIsBlockPtr(void *ptr)
{
#ifndef __32BIT__
    unsigned long abs;

    abs = ADDR2ABS(ptr);
    abs -= 0x10000UL;
    abs -= (unsigned long)PDOS16_MEMSTART * 16;
    abs /= 16;
    abs += (unsigned long)PDOS16_MEMSTART * 16;
    ptr = ABS2ADDR(abs);
#endif
    return memmgrIsBlockPtr(ptr);
}

/* Before process control blocks were implemented, the memory layout looked
 * like this:
 *    Main Code Segment:   |MEMMGRN|PSP|Program code...
 *    Other Segments:      |MEMMGRN|Code or data...
 * However, with the implementation of PCBs, it now looks like this:
 *    Main Code Segment:   |MEMMGRN|PCB|PSP|Program code...
 *    Other Segments:      |MEMMGRN|Code or data...
 * The memory management APIs expect to get the PSP pointer, and subtract the
 * MEMMGRN size, to get the MEMMGRN pointer. But, with the PCB in between the
 * MEMMGRN and PSP, that won't work. So, this method detects when there is a
 * PCB immediately before the pointer instead of a MEMMGRN, and in that case
 * it returns the PCB pointer (so the memory manager finds the MEMMGRN before
 * it). Otherwise, it just returns the input pointer.
 */
static void *translateProcessPtr(void *ptr)
{
    void *prev;
    unsigned long abs;

#ifdef __32BIT__
    prev = (void*)(((char*)ptr)-PDOS_PROCESS_SIZE);
#else
    abs = ADDR2ABS(ptr);
    abs -= PDOS_PROCESS_SIZE;
    prev = FP_NORM(ABS2ADDR(abs));
#endif

    if (!pdosMemmgrIsBlockPtr(ptr) && isProcessPtr(ptr) &&
            pdosMemmgrIsBlockPtr(prev))
    {
        return prev;
    }
    return ptr;
}

int PosFreeMem(void *ptr)
{
    ptr = translateProcessPtr(ptr);
#ifdef __32BIT__
    if ((unsigned long)ptr < 0x0100000)
    {
        memmgrFree(&btlmem, ptr);
        return (0);
    }
    kfree(ptr);
    return (0);
#endif
    memmgrFree(&memmgr, ptr);
    return (0);
}

#ifdef __32BIT__
int PosReallocPages(void *ptr, unsigned int newpages, unsigned int *maxp)
{
    /* Incompatible with krealloc() (and realloc() as well)
     * because the address of the memory cannot be changed.
     * Error is always returned to prevent memory corruption. */
    return (POS_ERR_INSUFFICIENT_MEMORY);
}
#else
int PosReallocPages(void *ptr, unsigned int newpages, unsigned int *maxp)
{
    int ret;

    ptr = translateProcessPtr(ptr);
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
void PosPowerOff(void)
{
    unsigned short newregs[11];

    runreal_p(dopoweroff, 0);
    return;
}
#else
void PosPowerOff(void)
{
    poweroff();
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
int int0E(unsigned int *regs)
{
    printf("Page Fault occured (Protected Mode Exception 0xE)\n");
    printf("while accessing virtual address 0x%08x\n", readCR2());
    printf("Error code is %08x\n", regs[8]);
    printf("System halting\n");
    for (;;);

    return (0);
}

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

int int80(unsigned int *regs)
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
    int80handler(&regsin, &regsout, &sregs);
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
                /* Sets if the Last access date should be recorded.
                 * Syntax is "[drive][sign]", '+' for yes,
                 * '-' for no, '[drive]' is the drive letter.
                 * Default option is no.
                 * Only the last entry and the last sign
                 * for drive are important.
                 * Invalid values or combinations are ignored. */
                else if (memcmp(buf + x, "ACCDATE=", 8) == 0)
                {
                    int drive = -1;

                    for (;
                         ((buf[x] != '\r') &&
                          (buf[x] != '\n') &&
                          (x < readbytes));
                         x++)
                    {
                        if ((buf[x] >= 'a' && buf[x] <= 'z') ||
                            (buf[x] >= 'A' && buf[x] <= 'Z'))
                        {
                            drive = toupper((unsigned char)buf[x]) - 'A';
                        }
                        else if (buf[x] == '+')
                        {
                            if (drive >= 0 && drive < MAXDISKS)
                            {
                                disks[drive].fat.last_access_recording = 1;
                            }
                        }
                        else if (buf[x] == '-')
                        {
                            if (drive >= 0 && drive < MAXDISKS)
                            {
                                disks[drive].fat.last_access_recording = 0;
                            }
                        }
                    }
                }
                /*pdosWriteText(buf[x]);*/
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
        strcpy(shell,"?:\\COMMAND.COM");
        shell[0] = bootDriveLogical + 'A';
        loadExe(shell, &p);
    }
    else
    {
        loadExe(shell, &altp);
    }
    return;
}

/* initPCB - Initialize process control block.
 * Sets the magic, flags, etc.
 */
static void initPCB(PDOS_PROCESS *pcb, unsigned long pid, char *prog,
                    char *envPtr)
{
    char *tmp;

    /* Skip any drive or directory in the program name */
    for (;;)
    {
        tmp = strchr(prog,':');
        if (tmp == NULL)
            tmp = strchr(prog,'\\');
        if (tmp == NULL)
            tmp = strchr(prog,'/');
        if (tmp == NULL)
            break;
        prog = tmp+1;
        continue;
    }

    /* Clear the PCB before using it.
     * This will set all fields to 0 except those we set explicitly below.
     */
    memset(pcb, 0, PDOS_PROCESS_SIZE);

    /* Set the PCB magic. This helps identify PCBs in memory */
    pcb->magic[0] = 'P';
    pcb->magic[1] = 'C';
    pcb->magic[2] = 'B';
    pcb->magic[3] = 0;

    /* Set the exe name */
    strncpy(pcb->exeName, prog, PDOS_PROCESS_EXENAMESZ-1);
    upper_str(pcb->exeName);

    /* Set the PID */
    pcb->pid = pid;

    /* Set the parent */
    pcb->parent = curPCB;

    /* Set the environment block */
    pcb->envBlock = envPtr;
}

/* Process has terminated, remove it from chain */
void removeFromProcessChain(PDOS_PROCESS *pcb)
{
    PDOS_PROCESS *cur;
    PDOS_PROCESS *prev = pcb->prev;
    PDOS_PROCESS *next = pcb->next;

    /* We don't support removing the init process, it can't terminate. */
    if (prev == NULL)
        return;

    /* Patch this PCB out of the chain. */
    prev->next = next;
    if (next != NULL)
        next->prev = prev;
    pcb->next = NULL;
    pcb->prev = NULL;

    /* Walk through chain, any of our surviving children are reparented
       to the init process. */
    cur = initProc;
    while (cur != NULL)
    {
        if (cur->parent == pcb)
        {
            cur->parent = initProc;
        }
        cur = cur->next;
    }
}

/* Add some new process to process chain */
void addToProcessChain(PDOS_PROCESS *pcb)
{
    PDOS_PROCESS *last = NULL;

    /* This is first process ever, it starts the chain. */
    if (initProc == NULL)
    {
        initProc = pcb;
        return;
    }

    /* Find last process in chain, this process goes in end. */
    last = initProc;
    while (last->next != NULL)
    {
        last = last->next;
    }

    /* We have last process, stick new process after it. */
    last->next = pcb;
    pcb->prev = last;
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

static void loadExe(char *prog, PARMBLOCK *parmblock)
{
#ifdef __32BIT__
    loadExe32(prog, parmblock);
    return;
#else
    unsigned char firstbit[10];
    unsigned int headerLen;
    unsigned char *header = NULL;
    unsigned char *envptr;
    unsigned long exeLen;
    unsigned char *psp;
    unsigned char *pcb;
    PDOS_PROCESS *newProc;
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
    int isexe = 0;
    char *olddta;

    if (fileOpen(prog, &fno)) return;
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

    /* If curPCB == NULL, we are launching init process (PCOMM),
     * setup initial environment.
     */
    if (curPCB == NULL)
    {
        char sBOOTDRIVE[2];
        sBOOTDRIVE[0] = 'A' + bootDriveLogical;
        sBOOTDRIVE[1] = 0;
        envptr = envAllocateEmpty(prog);
        envptr = envModify(envptr, "COMSPEC", prog);
        envptr = envModify(envptr, "BOOTDRIVE", sBOOTDRIVE);
    }
    else
    {
        /* Launching a child process, copy parent's environment */
        envptr = envCopy(curPCB->envBlock,prog);
    }

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
    if (((long)maxPages * 16) < (exeLen+PDOS_PROCESS_SIZE))
    {
        printf("insufficient memory to load program\n");
        printf("required %ld, available %ld\n",
            (exeLen+PDOS_PROCESS_SIZE), (long)maxPages * 16);
        memmgrFree(&memmgr, header);
        memmgrFree(&memmgr, envptr);
        return;
    }
    pcb = pdos16MemmgrAllocPages(&memmgr, maxPages, 0);
    if (pcb != NULL)
    {
        psp = pcb + PDOS_PROCESS_SIZE;
        psp = (unsigned char *)FP_NORM(psp);
        newProc = (PDOS_PROCESS*)pcb;
        initPCB(newProc, FP_SEG(psp), prog, envptr);
        memmgrSetOwner(&memmgr, pcb, FP_SEG(psp));
        memmgrSetOwner(&memmgr, envptr, FP_SEG(psp));
        memmgrSetOwner(&memmgr, header, FP_SEG(psp));
    }
    psp = (unsigned char *)FP_NORM(psp);
    if (pcb == NULL)
    {
        printf("insufficient memory to load program\n");
        if (header != NULL) memmgrFree(&memmgr, header);
        memmgrFree(&memmgr, envptr);
        fileClose(fno);
        return;
    }

    /* set various values in the psp */
    psp[0] = 0xcd;
    psp[1] = 0x20;
    *(unsigned int *)&psp[0x2c] = FP_SEG(envptr);
    /* set to say 640k in use */
    *(unsigned int *)(psp + 2) = FP_SEG(psp) +
            (maxPages-PDOS_PROCESS_SIZE_PARAS);
    if (parmblock != NULL)
    {
        /* 1 for the count and 1 for the return */
        memcpy(psp + 0x80, parmblock->cmdtail, parmblock->cmdtail[0] + 1 + 1);
    }

    exeStart = psp + 0x100;
    exeStart = (unsigned char *)FP_NORM(exeStart);

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
    fileClose(fno);

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

    /* Before executing program, set tsrFlag = 0.
     * If program is a TSR, it will change this to non-zero.
     */
    tsrFlag = 0;

    /* Add this process to the process chain */
    addToProcessChain(newProc);

    /* Set this process as the running process */
    curPCB = newProc;

#if (!defined(USING_EXE))
    olddta = dta;
    dta = psp + 0x80;
    newProc->status = PDOS_PROCSTATUS_ACTIVE;
    if (newProc->parent != NULL)
        newProc->parent->status = PDOS_PROCSTATUS_CHILDWAIT;
    memId += 256;
    ret = callwithpsp(exeEntry, psp, ss, sp);
    /* printf("ret is %x\n", ret); */
    memmgrFreeId(&memmgr, memId);
    memId -= 256;
    dta = olddta;
#endif
    lastrc = ret;

    /* Process finished, parent becomes current */
    curPCB = newProc->parent;
    if (curPCB != NULL && curPCB->status == PDOS_PROCSTATUS_CHILDWAIT)
        curPCB->status = PDOS_PROCSTATUS_ACTIVE;

    /*
     * Don't free all of program's memory if it is a TSR. Only the
     * "un-reserved" portion.
     */
    if (tsrFlag == 0)
    {
        newProc->status = PDOS_PROCSTATUS_TERMINATED;
        if (header != NULL) memmgrFree(&memmgr, header);
        memmgrFree(&memmgr, newProc->envBlock);
        removeFromProcessChain(newProc);
        memmgrFree(&memmgr, pcb);
    }
    else
    {
        /* PDOS EXTENSION: tsrFlag=-1 means free no memory */
        if (tsrFlag != -1)
        {
            PosReallocPages(pcb, tsrFlag, &maxPages);
        }
        /* Mark process as a TSR. */
        newProc->status = PDOS_PROCSTATUS_TSR;
    }

    /* Set TSR flag back to 0 */
    tsrFlag = 0;

    return;
#endif
}

#ifdef __32BIT__
#include "a_out.h"
#include "elf.h"
#include "exeload.h"

static int loadExe32(char *prog, PARMBLOCK *parmblock)
{
    unsigned char *envptr;
    unsigned char *psp;
    unsigned char *pcb;
    PDOS_PROCESS *newProc;
    EXELOAD *exeload;
    int ret;

    exeload = kmalloc(sizeof(EXELOAD));
    /* allocate exeLen + 0x100 (psp) + PDOS_PROCESS_SIZE (PCB) +
     * stack + extra (safety margin) */
    exeload->extra_memory_before = 0x100 + PDOS_PROCESS_SIZE;
    exeload->extra_memory_after = STACKSZ32 + 0x100;
    exeload->stack_size = STACKSZ32;

    ret = exeloadDoload(exeload, prog);
    if (ret)
    {
        kfree(exeload);
        return (1);
    }

    /* If curPCB == NULL, we are launching init process (PCOMM),
     * setup initial environment.
     */
    if (curPCB == NULL)
    {
        char sBOOTDRIVE[2];
        sBOOTDRIVE[0] = 'A' + bootDriveLogical;
        sBOOTDRIVE[1] = 0;
        envptr = envAllocateEmpty(prog);
        envptr = envModify(envptr, "COMSPEC", prog);
        envptr = envModify(envptr, "BOOTDRIVE", sBOOTDRIVE);
    }
    else
    {
        /* Launching a child process, copy parent's environment */
        envptr = envCopy(curPCB->envBlock,prog);
    }

    pcb = exeload->memStart;
    psp = pcb + PDOS_PROCESS_SIZE;
    newProc = (PDOS_PROCESS*)pcb;
    initPCB(newProc, (unsigned long)psp, prog, envptr);

    /* set various values in the psp */
    psp[0] = 0xcd;
    psp[1] = 0x20;
    if (parmblock != NULL)
    {
        /* 1 for the count and 1 for the return */
        memcpy(psp + 0x80, parmblock->cmdtail, parmblock->cmdtail[0] + 1 + 1);
    }

    /* Before executing program, set tsrFlag = 0.
     * If program is a TSR, it will change this to non-zero.
     */
    tsrFlag = 0;

    /* Add this process to the process chain */
    addToProcessChain(newProc);

    /* Set this process as the running process */
    curPCB = newProc;

    newProc->status = PDOS_PROCSTATUS_ACTIVE;
    if (newProc->parent != NULL)
        newProc->parent->status = PDOS_PROCSTATUS_CHILDWAIT;

    ret = fixexe32(psp, exeload->entry_point, exeload->sp,
                   exeload->cs_address,
                   exeload->ds_address);
    lastrc = ret;

    kfree(exeload);

    /* Process finished, parent becomes current */
    curPCB = newProc->parent;
    if (curPCB != NULL && curPCB->status == PDOS_PROCSTATUS_CHILDWAIT)
        curPCB->status = PDOS_PROCSTATUS_ACTIVE;

    /*
     * Don't free all of program's memory if it is a TSR. Only the
     * "un-reserved" portion.
     */
    if (tsrFlag == 0)
    {
        newProc->status = PDOS_PROCSTATUS_TERMINATED;
        kfree(newProc->envBlock);
        removeFromProcessChain(newProc);
        kfree(pcb);
    }
    else
    {
        /* PDOS EXTENSION: tsrFlag=-1 means free no memory */
        if (tsrFlag != -1)
        {
            /* tsrFlag is in paragraphs (16 bytes). */
            krealloc(pcb, tsrFlag * 16);
        }
        /* Mark process as a TSR. */
        newProc->status = PDOS_PROCSTATUS_TSR;
    }

    /* Set TSR flag back to 0 */
    tsrFlag = 0;

    return (0);
}

static int fixexe32(unsigned char *psp, unsigned long entry, unsigned int sp,
                    unsigned long exeStart, unsigned long dataStart)
{
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

    commandLine = psp + 0x80;

    /* now we need to record the subroutine's absolute offset fix */
    oldsubcor = subcor;
    subcor = dataStart;

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

    memId += 256;
    ret = call32(entry, ADDRFIXSUB(&exeparms), sp);
    /* printf("ret is %x\n", ret); */
    memId -= 256;

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

static int isDriveValid(int drive)
{
    return drive >= 0 && drive < MAXDISKS && disks[drive].valid;
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
    if (!isDriveValid(drive))
        return POS_ERR_INVALID_DRIVE;
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
    if (!isDriveValid(drive))
        return POS_ERR_INVALID_DRIVE;
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
    if (!isDriveValid(drive))
        return POS_ERR_INVALID_DRIVE;
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

    if (!isDriveValid(drive))
        return POS_ERR_INVALID_DRIVE;
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

static int fileSeek(int fno, long offset, int whence, long *newpos)
{
    /* Checks if whence value is valid. */
    if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
    {
        /* MSDOS returns 0x1 (function number invalid). */
        return (POS_ERR_FUNCTION_NUMBER_INVALID);
    }
    *newpos = fatSeek(fhandle[fno].fatptr, &(fhandle[fno].fatfile),
                      offset, whence);
    return (0);
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
    fatInit(&disks[drive].fat, bpb, readLogical, writeLogical, &disks[drive],
            getDateTime);
    strcpy(disks[drive].cwd, "");
    disks[drive].accessed = 1;
    disks[drive].valid = 1;
    return;
}

static void upper_str(char *str)
{
    int x;

    for (x = 0; str[x] != '\0'; x++)
    {
        str[x] = toupper((unsigned char)str[x]);
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
        pdosWriteText(buf[x]);
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
    dopoweroff = pp->dopoweroff;
    bootBPB = ABSADDR(pp->bpb);
    protintHandler(0x0E, int0E);
    protintHandler(0x20, int20);
    protintHandler(0x21, int21);
    protintHandler(0x25, int25);
    protintHandler(0x26, int26);
    protintHandler(0x80, int80);
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

static void pdos16MemmgrSetOwner(MEMMGR *memmgr, void *ptr,
                                 unsigned long owner)
{
    unsigned long abs;

    abs = ADDR2ABS(ptr);
    abs -= 0x10000UL;
    abs -= (unsigned long)PDOS16_MEMSTART * 16;
    abs /= 16;
    abs += (unsigned long)PDOS16_MEMSTART * 16;
    ptr = ABS2ADDR(abs);
    (memmgrSetOwner)(memmgr, ptr, owner);
    return;
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
    if (!isDriveValid(tempDrive))
    {
        return POS_ERR_INVALID_DRIVE;
    }
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
#ifndef __32BIT__
    memmgrGetStats(&memmgr, stats);
#endif
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

void PosProcessGetMemoryStats(unsigned long pid, void *stats)
{
    MEMMGRSTATS *s = (MEMMGRSTATS*)stats;
#ifndef __32BIT__
    memmgrGetOwnerStats(&memmgr, pid, stats);
#endif
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
    return curPCB != NULL ? curPCB->pid : 0;
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

/* Find process with given PID */
PDOS_PROCESS *findProc(unsigned long pid)
{
    PDOS_PROCESS *cur = initProc;

    if (pid == 0)
        return cur;
    while (cur != NULL)
    {
        if (cur->pid == pid)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

/* Func F6.0B - Get info about a process. */
int PosProcessGetInfo(unsigned long pid, PDOS_PROCINFO *info, size_t infoSz)
{
    /* Find the process */
    PDOS_PROCESS *proc = findProc(pid);

    /* We reuse FILE_NOT_FOUND error for process not found.
     * Maybe we should create PROC_NOT_FOUND instead??? */
    if (proc == NULL)
    {
        return POS_ERR_FILE_NOT_FOUND;
    }

    /* Validate structure */
    if (info == NULL || infoSz != sizeof(PDOS_PROCINFO))
    {
        return POS_ERR_DATA_INVALID;
    }

    /* Populate structure */
    strncpy(info->exeName, proc->exeName, PDOS_PROCESS_EXENAMESZ-1);
    info->status = proc->status;
    info->pid = proc->pid;
    info->ppid = proc->parent==NULL?0:proc->parent->pid;
    info->prevPid = proc->prev==NULL?0:proc->prev->pid;
    info->nextPid = proc->next==NULL?0:proc->next->pid;

    /* Report success */
    return POS_ERR_NO_ERROR;
}

void PosClearScreen()
{
    BosClearScreen(currentAttrib);
}

void PosMoveCursor(int row, int col) /* func f6.31 */
{
    BosSetCursorPosition(currentPage, row, col);
}

int PosGetVideoInfo(pos_video_info *info, size_t size) /* func f6.32 */
{
    if (info == NULL || size != sizeof(pos_video_info))
        return POS_ERR_DATA_INVALID;
    BosGetVideoMode(&(info->cols), &(info->mode), &(info->page));
    info->rows = BosGetTextModeRows();
    BosReadCursorPosition(
        info->page,
        &(info->cursorStart),
        &(info->cursorEnd),
        &(info->row),
        &(info->col)
    );
    info->currentAttrib = currentAttrib;
    currentPage = info->page;
    return POS_ERR_NO_ERROR;
}

int PosKeyboardHit(void) /* func f6.33 */
{
    int scancode, ascii, ret;

    ret = BosReadKeyboardStatus(&scancode, &ascii);
    return (ret == 0);
}

void PosYield(void) /* func f6.34 */
{
    /* Reduce battery consumption on laptops - tell BIOS we are doing nothing */
    BosCpuIdle();
}

/* INT 21,F6,35 - Sleep */
void PosSleep(unsigned long seconds)
{
    BosSleep(seconds);
}

/* INT 21,F6,36 - Get tick count */
unsigned long PosGetClockTickCount(void)
{
    return BosGetClockTickCount();
}

static int pdosWriteText(int ch)
{
    unsigned int cursorStart;
    unsigned int cursorEnd;
    unsigned int row;
    unsigned int column;
    if (ch == 8)
    {
        BosReadCursorPosition(currentPage,&cursorStart,&cursorEnd,&row,&column);
        if (column > 0)
        {
            column--;
            BosSetCursorPosition(currentPage,row,column);
            BosWriteCharAttrib(currentPage, ' ', currentAttrib, 1);
        }
    }
    else if (ch == 7 || ch == 0xA || ch == 0xD)
        BosWriteText(currentPage,ch,0);
    else
    {
        BosReadCursorPosition(currentPage,&cursorStart,&cursorEnd,&row,&column);
        if (column == BosGetTextModeCols())
        {
            BosWriteText(currentPage,'\r',0);
            BosWriteText(currentPage,'\n',0);
            BosReadCursorPosition(currentPage,
                    &cursorStart,&cursorEnd,&row,&column);
        }
        BosWriteCharAttrib(currentPage, ch, currentAttrib, 1);
        column++;
        BosSetCursorPosition(currentPage,row,column);
    }
}

void PosSetVideoAttribute(unsigned int attr) /* func f6.37 */
{
    currentAttrib = attr;
}

int PosSetVideoMode(unsigned int mode) /* func f6.38 */
{
    int actual, cols, page;
    BosSetVideoMode(mode);
    BosGetVideoMode(&cols, &actual, &page);
    if (mode == actual) {
        currentPage = page;
        return POS_ERR_NO_ERROR;
    }
    return POS_ERR_ACCESS_DENIED;
}

int PosSetVideoPage(unsigned int page) /* func f6.39 */
{
    int mode, cols, actual;
    BosSetActiveDisplayPage(page);
    BosGetVideoMode(&cols, &mode, &actual);
    if (page == actual) {
        currentPage = page;
        return POS_ERR_NO_ERROR;
    }
    return POS_ERR_ACCESS_DENIED;
}

/*======== ENVIRONMENT MANAGEMENT FUNCTIONS ========*/

/*
 * Structure of DOS environment block is as follows:
 * NAME=VALUE\0NAME=VALUE\0NAME=VALUE\0\0 - this is env vars
 * Note that it must always terminate with two NULs \0\0 even if empty. After
 * this comes the "footer" or tail", which starts with a 16 bit value which is
 * the number of ASCIIZ strings following. Then that many ASCIIZ strings. That
 * 16-bit value is almost always 1 (maybe it is always 1?). The first such
 * ASCIIZ string is full path to the application. I don't think any further
 * such ASCIIZ strings were ever used, although the format was designed to
 * support such future extensibility.
 */

/* Get variable defined in given environment block */
static char * envBlockGetVar(char *envptr, char *name)
{
    size_t namelen = strlen(name);
    for (;;)
    {
        size_t n = strlen(envptr);
        if (n == 0)
            return NULL;
        if (strncmp(envptr,name,namelen) == 0 && envptr[namelen] == '=')
        {
            return envptr;
        }
        envptr += n + 1;
    }
}

/* Return environment block tail start (footer count pointer) */
static char *envBlockTail(char *envptr)
{
    size_t size = 0;
    for (;;)
    {
        size_t n = strlen(envptr + size);
        if (n == 0)
        {
            return envptr + size + 1 + (size == 0 ? 1 : 0);
        }
        size += n + 1;
    }
}

/* Count variables in environment block */
static int envBlockVarCount(char *envptr)
{
    size_t size = 0;
    int count = 0;
    for (;;)
    {
        size_t n = strlen(envptr + size);
        if (n == 0)
        {
            return count;
        }
        size += n + 1;
        count ++;
    }
}

/* Computes the size of an environment block */
static size_t envBlockSize(char *envptr)
{
    size_t size = 0;
    int footers, i;

    /* Skip over the environment variables */
    for (;;)
    {
        size_t n = strlen(envptr + size);
        if (n == 0)
        {
            size++;
            break;
        }
        size += n + 1;
    }
    if (size < 2)
        size = 2;
    /* envptr is now a 16-bit word count of footer ASCIIZ strings. This is
     * always 1, but the format technically allows more than one such string,
     * but the meaning of the second and subsequent strings isn't defined.
     * The first such string is full path to program executable.
     */
    footers = *((unsigned short *)&(envptr[size]));
    size += sizeof(unsigned short);
    /* Now skip over the footer strings */
    for (i = 0; i < footers; i++)
    {
        size_t n = strlen(envptr + size);
        size += n + 1;
    }
    /* Return overall size */
    return size;
}

static char * envAllocateEmpty(char *progName)
{
    char *envptr = NULL;
    size_t  envSize =
        2 /* \0\0 empty variables marker */ +
        sizeof(unsigned short) /* footer string count */ +
        strlen(progName) /* the program name (footer string 1) */ +
        1; /* \0 terminator of program name */
#ifdef __32BIT__
    envptr = kmalloc(envSize);
#else
    envptr = memmgrAllocate(&memmgr, envSize, 0);
    envptr = (unsigned char *)FP_NORM(envptr);
#endif
    /* Empty environment is two NUL bytes */
    envptr[0] = 0;
    envptr[1] = 0;
    *((unsigned short *)(envptr + 2)) = 1; /* footer count */
    strcpy(envptr + 2 + sizeof(unsigned short), progName);
    return envptr;
}

static char * envCopy(char *previous, char *progName)
{
    char *envptr = NULL;
    unsigned char *envTail;
    size_t  envSize =
        strlen(progName) + 1 + sizeof(unsigned short) + envBlockSize(previous);

#ifdef __32BIT__
    envptr = kmalloc(envSize);
#else
    envptr = memmgrAllocate(&memmgr, envSize, 0);
    envptr = (unsigned char *)FP_NORM(envptr);
#endif
    memcpy(envptr,previous,envSize);
    envTail = envBlockTail(envptr);
    *((unsigned short *)(envTail)) = 1;
    strcpy(envTail + sizeof(unsigned short), progName);
    return envptr;
}

static void envBlockCopyWithMods(char *src, char *dest, char *name, char *value)
{
    /* Did we find the variable to modify? */
    int found = 0;
    /* Size of name, value */
    size_t nameLen = strlen(name);
    size_t valueLen = value == NULL ? 0 : strlen(value);
    /* Count of footers */
    unsigned short footerCount = 0;
    /* Loop counter */
    int i;
    int count = envBlockVarCount(src);

    if (count == 0)
    {
        strcpy(dest,name);
        dest[nameLen] = '=';
        dest += nameLen + 1;
        strcpy(dest,value);
        dest += valueLen + 1;
        *dest = 0;
        dest++;
        src += 2;
    }
    else
    {
        /* Copy the environment variables */
        for (i=0;;i++)
        {
            size_t n = strlen(src);
            if (n > 0)
            {
                if (strncmp(src,name,nameLen) == 0 && src[nameLen] == '=')
                {
                    if (value != NULL)
                    {
                        memcpy(dest,src,nameLen+1);
                        dest += nameLen + 1;
                        strcpy(dest,value);
                        dest += valueLen + 1;
                    }
                    found = 1;
                }
                else
                {
                    strcpy(dest,src);
                    dest += n + 1;
                }
                src += n + 1;
            }
            else
            {
                if (!found && value != NULL)
                {
                    strcpy(dest,name);
                    dest[nameLen] = '=';
                    dest += nameLen + 1;
                    strcpy(dest,value);
                    dest += valueLen + 1;
                }
                *dest = 0;
                dest++;
                src++;
                /*
                if (i == 0)
                {
                    *dest = 0;
                    dest++;
                    src++;
                }
                */
                break;
            }
        }
    }

    /* Copy footer count */
    footerCount = *((unsigned short *)src);
    *((unsigned short *)dest) = footerCount;
    src += sizeof(unsigned short);
    dest += sizeof(unsigned short);

    /* Copy the footers */
    for (i = 0; i < footerCount; i++)
    {
        size_t n = strlen(src);
        strcpy(dest, src);
        dest += n + 1;
        src += n + 1;
    }
}

static char *envModify(char *envPtr, char *name, char *value)
{
    size_t size = envBlockSize(envPtr);
    int count = envBlockVarCount(envPtr);
    char *existing = envBlockGetVar(envPtr,name);
    int offset;
    char *newPtr;

    /* Remove when value is NULL */
    if (value == NULL)
    {
        /* If name to be removed not in environment, just return success */
        if (existing == NULL)
            return envPtr;
        /* Removing doesn't require any extra memory, so we can use the
         * current environment block. We just have to update the existing
         * segment.
         */
        /* Handle case when we are emptying the environment.
         * This is special-cased because we still need two NUL bytes even
         * in an empty environment.
        */
        if (count == 1)
        {
            memmove(envPtr, envPtr + strlen(existing),
                    size - strlen(existing));
            /* Return success */
            return envPtr;
        }
        /* Find offset */
        offset = existing - envPtr;
        /* Remove the variable */
        memmove(existing, existing + strlen(existing) + 1,
                size - strlen(existing) - 1);
        /* Return success */
        return envPtr;
    }

    /* We need to allocate a new segment */
    size += strlen(name) + strlen(value) + 2;
    newPtr = PosAllocMemPages(((size/16)+1), NULL);
    if (newPtr == NULL)
    {
        return NULL;
    }
#ifndef __32BIT__
    memmgrSetOwner(&memmgr, newPtr, curPCB->pid);
#endif

    /* Populate new segment */
    envBlockCopyWithMods(envPtr, newPtr, name, value);

    /* Free old segment */
    PosFreeMem(envPtr);

    /* Return success */
    return newPtr;
}

/* F6,3A - Set Environment Variable */
int PosSetEnv(char *name, char *value)
{
    char *envPtr = curPCB->envBlock;
    size_t size = envBlockSize(envPtr);
    int count = envBlockVarCount(envPtr);
    char *existing = envBlockGetVar(envPtr,name);
    int offset;
    char *newPtr;
#ifndef __32BIT__
    char *psp;
#endif

    envPtr = envModify(envPtr, name, value);
    if (envPtr == NULL)
    {
        return POS_ERR_INSUFFICIENT_MEMORY;
    }

    /* Update PCB */
    curPCB->envBlock = envPtr;
#ifndef __32BIT__
    /* Update the PSP (in 16-bit only) */
    psp = MK_FP(curPCB->pid, 0);
    *(unsigned int *)&psp[0x2c] = FP_SEG(envPtr);
#endif

    /* Return success */
    return POS_ERR_NO_ERROR;
}

/* F6,3B - Get Environment Block */
void * PosGetEnvBlock(void)
{
    return curPCB->envBlock;
}

static int ins_strcmp(char *one, char *two)
{
    while (toupper((unsigned char)*one) == toupper((unsigned char)*two))
    {
        if (*one == '\0')
        {
            return (0);
        }
        one++;
        two++;
    }
    if (toupper((unsigned char)*one) < toupper((unsigned char)*two))
    {
        return (-1);
    }
    return (1);
}

/* F6,3C - Set Named Font */
int PosSetNamedFont(char *fontName)
{
    if (fontName == NULL)
        return POS_ERR_DATA_INVALID;
    if (ins_strcmp(fontName,"8x14") == 0)
    {
        if (BosLoadTextModeRomFont(BOS_TEXTMODE_ROMFONT_8X14,0) == 0)
            return POS_ERR_NO_ERROR;
    }
    if (ins_strcmp(fontName,"8x8") == 0)
    {
        if (BosLoadTextModeRomFont(BOS_TEXTMODE_ROMFONT_8X8,0) == 0)
            return POS_ERR_NO_ERROR;
    }
    if (ins_strcmp(fontName,"8x16") == 0)
    {
        if (BosLoadTextModeRomFont(BOS_TEXTMODE_ROMFONT_8X16,0) == 0)
            return POS_ERR_NO_ERROR;
    }
    return POS_ERR_FILE_NOT_FOUND;
}

static void getDateTime(FAT_DATETIME *ptr)
{
    int dow1, dow2;

    PosGetSystemDate(&ptr->year, &ptr->month, &ptr->day, &dow1);
    PosGetSystemTime(&ptr->hours, &ptr->minutes, &ptr->seconds,
                     &ptr->hundredths);
    PosGetSystemDate(&ptr->year, &ptr->month, &ptr->day, &dow2);
    if (dow1 != dow2)
    {
        getDateTime(ptr);
    }
    return;
}
