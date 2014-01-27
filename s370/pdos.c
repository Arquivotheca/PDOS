/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pdos.c - Public Domain Operating System                          */
/*  (version for S/370, S/380 and S/390 IBM mainframes)              */
/*                                                                   */
/*  When this (main) program is entered, interrupts are disabled,    */
/*  and the program should not assume anything about the status of   */
/*  low memory - ie this program needs to set the interrupt vectors, */
/*  initialize timer, and enable interrupts.                         */
/*                                                                   */
/*  To cope with multitasking, and multiple CPUs, from the outset,   */
/*  the following strategy is being used:                            */
/*                                                                   */
/*  Given that most time is spent in an application, rather than     */
/*  doing OS work, we can envision that 4 CPUs are busy doing some   */
/*  calculations, e.g. LA R2,7(R2). They each have their own version */
/*  of the registers, and they have each been loaded in advance      */
/*  with virtual memory settings that only give access to real       */
/*  memory that doesn't clash with any other CPU.                    */
/*                                                                   */
/*  Each of those CPUs can generate an interrupt at any time, by     */
/*  executing an illegal instruction, or having an expired timer,    */
/*  or requesting a service.                                         */
/*                                                                   */
/*  Each time there is an interrupt, interrupts will be switched     */
/*  off. The CPU receiving the interrupt needs to save its           */
/*  registers, and return to the point at which it last dispatched   */
/*  some interruptable work.                                         */
/*                                                                   */
/*  So a function like dispatch_until_interrupt() should be called,  */
/*  and it is expected to return with an interrupt identifier.       */
/*                                                                   */
/*  Since the interrupt may be a program check etc, this task        */
/*  should remain suspended until the interrupt has been             */
/*  analyzed. So what we're after is:                                */
/*                                                                   */
/*  while (!time_to_terminate)                                       */
/*  {                                                                */
/*      dispatch_until_interrupt();                                  */
/*      if (interrupt == ILLEGAL_INSTRUCTION) time_to_terminate = 1; */
/*      else if (interrupt == TIMER)                                 */
/*      {                                                            */
/*          checkElapsedTimeAgainstAllowedTime();                    */
/*          if (over_time_limit) time_to_terminate = 1;              */
/*      }                                                            */
/*  }                                                                */
/*                                                                   */
/*  In order to effect this simplicity, what that                    */
/*  dispatch_until_interrupt function translates into is:            */
/*                                                                   */
/*  LPSW of user's application code, with interrupts enabled.        */
/*  All interrupts point to an address after that LPSW.              */
/*                                                                   */
/*  Registers are provided and loaded prior to execution of the      */
/*  LPSW. The LPSW will refer to an address in low memory so that    */
/*  no base register (ie R0) can be used.                            */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "pdosutil.h"

#include "__memmgr.h"

#define S370_MAXMB 16 /* maximum MB in a virtual address space for S/370 */
#define NUM_GPR 16 /* number of general purpose registers */
#define NUM_CR 16 /* number of control registers */
#define SEG_BLK 16 /* number of segments in a block */

#define ASPACE_ALIGN 4096 /* DAT tables need to be aligned on 4k boundary */

#define PSA_ORIGIN 0 /* the PSA starts at literally address 0 */

#define CR13_DUMMY 0x00001000 /* disable ATL memory access. This
  doesn't serve any practical purpose and should not be used. CR13
  is ignored when CR0.10 = 1 (XA mode) */

/*

MEMORY MAPS

real (S/390)
0-16: PDOS BTL + ATL
16-160: address space 0 private area
160-592: address spaces 1-3 private area (144 MB each)

virt (S/390)
0-5: PDOS BTL
5-15: priv BTL
15-26: PDOS ATL mostly
26-160: priv ATL


real (S/370)
0-5: PDOS BTL
5-16: unused
16-26: address space 0 private area
26-56: address spaces 1-3 private area (10 MB each)

virt (S/370)
0-5: PDOS BTL
5-15: priv BTL
15-16: PDOS reserved


real (S/380)
0-5: PDOS BTL
5-16: unused
16-26: address space 0 BTL private area
26-56: address spaces 1-3 private area (10 MB each)
64-192: address space 0 ATL private area (128 MB each)
192-576: address spaces 1-3 private area (128 MB each)

virt (S/380)
0-5: PDOS BTL
5-15: priv BTL
15-16: PDOS reserved
16-144: priv ATL (128 MB)



old real memory map:

0 = PLOAD executable
0.5 = PLOAD stack (also used by PDOS, and can extend into heap area)
1 = PLOAD heap
2 = PDOS code
3 = PDOS heap
5 = PCOMM/application code (except S/380 - where it is reserved)
9 = PCOMM/application dynamic storage (stack and heap) - except S/380
15 = system use (reserved)
16 = used to support extended addressing in S/380, else ATL memory
64 = used to support multiple ATL address spaces in S/380
80 = top of memory for non-380
320 = top of memory in S/380



old virtual memory map:

0 = system use
5 = application start
9 = dynamic storage
15 = system use
17 = application ATL memory use (start point may change)

*/


/* pload is a standalone loader, and thus is normally loaded
   starting at location 0. */
#define PLOAD_START 0x0 /* 0 MB */

/* when pload is directly IPLed, and thus starts from the
   address in location 0, it knows to create its own stack,
   which it does at 0.5 MB in (thus creating a restriction
   of only being 0.5 MB in size unless this is changed) */
/* Note that these definitions need to match corresponding
   defines in sapstart - or else make them variables and
   get the infrastructure to inform PDOS of whatever it
   needs to know. */
#define PLOAD_STACK (PLOAD_START + 0x080000) /* 0.5 MB */

/* the heap - for the equivalent of getmains - is located
   another 0.5 MB in, ie at the 1 MB location */
#define PLOAD_HEAP (PLOAD_STACK + 0x080000) /* 1 MB */

/* PDOS is loaded another 1 MB above the PLOAD heap - ie the 2 MB location */
#define PDOS_CODE (PLOAD_HEAP + 0x100000) /* 2 MB */

/* The heap starts 1 MB after the code (ie the 3 MB location).
   So PDOS can't be more than 1 MB in size unless this is changed.
   Note that PDOS doesn't bother to create its own stack, and instead
   relies on the one that pload is mandated to provide being 
   big enough, which is a fairly safe bet given that it is
   effectively 1.5 MB in size, since it can eat into the old heap
   if required. */
#define PDOS_HEAP (PDOS_CODE + 0x100000)  /* 3 MB */

/* approximately where pcomm executable is loaded to */
#define PCOMM_LOAD (PDOS_HEAP + 0x200000) /* 5 MB */

/* entry point for MVS executables can be at known location 8
   into mvsstart, so long as that is linked first. In the future,
   this information will be obtained from the entry point as
   recorded in the IEBCOPY unload RDW executable */
/* we will round pcomm to be on a 64k boundary */
#define PCOMM_ENTRY (PCOMM_LOAD + 0x10000 + 8)

/* where to get getmains from - allow for 4 MB executables */
#define PCOMM_HEAP (PCOMM_LOAD + 0x400000) /* 9 MB */

#define PDOS_STORINC 0x080000 /* storage increment */

#define PCOMM_ATL_START 0x1100000

#define CONFIG_STARTCYL 2 /* cylinder where config.sys can be found -
  this assumes that PLOAD and PDOS are less than 1 cylinder in
  size and that they have been allocated on cylinder boundaries */

#ifndef EA_ON
#ifndef S390
#define EA_ON 1 /* use extended addressing in 370 and 380 mode */
#else
#define EA_ON 0 /* only supported in 380 mode */
#endif
#endif

#define EA_CR 10 /* hopefully we don't need this control register */
#define EA_OFFSET 0x01000000 /* where extended addressing starts */
#define EA_END 0x04000000 /* where extended addressing ends */

#define BTL_PRIVSTART PCOMM_LOAD /* private region starts at 5 MB */
#define BTL_PRIVLEN 10 /* how many MB to give to private region */

#ifndef MAXASIZE
#if defined(S390)
#define MAXASIZE 160
#elif defined(S370)
#define MAXASIZE 16
#else
#define MAXASIZE 144 /* maximum of 144 MB for address space */
                     /* note that this needs to be a multiple of 16
                        in order for the current logic (MAXASIZE/SEG_BLK)
                        to work */
#endif
#endif

#ifndef MAXANUM
#define MAXANUM  4   /* maximum of 4 address spaces */
#endif

/* do you want to debug all SVC calls? */
#ifndef SVCDEBUG
#define SVCDEBUG 0
#endif

/* do you want to debug all memory requests */
#ifndef MEMDEBUG
#define MEMDEBUG 0
#endif

/* if you want to detect memory leaks */
#ifndef REPORT_MEMLEAK
#define REPORT_MEMLEAK 0
#endif

/* do you want to debug all disk requests */
#ifndef DSKDEBUG
#define DSKDEBUG 0
#endif


#define MAXPAGE 256 /* maximum number of pages in a segment */


/* The S/380 architecture provides a lot of options, and they
   can be selectively enabled here.
   
   First, here is a description of S/380.
   
   If CR0.10 (XA) is off and CR13 = 0, then ATL memory accesses are 
   done to real storage, ignoring storage keys (for ATL accesses only),
   even though (S/370) DAT is switched on. This provides instant 
   access to ATL memory for applications, before having to make 
   any OS changes at all. This is not designed for serious use 
   though, due to the lack of integrity in ATL memory for multiple 
   running programs. It does provide very quick proof-of-concept though.
   
   If in S/370 mode (CR0.10 off), and CR13 is non-zero, e.g. even 
   setting a dummy value of 0x00001000, then ATL memory accesses 
   are done via a DAT mechanism instead of using real memory.
   
   With the above dummy value, the size of the table is only
   sufficient to address BTL memory (ie 16 MB), so that value of 
   CR13 (note that CR13 is of the same format as CR1) effectively 
   prevents access to ATL memory completely.
   
   When CR13 contains a length signifying more than 16 MB of
   memory, then that is used to access ATL memory, and CR1 is
   ignored. Also CR0 is ignored as far as 64K vs 1 MB segments
   is concerned. 1 MB is used unconditionally (for ATL memory).

   However, even if CR13 is non-zero, it is ignored if CR0.10
   is set to 1. Anyone who sets CR0.10 to signify XA DAT is
   expected to provide the sole DAT used, and not be required
   to mess around with CR13. This provides the flexibility of
   some ("legacy") tasks being able to use split DAT, while
   others can be made to use a proper XA DAT.
   
   
   PDOS/380 has options to exercise a lot of the above.
   
   BTL_XA will set CR13 to the dummy value and set up a pure
   XA DAT. Otherwise 370 mode will be used below, and CR13
   will point to a proper sized XA DAT.
   
   SEG_64K will force either the BTL XA DAT, or the 370 DAT,
   to use 64K segments instead of the normal 1 MB.
   
*/

/* The S/370 architecture offers 1 MB and 64K segments, and
   either may be selected here. It is also possible to
   enable extended addressing, which will allow multiple BTL
   address spaces.
*/

/* The S/390 normally only provides 1 MB segments, but with
   Hercules/380 you can also have 64K segments should that
   serve some purpose.
*/



#if !defined(SEG_64K)
/* this can be used to set 64K segments as MVS 3.8j does */
#define SEG_64K 0
#endif

#if !defined(BTL_XA)
/* S/380 allows a split DAT. The most usual reason for using
   this is to allow S/370 addressing below the line, and
   XA addressing above the line. However, there is nothing in
   the architecture that prevents XA DAT from being used below
   the line, and this could theoretically be of some use in
   some circumstances. So setting BTL_XA will set up the page
   tables for XA use */
#define BTL_XA 0
#endif

#if SEG_64K
#undef MAXPAGE
#define MAXPAGE 16
#endif


#if EA_ON
#if (MAXANUM * BTL_PRIVLEN) > (EA_END - S370_MAXMB)
#error too many address spaces defined of given length
#endif
#endif

#if MAXASIZE % 16 != 0
#error maximum address size must be multiple of 16
#endif


/* event control block */
typedef int ECB;


typedef struct {
    int abend;
} TASK;

