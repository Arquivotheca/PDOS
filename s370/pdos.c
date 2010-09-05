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
/*  no base register (ir R0) can be used.                            */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>


#define S370_MAXMB 16 /* maximum MB in a virtual address space for S/370 */
#define NUM_GPR 16 /* number of general purpose registers */
#define NUM_CR 16 /* number of control registers */
#define SEG_BLK 16 /* number of segments in a block */

#define ASPACE_ALIGN 4096 /* DAT tables need to be aligned on 4k boundary */

#define PSA_ORIGIN 0 /* the PSA starts at literally address 0 */

#define CR13_DUMMY 0x00001000 /* disable ATL memory access */

/*

real memory map:

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



virtual memory map:

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

/* where pcomm executable is loaded to */
#define PCOMM_LOAD (PDOS_HEAP + 0x200000) /* 5 MB */

/* entry point for MVS executables can be at known location 8
   into mvsstart, so long as that is linked first. In the future,
   this information will be obtained from the entry point as
   recorded in the IEBCOPY unload RDW executable */
#define PCOMM_ENTRY (PCOMM_LOAD + 8)

/* where to get getmains from - allow for 4 MB executables */
#define PCOMM_HEAP (PCOMM_LOAD + 0x400000) /* 9 MB */

#define PDOS_STORINC 0x080000 /* storage increment */

#define PCOMM_ATL_START 0x1100000

#define PCOMM_STARTCYL 2 /* cylinder where PCOMM can be found -
  this assumes that PLOAD and PDOS are less than 1 cylinder in
  size and that they have been allocated on cylinder boundaries */

#ifndef EA_ON
#ifdef S380
#define EA_ON 1 /* use extended addressing in 380 mode */
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
#define MAXASIZE 80 /* maximum of 80 MB for address space */
                    /* note that this needs to be a multiple of 16
                       in order for the current logic (MAXASIZE/SEG_BLK)
                       to work */
#endif

#ifndef MAXANUM
#define MAXANUM  4   /* maximum of 4 address spaces */
#endif

#define MAXPAGE 256 /* maximum number of pages in a segment */


/* The S/380 architecture provides a lot of options, and they
   can be selectively enabled here.
   
   First, here is a description of S/380.
   
   If CR13 = 0, then ATL memory accesses are done to real storage,
   ignoring storage keys (for ATL accesses only), even though (S/370)
   DAT is switched on. This provides instant access to ATL memory for
   applications, before having to make any OS changes at all. This
   is not designed for serious use though, due to the lack of
   integrity in ATL memory for multiple running programs. It does
   provide very quick proof-of-concept though.
   
   If CR13 is non-zero, e.g. even setting a dummy value of
   0x00001000, then ATL memory accesses are done via a DAT
   mechanism instead of using real memory.
   
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


#if !defined(SEG_64K)
/* this can be used to set 64K segments as MVS 3.8j does */
#define SEG_64K 0
#endif

#if !defined(BTL_XA)
/* this can be used to make an XA DAT be used, even below the line */
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


typedef struct {
    int abend;
} TASK;

typedef struct {
    int regs[NUM_GPR];
    unsigned int psw1;
    unsigned int psw2;
} CONTEXT;

typedef struct {
    char unused1[32];
    unsigned int svcopsw[2];
    char unused2[96];
    unsigned int svc_code;
    char unused[244];
    int flcgrsav[NUM_GPR];
} PSA;

typedef struct {
    char unused1[32];
    int *eodad;
    union
    {
        char dcbrecfm;
        int dcbexlsa;
    } u2;
    char unused2[8];
    union {
        char dcboflgs;
        int dcbput;
    } u1;
    int dcbcheck;
    char unused3[6];
    short dcbblksi;
    char unused4[18];
    short dcblrecl;
} DCB;


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
    CONTEXT context; /* current thread's context */
    PSA *psa;
    int exitcode;
    int shutdown;
    int ipldev;
    int curr_aspace; /* current address space */
    int cyl_upto; /* next cylinder to be read */
} PDOS;

/*static PDOS pdos;*/

static DCB *gendcb = NULL; /* need to eliminate this state info */

void gotret(void);
int adisp(void);
void dread(void);
void dwrite(void);
void dcheck(void);
void dexit(int oneexit, DCB *dcb);


int pdosRun(PDOS *pdos);
void pdosDefaults(PDOS *pdos);
int pdosInit(PDOS *pdos);
void pdosTerm(PDOS *pdos);
static int pdosDispatchUntilInterrupt(PDOS *pdos);
static void pdosInitAspaces(PDOS *pdos);
static void pdosProcessSVC(PDOS *pdos);
static int pdosLoadPcomm(PDOS *pdos);


