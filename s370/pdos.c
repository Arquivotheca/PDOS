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

int intrupt;
TASK *task;

#define CHUNKSZ 18452


void gotret(void);
int adisp(CONTEXT *context);
void dput(void);
void dcheck(void);
void dexit(int oneexit, DCB *dcb);

int main(int argc, char **argv)
{
    /* Note that the loader may pass on a parameter of some sort */
    /* Also under other (test) environments, may have options. */

#if 0
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
#endif

    int ret;
    int dev;
    char *load;
    char *start = (char *)0x400000;
    /* Standard C programs can start at a predictable offset */
    int (*entry)(void *) = (int (*)(void *))0x400008;
    int i;
    int j;
    struct { int dum;
             int len;
             char *heap; } pblock = { 0, 4, (char *)0x500000 };
    void (*fun)(void *);
    CONTEXT context;
    int savearea[20]; /* needs to be in user space */
    char mvsparm[] = { 0, 8, 'H', 'i', ' ', 'T', 'h', 'e', 'r', 'e' };
    char *pptrs[1];
    PSA *psa = 0;
    DCB *dcb;

    /* thankfully we are running under an emulator, so have access
       to printf debugging (to the Hercules console via DIAG8),
       and Hercules logging */
    printf("Welcome to PDOS!!!\n");

    printf("PCOMM should reside on cylinder 2, head 0 of IPL device\n");
    dev = initsys();
    printf("IPL device is %x\n", dev);
    load = start;
    for (i = 0; i < 10; i++)
    {
        for (j = 1; j < 4; j++)
        {
#if 0
            printf("loading to %p from 2, %d, %d\n", load, i, j);
#endif
            rdblock(dev, 2, i, j, load, CHUNKSZ);
            load += CHUNKSZ;
        }
    }
    memset(&context, 0x00, sizeof context);
    context.regs[1] = (int)&pblock;
    context.regs[13] = (int)savearea;
    context.regs[14] = (int)gotret;
    context.regs[15] = (int)entry;
    context.psw1 = 0x000c0000; /* need to enable interrupts */
    context.psw2 = (int)entry; /* 24-bit mode for now */

    pptrs[0] = mvsparm;
    
    context.regs[1] = (int)pptrs;
    
    for (;;) /* i = 0; i < 80; i++) */
    {
        int svc;
        static int getmain = 0x600000;
        
        ret = adisp(&context);  /* dispatch */
        svc = psa->svc_code & 0xffff;
        if (ret == 0) printf("SVC code is %d\n", svc);
        if (ret == 2)
        {
            /* need to fix bug in PDPCLIB - long lines should be truncated */
            printf("%.80s\n", context.regs[4]); /* wrong!!! */
            context.psw2 &= 0xffffff; /* move this to assembler with STCM/MVI */
        }
        else if (svc == 3)
        {
            /* normally the OS would not exit on program end */
            printf("return from PCOMM is %d\n", context.regs[15]);
            break;
        }
        else if ((svc == 120) || (svc == 10))
        {
            if ((svc == 10) || (context.regs[1] == 0)) /* if really getmain */
            {
                context.regs[1] = getmain;
                context.regs[15] = 0;
                getmain += 0x080000;
            }
            /* context.regs[1] = 0x4100000; */
        }
        else if (svc == 24) /* devtype */
        {
            /* hardcoded constants obtained from running MVS 3.8j system */
            memcpy((void *)context.regs[0], 
                   "\x30\x50\x20\x0B\x00\x00\xF6\xC0",
                   8);
            context.regs[15] = 0;
        }
        else if (svc == 64) /* rdjfcb */
        {
            int oneexit;

            dcb = (DCB *)context.regs[10]; 
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
            context.regs[15] = 0;
        }
        else if (svc == 22) /* open */
        {
            dcb->u1.dcboflgs |= DCBOFOPN;
            context.regs[15] = 0; /* is this required? */
        }
    }

#if 0
    printf("about to dispatch\n");
    ret = adisp(&context);  /* dispatch */
    printf("returned from dispatch with %d\n", ret);
    /*ret = entry(&pblock);*/
    printf("return from PCOMM is %d\n", ret);
    printf("SVC code is %d\n", psa->svc_code & 0xffff);
    printf("R15 is %x\n", context.regs[15]);
#endif

#if 0
    printf("about to read first block of PCOMM\n");
    rdblock(dev, 1, 0, 1, buf, sizeof buf);
    printf("got %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3]);
    printf("and %x %x %x %x\n", buf[4], buf[5], buf[6], buf[7]);
#endif

    return (0); /* In future, there may be an option to exit the OS */
}
