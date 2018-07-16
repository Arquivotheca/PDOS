/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  bos - some low-level BIOS functions                              */
/*                                                                   */
/*********************************************************************/

#include "bos.h"
#include "support.h"

/* x86 interrupt call with no input registers */
static void int86n(unsigned int intno);
/* x86 interrupt call with registers */
static void int86i(unsigned int intno, union REGS *regsin);

/* BosPrintScreen-BIOS Int 05h */
/*
    Input: None.
    Returns: 0 
    Notes: Some old BIOSes/applications appear to 
           destroy BP on return.
*/

void BosPrintScreen(void)
{
    int86n(0x05);
    
    return;
}


/* BosSetVideoMode - BIOS Int 10h Function 00h */
/* Mode 03h = text mode */
/*      13h = 320*200 graphics mode, location 0xa0000 */
/*
    Input: Mode of video
    Returns: AL video mode flag
    Notes: None.
*/

unsigned int BosSetVideoMode(unsigned int mode)
{
    union REGS regsin;
    union REGS regsout;

    regsin.h.ah = 0x00;
    regsin.h.al = (unsigned char)mode;
    
    int86(0x10,&regsin,&regsout);
    
    return (regsout.h.al);
}


/* BosSetCursorType - BIOS Int 10h Function 01h */
/*
    Input: Top and Bottom of cursor
    Returns: None
    Notes:
*/
    
void BosSetCursorType(unsigned int top, unsigned int bottom)
{
    union REGS regsin;

    regsin.h.ah = 0x01;
    regsin.h.ch = top;
    regsin.h.cl = bottom;
    
    int86i(0x10, &regsin);
    
    return;
}


/* BosSetCursorPosition - BIOS Int 10h Function 02h */
/*
   Input: page number, row and column
   Returns: None.
   Notes: None.
*/

void BosSetCursorPosition(unsigned int page, unsigned int row, 
                          unsigned int column)
{
    union REGS regsin;

    regsin.h.ah = 0x02;
    regsin.h.bh = page;
    regsin.h.dh = row;
    regsin.h.dl = column;
    
    int86i(0x10, &regsin);
    
    return;
}


/* BosReadCursorPositon - BIOS Int 10h Function 03h */
/*
    Input: page, retrives values into pointers cursorstart
           cursorend, row and column to point out exact position
           of the cursor.
    Returns: None.
    Notes: None.

*/

void BosReadCursorPosition(unsigned int page,
                           unsigned int *cursorStart,
                           unsigned int *cursorEnd,
                           unsigned int *row,
                           unsigned int *column)
{
    union REGS regs;

    regs.h.ah = 0x03;
    regs.h.bh = page;
    
    int86(0x10, &regs, &regs);
    
    *cursorStart = regs.h.ch;
    *cursorEnd = regs.h.cl;
    *row = regs.h.dh;
    *column = regs.h.dl;
    
    return;
}


/* BosReadLightPen - BIOS Int 10h Function 04h */
/*
    Input: Light Pen trigger,pixel row and pixel column
    Returns : None.
    Notes: On a CGA, returned column numbers are always 
           multiples of 2 (320- column modes) or 4 (640-column modes). 
           Returned row numbers are only accurate to two lines 
*/

void BosReadLightPen(int *trigger,
                     unsigned int *pcolumn,
                     unsigned int *prow1,
                     unsigned int *prow2,
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
    
    return;
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

int BosWriteCharAttrib(int page, int ch, int attrib, unsigned int num)
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

int BosWriteCharCursor(int page, int ch, int col, unsigned int num)
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


/* BosSetColourPalette - BIOS Int 10h Function 0Bh */

int BosSetColourPalette(int id, int val)
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
                          int colour,
                          unsigned int row,
                          unsigned int column)
{
    union REGS regsin;

    regsin.h.ah = 0x0c;
    regsin.h.al = colour;
    regsin.h.bh = page;
    regsin.x.cx = (unsigned int)column;
    regsin.x.dx = (unsigned int)row;
    int86i(0x10, &regsin);
    return (0);
}


/* BosReadGraphicsPixel - BIOS Int 10h Function 0Dh */

