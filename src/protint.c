/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  protint - protected mode interface                               */
/*                                                                   */
/*  This code is linked into the real mode executable and allows     */
/*  the execution of a protected mode binary, and also for the       */
/*  protected mode to call a real mode interrupt (runreal).          */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <string.h>

#include "protint.h"
#include "lldos.h"
#include "support.h"
#include "a_out.h"
#include "unused.h"

unsigned long rawprota(unsigned long corsubr, 
                       unsigned long codecor,
                       unsigned long newsp,
                       unsigned long parmlist);

unsigned long runreal(int intno, unsigned short *regs);
void rtop_stage2(void);
unsigned long protget32(void);

extern unsigned long newstack;

typedef struct {
    unsigned short limit;
    unsigned short base15_0;
    unsigned char base23_16;
    unsigned char access;
    unsigned char gran_limit;
    unsigned char base31_24;
} descriptor;

static struct {
    descriptor null_descriptor;
    descriptor os_code;
    descriptor os_data;
    descriptor small_code;
    descriptor small_data;
    descriptor spawn_code;
    descriptor spawn_data;
} descriptors = { 
  {0},
  { 0xffff, 0x0, 0xff, 0x9a, 0xcf, 0xff },
  { 0xffff, 0x0, 0xff, 0x92 /* not code, goes up, writable */, 0xcf, 0xff},
  { 0xffff, 0x0, /* scbase */ 0x00, 0x9a, 0x00, 0x00 },
  { 0xffff, 0x0, /* sdbase */ 0x00, 0x92, 0x00, 0x00 },
  { 0xffff, 0x0, 0x00, 0x9a, 0xcf, 0x00 },
  { 0xffff, 0x0, 0xff, 0x92, 0xcf, 0xff }
};

typedef struct {
    unsigned short len;
    unsigned long absaddr;
} descptr;

descptr gdtinfo = { 0x37 };

static unsigned long interrupts[256*2]; /* allow room for 256 interrupts */
descptr idtinfo = { 0x7ff };

/* this real mode idt pointer points to the standard real mode
   interrupts.  We allow room for 256 interrupts. */
descptr ridtinfo = { 0x3ff, 0L };

static unsigned short intbuffer[12];

static unsigned long mycbase;
static unsigned long mydbase;

static unsigned long dorealint(unsigned long parm);

unsigned long rawprot(unsigned long csbase,
                      unsigned long ip,
                      unsigned long dsbase,
                      unsigned long prot_sp,
                      unsigned long userparm,
                      unsigned long intloc)
{
    unsigned long absgdt;
    unsigned long codecor;
    unsigned long parmlist_p;
    rawprot_parms parmlist;
    unsigned long myc32base;

    myc32base = protget32();
    if (myc32base == 0)
    {
        myc32base = (unsigned long)(rtop_stage2);
    }
    myc32base = ADDR2ABS(myc32base);
    myc32base = myc32base & 0xfffffff0UL;
        
    mycbase = (unsigned long)rawprota;
    mycbase = (mycbase >> 16) << 4;

    mydbase = (unsigned long)&newstack;
    mydbase = (mydbase >> 16) << 4;

    descriptors.os_code.base15_0 = (unsigned short)csbase;
    descriptors.os_code.base23_16 = (unsigned char)(csbase >> 16);
    descriptors.os_code.base31_24 = (unsigned char)(csbase >> 24);
    descriptors.os_data.base15_0 = (unsigned short)dsbase;
    descriptors.os_data.base23_16 = (unsigned char)(dsbase >> 16);
    descriptors.os_data.base31_24 = (unsigned char)(dsbase >> 24);
    
    descriptors.small_code.base15_0 = (unsigned short)mycbase;
    descriptors.small_code.base23_16 = (unsigned char)(mycbase >> 16);
    descriptors.small_code.base31_24 = (unsigned char)(mycbase >> 24);
    descriptors.small_data.base15_0 = (unsigned short)mydbase;
    descriptors.small_data.base23_16 = (unsigned char)(mydbase >> 16);
    descriptors.small_data.base31_24 = (unsigned char)(mydbase >> 24);
    
    absgdt = ADDR2ABS(&descriptors);    
    gdtinfo.absaddr = absgdt;
    
    idtinfo.absaddr = intloc;
    
    codecor = myc32base - csbase;
    
    parmlist.dsbase = dsbase;
    parmlist.gdt = absgdt;
    parmlist.intloc = intloc;
    parmlist.userparm = userparm;
    parmlist_p = ADDR2ABS(&parmlist);
    parmlist_p -= dsbase;
    
    return (rawprota(ip, codecor, prot_sp, parmlist_p));
}