int main(int argc, char **argv)
{
    PDOS *pdos;
    char *p;
    int remain;
    int ret = EXIT_FAILURE;

    /* thankfully we are running under an emulator, so have access
       to printf debugging (to the Hercules console via DIAG8),
       and Hercules logging */
    printf("Welcome to PDOS!!!\n");

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

    lcreg1(pdos->aspaces[pdos->curr_aspace].o.cregs[1]);
#if defined(S380)
    lcreg13(pdos->aspaces[pdos->curr_aspace].o.cregs[13]);
#endif
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
    printf("CR0 is %08X\n", cr0);
    printf("PDOS structure is %d bytes\n", sizeof(PDOS));
    printf("aspace padding is %d bytes\n", sizeof pdos->aspaces[0].filler);

    pdos->ipldev = initsys();
    pdos->cyl_upto = PCOMM_STARTCYL;
    printf("IPL device is %x\n", pdos->ipldev);
    lcreg0(cr0);
    pdos->shutdown = 0;
    pdosInitAspaces(pdos);
    pdos->curr_aspace = 3;
    pdosLoadPcomm(pdos);
    return (1);
}


/* pdosTerm - any cleanup routines required */

void pdosTerm(PDOS *pdos)
{
    /* +++ should disable DAT here, to return in same status
       that we entered in */
    return;
}


static int pdosDispatchUntilInterrupt(PDOS *pdos)
{
    int ret;

    while (1)
    {
        /* store registers and PSW in low memory to be loaded */
        memcpy(pdos->psa->flcgrsav,
               pdos->context.regs,
               sizeof pdos->context.regs);

        pdos->psa->svcopsw[0] = pdos->context.psw1;
        pdos->psa->svcopsw[1] = pdos->context.psw2;
                       
        ret = adisp();  /* dispatch */
        
        /* restore registers and PSW from low memory */
        memcpy(pdos->context.regs,
               pdos->psa->flcgrsav,               
               sizeof pdos->context.regs);
        pdos->context.psw1 = pdos->psa->svcopsw[0];
        pdos->context.psw2 = pdos->psa->svcopsw[1];
        
        if (ret == 0) break;

        /* non-zero returns are requests for services */
        /* these "DLLs" don't belong in the operating system proper,
           but for now, we'll action them here. Ideally we want to
           get rid of that while loop, and run the DLL as normal
           user code. Only when it becomes time for WRITE to run
           SVC 0 (EXCP) or CHECK to run SVC 1 (WAIT) should it be
           reaching this bit of code. It's OK to keep the code
           here, but it needs to execut in user space. */
        if (ret == 2)
        {
            /* need to fix bug in PDPCLIB - long lines should be truncated */
            printf("%.80s\n", pdos->context.regs[4]); /* wrong!!! */
        }
        else if (ret == 3) /* got a READ request */
        {
            static int rcnt = 0;
            
            /* need to call EOF routine */
            /* printf("eodad routine has %p %x in it\n", gendcb->eodad,
                   *gendcb->eodad); */
            /* EOF routine usually sets R6 to 1, so just do it */
            if (rcnt != 0)
            {
                pdos->context.regs[6] = 1;
            }
            else
            {
                char *p;
                
                /* we know that they are using R8 for the buffer, via
                   the READ macro */
                p = (char *)pdos->context.regs[8];
                *p = 'A';
            }
            rcnt++;
        }
    }
    return (INTERRUPT_SVC); /* only doing SVCs at the moment */
}


/* we initialize all address spaces upfront, with fixed locations
   for all segments. The whole approach should be changed though,
   with address spaces created dynamically and the virtual space
   not being allocated to real memory until it is actually
   attempted to be used. */

/* we will eventually map the 4 MB - 9 MB location to various spots
   under 64 MB real (370 allowed this), giving up to 15 address
   spaces of 4 MB each. But for now, just map it directy on to
   the same location (ie all address spaces will clobber each
   other) */ 

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
        printf("aspace %d, seg370 %p, cr1 %08X\n",
               a, pdos->aspaces[a].o.seg370, pdos->aspaces[a].o.cregs[1]);
    }
#endif

    /* initialize XA-DAT tables */
#if defined(S380) || defined(S390)
    for (a = 0; a < MAXANUM; a++)
    {
#if SEG_64K && BTL_XA
        for (s = 0; s < (MAXASIZE * 16); s++)
#else
        for (s = 0; s < MAXASIZE; s++)
#endif
        {
            int adjust = 0;

/* If EA is on, it is mixed DAT, and thus 1M segment size */
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
        printf("aspace %d, seg %p, cr13 %08X\n",
               a, pdos->aspaces[a].o.segtable, pdos->aspaces[a].o.cregs[13]);
    }
#endif

    return;
}