typedef struct rb {
    /* char filler1[96]; */
    int regs[NUM_GPR];
    unsigned int psw1;
    unsigned int psw2;
    struct rb *rblinkb;
    ECB *postecb;
    char *next_exe; /* next executable's location */
    int savearea[20]; /* needs to be in user space */
    char *pptrs[1];
} RB;

/* note that the TCB has more data in a prefix area */
typedef struct {
    char unused1[16];
    int tcbcmp;
} TCB;

typedef struct {
    char unused1[32];
    unsigned int svcopsw[2];
    char unused2[96];
    unsigned int svc_code;
    char unused[244];
    int flcgrsav[NUM_GPR];
} PSA;

typedef struct {
    char unused1a[5];
    char dcbfdad[8];
    char unused1b[19];
    int  eodad;
    union
    {
        char dcbrecfm;
        int dcbexlsa;
    } u2;
    char dcbddnam[8];
    union {
        char dcboflgs;
        int dcbgetput;
    } u1;
    int dcbcheck;
    char unused3[6];
    short dcbblksi;
    char unused4[18];
    short dcblrecl;
} DCB;

typedef struct {
    char filler1[30-16]; /* this 16 comes from IOBSTDRD +++ - fix */
    short residual;
} IOB;

typedef struct {
    char unused1[20];
    short ds4dstrk;
    char unused2[74];
} DSCB4;

typedef struct {
    char ds1dsnam[44]; /* dataset name */
    char ds1fmtid; /* must be set to '1' */
    char ds1dssn[6]; /* volser */
    char ds1volsq[2]; /* volume sequence number */
    char ds1credt[3]; /* creation date */
    char ds1expdt[3]; /* expiry date */
    char ds1noepv; /* number of extents */
        /* The count does not include the label track, if any. If the
           first extent type is x'40', then the data set has LABEL=SUL, and
           in (MVS) limited to 15 extents. */
        
    char ds1nobdb; /* number of free bytes in the last block of a
                      PDS. Hercules currently using random values
                      like 0x78 */
    char ds1flag1; 
    char ds1syscd[13]; /* system code - presumably name of operating system */
    char ds1refd[3]; /* reference date - ie last time accessed */
    char ds1smsfg;
    char ds1scxtf;
    short ds1scxtv;
    short ds1dsorg; /* dataset organization, e.g. PS or PO */
    char ds1recfm; /* record format, e.g. F/V/U */
    char ds1optcd; /* mainly used for BDAM and ISAM */
    short ds1blkl; /* block size */
    short ds1lrecl; /* logical record length */
    char ds1keyl;
    char ds1rkp[2];
    char ds1dsind; /* flags, e.g. is this the last volume for this dataset? */
    char ds1scal1; /* is this a cylinder request? */
    char ds1scal3[3]; /* size of secondary allocation */
    char ds1lstar[3]; /* last TTR actually used - ie the EOF record */
    char ds1trbal[2]; /* TRKCALC??? */
    char resv1; /* reserved */
    char ds1ttthi; /* a extra byte for TTR number, ie make lstart TTTR */
    char ds1ext1[2]; /* first extent - actualy 10 bytes, but we use 
                        different names currently. First byte is an
                        extent indicator, second byte is the extent
                        sequence number. Then we have start CCHH and
                        end CCHH (4 bytes each) */
    char startcchh[4];
    char endcchh[4];
    char ds1ext2[10]; /* second extent */
    char ds1ext3[10]; /* third extent */
    char ds1ptrds[5]; /* CCHHR pointing to a format-2 or format-3
                         DSCB which allows unlimited chaining so your
                         dataset can grow to fill the disk */
} DSCB1;



/* A S/370 logical address consists of a segment index, which is
   bits 8-11 (for 1MB index), for the 16 possible values, then
   a page index, bits 12-19, ie 8 bits, ie 256 entries
   and then a byte index, bits 20-31, ie 12 bits, ie up to 4095 */

/* For S/370XA it is bits 1-11, ie 11 bits, ie 2048 segments,
   with other values same as S/370. */

/* There are 2 main control registers of interest.

   CR0 determines things like the segment size and whether XA 
   mode is on or not.
   
   CR1 points to a set of DAT translation tables.
   
   In S/380, there is also a CR13 which is similar to CR1,
   but is only used for ATL memory access.
*/

typedef unsigned short UINT2;
typedef unsigned int UINT4;


/* for CR0: */
/* bits 8-9 = 10 (4K pages), bits 11-12 = 10 (1 MB segments) */
/* for S/370XA, bit 10 also needs to be 1 */
/* 
              0123 4567 89AB CDEF
   00800000 = 0000 0000 1000 0000  4k pages
   00900000 = 0000 0000 1001 0000  1M segments
   00A00000 = 0000 0000 1010 0000  XA dat (CR0.10 = 1)
   00B00000 = 0000 0000 1011 0000  4k, 1M and XA
*/

#if defined(S390)
static int cr0 = 0x00B00000;
#elif BTL_XA && SEG_64K
static int cr0 = 0x00A00000; /* 64K, S/390 */
#elif BTL_XA && !SEG_64K
static int cr0 = 0x00B00000; /* 1MB, S/390 */
#elif !BTL_XA && SEG_64K
static int cr0 = 0x00800000; /* 64K, S/370 */
#elif !BTL_XA && !SEG_64K
static int cr0 = 0x00900000; /* 1MB, S/370 */
#endif


/* for CR1: */
/* bits 0-7 = number of blocks (of 16) segment table entries */
/* specify 1 less than what you want, because there's an
   automatic + 1 */
/* bits 8-25, plus 6 binary zeros = 24-bit address of segment table */
/* for 16 MB, using 1 MB segments, we only need 1 block */
/* note that MVS 3.8j uses 64k segments rather than 1 MB segments,
   but we don't care about that here */
/* for S/370XA, it is bits 1-19, plus 12 binary zeros, to give
   a 31-bit address. Also bits 25-31 have the length as before */
/*static int cr1 = 0x01000000;*/ /* need to fill in at runtime */



/* the S/370 and S/390 hardware requires a 4-byte integer for
   segment table entries. */

/* for S/370, bits 0-3 have length, with an amount 1 meaning 1/16 of the 
   maximum size of a page table, so just need to specify 1111 here */
/* bits 8-28, plus 3 binary zeros = address of page table */
/* bit 31 needs to be 0 for valid segments */
/* so this whole table is only 128 bytes (per address space) */

/* for S/370XA this changes to bits 1-25 having the page table
   origin, with 6 binary zeros on the end, giving a 31-bit address.
   Bit 26 is in the invalid segment bit.
   Also bits 28-31 have the length, according to the same length
   rules as CR1 - ie blocks of 16, with an implied + 1,
   giving a maximum of 16*16 = 256
   page entries, sufficient (256 * 4096) to map the 1 MB segment,
   so just set to 1111 */
/* the segment table will thus need 2048 + 16 = approx 8K in size */

typedef UINT4 SEG_ENT370;
typedef UINT4 SEG_ENTRY;


/* the S/370 hardware requires a 2-byte integer for page
   table entries. S/370XA requires 4-byte */

/* for S/370, bits 0-11, plus 12 binary zeros = real memory address */
/* bit 12 = 0 for valid pages. All other bits to be 0 */
/* unless extended addressing for 370 is in place. Then bits 13
   and 14 become bits 6 and 7 of a 26-bit address */
/* so array dimensions are 16 = 16 segments */
/* and with each page entry addressing 4096 bytes, we need
   1024*1024/4096 = 256 to address the full 1 MB */
/* this whole table is only 8K (per address space) */

/* with S/370XA this becomes a 4-byte integer, with bits
   1-19 containing an address (when you add 12 binary zeros on
   the end to give a total of 19+12=31 bits) */
/* the page table will thus become 2048 instead of 16, and
   thus be 2 MB in size. Since this is so large, a #define
   is required to give a more reasonable maximum address
   space at compile time (or else run time). */
/* for example, 4 address spaces, each 128 MB in size, is
   equal to 256K */

typedef UINT2 PAGE_ENT370;
typedef UINT4 PAGE_ENTRY;





/* for S/380, we have a mixed/split DAT. CR1 continues normal
   S/370 behaviour, but if CR13 is non-zero, it uses an XA dat
   for any storage references above 16 MB. Note that the first
   16 MB should still be included in the XA storage table, but
   they will be ignored (unless CR0.10 is set to 1 - with DAT
   still on, which switches off 370 completely) */

/* one address space */

typedef struct {

#if !defined(S370)
#if SEG_64K
    SEG_ENTRY segtable[MAXASIZE*16]; /* needs 4096-byte alignment */
    PAGE_ENTRY pagetable[MAXASIZE*16][MAXPAGE]; /* needs 64-byte alignment */
#else
    SEG_ENTRY segtable[MAXASIZE]; /* needs 4096-byte alignment */
      /* segments are in blocks of 64, so will remain 64-byte aligned */
    PAGE_ENTRY pagetable[MAXASIZE][MAXPAGE]; /* needs 64-byte alignment */
#endif
#endif

#if !defined(S390)
#if SEG_64K
    SEG_ENT370 seg370[S370_MAXMB*16]; /* needs 64-byte alignment */
    PAGE_ENT370 page370[S370_MAXMB*16][MAXPAGE]; /* needs 8-byte alignment */
#else
    SEG_ENT370 seg370[S370_MAXMB]; /* needs 64-byte alignment */
    PAGE_ENT370 page370[S370_MAXMB][MAXPAGE]; /* needs 8-byte alignment */
#endif
#endif

    int cregs[NUM_CR];

    MEMMGR btlmem; /* manage memory below the line */
    MEMMGR atlmem; /* manage memory above the line */

    RB first_rb; /* first request block */
    RB *curr_rb; /* current request block */
    TCB tcb;
} ONESPACE;


/* each address space structure starts with the required
   DAT, which needs to be 4k-aligned, so we make sure there
   is a filler */

typedef struct {
    ONESPACE o;
    
    /* because the DAT tables need to be 4k-byte aligned, we
       need to split the structure in two and add a filler here */
    char filler[ASPACE_ALIGN + ASPACE_ALIGN 
                - (sizeof(ONESPACE) % ASPACE_ALIGN)];
} ASPACE;


#define DCBOFOPN 0x10
#define DCBRECU  0xC0
#define DCBRECF  0x80
#define DCBRECV  0x40
#define DCBRECBR 0x10

#define INTERRUPT_SVC 1

#if 0
TASK *task;
#endif

#define CHUNKSZ 18452 /* typical block size for files */

#define MAXBLKSZ 32767 /* maximum size a disk block can be */

typedef struct {
    ASPACE aspaces[MAXANUM]; /* needs to be 8-byte aligned because
         the segment points to a page table, using an address
         with 3 binary 0s appended. Due to the implied 6 binary 0s
         in XA segment tables, 64-byte alignment is required */
    RB *context; /* current thread's context */
    PSA *psa;
    int exitcode;
    int shutdown;
    int ipldev;
    int curdev;
    int curr_aspace; /* current address space */
} PDOS;

/*static PDOS pdos;*/

static DCB *gendcb = NULL; /* +++ need to eliminate this state info */
static IOB geniob; /* +++ move this somewhere */
static char lastds[FILENAME_MAX]; /* needs to be in TIOT */

void gotret(void);
int adisp(void);
void dread(void);
void dwrite(void);
void dcheck(void);
void dexit(int oneexit, DCB *dcb);
void datoff(void);
void daton(void);
extern int __consdn;

int pdosRun(PDOS *pdos);
void pdosDefaults(PDOS *pdos);
int pdosInit(PDOS *pdos);
void pdosTerm(PDOS *pdos);
static int pdosDispatchUntilInterrupt(PDOS *pdos);
static void pdosDumpregs(PDOS *pdos);
static void pdosInitAspaces(PDOS *pdos);
static void pdosProcessSVC(PDOS *pdos);
static void pdosSVC99(PDOS *pdos);
static int pdosDoDIR(PDOS *pdos, char *parm);
static void brkyd(int *year, int *month, int *day);
static int pdosNewF(PDOS *pdos, char *parm);
static int pdosGetMaxima(PDOS *pdos, int *dircyl, int *dirhead,
                         int *dirrec, int *datacyl);
