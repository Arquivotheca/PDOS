/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  lldos.c - Low level MSDOS stuff that should be written in        */
/*            assembler!                                             */
/*                                                                   */
/*  Until then they are Turbo-C specific.                            */
/*                                                                   */
/*********************************************************************/

#include <string.h>

#include <alloc.h>

#include "lldos.h"

int getfar(long address)
{
    unsigned char far *add;
    
    add = (unsigned char far *)address;
    return (*add);
}

void putfar(long address, int ch)
{
    unsigned char far *add;
    
    add = (unsigned char far *)address;
    *add = ch;
    return;
}

void dint(int intno, DREGS *regsin, DREGS *regsout)
{
    union REGS in;
    union REGS out;
    struct SREGS inout;
    
    in.x.ax = regsin->ax;
    in.x.bx = regsin->bx;
    in.x.cx = regsin->cx;
    in.x.dx = regsin->dx;
    in.x.di = regsin->di;
    in.x.si = regsin->si;
    inout.ds = regsin->ds;
    inout.es = regsin->es;
    int86(intno, &in, &out);
    regsout->ax = out.x.ax;
    regsout->bx = out.x.bx;
    regsout->cx = out.x.cx;
    regsout->dx = out.x.dx;
    regsout->ds = inout.ds;
    regsout->es = inout.es;
    regsout->cf = out.x.cflag;
    regsout->flags = out.x.flags;
    return;
}

void splitfar(FARADDR *farAddr, unsigned int *x, unsigned int *y)
{
    *x = FP_SEG(*farAddr);
    *y = FP_OFF(*farAddr);    
    return;
}

void joinfar(FARADDR *farAddr, unsigned int x, unsigned int y)
{
    *farAddr = MK_FP(x, y);
    return;
}

void gfarmem(FARADDR *farAddr, long bytes)
{
    *farAddr = farmalloc(bytes);
    memset(*farAddr, '\0', bytes);
    return;
}

void ffarmem(FARADDR *farAddr)
{
    farfree(*farAddr);
    return;
}

void fromfar(FARADDR *farAddr, void *buf, size_t bytes)
{
    memcpy(buf, *farAddr, bytes);
    return;
}

void tofar(FARADDR *farAddr, void *buf, size_t bytes)
{
    memcpy(*farAddr, buf, bytes);
    return;
}