int BosReadGraphicsPixel(int page,
                         unsigned int row,
                         unsigned int column,
                         int *colour)
{
    union REGS regsin;
    union REGS regsout;

    regsin.h.ah = 0x0d;
    regsin.h.bh = page;
    regsin.x.cx = (unsigned int)column;
    regsin.x.dx = (unsigned int)row;
    int86(0x10, &regsin, &regsout);
    *colour = regsout.h.al;
    return (0);
}


/* BosWriteText - BIOS Int 10h Function 0Eh */

int BosWriteText(int page, int ch, int colour)
{
    union REGS regsin;

    regsin.h.ah = 0x0e;
    regsin.h.al = ch;
    regsin.h.bh = page;
    regsin.h.bl = colour;
    int86i(0x10, &regsin);
    /* wportb(0xe9, ch); */ /* works for PDOS-16 under Bochs */
    /* outp(0xe9, ch); */ /* works for PDOS-32 under Bochs */
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


/* BosVBEGetInfo - BIOS Int 10h Function 4F00h */

int BosVBEGetInfo(void *buffer)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;

    regsin.x.ax = 0x4f00;
    sregs.es = FP_SEG(buffer);
    regsin.x.di = FP_OFF(buffer);
#ifdef __32BIT__
    sregs.es = ((unsigned long)ADDR2ABS(buffer) >> 4) & 0xffffU;
    regsin.x.di = (unsigned long)ADDR2ABS(buffer) & 0xf;
    regsin.d.ebx = (sregs.es << 16);
#endif
    int86x(0x10, &regsin, &regsout, &sregs);
    if (regsout.h.al != 0x4f) return (2);
    return (regsout.h.ah);
}


/* BosVBEGetModeInfo - BIOS Int 10h Function 4F01h */

int BosVBEGetModeInfo(unsigned int mode, void *buffer)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;

    regsin.x.ax = 0x4f01;
    regsin.x.cx = mode;
    sregs.es = FP_SEG(buffer);
    regsin.x.di = FP_OFF(buffer);
#ifdef __32BIT__
    sregs.es = ((unsigned long)ADDR2ABS(buffer) >> 4) & 0xffffU;
    regsin.x.di = (unsigned long)ADDR2ABS(buffer) & 0xf;
    regsin.d.ebx = (sregs.es << 16);
#endif
    int86x(0x10, &regsin, &regsout, &sregs);
    if (regsout.h.al != 0x4f) return (2);
    return (regsout.h.ah);
}


/* BosVBESetMode - BIOS Int 10h Function 4F02h */

int BosVBESetMode(unsigned int mode, void *buffer)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;

    regsin.x.ax = 0x4f02;
    regsin.x.bx = mode;
    sregs.es = FP_SEG(buffer);
    regsin.x.di = FP_OFF(buffer);
#ifdef __32BIT__
    sregs.es = ((unsigned long)ADDR2ABS(buffer) >> 4) & 0xffffU;
    regsin.x.di = (unsigned long)ADDR2ABS(buffer) & 0xf;
    regsin.d.ebx = (sregs.es << 16) | regsin.x.bx;
#endif
    int86x(0x10, &regsin, &regsout, &sregs);
    if (regsout.h.al != 0x4f) return (2);
    return (regsout.h.ah);
}

/* BosVBEGetMode - BIOS Int 10h Function 4F03h */

int BosVBEGetMode(unsigned int *mode)
{
    union REGS regsin;
    union REGS regsout;

    regsin.x.ax = 0x4f03;
    int86(0x10, &regsin, &regsout);
    if (regsout.h.al != 0x4f) return (2);
    *mode = regsout.x.bx;
    return (regsout.h.ah);
}

/* BosVBEPaletteOps - BIOS Int 10h Function 4F09h */

int BosVBEPaletteOps(unsigned int operation,
                     unsigned int entries,
                     unsigned int start_index,
                     void *buffer)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;

    regsin.x.ax = 0x4f09;
    /* Operations:
     * 00h set primary palette
     * 01h get primary palette
     * 02h set secondary palette data
     * 03h get secondary palette data
     * 80h set palette during vertical retrace */
    regsin.h.bl = operation;
    regsin.x.cx = entries;
    regsin.x.dx = start_index;
    sregs.es = FP_SEG(buffer);
    regsin.x.di = FP_OFF(buffer);