static int pdosDumpBlk(PDOS *pdos, char *parm);
static int pdosZapBlk(PDOS *pdos, char *parm);
static int pdosLoadExe(PDOS *pdos, char *prog, char *parm);
static int pdosDumpMem(PDOS *pdos, void *buf, int cnt);
#if 0
static int pdosLoadPE(PDOS *pdos, char *prog, char *parm);
#endif
static int pdosLoadPcomm(PDOS *pdos);

static void split_cchhr(char *cchhr, int *cyl, int *head, int *rec);
static void join_cchhr(char *cchhr, int cyl, int head, int rec);


int __consrd(int len, char *buf);
int __conswr(int len, char *buf, int cr);

int main(int argc, char **argv)
{
    PDOS *pdos;
    char *p;
    int remain;
    int ret = EXIT_FAILURE;

    /* we need to align the PDOS structure on a 4k boundary */
    p = malloc(sizeof(PDOS) + ASPACE_ALIGN);
    if (p != NULL)
    {
        remain = (unsigned int)p % ASPACE_ALIGN;
        p += (ASPACE_ALIGN - remain);
        pdos = (PDOS *)p;

        pdosDefaults(pdos);
        if (pdosInit(pdos)) /* initialize address spaces etc */
        {
            ret = pdosRun(pdos); /* dispatch tasks */
            pdosTerm(pdos);
        }
    }
    return (ret);
}


/* pdosRun - the heart of PDOS */

int pdosRun(PDOS *pdos)
{   
    int intrupt;

    while (!pdos->shutdown)
    {
        intrupt = pdosDispatchUntilInterrupt(pdos);
        if (intrupt == INTERRUPT_SVC)
        {
            pdosProcessSVC(pdos);
        }
    }

    return (pdos->exitcode);
}


/* initialize any once-off variables */
/* cannot fail */

void pdosDefaults(PDOS *pdos)
{
    pdos->psa = PSA_ORIGIN;    
    return;
}


/* initialize PDOS */
/* zero return = error */

int pdosInit(PDOS *pdos)
{
    int cyl;
    int head;
    int rec;

    pdos->ipldev = initsys();
    pdos->curdev = pdos->ipldev;
    lcreg0(cr0);
    if (findFile(pdos->ipldev, "CONFIG.SYS", &cyl, &head, &rec) != 0)
    {
        printf("config.sys missing\n");
        return (0);
    }
    if (__consdn == 0)
    {
        char tbuf[MAXBLKSZ + 2];
        int cnt;

#if DSKDEBUG
        printf("loading to %p from %d, %d, %d\n", tbuf,
               cyl, head, rec);
#endif
        /* the chances of anyone having a herc config file more
           than a block in size are like a million gazillion to one */
        cnt = rdblock(pdos->ipldev, cyl, head, rec, tbuf, MAXBLKSZ);
#if DSKDEBUG
        printf("cnt is %d\n", cnt);
#endif
        /* find out which device console really is */
        if (cnt > 0)
        {
            char *p;
            char *q;
            char *fin;
            int ctr = 0;

            strcpy(tbuf + cnt, "\n"); /* ensure sentinel */
            fin = tbuf + cnt;
            p = tbuf;
            while (p < fin)
            {
                q = strchr(p, '\n');
                *q = '\0';
                if (isdigit((unsigned char)p[0]))
                {
                    if (strstr(p, "3215") != NULL)
                    {
#if defined(S390)
                        __consdn = 0x10000 + ctr;
#else
                        sscanf(p, "%x", &__consdn);
#endif
                        break;
                    }
                    ctr++;
                }
                p = q + 1;
            }
        }
    }
    printf("Welcome to PDOS!!!\n");
#if 0
    printf("CR0 is %08X\n", cr0);
    printf("PDOS structure is %d bytes\n", sizeof(PDOS));
    printf("aspace padding is %d bytes\n", sizeof pdos->aspaces[0].filler);

    printf("IPL device is %x\n", pdos->ipldev);
#endif
    pdos->shutdown = 0;
    pdosInitAspaces(pdos);
    pdos->curr_aspace = 0;
    pdos->context =
        pdos->aspaces[pdos->curr_aspace].o.curr_rb =
        &pdos->aspaces[pdos->curr_aspace].o.first_rb;
    
    /* Now we set the DAT pointers for our first address space,
       in preparation for switching DAT on. */
    lcreg1(pdos->aspaces[pdos->curr_aspace].o.cregs[1]);
#if defined(S380)
    lcreg13(pdos->aspaces[pdos->curr_aspace].o.cregs[13]);
#endif
    daton();

    if (pdosLoadPcomm(pdos) != 0)
    {
        datoff();
        return (0);
    }
    return (1);
}


/* pdosTerm - any cleanup routines required */

void pdosTerm(PDOS *pdos)
{
    /* switch DAT off so that we return to the caller (probably
       pload) in the same state we were called. */
    datoff();
    return;
}


static int pdosDispatchUntilInterrupt(PDOS *pdos)
{
    int ret;

    while (1)
    {
        /* store registers and PSW in low memory to be loaded */
        memcpy(pdos->psa->flcgrsav,
               pdos->context->regs,
               sizeof pdos->context->regs);

        pdos->psa->svcopsw[0] = pdos->context->psw1;
        pdos->psa->svcopsw[1] = pdos->context->psw2;
                       
        ret = adisp();  /* dispatch */
        
        /* restore registers and PSW from low memory */
        memcpy(pdos->context->regs,
               pdos->psa->flcgrsav,               
               sizeof pdos->context->regs);
        pdos->context->psw1 = pdos->psa->svcopsw[0];
        pdos->context->psw2 = pdos->psa->svcopsw[1];
        
        if (ret == 0) break;

        /* non-zero returns are requests for services */
        /* these "DLLs" don't belong in the operating system proper,
           but for now, we'll action them here. Ideally we want to
           get rid of that while loop, and run the DLL as normal
           user code. Only when it becomes time for WRITE to run
           SVC 0 (EXCP) or CHECK to run SVC 1 (WAIT) should it be
           reaching this bit of code. It's OK to keep the code
           here, but it needs to execut in user space. */
        if (ret == 2) /* got a WRITE request */
        {
            int len;
            char *buf;
            int cr = 0;
            char tbuf[MAXBLKSZ + 8];
            char *fin;
            int l;
            char *p;
            char *q;
            char *decb;
            DCB *dcb;
            int *pptr;

            /* A write request has R1 pointing to a parameter list
               (of fullwords etc), known as the DECB.
               first is the ECB (fullword)
               then 8 bits of read flags
               then 8 bits of write flags
               then halfword length of buffer to write.
               then DCB address (fullword)
               then buffer address (fullword)
               then optional record address (fullword) */

            decb = (char *)pptr;
            pptr = (int *)pdos->context->regs[1];
            len = pptr[1] & 0xffff;
            buf = (char *)pptr[3];
            dcb = (DCB *)pptr[2];

#if 0
            printf("got a request to write block %p, len %d\n", buf, len);
#endif
            if (memcmp(dcb->dcbfdad,
                       "\x00\x00\x00\x00\x00\x00\x00\x00", 8) == 0)
            {
                /* assume RECFM=U, with caller providing NL */            
                /* although this was specified to be a unit record
                   device, some applications ignore that, so be prepared
                   for more than one NL */
                fin = buf + len;
                p = buf;
                while (p < fin)
                {
                    q = memchr(p, '\n', fin - p);
                    if (q == NULL)
                    {
                        cr = 0;
                        q = fin;
                    }
                    else
                    {
                        cr = 1; /* assembler needs to do with CR */
                    }
                    memcpy(tbuf, p, q - p);
                    __conswr(q - p, tbuf, cr);
                    p = q;
                    if (cr)
                    {
                        p++;
                    }
                }
            }
            else
            {
                int cyl;
                int head;
                int rec;
                int cnt;
                
                split_cchhr(dcb->dcbfdad + 3, &cyl, &head, &rec);
                rec++;
#if DSKDEBUG
                printf("writing from %p to %d, %d, %d\n",
                       buf,
                       cyl, head, rec);
#endif
                memcpy(tbuf, "\x00\x01\x00\x00\x0d\x2c\x00\x60", 8);
                *(short *)tbuf = cyl;
                *(short *)(tbuf + 2) = head;
                tbuf[4] = rec;
                tbuf[5] = 0;
                *(short *)(tbuf + 6) = len;
                memcpy(tbuf + 8, buf, len);

                /* record number must be one less when using 0x1d
                   destructive (of track) write (to set block size) */
                cnt = wrblock(pdos->ipldev, cyl, head, rec - 1,
                              tbuf, len + 8, 0x1d);
#if DSKDEBUG
                printf("cnt is %d\n", cnt);
#endif
                /* we get a count of 0 when full, so need to retry */
                if (cnt <= 0)
                {
                    rec = 1;
                    head++;
                    *(short *)(tbuf + 2) = head;
                    tbuf[4] = rec;
                    cnt = wrblock(pdos->ipldev, cyl, head, rec - 1,
                                  tbuf, len + 8, 0x1d);
#if DSKDEBUG
                    printf("cnt is %d\n", cnt);
#endif
                    if (cnt <= 0)
                    {
                        head = 0;
                        cyl++;
                        *(short *)tbuf = cyl;
                        *(short *)(tbuf + 2) = head;
                        cnt = wrblock(pdos->ipldev, cyl, head, rec - 1,
                                      tbuf, len + 8, 0x1d);
                    }
                }
#if DSKDEBUG
                printf("cnt is %d\n", cnt);
#endif
                join_cchhr(dcb->dcbfdad + 3, cyl, head, rec);
            }
        }
        else if (ret == 3) /* got a READ request */
        {
            int len;
            char *buf;
            char tbuf[MAXBLKSZ];
            int cnt;
            char *decb;
            DCB *dcb;
            int *pptr;

            /* A read request has R1 pointing to a parameter list
               (of fullwords etc), known as the DECB.
               first is the ECB (fullword)
               then 8 bits of read flags
               then 8 bits of write flags
               then halfword length of buffer to read.
               then DCB address (fullword)
               then buffer address (fullword)
               then optional record address (fullword) */

            pptr = (int *)pdos->context->regs[1];
            decb = (char *)pptr;
            len = pptr[1] & 0xffff;
            buf = (char *)pptr[3];
            dcb = (DCB *)pptr[2];
#if 0
            printf("dcb read is for lrecl %d\n", dcb->dcblrecl);
#endif
            if (memcmp(dcb->dcbfdad,
                       "\x00\x00\x00\x00\x00\x00\x00\x00", 8) == 0)
            {
                cnt = __consrd(300, tbuf);
                if (cnt >= 0)
                {
                    tbuf[cnt++] = '\n';
                }
            }
            else
            {
                int cyl;
                int head;
                int rec;
                
                split_cchhr(dcb->dcbfdad + 3, &cyl, &head, &rec);
                rec++;
#if DSKDEBUG
                printf("reading into %p from %d, %d, %d\n",
                       buf,
                       cyl, head, rec);
#endif
                cnt = rdblock(pdos->ipldev, cyl, head, rec, 
                              tbuf, len);
                if (cnt < 0)
                {
                    rec = 1;
                    head++;
                    cnt = rdblock(pdos->ipldev, cyl, head, rec, 
                                  tbuf, len);
                    if (cnt < 0)
                    {
                        head = 0;
                        cyl++;
                        cnt = rdblock(pdos->ipldev, cyl, head, rec, 
                                      tbuf, len);
                    }
                }
#if DSKDEBUG
                printf("cnt is %d\n", cnt);
#endif
                join_cchhr(dcb->dcbfdad + 3, cyl, head, rec);
            }
            if (cnt <= 0)
            {
                /* need to return to EOF routine - don't need to
                   return here */
                pdos->context->psw2 = ((dcb->eodad & 0x7fffffff)
                                       | (pdos->context->psw2 & 0x80000000));
            }
            else
            {
                memcpy(buf, tbuf, cnt);
                geniob.residual = (short)(len - cnt);
                *(IOB **)(decb + 16) = &geniob;
            }
        }
    }
    return (INTERRUPT_SVC); /* only doing SVCs at the moment */
}


