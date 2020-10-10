/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  funcs - some low-level functions                                 */
/*                                                                   */
/*********************************************************************/

#include <dos.h>

static void int86n(unsigned int intno);
static void int86i(unsigned int intno, union REGS *regsin);


int BosDiskSectorRead(void         *buffer,
                      unsigned long sectors,
                      unsigned long drive,
                      unsigned long track,
                      unsigned long head,
                      unsigned long sector)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
    
    regsin.h.ah = 0x02;
    regsin.h.al = (unsigned char)sectors;
    regsin.h.ch = (unsigned char)(track & 0xff);
    regsin.h.cl = (unsigned char)sector;
    regsin.h.cl |= ((track & 0x0300U) >> 2);
    regsin.h.dh = (unsigned char)head;
    regsin.h.dl = (unsigned char)drive;
    sregs.es = FP_SEG(buffer);
    regsin.x.bx = FP_OFF(buffer);
    int86x(0x13, &regsin, &regsout, &sregs);
    return (regsout.x.cflag);
}

int BosDriveParms(unsigned long drive,
                  unsigned long *tracks,
                  unsigned long *sectors,
                  unsigned long *heads,
                  unsigned long *attached,
                  unsigned char **parmtable,
                  unsigned long *drivetype)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
    
    regsin.h.ah = 0x08;
    regsin.h.dl = (unsigned char)drive;
    
    int86x(0x13, &regsin, &regsout, &sregs);
    *tracks = ((regsout.h.cl & 0xC0U) << 2) | regsout.h.ch + 1;
    *sectors = (regsout.h.cl & 0x3F) + 1;
    *heads = regsout.h.dh + 1;
    *attached = regsout.h.dl;
    *parmtable = MK_FP(sregs.es, regsout.x.di);
    *drivetype = regsout.h.bl;
    if (regsout.x.cflag == 0)
    {
        return (0);
    }
    else
    {
        return (regsout.h.ah);
    }
}

int BosDiskReset(unsigned long drive)
{
    union REGS regsin;
    union REGS regsout;

    regsin.h.ah = 0x00;
    regsin.h.dl = (unsigned char)drive;    
    int86(0x13, &regsin, &regsout);
    return (regsout.x.cflag);
}

int BosDiskStatus(unsigned long drive, unsigned long *status)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x01;
    regsin.h.dl = drive;    
    int86(0x13, &regsin, &regsout);
    *status = regsout.h.ah;
    return (0);
}


/* int86n - do an interrupt with no registers */

static void int86n(unsigned int intno)
{
    union REGS regsin;
    union REGS regsout;
    
    int86(intno, &regsin, &regsout);
    return;
}    


/* int86i - do an interrupt with input registers only */

static void int86i(unsigned int intno, union REGS *regsin)
{
    union REGS regsout;
    
    int86(intno, regsin, &regsout);
    return;
}    


/* BosPrintScreen - BIOS Int 05h */

int BosPrintScreen(void)
{
    int86n(0x05);
    return (0);
}


/* BosSetVideoMode - BIOS Int 10h Function 00h */

int BosSetVideoMode(unsigned long mode)
{
    union REGS regsin;
    
    regsin.h.ah = 0x00;
    regsin.h.al = (unsigned char)mode;
    int86i(0x10, &regsin);
    return (0);
}


/* BosSetCursorType - BIOS Int 10h Function 01h */

int BosSetCursorType(int top, int bottom)
{
    union REGS regsin;
    
    regsin.h.ah = 0x01;
    regsin.h.ch = top;
    regsin.h.cl = bottom;
    int86i(0x10, &regsin);
    return (0);
}


/* BosSetCursorPosition - BIOS Int 10h Function 02h */

int BosSetCursorPosition(int page, int row, int column)
{
    union REGS regsin;
    
    regsin.h.ah = 0x02;
    regsin.h.bh = page;
    regsin.h.dh = row;
    regsin.h.dl = column;
    int86i(0x10, &regsin);
    return (0);
}


/* BosReadCursorPositon - BIOS Int 10h Function 03h */

int BosReadCursorPosition(int page, 
                          int *cursorStart,
                          int *cursorEnd,
                          int *row,
                          int *column)
{
    union REGS regs;
    
    regs.h.ah = 0x03;
    regs.h.bh = page;
    int86(0x10, &regs, &regs);
    *cursorStart = regs.h.ch;
    *cursorEnd = regs.h.cl;
    *row = regs.h.dh;
    *column = regs.h.dl;
    return (0);
}


/* BosReadLightPen - BIOS Int 10h Function 04h */

int BosReadLightPen(int *trigger,
                    unsigned long *pcolumn,
                    unsigned long *prow1,
                    unsigned long *prow2,
                    int *crow,
                    int *ccol)
{
    union REGS regs;
    
    regs.h.ah = 0x04;
    int86(0x10, &regs, &regs);
    *trigger = regs.h.ah;
    *pcolumn = regs.x.bx;
    *prow1 = regs.h.ch;
    *prow2 = regs.x.cx;
    *crow = regs.h.dh;
    *ccol = regs.h.dl;
    return (0);
}                    


