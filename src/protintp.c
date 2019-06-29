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
/*  This is linked in to the protected mode executable. Interrupts   */
/*  are channeled into this code, and either get handled by PDOS     */
/*  or redirected to real mode                                       */
/*                                                                   */
/*********************************************************************/

#include "support.h"
#include "protint.h"
#include "unused.h"

unsigned long dorealint;
unsigned long (*runreal_p)(unsigned long func, unsigned short *regs);
rawprot_parms *rp_parms;

void inthdlr_D(void);
void inthdlr_E(void);
void inthdlr_20(void);
void inthdlr_21(void);
void inthdlr_25(void);
void inthdlr_26(void);
void inthdlr_80(void);
void inthdlr_A0(void);
void inthdlr_A3(void);
void inthdlr_A5(void);
void inthdlr_A6(void);
void inthdlr_AA(void);
void inthdlr_B0(void);
void inthdlr_B1(void);
void int_enable(void);

static unsigned short *intbuffer;
static int numUserInts;
static struct {
    int intno;
    int (*func)(unsigned int *);
} userInt[256];


/* registers come in as eax, ebx, ecx, edx, esi, edi */
/* When translating to a real mode routine we need to set up
   ax, bx, cx, dx, si, di, cflag, flags, es, blank, blank, ds */
/* also note that since it is inappropriate to be using segment
   registers in protected mode, we use the upper 16 bits of esi
   for ds, and the upper 16 bits of edi for es, when translating
   to real mode */
void gotint(int intno, unsigned int *save)
{
    unsigned short newregs[12];
    unsigned short *ssave;
    int x;

    /* clear all flags by default */
    save[6] = 0; /* cflag */
    save[7] = 0; /* flags */
    for (x = 0; x < numUserInts; x++)
    {
        if (userInt[x].intno == intno)
        {
            /* Protected mode handler of IRQ 0 is called after the BIOS one
             * so the BIOS handler is not affected by task switch. */
            if (userInt[x].intno == 0xB0) break;
            /* An int 21h e.g. will be handled by PDOS and will
               return 0 indicating that no further action is
               required. If it returns non-zero, then it will fall
               through and call the real-mode interrupt. */
            if (userInt[x].func(save) == 0)
            {
                if (save[6])
                {
                    save[7] |= 1;
                }
                return;
            }
            break;
        }
    }

    /* The default behaviour is to convert any protected mode
       interrupt into a real mode interrupt. */
    newregs[0] = (unsigned short)save[0]; /* ax */
    newregs[1] = (unsigned short)save[1]; /* bx */
    newregs[2] = (unsigned short)save[2]; /* cx */
    newregs[3] = (unsigned short)save[3]; /* dx */
    newregs[4] = (unsigned short)save[4]; /* si */
    newregs[5] = (unsigned short)save[5]; /* di */
    newregs[6] = 0;                       /* cflag */
    newregs[7] = 0;                       /* flags */
    newregs[8] = (unsigned short)(save[5] >> 16); /* es = top edi */
    newregs[9] = 0;                       /*  */
    newregs[10] = 0;                       /*  */
    newregs[11] = (unsigned short)(save[4] >> 16); /* ds = top esi */
    *intbuffer = intno;
    memcpy(intbuffer + 1, newregs, sizeof newregs);
    runreal_p(dorealint, 0);
    memcpy(newregs, intbuffer + 1, sizeof newregs);
    ssave = (unsigned short *)save;
    ssave[0*2] = newregs[0]; /* ax */
    ssave[1*2] = newregs[1]; /* bx */
    ssave[2*2] = newregs[2]; /* cx */
    ssave[3*2] = newregs[3]; /* dx */
    ssave[4*2] = newregs[4]; /* si */
    ssave[4*2 + 1] = newregs[11]; /* ds */
    ssave[5*2] = newregs[5]; /* di */
    ssave[5*2 + 1] = newregs[8]; /* es */
    save[6] = newregs[6]; /* cflag */
    save[7] = newregs[7]; /* flags */
    /* Protected mode handler of IRQ 0 is called after the BIOS one
     * so the BIOS handler is not affected by task switch. */
    if (userInt[x].intno == 0xB0) userInt[x].func(save);
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
    int x;
    unsigned long intaddr;
    runprot_parms *runparm;
    static struct {
        int number;
        void (*handler)(void);
        int maxRing; /* current ring <= maxRing can call the interrupt. */
    } handlerlist[] = {
        { 0xD, inthdlr_D, 0 },
        { 0xE, inthdlr_E, 0 },
        { 0x20, inthdlr_20, 3 },
        { 0x21, inthdlr_21, 3 },
        { 0x25, inthdlr_25, 3 },
        { 0x26, inthdlr_26, 3 },
        { 0x80, inthdlr_80, 3 },
        { 0xA0, inthdlr_A0, 0 },
        { 0xA3, inthdlr_A3, 0 },
        { 0xA5, inthdlr_A5, 0 },
        { 0xA6, inthdlr_A6, 0 },
        { 0xAA, inthdlr_AA, 0 },
        { 0xB0, inthdlr_B0, 0 },
        { 0xB1, inthdlr_B1, 0 },
        { 0, 0, 0 } };
    struct {
        unsigned short offset_15_0;
        unsigned short selector;
        unsigned char zero; /* Must be 0. */
        unsigned char type_attr;
        unsigned short offset_31_16;
    } *orig_intloc, *intloc;

    orig_intloc = (void *)(parmlist->intloc);
    runparm = (runprot_parms *)parmlist->userparm;
    intbuffer = (void *)(runparm->intbuffer);
    dorealint = runparm->dorealint;
    runreal_p = ((unsigned long (*)(unsigned long func, unsigned short *regs))
                 (runparm->runreal));

    /* Configures the Interrupt Descriptor Table (IDT). */
    /* Marks all interrupts as unused. */
    intloc = orig_intloc;
    for (x = 0; x < 256; x++)
    {
        intloc->offset_15_0 = 0;
        intloc->offset_31_16 = 0;
        intloc->zero = 0;
        intloc->selector = 0;
        intloc->type_attr = 0;
        intloc++;
    }

    /* Installs the handlers from handlerlist. */
    intloc = orig_intloc;
    for (x = 0; handlerlist[x].number != 0; x++)
    {
        intaddr = (unsigned long)handlerlist[x].handler;
        intloc = orig_intloc + handlerlist[x].number;
        intloc->offset_15_0 = (intaddr & 0xffff);
        intloc->offset_31_16 = (intaddr & 0xffff0000) >> 16;
        intloc->zero = 0;
        intloc->selector = 0x8;
        intloc->type_attr = 0x8E | (handlerlist[x].maxRing << 5);
    }
    /* End of IDT configuration. */

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


/* Allow PDOS to get access to an interrupt. */
void protintHandler(int intno, int (*func)(unsigned int *))
{
    int x;

    for (x = 0; x < numUserInts; x++)
    {
        if (userInt[x].intno == intno)
        {
            userInt[x].func = func;
            return;
        }
    }
    userInt[x].intno = intno;
    userInt[x].func = func;
    numUserInts++;
    return;
}