static void pdosDumpregs(PDOS *pdos)
{
    int x;
    int alt1;
    int p;
    int alt2;
    
    p = pdos->context->regs[1];
    p &= 0x7fffffff;
    if ((p >= 0) && (p < MAXASIZE * 1024 * 1024))
    {
        /* nothing */
    }
    else
    {
        p = 0;
    }
    for (x = 0; x < 16; x++)
    {
        alt1 = pdos->context->regs[x];
        alt1 &= 0x7fffffff;
        if ((alt1 >= 0) && (alt1 < MAXASIZE * 1024 * 1024))
        {
            alt1 = *(int *)alt1;
        }
        else
        {
            alt1 = -1;
        }        
        alt2 = ((int *)p)[x];
        alt2 &= 0x7fffffff;
        if ((alt2 >= 0) && (alt2 < MAXASIZE * 1024 * 1024))
        {
            alt2 = *(int *)alt2;
        }
        else
        {
            alt2 = -1;
        }        
        printf("register %2d is %08X, alt %08X, parm %08X %08X\n",
               x, pdos->context->regs[x], alt1, ((int *)p)[x], alt2);
    }
    return;
}


/* we initialize all address spaces upfront, with fixed locations
   for all segments. The whole approach should be changed though,
   with address spaces created dynamically and the virtual space
   not being allocated to real memory until it is actually
   attempted to be used. */

static void pdosInitAspaces(PDOS *pdos)
{
    int s;
    int a;
    int p;

    /* initialize 370-DAT tables */
#if (defined(S370) || defined(S380)) && !BTL_XA
    for (a = 0; a < MAXANUM; a++)
    {
#if SEG_64K
        for (s = 0; s < (S370_MAXMB * 16); s++)
#else
        for (s = 0; s < S370_MAXMB; s++)
#endif
        {
            /* we only store the upper 21 bits of the page
               table address. therefore, the lower 3 bits
               will need to be truncated (or in this case,
               assumed to be 0), and no shifting or masking
               is required in this case. The assumption of
               0 means that the page table must be 8-byte aligned */
            pdos->aspaces[a].o.seg370[s] = (0xfU << 28)
                                 | (unsigned int)pdos->aspaces[a].o.page370[s];
            for (p = 0; p < MAXPAGE; p++)
            {
                int real = 0; /* real memory location */
                int extrabits = 0;

                /* because the address starts in bit 0, it is
                   8 bits too far to be a normal 24-bit address,
                   otherwise we could shift 20 bits to get 1 MB.
                /* and 12 bits to move the 4K page into position */
                /* However, since we only have a 16-bit target,
                   need to subtract 16 bits in the shifting.
                   But after allowing for the 8 bits, it's only
                   8 that needs to be subtracted */
                /* Also if we're using extended addressing, place
                   private region up there */
                real =
#if SEG_64K
                       (s << (20-4))
#else
                       (s << 20)
#endif
                       | (p << 12);

                /* the private areas are all put into the EA
                   region which starts at 16 MB */
#if EA_ON
#if SEG_64K
                if ((s * 1024 * 1024/16 >= BTL_PRIVSTART) 
                    && (s * 1024 * 1024/16 < 
                        (BTL_PRIVSTART + BTL_PRIVLEN * 1024 * 1024)))
#else
                if ((s * 1024 * 1024 >= BTL_PRIVSTART) 
                    && (s * 1024 * 1024 < 
                        (BTL_PRIVSTART + BTL_PRIVLEN * 1024 * 1024)))
#endif
                {
                    real += EA_OFFSET + a * BTL_PRIVLEN * 1024 * 1024;
                }

                if (real >= (S370_MAXMB * 1024 * 1024))
                {
                    /* get bits 6 and 7 of 26-bit address */
                    /* the - 1 is to position rightmost bit on bit 14 */
                    extrabits = (real & 0x03000000) >> (24 - 1);
                    /* trim to 24-bit address */
                    real = real & 0x00FFFFFF;
                }
#endif
                pdos->aspaces[a].o.page370[s][p] = 
                    ((real >> 8) | extrabits) & 0xFFFF;
            }
        }

        /* 1 - 1 is for 1 block of 16, minus 1 implied */
        pdos->aspaces[a].o.cregs[1] = 
#if SEG_64K
                               ((S370_MAXMB - 1) << 24)
#else
                               ((1 - 1) << 24)
#endif
                               | (unsigned int)pdos->aspaces[a].o.seg370;
        /* note that the CR1 needs to be 64-byte aligned, to give
           6 low zeros */
#if 0
        printf("aspace %d, seg370 %p, cr1 %08X\n",
               a, pdos->aspaces[a].o.seg370, pdos->aspaces[a].o.cregs[1]);
#endif
        /* now set up the memory manager for BTL storage */
        lcreg1(pdos->aspaces[a].o.cregs[1]);
        daton();
        memmgrDefaults(&pdos->aspaces[a].o.btlmem);
        memmgrInit(&pdos->aspaces[a].o.btlmem);
        memmgrSupply(&pdos->aspaces[a].o.btlmem,
                     (char *)BTL_PRIVSTART,
                     BTL_PRIVLEN * 1024 * 1024);
        datoff();
    }
#endif


    /* initialize XA-DAT tables */
#if defined(S380)
    for (a = 0; a < MAXANUM; a++)
    {
/* normally even if it is SEG_64K, we ignore that for the ATL DAT
   tables, because ATL is still 1 MB segments. However, if we're
   using XA BTL, then we accept it ATL too. */
#if SEG_64K && BTL_XA
        for (s = 0; s < (MAXASIZE * 16); s++)
#else
        for (s = 0; s < MAXASIZE; s++)
#endif
        {
            int adjust = 0;

/* If EA is on, it is mixed DAT, and thus implied 1M segment size */
/* If we're not using EA, then just map all address spaces onto
   the same one, implying that a single address space should be
   used. */
#if EA_ON
            if (s >= S370_MAXMB)
            {
                /* steer clear of the bottom 64 MB */
                adjust = EA_END / (1024 * 1024);
                /* for the ATL portion of the virtual address, adjust
                   appropriately */
                adjust += a * (MAXASIZE - S370_MAXMB) - S370_MAXMB;
            }
#endif

            /* no shifting of page table address is required,
               but the low order 12 bits must be 0, ie address must
               be aligned on 64-byte boundary */
            pdos->aspaces[a].o.segtable[s] = 
#if SEG_64K && BTL_XA
                  0x0U
#else
                  0xfU
#endif
                  | (unsigned int)pdos->aspaces[a].o.pagetable[s];
            for (p = 0; p < MAXPAGE; p++)
            {
                /* because the address begins in bit 1, just like
                   a 31-bit bit address, we just need to shift
                   20 bits (1 MB) to get the right address. Plus
                   add in the page number, by shifting 12 bits
                   for the 4K multiple */
                pdos->aspaces[a].o.pagetable[s][p] = 
#if SEG_64K && BTL_XA
                                  (s << 16)
#else
                                  ((s + adjust) << 20)
#endif
                                  | (p << 12);
            }
        }
#if defined(S380) && !BTL_XA
        /* S/380 references XA-DAT memory via CR13, not CR1 */
        pdos->aspaces[a].o.cregs[13] =
#else
        pdos->aspaces[a].o.cregs[1] = 
#endif
            /* - 1 because architecture implies 1 extra block of 16 */
#if SEG_64K && BTL_XA
            (MAXASIZE/SEG_BLK * 16 - 1)
#else
            (MAXASIZE/SEG_BLK - 1)
#endif
            | (unsigned int)pdos->aspaces[a].o.segtable;
            /* note that the CR1 needs to be 4096-byte aligned, to give
               12 low zeros */
#if defined(S380) && BTL_XA
        /* set a dummy CR13 since we're using XA below the line,
           and don't want the simple version of S/380 to take effect */
        pdos->aspaces[a].o.cregs[13] = CR13_DUMMY;
#endif
#if 0
        printf("aspace %d, seg %p, cr13 %08X\n",
               a, pdos->aspaces[a].o.segtable, pdos->aspaces[a].o.cregs[13]);
#endif
        /* now set up the memory manager for BTL and ATL storage */
        lcreg13(pdos->aspaces[a].o.cregs[13]);
        lcreg1(pdos->aspaces[a].o.cregs[1]);
        daton();
/* if BTL is XA, we won't have defined memmgr yet */
#if BTL_XA
        memmgrDefaults(&pdos->aspaces[a].o.btlmem);
        memmgrInit(&pdos->aspaces[a].o.btlmem);
        memmgrSupply(&pdos->aspaces[a].o.btlmem,
                     (char *)BTL_PRIVSTART,
                     BTL_PRIVLEN * 1024 * 1024);
#endif
        memmgrDefaults(&pdos->aspaces[a].o.atlmem);
        memmgrInit(&pdos->aspaces[a].o.atlmem);
        memmgrSupply(&pdos->aspaces[a].o.atlmem,
                     (char *)(S370_MAXMB * 1024 * 1024),
                     (MAXASIZE-S370_MAXMB) * 1024 * 1024);
        datoff();
    }
#endif


/* For S/390 DAT there are no EA considerations. So we set
   aside the first 5 MB virtual for the OS, then 10 MB for 
   BTL address space, then 11 MB for the OS, then the rest
   for the ATL address space. This requires 16 MB real for
   PDOS, plus (10 + ATL) * # address spaces. */

#if defined(S390)
    for (a = 0; a < MAXANUM; a++)
    {
#if SEG_64K
        for (s = 0; s < (MAXASIZE * 16); s++)
#else
        for (s = 0; s < MAXASIZE; s++)
#endif
        {
            int r = 0;
            static int atl = MAXASIZE - S370_MAXMB - BTL_PRIVLEN;
            static int btl = BTL_PRIVLEN;
            static int pbtl = BTL_PRIVSTART / (1024 * 1024);
            static int patl = S370_MAXMB - BTL_PRIVSTART / (1024 * 1024);

            r = s;
            /* if less than 5 MB, it is PDOS, so no adjustment required */
            if (s < pbtl)
            {
                /* no adjustment required, V=R */
            }
            
            /* if we're in the BTL private region, that requires us to
               skip previous address spaces plus all PDOS memory
               (PDOS gets the entire 16 MB BTL real), then zero base it */
            else if (s < (pbtl + btl))
            {
                r += a * (btl + atl);
                r += pbtl + patl;
                r -= pbtl;
            }
            
            /* if we're in the PDOS atl area, map to BTL real (PDOS
               gets the lot) */
            else if (s < (pbtl + btl + patl))
            {
                r -= (pbtl + btl);
                r += pbtl;
            }
            
            /* if we're in the ATL private area, need to skip previous
               address spaces, plus pdos, then zero-base */
            else if (s < (pbtl + btl + patl))
            {
                r += a * (btl + atl);
                r += (pbtl + patl);
                r -= (pbtl + btl + patl);
            }

            /* no shifting of page table address is required,
               but the low order 12 bits must be 0, ie address must
               be aligned on 64-byte boundary */
            pdos->aspaces[a].o.segtable[s] = 
#if SEG_64K
                  0x0U
#else
                  0xfU
#endif
                  | (unsigned int)pdos->aspaces[a].o.pagetable[s];
            for (p = 0; p < MAXPAGE; p++)
            {
                /* because the address begins in bit 1, just like
                   a 31-bit bit address, we just need to shift
                   20 bits (1 MB) to get the right address. Plus
                   add in the page number, by shifting 12 bits
                   for the 4K multiple */
                pdos->aspaces[a].o.pagetable[s][p] = 
#if SEG_64K
                                  (s << 16)
#else
                                  (r << 20)
#endif
                                  | (p << 12);
            }
        }
        pdos->aspaces[a].o.cregs[1] = 
            /* - 1 because architecture implies 1 extra block of 16 */
#if SEG_64K
            (MAXASIZE/SEG_BLK * 16 - 1)
#else
            (MAXASIZE/SEG_BLK - 1)
#endif
            | (unsigned int)pdos->aspaces[a].o.segtable;
            /* note that the CR1 needs to be 4096-byte aligned, to give
               12 low zeros */
#if 0
        printf("aspace %d, seg %p, cr13 %08X\n",
               a, pdos->aspaces[a].o.segtable, pdos->aspaces[a].o.cregs[13]);
#endif

        /* now set up the memory manager for BTL and ATL storage */
        lcreg1(pdos->aspaces[a].o.cregs[1]);
        daton();
        memmgrDefaults(&pdos->aspaces[a].o.btlmem);
        memmgrInit(&pdos->aspaces[a].o.btlmem);
        memmgrSupply(&pdos->aspaces[a].o.btlmem,
                     (char *)BTL_PRIVSTART,
                     BTL_PRIVLEN * 1024 * 1024);

        memmgrDefaults(&pdos->aspaces[a].o.atlmem);
        memmgrInit(&pdos->aspaces[a].o.atlmem);
        memmgrSupply(&pdos->aspaces[a].o.atlmem,
                     (char *)(S370_MAXMB * 1024 * 1024),
                     (MAXASIZE - S370_MAXMB) * 1024 * 1024);
        datoff();
    }
    
#endif

    return;
}



