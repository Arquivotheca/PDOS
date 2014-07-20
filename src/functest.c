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
    /*sleep(5);*/
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

/* Test function to test BIOS call Int 10/AH=03h*/
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

/* Test function to test BIOS call Int 10/AH=02h*/
static int testBosSetCursorPosition(void)
{
    BosSetCursorPosition(0, 5, 5);
    return (0);
}

/* Test function to test BIOS call Int 10/AH=01h*/
static int testBosSetCursorType(void)
{
    BosSetCursorType(0x0b, 0x0c);
    return (0);
}

/* Test function to test BIOS call Int 10/AH=00h */
static int testBosSetVideoMode(void)
{
    int retval;
    
    retval = BosSetVideoMode(0x00);
    printf("Return Value is %d",retval);
    
    return (0);
}

/* Test function to test BIOS call Int 05 */
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
    printf("return is %d\n",ret);
    return(0);
}
/**/

/*Function for testing int 25 call*/
static int testAbsoluteDiskRead(void)
{
    unsigned int ret;
    int x;
    int y;
    char buf[512];
    char *p=&buf;

    ret=PosAbsoluteDiskRead(0x04,0x01,0x01,buf);
    /*Hexdump of dp for freedos*/
    for (x = 0; x < 5; x++)
    {
        for (y = 0; y < 16; y++)
        {
            printf("%02x", *((unsigned char *)p + 16 * x + y));
        }
        printf("\n");
    }
    /**/
    printf("return is %d\n",ret);
    return(0);
}
/**/

/* Test function to test BIOS Call Int 1A/AH=00h */
static int testBosGetSystemTime(void)
{
    unsigned long ticks;
    unsigned int midnight;
    unsigned long t1,t2,t3,t4,t5,t6,t7;

    BosGetSystemTime(&ticks,&midnight);
    
    t1=(ticks*1000)/182;
    t2=t1%100;
    t3=t1/100;
    t4=t3%60;
    t5=t3/60;
    t6=t5%60;
    t7=t5/60;
    printf("Number of ticks is %lu \n",ticks);
    printf("After conversion %lu \n",t1);
    printf("Time in hundredth %lu \n",t2);
    printf("Time in seconds %lu \n",t4);
    printf("Time in minutes %lu \n",t6);
    printf("Time in hours %lu \n",t7);
    printf("Midnight Flag %d \n",midnight);
    return 0;
}

/* Test function to test BIOS call Int 1a/AH=04h */
static int testBosGetSystemDate(void)
{
    int c,y,m,d;
    int ret;

    ret = BosGetSystemDate(&c,&y,&m,&d);
    printf("Century %x \n",c);
    printf("Year %x \n",y);
    printf("Month %x \n",m);
    printf("Day %x \n",d);
    printf("Return Code is %d",ret);
    
    return 0;
}

/*Testing function - to get the Date using PDOS call*/
static int testPosGetSystemDate(void)
{
    int y,m,d,dw;
    int ret=0;

    PosGetSystemDate(&y,&m,&d,&dw);
    printf("Year %d \n",y);
    printf("Month %d \n",m);
    printf("Day %d \n",d);
    printf("Day of week %d \n",dw);
    printf("Return Code is %d",ret);
    return 0;

}
/**/

/*Test function - to get the time using PDOS call*/
static int testPosGetSystemTime(void)
{
    int hr,min,sec,hund;
    int ret=0;

    PosGetSystemTime(&hr,&min,&sec,&hund);
    printf("hour %d \n",hr);
    printf("minute %d \n",min);
    printf("seconds %d \n",sec);
    printf("hundreth %d \n",hund);
    printf("Return Code is %d",ret);
}
/**/

/*Test function- Delete a File*/
static int testPosDeleteFile(void)
{
    int ret;

    ret=PosDeleteFile("3.c");
    printf("The Return code is %d \n",ret);
}
/**/

/*Test function- Rename a file*/
static int testPosRenameFile(void)
{
    int ret;

    ret=PosRenameFile("temp1.txt","temp2324.txt");
    printf("The Return code is %d \n",ret);
}
/**/

/*Test function - To get the attributes of a given file*/
static int testPosGetFileAttributes(void)
{
    int ret;
    int attr;

    ret=PosGetFileAttributes("refer.txt",&attr);
    printf("The attribute of the file is x %d x \n",attr);
    printf("The Return code is %d \n",ret);
}
/**/

/*Test function - To get the free space of a given hard disk*/
static int testPosGetFreeSpace(void)
{
    unsigned int secperclu;
    unsigned int numfreeclu;
    unsigned int bytpersec;
    unsigned int totclus;

    PosGetFreeSpace(4,&secperclu,&numfreeclu,&bytpersec,&totclus);
    printf("Sectors per cluster x %d x \n",secperclu);
    printf("Number of free clusters x %d x \n",numfreeclu);
    printf("Bytes per sector x %d x \n",bytpersec);
    printf("Total Clusters x %d x \n",totclus);
}

/*Test function for fetching last written date and time*/
static int testPosGetFileLastWrittenDateAndTime(void)
{
    int handle;
    unsigned int fdate;
    unsigned int ftime;
    int ret;
    
    PosOpenFile("pdos.c",0,&handle);
    ret=PosGetFileLastWrittenDateAndTime(handle,&fdate,&ftime);
    PosCloseFile(handle);
    printf("The date is %d \n",fdate);
    printf("The time is %d \n",ftime);
    printf("The return code is %d \n",ret);
}
/**/

/*Test to Set the attributes of a file*/
static int testPosSetFileAttributes(void)
{
    int ret;
    
    ret=PosSetFileAttributes("",0x20);
    printf("The return code is x %d x \n",ret);
}
/**/

/*Test for empty file creation*/
static int testPosCreatFile(void)
{
    int ret;
    int handle;
    
    ret=PosCreatFile("temp1.txt",0x20,&handle);
    printf("The handle of the file is x %d x \n",handle);
    printf("The return code is x %d x \n",ret);
}
/**/

int main(void)
{
    /* Successful Tests*/
    /*testBosGetSystemTime();*/    
    /*testBosGetSystemDate();*/
    /*testBosPrintScreen();*/
    /*testBosSetVideoMode();*/    
    /*testBosSetCursorType();*/    
    /*testBosSetCursorPosition();*/
    
    testBosReadCursorPosition();
    
    /*testDriveParms();*/
    /*testDisk();*/
    /*testExtendedMemory();*/
    /*testGenericBlockDeviceRequest();*/
    /*testAbsoluteDiskRead();*/
    /*testPosGetSystemDate();*/
    /*testPosGetSystemTime();*/    
    /*testPosRenameFile();*/
    /*testPosDeleteFile();*/
    /*testPosGetFileAttributes();*/
    /*testPosGetFreeSpace();*/
    /*testPosGetFileLastWrittenDateAndTime();*/ 
    /*testPosSetFileAttributes();*/
    /*testPosCreatFile();*/
    return (0);
}
