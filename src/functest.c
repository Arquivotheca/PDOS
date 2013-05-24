/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  functest - test some low-level functions                         */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <string.h>
#include "pos.h"
#include "bos.h"

static int testBosGetVideoMode(void)
{
    int columns, mode, page;
    
    BosGetVideoMode(&columns, &mode, &page);
    printf("columns is %d\n", columns);
    printf("mode is %x\n", mode);
    printf("page is %d\n", page);
    return (0);
}

static int testBosWriteText(void)
{
    BosWriteText(0, 'A', 0);
    return (0);
}

static int testBosReadGraphics(void)
{
    int colour;
    
    BosSetVideoMode(0x12);   
    BosReadGraphicsPixel(0, 20, 40, &colour);
    BosSetVideoMode(0x02);
    printf("colour is %x\n", colour);
    return (0);
}

static int testBosWriteGraphicsPixel(void)
{
    int x, y;
 
    BosSetVideoMode(0x12);   
    for (x=0; x<480; x++)
    {
        for (y=0; y<640; y++)
        {
            BosWriteGraphicsPixel(0, 3, x, y);
        }
    }
    sleep(5);
    BosSetVideoMode(0x02);
    return (0);
}

static int testBosSetColourPalette(void)
{
    BosSetColourPalette(0, 0x03);
    return (0);
}

static int testBosWriteCharCursor(void)
{
    BosWriteCharCursor(0, 'A', 0, 50);
    return (0);
}

static int testBosWriteCharAttrib(void)
{
    BosWriteCharAttrib(0, 'A', 0x07, 50);
    return (0);
}

static int testBosReadCharAttrib(void)
{
    int ch, attrib;
    
    BosReadCharAttrib(0, &ch, &attrib);
    printf("ch is %x, attrib is %x\n", ch, attrib);
    return (0);
}

static int testBosScroll(void)
{
    BosScrollWindowUp(2, 0, 2, 2, 8, 20);
    BosScrollWindowDown(2, 0, 12, 2, 20, 40);
    return (0);
}

static int testBosReadLightPen(void)
{
    int trigger;
    unsigned long pcolumn, prow1, prow2;
    int crow, ccol;
    
    BosReadLightPen(&trigger, &pcolumn, &prow1, &prow2, &crow, &ccol);
    printf("trigger is %d\n", trigger);
    return (0);
}

static int testBosReadCursorPosition(void)
{
    int cursorStart, cursorEnd, row, column;
    
    BosReadCursorPosition(0, 
                          &cursorStart,
                          &cursorEnd,
                          &row,
                          &column);
    printf("cursorStart is %d\n", cursorStart);
    printf("cursorEnd is %d\n", cursorEnd);
    printf("row is %d\n", row);
    printf("column is %d\n", column);
    return (0);
}

static int testBosSetCursorPosition(void)
{
    BosSetCursorPosition(0, 5, 5);
    return (0);
}

static int testBosSetCursorType(void)
{
    BosSetCursorType(0x0b, 0x0c);
    return (0);
}

static int testBosSetVideoMode(void)
{
    BosSetVideoMode(0x00);
    return (0);
}

static int testBosPrintScreen(void)
{
    BosPrintScreen();
    return (0);
}

/* Note that the drive is 0 = floppy A, 0x80 = hard disk C */

static int testDisk(void)
{
    unsigned char buf[512];
    int rc;
    int sectors = 1;
    int drive = 0x80;
    int track = 0;
    int head = 0;
    int sector = 1;
    FILE *fq;
    int x;
    unsigned long status;

    rc = BosDiskReset(drive);
    printf("rc is %d\n", rc);       
    rc = BosDiskStatus(drive, &status);
    printf("rc is %d\n", rc);
    printf("status is %lx\n", status);
    rc = BosDiskSectorRead(buf, sectors, drive, track, head, sector);
    printf("rc is %d\n", rc);
    if (rc == 0)
    {
        fq = fopen("temp.txt", "wb");
        if (fq != NULL)
        {
            for (x=0; x < 512; x++)
            {
                fputc(buf[x], fq);
            }
            fclose(fq);
        }
    } 
    return (0);
}

static int testExtendedMemory(void)
{
    long memsize;
    
    memsize = BosExtendedMemorySize();
    printf("memsize is %ld\n", memsize);
    return (0);
}

static int testDriveParms(void)
{
    int rc;
    unsigned long tracks;
    unsigned long sectors;
    unsigned long heads;
    unsigned long attached;
    unsigned char *parmtable;
    unsigned long drivetype;
    int x;
    
    for (x = 0x80; x < 0x84; x++)
    {
        rc = BosDriveParms(x, 
                           &tracks,
                           &sectors,
                           &heads,
                           &attached,
                           &parmtable,
                           &drivetype);
        printf("rc is %d\n", rc);
        printf("tracks is %lu\n", tracks);
        printf("sectors is %lu\n", sectors);
        printf("heads is %lu\n", heads);
        printf("attached is %lu\n", attached);
        printf("drivetype is %lu\n", drivetype);
    }
    return (0);
}

/*Function For testing function call 440D*/
static int testGenericBlockDeviceRequest(void)
{
    int ret;
    int x;
    int y;
    PB_1560 parm_block;
    char *p=&parm_block;

    memset(&parm_block,'\x00',sizeof parm_block);
    memset(parm_block.bpb.FatSize16,'\xff',sizeof parm_block.bpb.FatSize16);
    parm_block.special_function=4;
    /*Hexdump of parm_block for freedos*/
    for (x = 0; x < 5; x++) 
    {
        for (y = 0; y < 16; y++)
        {
           printf("%02X", *((unsigned char *)p + 16 * x + y));
        }
           printf("\n");
    }
    /**/
    ret=PosGenericBlockDeviceRequest(0x04,0x08,0x60,&parm_block);
    /*Hexdump of parm_block for freedos*/
    for (x = 0; x < 5; x++) 
    {
        for (y = 0; y < 16; y++)
        {
           printf("%02X", *((unsigned char *)p + 16 * x + y));
        }
           printf("\n");
    }
    /**/
    printf("return is\n",ret);
    return(0);
}

/**/
int main(void)
{
/*    testDriveParms();
    testDisk(); */

    /*testExtendedMemory()*/
    testGenericBlockDeviceRequest();
    return (0);
}
