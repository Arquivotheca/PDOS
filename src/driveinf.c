/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards                             */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  driveinf - show drive information                                */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>

#include "bos.h"

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
        rc = BosFixedDiskStatus(x);
        printf("rc is %d\n", rc);
        printf("\n");
    }
    return (0);
}

/* Note that the drive is 0 = floppy A, 0x80 = hard disk C */

static int testDisk(void)
{
    static unsigned char buf[512];
    static unsigned char buf2[512];
    int rc;
    int sectors = 1;
    int drive = 0x80;
    int track = 0;
    int head = 0;
    int sector = 1;
    int x;
    unsigned long status;        
    int y;
    int fin;
    int cnt;

    rc = BosDiskStatus(drive, &status);
    printf("rc is %d\n", rc);
    printf("status is %lx\n", status);
    rc = BosDiskSectorRead(buf, sectors, drive, track, head, sector);
    printf("rc is %d\n", rc);
    if (rc == 0)
    {
        printf("Partition info:\n");
        for (x = 0; x < 4; x++)
        {
            printf("partition %d\n", x);
            printf("boot indicator %x\n", buf[0x1be + x * 16]);
            head = buf[0x1be + x * 16 + 1];
            printf("starting head %d\n", head);
            sector = buf[0x1be + x * 16 + 2] & 0x1f;
            printf("starting sector %d\n", sector);                       
            track = (((unsigned int)buf[0x1be + x * 16 + 2] & 0xc0) << 2)
                    | buf[0x1be + x * 16 + 3];
            printf("starting cylinder %d\n", track);                       
            printf("system id %x\n", buf[0x1be + x * 16 + 4]);
            if (buf[0x1be + x * 16 + 4] == 0x05)
            {
                fin = 0;
                cnt = 0;
                while (!fin)
                {
                    BosDiskSectorRead(buf2, 
                                      sectors, 
                                      drive, 
                                      track,
                                      head,
                                      sector);
                    printf("name is %.5s\n", buf2 + 0x18b);
                    head = buf2[0x1be + 1 * 16 + 1];
                    sector = buf2[0x1be + 1 * 16 + 2] & 0x1f;
                    track = (((unsigned int)buf2[0x1be + 1 * 16 + 2] 
                             & 0xc0) << 2)
                             | buf2[0x1be + 1 * 16 + 3];
                    printf("new head, sector, track is %d %d %d\n",
                           head, sector, track);                            
                    cnt++;
                    if (cnt == 8)
                    {
                        fin = 1;
                    }
                    if (sector == 0)
                    {
                        fin = 1;
                    }
                }
                printf("\n");
            }
        }
    } 
    return (0);
}

int main(void)
{
    testDriveParms();
    testDisk();
    return (0);
}