#ifdef __32BIT__
    sregs.es = ((unsigned long)ADDR2ABS(buffer) >> 4) & 0xffffU;
    regsin.x.di = (unsigned long)ADDR2ABS(buffer) & 0xf;
    regsin.d.ebx = (sregs.es << 16) | regsin.x.bx;
#endif
    int86x(0x10, &regsin, &regsout, &sregs);
    if (regsout.h.al != 0x4f) return (2);
    return (regsout.h.ah);
}

int BosDiskReset(unsigned int drive)
{
    union REGS regsin;
    union REGS regsout;

    regsin.h.ah = 0x00;
    regsin.h.dl = (unsigned char)drive;
    int86(0x13, &regsin, &regsout);
    return (regsout.x.cflag);
}

int BosDiskStatus(unsigned int drive, unsigned int *status)
{
    union REGS regsin;
    union REGS regsout;

    regsin.h.ah = 0x01;
    regsin.h.dl = drive;
    int86(0x13, &regsin, &regsout);
    *status = regsout.h.ah;
    return (0);
}


/* BosDiskSectorRead - read using track etc Int 13h Function 02h */

int BosDiskSectorRead(void         *buffer,
                      unsigned int sectors,
                      unsigned int drive,
                      unsigned int track,
                      unsigned int head,
                      unsigned int sector)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;

    regsin.h.ah = 0x02;
    regsin.h.al = (unsigned char)sectors;
    regsin.h.ch = (unsigned char)(track & 0xff);
    regsin.h.cl = (unsigned char)sector;
    regsin.h.cl |= (((unsigned int)track & 0x0300U) >> 2);
    regsin.h.dh = (unsigned char)head;
    regsin.h.dl = (unsigned char)drive;
    sregs.es = FP_SEG(buffer);
    regsin.x.bx = FP_OFF(buffer);
#ifdef __32BIT__
    sregs.es = ((unsigned long)ADDR2ABS(buffer) >> 4) & 0xffffU;
    regsin.x.bx = (unsigned long)ADDR2ABS(buffer) & 0xf;
    regsin.d.ebx = (sregs.es << 16) | regsin.x.bx;
#endif
    int86x(0x13, &regsin, &regsout, &sregs);
    return (regsout.x.cflag);
}

int BosDiskSectorWrite(void         *buffer,
                       unsigned int sectors,
                       unsigned int drive,
                       unsigned int track,
                       unsigned int head,
                       unsigned int sector)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;

    regsin.h.ah = 0x03;
    regsin.h.al = (unsigned char)sectors;
    regsin.h.ch = (unsigned char)(track & 0xff);
    regsin.h.cl = (unsigned char)sector;
    regsin.h.cl |= (((unsigned int)track & 0x0300U) >> 2);
    regsin.h.dh = (unsigned char)head;
    regsin.h.dl = (unsigned char)drive;
    sregs.es = FP_SEG(buffer);
    regsin.x.bx = FP_OFF(buffer);
#ifdef __32BIT__
    sregs.es = ((unsigned long)ADDR2ABS(buffer) >> 4) & 0xffffU;
    regsin.x.bx = (unsigned long)ADDR2ABS(buffer) & 0xf;
    regsin.d.ebx = (sregs.es << 16) | regsin.x.bx;
#endif
    int86x(0x13, &regsin, &regsout, &sregs);
    return (regsout.x.cflag);
}

int BosDriveParms(unsigned int drive,
                  unsigned int *tracks,
                  unsigned int *sectors,
                  unsigned int *heads,
                  unsigned int *attached,
                  unsigned char **parmtable,
                  unsigned int *drivetype)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;

    regsin.h.ah = 0x08;
    regsin.h.dl = (unsigned char)drive;

    int86x(0x13, &regsin, &regsout, &sregs);
    *tracks = (((regsout.h.cl & 0xC0U) << 2) | regsout.h.ch) + 1;
    *sectors = (regsout.h.cl & 0x3F);
    *heads = regsout.h.dh + 1;
    *attached = regsout.h.dl;
    *parmtable = (unsigned char *)MK_FP(sregs.es, regsout.x.di);
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

int BosFixedDiskStatus(unsigned int drive)
{
    union REGS regsin;
    union REGS regsout;

    regsin.h.ah = 0x10;
    regsin.h.dl = (unsigned char)drive;
    int86(0x13, &regsin, &regsout);
    if (regsout.x.cflag)
    {
        return (regsout.x.ax);
    }
    return (0);
}


