/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  protintp - part of protint                                       */
/*                                                                   */
/*********************************************************************/

#include "support.h"
#include "protint.h"
#include "unused.h"

unsigned long dorealint;
unsigned long (*runreal_p)(unsigned long func, unsigned short *regs);
rawprot_parms *rp_parms;

void inthdlr(void);
void inthdlr_8(void);
void inthdlr_9(void);
void inthdlr_10(void);
void inthdlr_13(void);
void inthdlr_15(void);
void inthdlr_16(void);
void inthdlr_21(void);
void int_enable(void);

static unsigned short *intbuffer;
static int numUserInts;
static struct {
    int intno;
    int (*func)(unsigned int *);
} userInt[256];


/* registers come in as eax, ebx, ecx, edx, esi, edi */
/* When translating to a real mode routine we need to set up
   ax, bx, cx, dx, si, di, cflag, es, blank, blank, ds */
/* also note that since it is inappropriate to be using segment
   registers in protected mode, we use the upper 16 bits of edx
   for ds, and the upper 16 bits of ebx for es, when translating
   to real mode */
void gotint(int intno, unsigned int *save)
{
    unsigned short newregs[11];
    unsigned short *ssave;
    int x;
    
    for (x = 0; x < numUserInts; x++)
    {
        if (userInt[x].intno == intno)
        {
            if (userInt[x].func(save) == 0)
            {
                return;
            }
            break;
        }
    }

    newregs[0] = (unsigned short)save[0]; /* ax */    
    newregs[1] = (unsigned short)save[1]; /* bx */
    newregs[2] = (unsigned short)save[2]; /* cx */
    newregs[3] = (unsigned short)save[3]; /* dx */
    newregs[4] = (unsigned short)save[4]; /* si */
    newregs[5] = (unsigned short)save[5]; /* di */
    newregs[6] = 0;                       /* cflag */
    newregs[7] = (unsigned short)(save[1] >> 16); /* es = top ebx */
    newregs[8] = 0;                       /*  */
    newregs[9] = 0;                       /*  */
    newregs[10] = (unsigned short)(save[3] >> 16); /* ds = top edx */
    *intbuffer = intno;
    memcpy(intbuffer + 1, newregs, sizeof newregs);
    runreal_p(dorealint, 0);
    memcpy(newregs, intbuffer + 1, sizeof newregs);
    ssave = (unsigned short *)save;
    ssave[0*2] = newregs[0]; /* ax */
    ssave[1*2] = newregs[1]; /* bx */
    ssave[1*2 + 1] = newregs[7]; /* es */
    ssave[2*2] = newregs[2]; /* cx */
    ssave[3*2] = newregs[3]; /* dx */
    ssave[3*2 + 1] = newregs[10]; /* ds */
    ssave[4*2] = newregs[4]; /* si */
    ssave[5*2] = newregs[5]; /* di */
    save[6] = newregs[6]; /* cflag */
    return;
}


/* we don't need to do anything much up here currently for a raw call */

unsigned long rawprot_p(rawprot_parms *parmlist)
{
    rp_parms = parmlist;
    return (parmlist->userparm);
}


/* we need to reestablish the interrupts here */

unsigned long runprot_p(rawprot_parms *parmlist)
{
    char *intloc;
    int x;
    unsigned long intdesc1;
    unsigned long intdesc2;
    unsigned long intaddr;
    runprot_parms *runparm;
    static struct {
        int number;
        void (*handler)(void);
    } handlerlist[] = { 
        { 0x8, inthdlr_8 },
        { 0x9, inthdlr_9 },
        { 0x10, inthdlr_10 },
        { 0x13, inthdlr_13 },
        { 0x15, inthdlr_15 },
        { 0x16, inthdlr_16 },
        { 0x21, inthdlr_21 },
        { 0, 0 } };

    __abscor = parmlist->dsbase;
    intloc = ABSADDR(parmlist->intloc);
    runparm = (runprot_parms *)parmlist->userparm;
    intbuffer = ABSADDR(runparm->intbuffer);
    dorealint = runparm->dorealint;
    runreal_p = (unsigned long (*)(unsigned long func, unsigned short *regs))
             ABSADDR(runparm->runreal);

    memset(intloc, 0, 8 * 32);
    
    intaddr = (unsigned long)(&inthdlr);
    intdesc1 = (0x8 << 16) | (intaddr & 0xffff);
    intdesc2 = (intaddr & 0xffff0000)
               | (1 << 15)
               | (0 << 13)
               | (0x0e << 8);
               
    /* install the generic interrupt handler */
    for (x = 0; x < 256; x++)
    {
        *((unsigned long *)intloc + x * 2) = intdesc1;
        *((unsigned long *)intloc + x * 2 + 1) = intdesc2;
    }

    for (x = 0; handlerlist[x].number != 0; x++)
    {    
        intaddr = (unsigned long)handlerlist[x].handler;
        intdesc1 = (0x8 << 16) | (intaddr & 0xffff);
        *((unsigned long *)intloc + handlerlist[x].number * 2) = intdesc1;
        *((unsigned long *)intloc + handlerlist[x].number * 2 + 1) = intdesc2;
    }
    rawprot_p(parmlist);
    numUserInts = 0;
    int_enable();
    return (runparm->userparm);
}


/* we don't need to do anything special for an a.out executable
   currently.  In the past we used to have to relocate the data
   section, but that's now done in the real mode code as part of
   the load procedure */

unsigned long runaout_p(rawprot_parms *parmlist)
{
    return (runprot_p(parmlist));
}


void protintHandler(int intno, int (*func)(unsigned int *))
{
    int x;
    
    for (x = 0; x < numUserInts; x++)
    {
        if (userInt[x].intno == intno)
        {
            userInt[x].func = func;
            if (userInt[x].func == 0)
            {
                numUserInts--;
                if (numUserInts > 0)
                {
                    userInt[x].intno = userInt[numUserInts].intno;
                    userInt[x].func = userInt[numUserInts].func;
                }
            }
        }
        return;
    }
    userInt[numUserInts].intno = intno;
    userInt[numUserInts].func = func;
    numUserInts++;
    return;
}