static void pdosProcessSVC(PDOS *pdos)
{
    int svc;
    int getmain;
       /* should move to PDOS and use memmgr - but virtual memory
          will obsolete anyway */

    svc = pdos->psa->svc_code & 0xffff;
#if SVCDEBUG
    printf("SVC code is %d\n", svc);
#endif
    if (svc == 3)
    {
        /* if this is a LINKed program, then restore old context
           rather than shutting down */
        if (pdos->context->rblinkb == 
            &pdos->aspaces[pdos->curr_aspace].o.first_rb)
        {
            /* normally the OS would not exit on program end */
            printf("return from PCOMM is %d\n", pdos->context->regs[15]);
            pdos->shutdown = 1;
            pdos->exitcode = pdos->context->regs[15];
        }
        else
        {
            /* set ECB of person waiting */
            /* +++ needs to run at user priviledge */
            *pdos->context->rblinkb->postecb =
                pdos->aspaces[pdos->curr_aspace].o.tcb.tcbcmp =
                pdos->context->regs[15];
            pdos->aspaces[pdos->curr_aspace].o.curr_rb = 
                pdos->context->rblinkb;

            /* free old context */
            memmgrFree(&pdos->aspaces[pdos->curr_aspace].o.btlmem,
                       pdos->context);

            pdos->context = pdos->aspaces[pdos->curr_aspace].o.curr_rb;
            pdos->context->regs[15] = 0; /* signal success to caller */
            /* R1 they are expecting the TCB of the (now-completed)
               task - ie, yes, this is fudged, as we only support LINK */
            pdos->context->regs[1] = 
                (int)&pdos->aspaces[pdos->curr_aspace].o.tcb;

            /* free the memory that was allocated to the executable */
            memmgrFree(&pdos->aspaces[pdos->curr_aspace].o.btlmem,
                       pdos->context->next_exe);
#if REPORT_MEMLEAK
            printf("exited program, total free space is %d\n",
                   memmgrTotSize(&pdos->aspaces[pdos->curr_aspace].o.btlmem));
#endif
        }
    }
    else if ((svc == 120) || (svc == 10))
    {
        /* if really getmain */
        if (((svc == 10) && (pdos->context->regs[1] < 0))
            || ((svc == 120) && (pdos->context->regs[1] == 0)))
        {
            int len;

            pdos->context->regs[15] = 0;
            len = pdos->context->regs[0];
            if (svc == 10)
            {
                len &= 0xffffff;
            }
            if (len < (16 * 1024 * 1024))
            {
                getmain = (int)memmgrAllocate(
                    &pdos->aspaces[pdos->curr_aspace].o.btlmem,
                    len,
                    0);
#if MEMDEBUG
                printf("allocated %x for r0 %x, r1 %x, r15 %x\n", getmain,
                    len, pdos->context->regs[1],
                    pdos->context->regs[15]);
#endif
            }
            else
            {
#ifdef S370
                /* trim down excessive getmains in S/370 to cope
                   with the user of memmgr */
                getmain = (int)memmgrAllocate(
                    &pdos->aspaces[pdos->curr_aspace].o.btlmem,
                    PDOS_STORINC,
                    0);
#else
                getmain = (int)memmgrAllocate(
                    &pdos->aspaces[pdos->curr_aspace].o.atlmem,
                    len,
                    0);
#endif
#if MEMDEBUG
                printf("allocated %x for r0 %x, r1 %x, r15 %x\n", getmain,
                    len, pdos->context->regs[1],
                    pdos->context->regs[15]);
#endif
            }
            pdos->context->regs[1] = getmain;
        }
        /* freemain */
        else
        {
#if MEMDEBUG
            printf("freeing r0 %x, r1 %x\n",
                    pdos->context->regs[0], pdos->context->regs[1]);
#endif
            if (pdos->context->regs[1] < (16 * 1024 * 1024))
            {
                memmgrFree(&pdos->aspaces[pdos->curr_aspace].o.btlmem,
                           (char *)pdos->context->regs[1]);
            }
            else
            {
                memmgrFree(&pdos->aspaces[pdos->curr_aspace].o.atlmem,
                           (char *)pdos->context->regs[1]);
            }
        }
    }
    else if (svc == 99)
    {
        pdosSVC99(pdos);
    }
    else if (svc == 24) /* devtype */
    {
        /* hardcoded constants that fell off the back of a truck */
        memcpy((void *)pdos->context->regs[0], 
               "\x30\x50\x08\x0B" /* 08 = unit record */
               "\x00\x00\x7f\xff", /* device size = 32767 byte blocks */
               8);
        if (memcmp((void *)pdos->context->regs[1], "SYS", 3) != 0)
        {
            ((char *)pdos->context->regs[0])[2] = 0;
        }
        pdos->context->regs[15] = 0;
    }
    else if (svc == 64) /* rdjfcb */
    {
        int *p;

        pdos->context->regs[15] = 0;
        p = (int *)pdos->context->regs[1];
        /* DCB is pointed to by first parameter */
        /* needs validation */
        gendcb = (DCB *)(p[0] & 0xffffff);
#if 0
        printf("got rdjfcb for %.8s\n", gendcb->dcbddnam);
#endif
        /* we assume that any SYS files are terminal input/output,
           which by default they certainly are. Anything else, we
           assume that it is a reference to the most recent dynamic
           allocation, since there is normally a dynamic allocation,
           rdjfcb then open sequence. We should really have a TIOT
           table and look that up though. */
        if (memcmp(gendcb->dcbddnam, "SYS", 3) != 0)
        {
            int cyl;            
            int head;
            int rec;
#if 0
            printf("must be dataset name %s\n", lastds);
#endif
            if (findFile(pdos->ipldev, lastds, &cyl, &head, &rec) == 0)
            {
                rec = 0; /* so that we can do increments */
                join_cchhr(gendcb->dcbfdad + 3, cyl, head, rec);
#if 0
                printf("located at %d %d %d\n", cyl, head, rec);
#endif
            }
            else
            {
                pdos->context->regs[15] = 12; /* +++ find out
                    something sensible, and then go no further */
            }
        }
        /* we only support RECFM=U files currently */
        /* for non-SYS files we should really get this info from
           the VTOC */
        gendcb->u2.dcbrecfm |= DCBRECU;
        gendcb->dcblrecl = 0;
        gendcb->dcbblksi = 18452;
    }
    else if (svc == 22) /* open */
    {
        int oneexit;
        unsigned int ocode;
        int *p;

        p = (int *)pdos->context->regs[1];
        /* DCB is pointed to by first parameter */
        /* needs validation */
        gendcb = (DCB *)(p[0] & 0xffffff);
#if 0
        printf("in open, DCB is %p\n", gendcb);
#endif

        ocode = p[0];
        ocode >>= 24;
        pdos->context->regs[15] = 0;
        gendcb->dcbcheck = (int)dcheck;
        oneexit = gendcb->u2.dcbexlsa & 0xffffff;
        if (oneexit != 0)
        {
            oneexit = *(int *)oneexit & 0xffffff;
            if (oneexit != 0)
            {
                dexit(oneexit, gendcb);
            }
        }
        if (ocode == 0x80) /* in */
        {
#if 0
            printf("opening for input\n");
#endif
            gendcb->u1.dcbgetput = (int)dread;
        }
        else if (ocode == 0x8f) /* out */
        {
#if 0
            printf("opening for output\n");
#endif
            gendcb->u1.dcbgetput = (int)dwrite;
        }
        else /* don't understand - refuse to open */
        {
            pdos->context->regs[15] = 12; /* is this correct? */
        }
        if (pdos->context->regs[15] == 0)
        {
            gendcb->u1.dcboflgs |= DCBOFOPN;
        }
    }
    else if (svc == 35) /* WTO */
    {
        char *p;
        int len;
        int flags;

        p = (char *)pdos->context->regs[1];
        len = *(short *)p;
        if (len >= (2 * sizeof(short)))
        {
            len -= (2 * sizeof(short));
        }
        p += sizeof(short);
        flags = *(short *)p;
        p += sizeof(short);
        printf("%.*s\n", len, p);
    }
    else if (svc == 42) /* attach */
    {
        char *prog;
        char *parm;
        int newcont = -1;
        struct {
            int eploc;
            int unused1;
            int ecb;
        } *sysatlst;
        
        sysatlst = (void *)pdos->context->regs[15];
        prog = (char *)sysatlst->eploc;
#if 0
        printf("got request to run %.8s\n", prog);
#endif
        parm = *(char **)pdos->context->regs[1];
#if 0
        printf("parameter string is %d bytes\n", *(short *)parm);
#endif
        
        /* We save the ECB (which specifies where the return code
           is meant to go) in our current RB. But does it logically
           belong in this one or the next? Or somewhere else entirely */
        pdos->context->postecb = (ECB *)(sysatlst->ecb & 0xffffff);

        /* special exception for special program to look at
           disk blocks */
        if (memcmp(prog, "DUMPBLK", 7) == 0)
        {
            *pdos->context->postecb = 
                pdos->aspaces[pdos->curr_aspace].o.tcb.tcbcmp =
                pdosDumpBlk(pdos, parm);
            pdos->context->regs[15] = 0;
            pdos->context->regs[1] = 
                (int)&pdos->aspaces[pdos->curr_aspace].o.tcb;
        }
        else if (memcmp(prog, "ZAPBLK", 6) == 0)
        {
            *pdos->context->postecb = 
                pdos->aspaces[pdos->curr_aspace].o.tcb.tcbcmp =
                pdosZapBlk(pdos, parm);
            pdos->context->regs[15] = 0;
            pdos->context->regs[1] = 
                (int)&pdos->aspaces[pdos->curr_aspace].o.tcb;
        }
        else if (memcmp(prog, "DIR", 3) == 0)
        {
            pdosDoDIR(pdos, parm);
            *pdos->context->postecb = 0;
            pdos->context->regs[15] = 0;
        }
        else if ((newcont = pdosLoadExe(pdos, prog, parm)) != 0)
        {
            /* +++ not sure what proper return code is */
            pdos->context->regs[15] = 4;
        }
        /* we usually have a new context loaded, so don't
           mess with R15 */
        if (newcont != 0)
        {
            /* ECB is no longer applicable */
            pdos->context->postecb = NULL;
        }
    }
    else if (svc == 1) /* wait */
    {
        /* do nothing */
    }
    else if (svc == 62) /* detach */
    {
        /* do nothing - but should free memory and switch back context */
    }
    else if (svc == 20) /* close */
    {
        DCB *dcb;
        int *p;
        unsigned int ocode;

        p = (int *)pdos->context->regs[1];
        /* DCB is pointed to by first parameter */
        /* needs validation */
        dcb = (DCB *)(p[0] & 0xffffff);
#if 0
        printf("in close, DCB is %p\n", dcb);
        printf("ddname to close is %.8s\n", dcb->dcbddnam);
#endif

        ocode = p[0];
        ocode >>= 24;

        /* if this is a disk file, and, it was open for write, need to 
           write an EOF record */
        if ((memcmp(dcb->dcbfdad,
                   "\x00\x00\x00\x00\x00\x00\x00\x00", 8) != 0)
            && (ocode == 0x8f))
        {
            int cyl;
            int head;
            int rec;
            int cnt;
            int len = 0;
            char tbuf[8]; /* +++ needs to be short-aligned */
            
            split_cchhr(dcb->dcbfdad + 3, &cyl, &head, &rec);
            rec++;
#if DSKDEBUG
            printf("writing from %p to %d, %d, %d\n",
                   tbuf,
                   cyl, head, rec);
#endif
            memcpy(tbuf, "\x00\x01\x00\x00\x0d\x2c\x00\x60", 8);
            *(short *)tbuf = cyl;
            *(short *)(tbuf + 2) = head;
            tbuf[4] = rec;
            tbuf[5] = 0;
            *(short *)(tbuf + 6) = len;

            /* record number must be one less when using 0x1d
               destructive (of track) write (to set block size) */
            cnt = wrblock(pdos->ipldev, cyl, head, rec - 1,
                          tbuf, len + 8, 0x1d);
#if 0
            printf("destructing write %d %d %d gave %d\n",
                cyl, head, rec - 1, cnt);
#endif
            /* if track is full, count is 0 */
            if (cnt <= 0)
            {
                rec = 1;
                head++;
                *(short *)(tbuf + 2) = head;
                tbuf[4] = rec;
                cnt = wrblock(pdos->ipldev, cyl, head, rec - 1,
                              tbuf, len + 8, 0x1d);
                if (cnt <= 0)
                {
                    head = 0;
                    cyl++;
                    *(short *)tbuf = cyl;
                    *(short *)(tbuf + 2) = head;
                    cnt = wrblock(pdos->ipldev, cyl, head, rec - 1,
                                  tbuf, len + 8, 0x1d);
                }
            }
#if DSKDEBUG
            printf("cnt is %d\n", cnt);
#endif
            join_cchhr(dcb->dcbfdad + 3, cyl, head, rec);
        }
    }
    return;
}