/* BosDiskSectorRLBA - read using LBA Int 13h Function 42h */
/* Returns 0 if successful, otherwise error code */

int BosDiskSectorRLBA(void         *buffer,
                      unsigned int sectors,
                      unsigned int drive,
                      unsigned long sector,
                      unsigned long hisector)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
    unsigned char lpacket[16];
    unsigned char *packet;

    regsin.h.ah = 0x42;
    regsin.h.dl = (unsigned char)drive;
    packet = lpacket;
#ifdef __32BIT__
    /* kludge - the caller will be giving us an
       address in low memory for the buffer, which
       will have enough room for the packet too */
    packet = (unsigned char *)buffer + 512;
#endif
    packet[0] = sizeof lpacket;
    packet[1] = 0;
    packet[2] = sectors & 0xff;
    packet[3] = (sectors >> 8) & 0xff;
    packet[4] = FP_OFF(buffer) & 0xff;
    packet[5] = (FP_OFF(buffer) >> 8) & 0xff;
    packet[6] = FP_SEG(buffer) & 0xff;
    packet[7] = (FP_SEG(buffer) >> 8) & 0xff;
    packet[8] = sector & 0xff;
    packet[9] = (sector >> 8) & 0xff;
    packet[10] = (sector >> 16) & 0xff;
    packet[11] = (sector >> 24) & 0xff;
    packet[12] = hisector & 0xff;
    packet[13] = (hisector >> 8) & 0xff;
    packet[14] = (hisector >> 16) & 0xff;
    packet[15] = (hisector >> 24) & 0xff;
    sregs.ds = FP_SEG(packet);
    regsin.x.si = FP_OFF(packet);
#ifdef __32BIT__
    /* first of all fix up the buffer address in the packet */
    sregs.ds = ((unsigned long)ADDR2ABS(buffer) >> 4) & 0xffffU;
    regsin.x.si = (unsigned long)ADDR2ABS(buffer) & 0xf;
    packet[4] = regsin.x.si & 0xff;
    packet[5] = (regsin.x.si >> 8) & 0xff;
    packet[6] = sregs.ds & 0xff;
    packet[7] = (sregs.ds >> 8) & 0xff;

    /* now we set the packet address */
    sregs.ds = ((unsigned long)ADDR2ABS(packet) >> 4) & 0xffffU;
    regsin.x.si = (unsigned long)ADDR2ABS(packet) & 0xf;

    /* and ds gets stored in upper edx for real mode */
    regsin.d.edx = (sregs.ds << 16) | regsin.x.dx;
#endif

    int86x(0x13, &regsin, &regsout, &sregs);
    return (regsout.h.ah);
}

/* BosDiskSectorWLBA - write using LBA Int 13h Function 43h */
/* Returns 0 if successful, otherwise error code */

int BosDiskSectorWLBA(void         *buffer,
                      unsigned int sectors,
                      unsigned int drive,
                      unsigned long sector,
                      unsigned long hisector)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
    unsigned char lpacket[16];
    unsigned char *packet;

    regsin.h.ah = 0x43;
    regsin.h.dl = (unsigned char)drive;
    packet = lpacket;
#ifdef __32BIT__
    /* kludge - the caller will be giving us an
       address in low memory for the buffer, which
       will have enough room for the packet too */
    packet = (unsigned char *)buffer + 512;
#endif
    packet[0] = sizeof lpacket;
    packet[1] = 0;
    packet[2] = sectors & 0xff;
    packet[3] = (sectors >> 8) & 0xff;
    packet[4] = FP_OFF(buffer) & 0xff;
    packet[5] = (FP_OFF(buffer) >> 8) & 0xff;
    packet[6] = FP_SEG(buffer) & 0xff;
    packet[7] = (FP_SEG(buffer) >> 8) & 0xff;
    packet[8] = sector & 0xff;
    packet[9] = (sector >> 8) & 0xff;
    packet[10] = (sector >> 16) & 0xff;
    packet[11] = (sector >> 24) & 0xff;
    packet[12] = hisector & 0xff;
    packet[13] = (hisector >> 8) & 0xff;
    packet[14] = (hisector >> 16) & 0xff;
    packet[15] = (hisector >> 24) & 0xff;
    sregs.ds = FP_SEG(packet);
    regsin.x.si = FP_OFF(packet);