/* BosSetActiveDisplayPage - BIOS Int 10h Function 05h */

int BosSetActiveDisplayPage(int page)
{
    union REGS regsin;
    
    regsin.h.ah = 0x05;
    regsin.h.al = page;
    int86i(0x10, &regsin);
    return (0);
}


/* BosScrollWindowUp - BIOS Int 10h Function 06h */

int BosScrollWindowUp(int numLines,
                      int attrib,
                      int startRow,
                      int startCol,
                      int endRow,
                      int endCol)
{
    union REGS regsin;
    
    regsin.h.ah = 0x06;
    regsin.h.al = numLines;
    regsin.h.bh = attrib;
    regsin.h.ch = startRow;
    regsin.h.cl = startCol;
    regsin.h.dh = endRow;
    regsin.h.dl = endCol;
    int86i(0x10, &regsin);
    return (0);
}


/* BosScrollWindowDown - BIOS Int 10h Function 07h */

int BosScrollWindowDown(int numLines,
                        int attrib,
                        int startRow,
                        int startCol,
                        int endRow,
                        int endCol)
{
    union REGS regsin;
    
    regsin.h.ah = 0x07;
    regsin.h.al = numLines;
    regsin.h.bh = attrib;
    regsin.h.ch = startRow;
    regsin.h.cl = startCol;
    regsin.h.dh = endRow;
    regsin.h.dl = endCol;
    int86i(0x10, &regsin);
    return (0);
}


/* BosReadCharAttrib - BIOS Int 10h Function 08h */

int BosReadCharAttrib(int page, int *ch, int *attrib)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x08;
    regsin.h.bh = page;
    int86(0x10, &regsin, &regsout);
    *attrib = regsout.h.ah;
    *ch = regsout.h.al;
    return (0);
}


/* BosWriteCharAttrib - BIOS Int 10h Function 09h */

int BosWriteCharAttrib(int page, int ch, int attrib, unsigned long num)
{
    union REGS regsin;
    
    regsin.h.ah = 0x09;
    regsin.h.al = ch;
    regsin.h.bh = page;
    regsin.h.bl = attrib;
    regsin.x.cx = (unsigned int)num;
    int86i(0x10, &regsin);
    return (0);
}


/* BosWriteCharCursor - BIOS Int 10h Function 0Ah */

int BosWriteCharCursor(int page, int ch, int col, unsigned long num)
{
    union REGS regsin;
    
    regsin.h.ah = 0x0a;
    regsin.h.al = ch;
    regsin.h.bh = page;
    regsin.h.bl = col;
    regsin.x.cx = (unsigned int)num;
    int86i(0x10, &regsin);
    return (0);
}


/* BosSetColorPalette - BIOS Int 10h Function 0Bh */

int BosSetColorPalette(int id, int val)
{
    union REGS regsin;
    
    regsin.h.ah = 0x0b;
    regsin.h.bh = id;
    regsin.h.bl = val;
    int86i(0x10, &regsin);
    return (0);
}


/* BosWriteGraphicsPixel - BIOS Int 10h Function 0Ch */

int BosWriteGraphicsPixel(int page, 
                          int color, 
                          unsigned long row,
                          unsigned long column)
{
    union REGS regsin;
    
    regsin.h.ah = 0x0c;
    regsin.h.al = color;
    regsin.h.bh = page;
    regsin.x.cx = (unsigned int)column;
    regsin.x.dx = (unsigned int)row;
    int86i(0x10, &regsin);
    return (0);
}


/* BosReadGraphicsPixel - BIOS Int 10h Function 0Dh */

int BosReadGraphicsPixel(int page, 
                         unsigned long row,
                         unsigned long column,
                         int *color)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x0d;
    regsin.h.bh = page;
    regsin.x.cx = (unsigned int)column;
    regsin.x.dx = (unsigned int)row;
    int86(0x10, &regsin, &regsout);
    *color = regsout.h.al;
    return (0);
}


/* BosWriteText - BIOS Int 10h Function 0Eh */

int BosWriteText(int page, int ch, int color)
{
    union REGS regsin;
    
    regsin.h.ah = 0x0e;
    regsin.h.al = ch;
    regsin.h.bh = page;
    regsin.h.bl = color;
    int86i(0x10, &regsin);
    return (0);
}


/* BosGetVideoMode - BIOS Int 10h Function 0Fh */

int BosGetVideoMode(int *columns, int *mode, int *page)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x0f;
    int86(0x10, &regsin, &regsout);
    *columns = regsout.h.ah;
    *mode = regsout.h.al;
    *page = regsout.h.bh;
    return (0);
}
