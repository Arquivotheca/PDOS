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
/*  All interrupts point to the address after that LPSW.             */
/*                                                                   */
/*  Registers are provided and loaded prior to execution of the      */
/*  LPSW. The LPSW will refer to an address in low memory so that    */
/*  the (now invalid) register 0 can be used.                        */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int abend;
} TASK;

typedef struct {
    int regs[16];
    unsigned int psw1;
    unsigned int psw2;
} CONTEXT;

typedef struct {
    char unused1[136];
    unsigned int svc_code;
    char unused[244];
    int flcgrsav[16];
} PSA;

typedef struct {
    char unused1[36];
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

#define DCBOFOPN 0x10
#define DCBRECU  0xC0
#define DCBRECF  0x80
#define DCBRECV  0x40
#define DCBRECBR 0x10

#define INTERRUPT_SVC 1

#if 0
TASK *task;
#endif

#define CHUNKSZ 18452

typedef struct {
    CONTEXT context; /* current thread's context */
    PSA *psa;
    int exitcode;
    int shutdown;
    int ipldev;
} PDOS;

static PDOS pdos;


void gotret(void);
int adisp(CONTEXT *context);
void dput(void);
void dcheck(void);
void dexit(int oneexit, DCB *dcb);


int pdosRun(PDOS *pdos);
void pdosDefaults(PDOS *pdos);
int pdosInit(PDOS *pdos);
void pdosTerm(PDOS *pdos);
static int pdosDispatchUntilInterrupt(PDOS *pdos);
static void pdosProcessSVC(PDOS *pdos);
static int pdosLoadPcomm(PDOS *pdos);


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


int main(int argc, char **argv)
{
    static PDOS pdos;
    int ret = EXIT_FAILURE;
    
    pdosDefaults(&pdos);
    if (pdosInit(&pdos))
    {
        ret = pdosRun(&pdos);
        pdosTerm(&pdos);
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
    pdos->psa = 0;
    return;
}


/* initialize PDOS */
/* zero return = error */

int pdosInit(PDOS *pdos)
{
    /* thankfully we are running under an emulator, so have access
       to printf debugging (to the Hercules console via DIAG8),
       and Hercules logging */
    printf("Welcome to PDOS!!!\n");

    pdos->ipldev = initsys();
    printf("IPL device is %x\n", pdos->ipldev);
    pdos->shutdown = 0;
    pdosLoadPcomm(pdos);
    return (1);
}


/* pdosTerm - any cleanup routines required */

void pdosTerm(PDOS *pdos)
{
    return;
}


static int pdosDispatchUntilInterrupt(PDOS *pdos)
{
    int ret;

    while (1)
    {
        /* store registers in low memory to be loaded */
        memcpy(pdos->psa->flcgrsav,
               pdos->context.regs,
               sizeof pdos->context.regs);
               
        ret = adisp(&pdos->context);  /* dispatch */
        
        /* restore registers from low memory */
        memcpy(pdos->context.regs,
               pdos->psa->flcgrsav,               
               sizeof pdos->context.regs);
        
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
    }
    return (INTERRUPT_SVC); /* only doing SVCs at the moment */
}


#define PDOS_STORSTART 0x600000 /* where free storage starts */
#define PDOS_STORINC   0x080000 /* storage increment */

static void pdosProcessSVC(PDOS *pdos)
{
    int svc;
    static DCB *dcb = NULL; /* need to eliminate this state info */
    static int getmain = PDOS_STORSTART;
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
            getmain += PDOS_STORINC;
        }
        /* pdos->context.regs[1] = 0x4100000; */
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

        dcb = (DCB *)pdos->context.regs[10]; 
            /* need to protect against this */
            /* and it's totally wrong anyway */
        dcb->u1.dcbput = (int)dput;
        dcb->dcbcheck = (int)dcheck;
        dcb->u2.dcbrecfm |= DCBRECF;
        dcb->dcblrecl = 80;
        dcb->dcbblksi = 80;
        oneexit = dcb->u2.dcbexlsa & 0xffffff;
        if (oneexit != 0)
        {
            oneexit = *(int *)oneexit & 0xffffff;
            if (oneexit != 0)
            {
                dexit(oneexit, dcb);
            }
        }
        pdos->context.regs[15] = 0;
    }
    else if (svc == 22) /* open */
    {
        dcb->u1.dcboflgs |= DCBOFOPN;
        pdos->context.regs[15] = 0; /* is this required? */
    }
    return;
}


#define PCOMM_LOAD 0x400000
#define PCOMM_ENTRY 0x400008
#define PCOMM_HEAP 0x500000
#define PSW_ENABLE_INT 0x000c0000 /* actually disable interrupts for now */

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
    static struct { int dum;
                    int len;
                    char *heap; } pblock = 
        { 0,
          sizeof(char *),
          (char *)PCOMM_HEAP };
    static int savearea[20]; /* needs to be in user space */
    static char mvsparm[] = { 0, 8, 'H', 'i', ' ', 'T', 'h', 'e', 'r', 'e' };
    static char *pptrs[1];

    printf("PCOMM should reside on cylinder 2, head 0 of IPL device\n");
    for (i = 0; i < 10; i++)
    {
        for (j = 1; j < 4; j++)
        {
#if 0
            printf("loading to %p from 2, %d, %d\n", load, i, j);
#endif
            rdblock(pdos->ipldev, 2, i, j, load, CHUNKSZ);
            load += CHUNKSZ;
        }
    }
    memset(&pdos->context, 0x00, sizeof pdos->context);
    pdos->context.regs[1] = (int)&pblock;
    pdos->context.regs[13] = (int)savearea;
    pdos->context.regs[14] = (int)gotret;
    pdos->context.regs[15] = (int)entry;
    pdos->context.psw1 = PSW_ENABLE_INT; /* need to enable interrupts */
    pdos->context.psw2 = (int)entry; /* 24-bit mode for now */

    pptrs[0] = mvsparm;
    
    pdos->context.regs[1] = (int)pptrs;
    
    return (1);
}