#ifdef __32BIT__
    /* first of all fix up the buffer address in the packet */
    sregs.ds = ((unsigned long)ADDR2ABS(buffer) >> 4) & 0xffffU;
    regsin.x.si = (unsigned long)ADDR2ABS(buffer) & 0xf;
    packet[4] = regsin.x.si & 0xff;
    packet[5] = (regsin.x.si >> 8) & 0xff;
    packet[6] = sregs.ds & 0xff;
    packet[7] = (sregs.ds >> 8) & 0xff;

    /* now we set the packet address */
    sregs.ds = ((unsigned long)ADDR2ABS(packet) >> 4) & 0xffffU;
    regsin.x.si = (unsigned long)ADDR2ABS(packet) & 0xf;

    /* and ds gets stored in upper edx for real mode */
    regsin.d.edx = (sregs.ds << 16) | regsin.x.dx;
#endif

    int86x(0x13, &regsin, &regsout, &sregs);
    return (regsout.h.ah);
}

long BosExtendedMemorySize(void)
{
    union REGS regsin;
    union REGS regsout;

    regsin.h.ah = 0x88;
    int86(0x15, &regsin, &regsout);

    /* unfortunately it appears as if the carry flag is normally
       not changed by this interrupt, so what we want is to clear
       it before doing the interrupt and check it more thoroughly
       on the way out */
    if (regsout.x.cflag
        && ((regsout.h.ah == 0x80) || (regsout.h.ah == 0x86)))
    {
        return (0);
    }
    else
    {
        return ((long)regsout.x.ax << 10);
    }
}

/* BosReadKeyboardCharacter - BIOS Int 16h Function 00h */
/*
    Input: None.
    Returns: Scan Code and ASCII character.
    Notes: None.
*/

void BosReadKeyboardCharacter(int *scancode, int *ascii)
{
    union REGS regsin;
    union REGS regsout;

    regsin.h.ah = 0x00;
    
    int86(0x16, &regsin, &regsout);
    
    *scancode = regsout.h.ah;
    *ascii = regsout.h.al;
    
    return;
}

/* BosSystemWarmBoot - BIOS Int 19h */

void BosSystemWarmBoot(void)
{
    int86n(0x19);
    return;
}

/* BosGetSystemTime-BIOS Int 1Ah/AH=00h */
/* 
   Input: Pointers to variable Ticks(long) and midnight(int).
   Returns: None.
   Notes: None.
*/

void BosGetSystemTime(unsigned long *ticks, unsigned int *midnight)
{
    union REGS regsin;
    union REGS regsout;

    regsin.h.ah = 0x00;
    
    int86(0x1A,&regsin,&regsout);
    
    *ticks = ((unsigned long)regsout.x.cx << 16 | regsout.x.dx);
    *midnight = regsout.h.al;
    
    return;
}

/* BosGetSystemDate-BIOS Int 1Ah/AH=04h */
/*
   Input: Pointers to century(int),year(int),month(int),day(int).
   Returns: 0 on successful call.
   Notes: Clear the carry flag before interrupt Some ROM leave carry
          flag unchanged on successful call.
*/

unsigned int BosGetSystemDate(int *century,int *year,int *month,int *day)
{
    union REGS regsin;
    union REGS regsout;

    regsin.h.ah = 0x04;
    /*regsin.x.cflag = 0x00; TODO: Clearing the Carry flag.*/
    
    int86(0x1A,&regsin,&regsout);
    
    *century = regsout.h.ch;
    *year = regsout.h.cl;
    *month = regsout.h.dh;
    *day = regsout.h.dl;

    if(regsout.x.cflag)
    {
        return(BOS_ERROR);
    }
    else
    {
        return(0);
    }
}

/* BosSetSystemDate-BIOS Int 1Ah/AH=05h */
/*
   Input: Values to century(int),year(int),month(int),day(int).
   Returns: None.
   Notes: Used to set the Date.
*/

void BosSetSystemDate(int century,int year,int month,int day)
{
    union REGS regsin;

    regsin.h.ah = 0x05;
    
    regsin.h.ch = century;
    regsin.h.cl = year;
    regsin.h.dh = month;
    regsin.h.dl = day;
    
    int86i(0x1A,&regsin);
    
    return;
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