static void pdosProcessSVC(PDOS *pdos)
{
    int svc;
    static int getmain = PCOMM_HEAP;
       /* should move to PDOS and use memmgr - but virtual memory
          will obsolete anyway */

    svc = pdos->psa->svc_code & 0xffff;
#if 1
    printf("SVC code is %d\n", svc);
#endif
    if (svc == 3)
    {
        /* normally the OS would not exit on program end */
        printf("return from PCOMM is %d\n", pdos->context.regs[15]);
        pdos->shutdown = 1;
        pdos->exitcode = pdos->context.regs[15];
    }
    else if ((svc == 120) || (svc == 10))
    {
        /* if really getmain */
        if ((svc == 10) || (pdos->context.regs[1] == 0))
        {
            pdos->context.regs[1] = getmain;
            pdos->context.regs[15] = 0;
            if (pdos->context.regs[0] < 16000000L)
            {
                /* just allocate storage consecutively for now */
                getmain += pdos->context.regs[0];
            }
            else
            {
                /* one shot at ATL memory */
                pdos->context.regs[1] = PCOMM_ATL_START;
            }
        }
    }
    else if (svc == 24) /* devtype */
    {
        /* hardcoded constants obtained from running MVS 3.8j system */
        memcpy((void *)pdos->context.regs[0], 
               "\x30\x50\x20\x0B\x00\x00\xF6\xC0",
               8);
        pdos->context.regs[15] = 0;
    }
    else if (svc == 64) /* rdjfcb */
    {
        int oneexit;
        static int fcnt = 0;

        gendcb = (DCB *)pdos->context.regs[10]; 
            /* need to protect against this */
            /* and it's totally wrong anyway */
        fcnt++;
        if (fcnt > 2)
        {
            gendcb->u1.dcbput = (int)dread;
        }
        else
        {
            gendcb->u1.dcbput = (int)dwrite;
        }
        gendcb->dcbcheck = (int)dcheck;
        gendcb->u2.dcbrecfm |= DCBRECF;
        gendcb->dcblrecl = 80;
        gendcb->dcbblksi = 80;
        oneexit = gendcb->u2.dcbexlsa & 0xffffff;
        if (oneexit != 0)
        {
            oneexit = *(int *)oneexit & 0xffffff;
            if (oneexit != 0)
            {
                dexit(oneexit, gendcb);
            }
        }
        pdos->context.regs[15] = 0;
    }
    else if (svc == 22) /* open */
    {
        gendcb->u1.dcboflgs |= DCBOFOPN;
        pdos->context.regs[15] = 0; /* is this required? */
    }
    return;
}


#define PSW_ENABLE_INT 0x040C0000 /* actually disable interrupts for now */

/* load the PCOMM executable. Note that this should
   eventually be changed to call a more generic
   loadExe() routine */

static int pdosLoadPcomm(PDOS *pdos)
{
    char *load = (char *)PCOMM_LOAD;
    /* Standard C programs can start at a predictable offset */
    int (*entry)(void *) = (int (*)(void *))PCOMM_ENTRY;
    int i;
    int j;
    static int savearea[20]; /* needs to be in user space */
    static char mvsparm[] = { "\x00" "\x06" "dd:inp" };
    static char *pptrs[1];
    char tbuf[MAXBLKSZ];
    int cnt = -1;
    int lastcnt = 0;

#if EA_ON
    /* +++ we should really enable DAT first, so that this is
       naturally mapped - but we can't do I/O directly in
       regardless, so the memcpy will still be required.
       It's not really guaranteed that pages will be
       continuous in real memory, even if we know where it is,
       so it is appropriate to go via a buffer regardless. */
    load += EA_OFFSET + pdos->curr_aspace * (BTL_PRIVLEN * 1024 * 1024);
#endif
    printf("PCOMM should reside on cylinder %d, head 0 of IPL device\n",
           pdos->cyl_upto);
    /* Note that we read until we get EOF (a zero-length block). */
    i = 0;
    j = 1;
    while (cnt != 0)
    {
#if 0
        printf("loading to %p from %d, %d, %d\n", load,
               pdos->cyl_upto, i, j);
#endif
        cnt = rdblock(pdos->ipldev, pdos->cyl_upto, i, j, tbuf, MAXBLKSZ);
        if (cnt == -1)
        {
            if (lastcnt == -2)
            {
                printf("three I/O errors in a row - terminating load\n");
                break;
            }
            else if (lastcnt == -1)
            {
                printf("probably last track on cylinder\n");            
                /* probably reached last track on cylinder */
                lastcnt = -2;
                pdos->cyl_upto++;
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
    memset(&pdos->context, 0x00, sizeof pdos->context);
    pdos->context.regs[13] = (int)savearea;
    pdos->context.regs[14] = (int)gotret;
    pdos->context.regs[15] = (int)entry;
    pdos->context.psw1 = PSW_ENABLE_INT; /* need to enable interrupts */
    pdos->context.psw2 = (int)entry; /* 24-bit mode for now */

    pptrs[0] = mvsparm;
    
    pdos->context.regs[1] = (int)pptrs;
    
    return (1);
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
        task  = getNextAvailTask();
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