unsigned long runprot(unsigned long csbase,
                      unsigned long ip,
                      unsigned long dsbase,
                      unsigned long prot_sp,
                      unsigned long userparm)
{
    unsigned long intloc;
    runprot_parms runparm;
    unsigned long runparm_p;
    unsigned long myc32base;
    
    intloc = ADDR2ABS(interrupts);
    
    runparm.userparm = userparm;    
    runparm.intbuffer = ADDR2ABS(intbuffer);
    myc32base = protget32();
    if (myc32base == 0)
    {
        runparm.runreal = ADDR2ABS(runreal);
    }
    else
    {
        runparm.runreal = ((myc32base >> 16) << 4) + (unsigned short)runreal;
    }
    runparm.dorealint = (unsigned long)dorealint;
    
    runparm_p = ADDR2ABS(&runparm);
    runparm_p -= dsbase;
    
    return (rawprot(csbase, ip, dsbase, prot_sp, runparm_p, intloc));
}

unsigned long runaout(char *fnm, unsigned long absaddr, unsigned long userparm)
{
    FILE *fp;
    int ret;
    unsigned long curraddr;
    char buf[512];
    int x;
    int y;
    struct exec firstbit;
    int numreads;
    unsigned long sp;

    fp = fopen(fnm, "rb");
    if (fp == NULL) return (-1);
    fread(buf, 1, sizeof buf, fp);
    memcpy(&firstbit, buf, sizeof firstbit);

    /* skip rest of header info */
    numreads = N_TXTOFF(firstbit) / sizeof buf;
    numreads--;
    for (x = 0; x < numreads; x++)
    {
        fread(buf, 1, sizeof buf, fp);
    }
    
    /* read code */
    numreads = (int)(firstbit.a_text / sizeof buf);
    curraddr = absaddr;
    for (x = 0; x < numreads; x++)
    {
        ret = fread(buf, 1, sizeof buf, fp);
        for (y = 0; y < ret; y++)
        {
            putabs(curraddr + y, buf[y]);
        }
        curraddr += ret;
    }

    /* read data */    
    numreads = (int)(firstbit.a_data / sizeof buf);
    curraddr = absaddr + N_DATADDR(firstbit) - N_TXTADDR(firstbit);
    for (x = 0; x < numreads; x++)
    {
        ret = fread(buf, 1, sizeof buf, fp);
        for (y = 0; y < ret; y++)
        {
            putabs(curraddr + y, buf[y]);
        }
        curraddr += ret;
    }
    
    fclose(fp);
    
    /* initialise BSS */
    curraddr = absaddr + N_BSSADDR(firstbit) - N_TXTADDR(firstbit);
    for (y = 0; y < firstbit.a_bss; y++)
    {
        putabs(curraddr + y, '\0');
    }
    
    sp = N_BSSADDR(firstbit) + firstbit.a_bss + 0x8000;
    absaddr -= 0x10000UL;
    return (runprot(absaddr, 0x10000UL, absaddr, sp, userparm));
}

unsigned long realsub(unsigned long func, unsigned long parm)
{
    unsigned long (*functocall)(unsigned long parm);
    
    functocall = (unsigned long (*)(unsigned long))func;
    return (functocall(parm));
}

static unsigned long dorealint(unsigned long parm)
{
    unsigned short *genshort;
    
    unused(parm);
    genshort = intbuffer;
    int86x(*genshort, 
           (union REGS *)(genshort + 1),
           (union REGS *)(genshort + 1),
           (struct SREGS *)(genshort + 8));
    return (0);
}