typedef struct {
  char len; /* length of request block, always 20 */
  char verb; /* dynamic allocation function requested */
  char flag1;
  char flag2;
  short error_reason; /* returned */
  short info_reason; /* returned */
  void *tu_list; /* list of pointers to text units */
  int reserved;
  char moreflags[4]; /* extraflags */
} SVC99RB;

typedef struct {
  short key; /* key defining what this text unit is */
  short numparms; /* number of parms that follow */
  short parm1_len;
  char parm1[98];
  /* parm2_len etc would theoretically follow, but we
  /* can't define them, because the length of 98 is probably
     not correct in the first place */
} SVC99TU;


static void pdosSVC99(PDOS *pdos)
{
    int cyl;
    int head;
    int rec;
    SVC99RB *svc99rb;
    SVC99TU **svc99tu;
    int new = -1;

    svc99rb = *(SVC99RB **)pdos->context->regs[1];
    if (svc99rb->verb == 2) /* unallocate */
    {
        /* unconditional success */
        pdos->context->regs[15] = 0;
        pdos->context->regs[0] = 0;
        return;
    }
    else if (svc99rb->verb == 1) /* allocate */
    {
        svc99tu = svc99rb->tu_list;
        while (1)
        {
            if (((int)*svc99tu & 0x7fffffff) != 0)
            {
                if ((*svc99tu)->key == 1) /* DDNAME */
                {
#if 0
                    printf("dynalloc ddname is %.8s\n", (*svc99tu)->parm1);
#endif
                }
                else if ((*svc99tu)->key == 2) /* DSNAME */
                {
#if 0
                    printf("dynalloc dsname is %.44s\n", (*svc99tu)->parm1);
#endif
                    /* save dataset away */
                    sprintf(lastds, "%.44s", (*svc99tu)->parm1);
                }
                else if ((*svc99tu)->key == 4) /* DISP */
                {
                    int disp;

                    disp = (*svc99tu)->parm1[0];
                    if (disp == 4) /* NEW */
                    {
                        new = 1;
#if 0
                        printf("dynalloc disp is NEW\n");
#endif
                    }
                    else if (disp == 8) /* SHR */
                    {
                        new = 0;
#if 0
                        printf("dynalloc disp is SHR\n");
#endif
                    }
                }
                else if ((*svc99tu)->key == 0x49) /* RECFM */
                {
#if 0
                    int recfm;
                    
                    recfm = (*svc99tu)->parm1[0];
                    if (recfm == 0x40) /* V */
                    {
                        printf("file is RECFM V\n");
                    }
#endif
                }
                else if ((*svc99tu)->key == 0x42) /* LRECL */
                {
#if 0
                    int lrecl;
                    
                    lrecl = *(short *)((*svc99tu)->parm1);
                    printf("lrecl is %d\n", lrecl);
#endif
                }
            }
            if (((int)*svc99tu & 0x80000000) != 0) break;
            svc99tu++;
        }
        if (new == -1)
        {
            pdos->context->regs[15] = 12;
            pdos->context->regs[0] = 12;
            svc99rb->error_reason = 12;
            strcpy(lastds, "");
        }
        else
        {
            if (findFile(pdos->ipldev, lastds, &cyl, &head, &rec) != 0)
            {
                if (new == 1)
                {
                    pdosNewF(pdos, lastds);
                }
                else
                {
                    strcpy(lastds, "");
                    pdos->context->regs[15] = 12;
                    pdos->context->regs[0] = 12;
                    svc99rb->error_reason = 12;
                }
            }
            else
            {
                pdos->context->regs[15] = 0;
                pdos->context->regs[0] = 0;
            }
        }
        return;
    }
    /* fail anything else */
    pdos->context->regs[15] = 12;
    pdos->context->regs[0] = 12;
    svc99rb->error_reason = 12;
    return;
}




#define PSW_ENABLE_INT 0x040C0000 /* actually disable interrupts for now */


#define DS1RECFF 0x80
#define DS1RECFV 0x40
#define DS1RECFU 0xc0
#define DS1RECFB 0x10

/* do DIR command */

static int pdosDoDIR(PDOS *pdos, char *parm)
{
    int cyl;
    int head;
    int rec;
    char tbuf[MAXBLKSZ];
    DSCB1 dscb1;
    int cnt;
    char *p;
    int year;
    int month;
    int day;
    int recfm;
    char *blk;
    int errcnt = 0;
    int dev = pdos->curdev;
    
    /* read VOL1 record which starts on cylinder 0, head 0, record 3 */
    cnt = rdblock(dev, 0, 0, 3, tbuf, MAXBLKSZ);
    if (cnt >= 20)
    {
        split_cchhr(tbuf + 15, &cyl, &head, &rec);
        rec += 2; /* first 2 blocks are of no interest */
        
        while (errcnt < 4)
        {
            int c, h, r;
            
            cnt = rdblock(dev, cyl, head, rec, &dscb1, sizeof dscb1);
            if (cnt < 0)
            {
                errcnt++;
                if (errcnt == 1)
                {
                    rec++;
                }
                else if (errcnt == 2)
                {
                    rec = 1;
                    head++;
                }
                else if (errcnt == 3)
                {
                    rec = 1;
                    head = 0;
                    cyl++;
                }
                continue;
            }
            errcnt = 0;
            if (dscb1.ds1dsnam[0] == '\0') break;
            dscb1.ds1dsnam[44] = '\0';
            p = strchr(dscb1.ds1dsnam, ' ');
            if (p != NULL)
            {
                *p = '\0';
            }
            split_cchhr(dscb1.startcchh, &c, &h, &r);
            r = 1;
            year = dscb1.ds1credt[0];
            day = dscb1.ds1credt[1] << 8 | dscb1.ds1credt[2];
            brkyd(&year, &month, &day);
            recfm = dscb1.ds1recfm;
            recfm &= 0xc0;
            if (recfm == DS1RECFU)
            {
                recfm = 'U';
            }
            else if (recfm == DS1RECFV)
            {
                recfm = 'V';
            }
            else if (recfm == DS1RECFF)
            {
                recfm = 'F';
            }
            else
            {
                recfm = 'X';
            }
            blk = ((dscb1.ds1recfm & DS1RECFB) != 0) ? "B" : "";
            
            printf("%-44s %04d-%02d-%02d %c%s %d %d (%d %d %d)\n",
                   dscb1.ds1dsnam, year, month, day,
                   recfm, blk, dscb1.ds1lrecl, dscb1.ds1blkl,
                   c, h, r);
            rec++;
        }
    }
    
    return (0);
}


static void brkyd(int *year, int *month, int *day)
{
    time_t tt;
    struct tm tms = { 0 };
    
    
    tms.tm_year = *year;
    tms.tm_mon = 0;
    tms.tm_mday = *day;
    tt = mktime(&tms);
    *year = 1900 + tms.tm_year;
    *month = tms.tm_mon + 1;
    *day = tms.tm_mday;
}


static int pdosNewF(PDOS *pdos, char *parm)
{
    char tbuf[MAXBLKSZ];
    DSCB1 *dscb1;
    int len;
    char *dsn;
    int datacyl;
    int dircyl;
    int dirhead;
    int dirrec;
    int cnt;

    dsn = parm;
#if 0
    printf("got request to create file %s\n", dsn);
#endif
    pdosGetMaxima(pdos, &dircyl, &dirhead, &dirrec, &datacyl);
#if 0
    printf("maxima is %d %d %d and cyl %d\n", dircyl, dirhead, dirrec, datacyl);
#endif

    dscb1 = (DSCB1 *)tbuf;

    len = strlen(dsn);
    if (len > sizeof dscb1->ds1dsnam)
    {
        len = sizeof dscb1->ds1dsnam;
    }
    memset(dscb1, '\0', sizeof(DSCB1));
    memset(dscb1->ds1dsnam, ' ', sizeof dscb1->ds1dsnam);
    memcpy(dscb1->ds1dsnam, dsn, len);
    dscb1->ds1fmtid = '1'; /* format 1 DSCB */
    memcpy(dscb1->ds1dssn, "PDOS00", 6);
    dscb1->ds1volsq[1] = 1; /* volume sequence number */
    memcpy(dscb1->ds1credt, "\x6e\x00\x01", 3); /* 2010-01-01 */
    memcpy(dscb1->ds1expdt, "\x00\x00\x00", 3); /* 1900-01-00 ? */
    dscb1->ds1noepv = 1; /* number of extents */
    strcpy(dscb1->ds1syscd, "PDOS         "); /* beware NUL */
    dscb1->ds1dsorg = 0x4000; /* DS1DSGPS */
    dscb1->ds1recfm = 0xc0; /* DS1RECFU */
    dscb1->ds1blkl = 18452;
    dscb1->ds1lrecl = 0;
    dscb1->ds1dsind = 0x80; /* DS1IND80 = last volume for this dataset */
    dscb1->ds1scal1 = 0xc0; /* DS1CYL = cylinder request */
    dscb1->ds1scal3[2] = 1; /* secondary allocation */
    /* this is meant to be the last used track and block for the dataset
       in TTR format */
    /* since we don't know what that will be, just set it to relative
       track 14 (since this is 0-based, and record 1 */
    memcpy(dscb1->ds1lstar, "\x00\x0e\x01", 3);
    /* this is some value apparently from TRKCALC, whatever that is. */
    /* lower would seem to be more prudent */
    memcpy(dscb1->ds1trbal, "\x01\x01", 2);
    dscb1->ds1ext1[0] = 0x81; /* on a cylinder boundary */
    
    /* space starts on specified cylinder */
    memcpy(dscb1->startcchh, (char *)&datacyl + sizeof datacyl - 2, 2);
    /* and ends on the next cylinder (thus 2 cylinders per file) */
    datacyl++;
    memcpy(dscb1->endcchh, (char *)&datacyl + sizeof datacyl - 2, 2);
    /* last head on that cylinder too */
    memcpy(dscb1->endcchh + 2, "\x00\x0e", 2);
    
    cnt = wrblock(pdos->ipldev, dircyl, dirhead, dirrec, tbuf, 140, 0x0d);
#if 0
    printf("cnt from wrblock is %d\n", cnt);
#endif
    
#if 0
    memset(tbuf, '\0', sizeof tbuf);
    memcpy(tbuf, "\x00\x01\x00\x00\x0d\x2c\x00\x60", 8);
    strcpy(tbuf + 8, "AAAA");
    /* free directory space starts at 1 0 12 */    
    wrblock(pdos->ipldev, 1, 0, 12, tbuf, 140, 0x1d);
#endif

#if 0

1c1
< 000000  C3D6D4D4 C1D5C44B C5E7C540 40404040  COMMAND.EXE
---
> 000000  C1E4E3D6 C5E7C5C3 4BC2C1E3 40404040  AUTOEXEC.BAT
7,8c7,8
< 000060  00010001 03754600 00810000 04000000  .........a......
< 000070  04000E00 00000000 00000000 00000000  ................
---
> 000060  00010000 02DFEC00 00810000 05000000  .........a......
> 000070  05000E00 00000000 00000000 00000000  ................

000000  C3D6D4D4 C1D5C44B C5E7C540 40404040  COMMAND.EXE
000010  40404040 40404040 40404040 40404040
000020  40404040 40404040 40404040 F1D7C4D6              1PDO
000030  E2F0F000 016E012F 00000001 7800C8C5  S00..>........HE
000040  D9C3E4D3 C5E24040 40404000 00000000  RCULES     .....
000050  00004000 C0004814 00000000 0080C000  .. .{.........{.
000060  00010001 03754600 00810000 04000000  .........a......
000070  04000E00 00000000 00000000 00000000  ................
000080  00000000 00000000 00000000           ............

#endif


    return (0);
}


/* the directory will point to the first empty directory.
   The data cylinder will point to the first empty space */

static int pdosGetMaxima(PDOS *pdos, int *dircyl, int *dirhead,
                         int *dirrec, int *datacyl)
{
    int cyl;
    int head;
    int rec;
    char tbuf[MAXBLKSZ];
    DSCB1 dscb1;
    int cnt;
    char *p;
    int year;
    int month;
    int day;
    int recfm;
    char *blk;
    int errcnt = 0;
    
    *dirrec = *dirhead = *dircyl = *datacyl = 0;
    /* read VOL1 record which starts on cylinder 0, head 0, record 3 */
    cnt = rdblock(pdos->ipldev, 0, 0, 3, tbuf, MAXBLKSZ);
    if (cnt >= 20)
    {
        split_cchhr(tbuf + 15, &cyl, &head, &rec);
        rec += 2; /* first 2 blocks are of no interest */
        
        while (errcnt < 4)
        {
            cnt = rdblock(pdos->ipldev, cyl, head, rec, &dscb1,
                          sizeof dscb1);
            if (cnt < 0)
            {
                errcnt++;
                if (errcnt == 1)
                {
                    rec++;
                }
                else if (errcnt == 2)
                {
                    rec = 1;
                    head++;
                }
                else if (errcnt == 3)
                {
                    rec = 1;
                    head = 0;
                    cyl++;
                }
                continue;
            }
            else
            {
                int c, h, r;

                errcnt = 0;
                if (dscb1.ds1dsnam[0] == '\0') break;
                split_cchhr(dscb1.endcchh, &c, &h, &r);
                if (c >= *datacyl)
                {
                    *datacyl = c + 1;
                }
                rec++;
            }
        }
        *dirrec = rec;
        *dirhead = head;
        *dircyl = cyl;
    }
    
    return (0);
}


/* dump block */

static int pdosDumpBlk(PDOS *pdos, char *parm)
{
    int cyl;
    int head;
    int rec;
    char tbuf[MAXBLKSZ];
    long cnt = -1;
    int lastcnt = 0;
    int ret = 0;
    int c, pos1, pos2;
    long x = 0L;
    char prtln[100];
    long i;
    long start = 0;
    int dev;

    tbuf[0] = '\0';
    i = *(short *)parm;
    parm += sizeof(short);
    if (i < (sizeof tbuf - 1))
    {
        memcpy(tbuf, parm, i);
        tbuf[i] = '\0';
    }
    sscanf(tbuf, "%x %d %d %d", &dev, &cyl, &head, &rec);
    if (dev == 0)
    {
        dev = pdos->ipldev;
    }
    printf("dumping cylinder %d, head %d, record %d of device %x\n",
           cyl, head, rec, dev);
    cnt = rdblock(dev, cyl, head, rec, tbuf, MAXBLKSZ);
    if (cnt > 0)
    {
        for (i = 0; i < cnt; i++)
        {
            c = tbuf[i];
            if (x % 16 == 0)
            {
                memset(prtln, ' ', sizeof prtln);
                sprintf(prtln, "%0.6lX   ", start + x);
                pos1 = 8;
                pos2 = 45;
            }
            sprintf(prtln + pos1, "%0.2X", c);
            if (isprint((unsigned char)c))
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
    }
    
    return (cnt);
}


/* zap block */

static int pdosZapBlk(PDOS *pdos, char *parm)
{
    int cyl;
    int head;
    int rec;
    char tbuf[MAXBLKSZ];
    long cnt = -1;
    int lastcnt = 0;
    int ret = 0;
    int c, pos1, pos2;
    long x = 0L;
    char prtln[100];
    long i;
    long start = 0;
    int off;
    int nval;
    int dev;

    tbuf[0] = '\0';
    i = *(short *)parm;
    parm += sizeof(short);
    if (i < (sizeof tbuf - 1))
    {
        memcpy(tbuf, parm, i);
        tbuf[i] = '\0';
    }
    sscanf(tbuf, "%x %d %d %d %i %i", &dev, &cyl, &head, &rec, &off, &nval);
    if (dev == 0)
    {
        dev = pdos->ipldev;
    }
    printf("zapping cylinder %d, head %d, record %d, "
           "byte %X of device %x to %0.2X\n",
           cyl, head, rec, off, dev, nval);
    cnt = rdblock(dev, cyl, head, rec, tbuf, MAXBLKSZ);
    if (cnt > 0)
    {
        if (off >= cnt)
        {
            printf("byte to zap is out of range\n");
        }
        else
        {
            tbuf[off] = nval;
            wrblock(dev, cyl, head, rec, tbuf, cnt, 0x0d);
        }
    }
    
    return (cnt);
}


/* load executable into memory, on a predictable 1 MB boundary,
   by requesting 5 MB */

static int pdosLoadExe(PDOS *pdos, char *prog, char *parm)
{
    char *raw;
    char *initial;
    char *load;
    /* Standard C programs can start at a predictable offset */
    int entry;
    int cyl;
    int head;
    int rec;
    int i;
    int j;
    char tbuf[MAXBLKSZ];
    char srchprog[FILENAME_MAX+10]; /* give an extra space */
    int cnt = -1;
    int lastcnt = 0;
    int ret = 0;
    DSCB1 dscb1;
    int pe = 0;
    int exeLen;

    /* try to find the load module's location */
    
    /* +++ replace this 8 with some constant */
    memcpy(srchprog, prog, 8);
    srchprog[8] = ' ';
    *strchr(srchprog, ' ') = '\0';
    strcat(srchprog, ".EXE "); /* extra space deliberate */
    
    /* read VOL1 record */
    cnt = rdblock(pdos->ipldev, 0, 0, 3, tbuf, MAXBLKSZ);
    if (cnt >= 20)
    {
        cyl = head = rec = 0;
        /* +++ probably time to create some macros for this */
        memcpy((char *)&cyl + sizeof(int) - 2, tbuf + 15, 2);
        memcpy((char *)&head + sizeof(int) - 2, tbuf + 17, 2);
        memcpy((char *)&rec + sizeof(int) - 1, tbuf + 19, 1);
        
        while ((cnt =
               rdblock(pdos->ipldev, cyl, head, rec, &dscb1, sizeof dscb1))
               > 0)
        {
            if (cnt >= sizeof dscb1)
            {
                if (dscb1.ds1fmtid == '1')
                {
                    dscb1.ds1fmtid = ' '; /* for easy comparison */
                    if (memcmp(dscb1.ds1dsnam,
                               srchprog,
                               strlen(srchprog)) == 0)
                    {
                        cyl = head = 0;
                        rec = 1;
                        /* +++ more macros needed here */
                        memcpy((char *)&cyl + sizeof(int) - 2, 
                               dscb1.startcchh, 2);
                        memcpy((char *)&head + sizeof(int) - 2,
                               dscb1.startcchh + 2, 2);
                        break;
                    }
                }
            }
            rec++;
        }        
    }
    
    if (cnt <= 0)
    {
        *strchr(srchprog, ' ') = '\0';
        printf("executable %s not found!\n", srchprog);
        return (-1);
    }
    
    cnt = rdblock(pdos->ipldev, cyl, head, rec, tbuf, MAXBLKSZ);
    if ((cnt > 8) && (*((int *)tbuf + 1) == 0xca6d0f))
    {
        pe = 1;
    }
    else
    {
        printf("only MVS PE executables are supported\n");
        return (-1);
    }

    /* assume 4 MB max */
    raw = memmgrAllocate(&pdos->aspaces[pdos->curr_aspace].o.btlmem,
                         5 * 1024 * 1024, 0);
    if (raw == NULL)
    {
        printf("insufficient memory to load program\n");
        return (-1);
    }

#if 0
    /* should be no need for specific alignment */
    /* round to 16 byte boundary */
    load = (char *)(((int)raw & ~0xf) + 0x10);
#endif
    load = raw;
    initial = load;
#if 0
    printf("load point designated as %p\n", load);
#endif
    /* should store old context first */
    i = head;
    j = rec;
    /* Note that we read until we get EOF (a zero-length block). */
    /* +++ note that we need a security check in here to ensure
       that people don't leave out an EOF to read the next guy's
       data - use the endcchh */
    while (cnt != 0)
    {
#if DSKDEBUG
        printf("loading to %p from %d, %d, %d\n", load,
               cyl, i, j);
#endif
        cnt = rdblock(pdos->ipldev, cyl, i, j, tbuf, MAXBLKSZ);
#if DSKDEBUG
        printf("cnt is %d\n", cnt);
#endif
        if (cnt == -1)
        {
            if (lastcnt == -2)
            {
                printf("three I/O errors in a row - terminating load\n");
                break;
            }
            else if (lastcnt == -1)
            {
#if DSKDEBUG
                printf("probably last track on cylinder\n");
#endif
                /* probably reached last track on cylinder */
                lastcnt = -2;
                cyl++;
                i = 0;
                j = 1;
                continue;
            }
            /* probably reached last record on track */
            lastcnt = -1;
            j = 1;
            i++;
            continue;
        }
        lastcnt = cnt;
        memcpy(load, tbuf, cnt);
        load += cnt;
        j++;
    }

    exeLen = load - initial;    
    if (pe)
    {
        if (fixPE(initial, &exeLen, &entry, (int)initial) != 0)
        {
            memmgrFree(&pdos->aspaces[pdos->curr_aspace].o.btlmem, raw);
            return (-1);
        }
        exeLen += 0x1000; /* give some buffer */
        /* unconditionally reduce the 5 MB to something reasonable */
        memmgrRealloc(&pdos->aspaces[pdos->curr_aspace].o.btlmem,
                      raw, exeLen);
        /* if we can't adjust down, don't care */
    }

    pdos->context->next_exe = raw;

    /* get a new RB */
    pdos->context = memmgrAllocate(
        &pdos->aspaces[pdos->curr_aspace].o.btlmem,
        sizeof *pdos->context, 0);
    if (pdos->context == NULL)
    {
        /* free the memory that was allocated to the executable */
        memmgrFree(&pdos->aspaces[pdos->curr_aspace].o.btlmem, raw);
        ret = -1;
    }
    else
    {
        /* switch to new context */
        memset(pdos->context, 0x00, sizeof *pdos->context);
        pdos->context->rblinkb =
            pdos->aspaces[pdos->curr_aspace].o.curr_rb;
        pdos->aspaces[pdos->curr_aspace].o.curr_rb = pdos->context;

        /* fill in details of new context */
        pdos->context->regs[13] = (int)pdos->context->savearea;
        pdos->context->regs[14] = (int)gotret;
        pdos->context->regs[15] = entry;
        pdos->context->psw1 = PSW_ENABLE_INT; /* need to enable interrupts */
        pdos->context->psw2 = entry; /* 24-bit mode for now */
#if defined(S390)
        pdos->context->psw2 |= 0x80000000; /* dispatch in 31-bit mode */
#endif

        pdos->context->pptrs[0] = parm;
    
        pdos->context->regs[1] = (int)pdos->context->pptrs;

#if 0
        printf("finished loading executable\n");
#endif
    }
    return (ret);
}


static int pdosDumpMem(PDOS *pdos, void *buf, int cnt)
{
    int ret = 0;
    int c, pos1, pos2;
    long x = 0L;
    char prtln[100];
    long i;
    long start = 0;
    char *tbuf = (char *)buf;

    if (cnt > 0)
    {
        for (i = 0; i < cnt; i++)
        {
            c = tbuf[i];
            if (x % 16 == 0)
            {
                memset(prtln, ' ', sizeof prtln);
                sprintf(prtln, "%0.6lX   ", start + x);
                pos1 = 8;
                pos2 = 45;
            }
            sprintf(prtln + pos1, "%0.2X", c);
            if (isprint((unsigned char)c))
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
    }
    
    return (0);
}


/* load the PCOMM executable. Note that this should
   eventually be changed to call a more generic
   loadExe() routine */

static int pdosLoadPcomm(PDOS *pdos)
{
    static char mvsparm[] = { "\x00" "\x02" "/P" };

    if (pdosLoadExe(pdos, "COMMAND", mvsparm) != 0)
    {
        printf("can't load COMMAND.EXE\n");
        printf("PDOS terminating\n");
        return (-1);
    }
    return (0);
}


static void split_cchhr(char *cchhr, int *cyl, int *head, int *rec)
{
    *cyl = *head = *rec = 0;
    memcpy((char *)cyl + sizeof *cyl - 2, cchhr, 2);
    memcpy((char *)head + sizeof *head - 2, cchhr + 2, 2);
    *rec = cchhr[4];
    return;
}


static void join_cchhr(char *cchhr, int cyl, int head, int rec)
{
    memcpy(cchhr, (char *)&cyl + sizeof cyl - 2, 2);
    memcpy(cchhr + 2, (char *)&head + sizeof head - 2, 2);
    cchhr[4] = rec;
    return;
}




#if 0
/* start an independent process (with its own virtual address space) */

static int pdosStart(PDOS *pdos, char *pgm, char *parm)
{
    
}
#endif












#if 0

/* This is a high-level conceptual version of what PDOS
   needs to do, but this code is just a big comment. The
   heart of PDOS is in pdosRun which is called from the
   real main. */

int main(int argc, char **argv)
{
    /* Note that the loader may pass on a parameter of some sort */
    /* Also under other (test) environments, may have options. */

    /* Note that the bulk of the operating system is set up in
       this initialization, but this main routing focuses on
       the heart of the OS - the actual millisecond by millisecond
       execution, where people (ie programs) are expecting to get
       some CPU time. */
    init();
    
    task = NULL;
    
    /* The OS is not designed to exit */
    while (1)
    {
        /* all this will be done with interrupts disabled,
           and in supervisor status. This function will take
           into account the last executing task, to decide
           which task to schedule next. As part of that, it
           will also mark the last task as "no longer executing" */
        task = getNextAvailTask();
        if (task == NULL)
        {
            /* suspend the computer until an interrupt occurs */
            intrupt = goWaitState();
            if (intrupt == INTERRUPT_TIMER)
            {
                /* may eventually do some housekeeping in this
                   idle time */
            }
            else if (intrupt == INTERRUPT_IO)
            {
                processIO();
            }
            /* we're not expecting any other interrupts. If we get
               any, put it in the too-hard basket! */
            
            continue; /* go back and check for a task */
        }
        
        /* we have a task, and now it is time to run it, in user
           mode (usually) until an interrupt of some sort occurs */
        sliceOver = 0;
        while (!sliceOver)
        {
            sliceOver = 1; /* most interrupts will cause this timeslice
                              to end, so set that expectation by
                              default. */
        
            /* run the user's code (stock-standard general instructions,
               e.g. LA, LR, AR) until some interrupt occurs where the 
               OS gets involved. On return from this function, interrupts
               will be redisabled. In addition, the task will be in a
               saved state, ie all registers saved. The handling of
               the interrupt may alter the state of the task, to make
               it e.g. shift to an SVC routine. */
               
            intrupt = dispatch_until_interrupt();
            if (intrupt == INTERRUPT_TIMER)
            {
                /* this will check how long the program has been
                   running for, and if the limit has been reached,
                   the context will be switched to an abend routine,
                   which will be dispatched at this task's next
                   timeslice. */
                checkElapsedTime();
            }
        
            else if (intrupt == INTERRUPT_ILLEGAL)
            {
                /* This is called if the user program has executed
                   an invalid instruction, or accessed memory that
                   doesn't belong to it, etc */
                /* this function will switch the context to an abend
                   handler to do the dump, timesliced. */
                /* If the task has abended, the equivalent of a
                   branch to user-space code is done, to perform
                   the cleanup, as this is an expensive operation,
                   so should be timesliced. */
                processIllegal();
            }
        
            else if (intrupt == INTERRUPT_SVC)
            {
                /* This function switches context to an SVC handler,
                   but it won't actually execute it until this task's
                   next timeslice, to avoid someone doing excessive
                   SVCs that hog the CPU. It will execute in
                   supervisor status when eventually dispatched though. */
                /* We can add in some smarts later to allow someone
                   to do lots of quick SVCs, so long as it fits into
                   their normal timeslice, instead of being constrained
                   to one per timeslice. But for now, avoid the
                   complication of having an interruptable SVC that
                   could be long-running. */
                startSVC();
            }
        
            else if (intrupt == INTERRUPT_IO)
            {
                /* This running app shouldn't have to pay the price
                   for someone else's I/O completing. */
                
                saveTimer();
                
                processIO();
                
                /* If the timeslice has less than 10% remaining, it
                   isn't worth dispatching again. Otherwise, set
                   this slice to be run again. */
                if (resetTimer())
                {
                    sliceOver = 0;
                }
            }
        }
    }
    return (0); /* the OS may exit at some point */
}
#endif





#if 0

/*
Suggestions for a general-purpose OS:


1. Real-time - within a general-purpose system, I
should be able to define a task that will be
certain to be dispatched as soon as the I/O it is
waiting on has been completed. But I don't expect
such tasks to require 100% of the CPU. 10% of the
CPU seems more reasonable. If anyone has complaints
about that, they should buy a bigger CPU. But within
that 10% powerful CPU, I do expect immediate dispatch
of real-time tasks, as soon as the device they are
waiting on is complete. So a link from the I/O
interrupt handler to real time tasks associated with
that I/O device.


2. If I am an end user, doing light activity, I
expect near-immediate responses, like 0.1 seconds.
ie my light activity is high priority by default.
So the time to dispatch for a new task should be
low. I guess that means the terminal controller.
But since there will be users abusing that, by
running ind$file, when those repeat (ab)users are
detected, they are put on a "slow dispatch queue",
and treated like batch jobs (infrequently
dispatched - but they can be dispatched for longer
periods of time when they do get dispatched).

But the principle is to find out where the humans
are, and ensure that so long as they are just
editting a file in fullscreen mode, that the
light CPU activity of reading a block from disk
and forming it into a 3270 screen, is done within
0.1 seconds, from the time they hit enter after
spending 5 seconds to find the "e" on their
keyboard.

I'll assume the general case where there's no
deliberate Denial of Service attack, or users on
a production system risking their jobs by
deliberately running software masquerading as
light users.

So it's not real-time (as in a device needs a
response within 0.01 seconds, otherwise the
nuclear plant goes into Two Mile Island mode).
But it is human-time, as in failure to respond
as quickly as a monkey can put his finger on
and off the PF8 button, means you're making a
C programmer cranky, especially when you know
that after doing this 10 times, he's going to
spend the next 10 minutes doing nothing that
the computer sees, so he's hardly the anti-social 
prick he's made out to be.

The same goes for CICS users. I don't see why
CICS is required. Why can't the OS do what CICS
does? Once again, after displaying the results
of a balance inquiry, a human needs to move
their jawbone to say "have a nice day", so in
the brief period of time that they are actually
requesting services, response should be fast.


3. People running a long-running batch job don't
need frequent dispatching if the overhead of
dispatching is going to be great. So infrequent
dispatching, but within that, prioritize I/O-bound
tasks up to the point that they're getting the
same amount of CPU as their CPU-bound cousins.


So, a strategy for real-time, a strategy for humans
and a strategy for batch.

As each I/O interrupt comes in, it should (a
priori) be associated with one of those things,
which will determine what to do next.


In the general case of a mixed workload, and
trying to minimize the overhead of gratuitous
task switching, we need the following:

A linked list of currently dispatchable tasks.
Taking the general case of them all being
CPU-bound, and with the say 2 real-time tasks 
only taking 10% of the CPU, and the other (say
40) dispatchable human tasks taking 80%, and 
the (say 20) batch jobs taking 10%, then in
0.1 seconds we would want to see the 0.1 seconds
divided up amongst the 2 real-time, 40 human, 
and say 1 batch job (meaning the batch jobs are
only dispatched once every 2 seconds).

So let's say we're on a 100 MIPS CPU.

That means we have 10 million instructions for 
the next 0.1 seconds.

each real time task will get 50% * 10% * 10 = 0.5 million

each human task will get 1/40 * 80% * 10 = 0.2 million

the one batch job will get 10% * 10 = 1 million instructions.

And it means 43 task switches.

Because these numbers will vary, we want to 
make the task switching as efficient as possible.

So a single function which scans through the 52
tasks. Since 9 of the batch jobs aren't meant to
be dispatched, they instead have countdown timers.
They've all been seeded with numbers from 1-10,
and when it gets down to 0, they get dispatched,
and the countdown is reset to 10.

Any new tasks joining the queue will fit in with
this schema in an appropriate position, starting
off as human, downgrading to batch.

That process of downgrading and initial setup will
be a logically separate task that is nominally
timesliced itself.

As well as a countdown, they also have a timer
associated with them. So each realtime task would
get 0.5 million in its timer. This number would
vary as appropriate as new tasks are added and
removed.

So, it's a case of

for each dispatchable task (in an infinite loop) repeat
  if (counter == 0) or (--counter == 0)
     set timer to value specified
     copy registers and PSW into low memory
     LM
     LPSW
     (wait for timer)
     LM
     copy registers and PSW back
  end
end

Keep it simple and fast. Perhaps eventually written
in assembler. It may be possible to complicate it a
bit in order to avoid the movement into low memory.
ie do the LM directly from the task's context, and
then load one register from low memory, instead of 16.
If the total number of dispatchable tasks is under
say 200, one register, plus the two PSW words, could
be kept below the 4096 mark.

When an I/O interrupt comes in, and it's a realtime
task, that is added into the linked list, and
immediately dispatched for its 0.5 million alloted
instructions.

Human I/O is added at the end of the queue, to avoid
possible thrashing.

*/
#endif




#if 0

Suggestions for MVS 3.8j:

A normal MVS 3.8j address space has a private
region of say 2MB-10MB, and uses 64k segments.

So.

For a particular job class, say CLASS=X (X=extended),
and this will only work for about 6 or thereabout
address spaces before you run out of EA memory,
what you do at address space creation time is:

1. Make MVS 3.8j allocate the 2 MB-10MB region
to 16MB-24MB in extended real memory, and be
page fixed.

2. The 370 DAT points to BTL real memory for the
0-2MB and 10-16MB areas, and that gets paged.

3. The 370 DAT points to EA for the 2-10MB region.

4. An XA DAT also points to the same areas of
memory as 370 for the 0-16 MB region, but then
starts pointing to say 256-368 MB for the
next 112 MB (giving a total address space of
128 MB) - all via 64k segments.

5. Not sure what happens in an interrupt, ie
each interrupt may need to have CR0.10 saved,
set to 0 for MVS 3.8j use, then restored.

MVS 3.8j page tables can remain as 2-byte,
370 mode, yet 6 or whatever address spaces
can get up to 128 MB (it may be possible to steal
more bits to get up to 2 GB if required).

#endif
