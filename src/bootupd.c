/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  bootupd - update disk image with given boot sector               */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    FILE *boot;
    FILE *disk;
    char buf[512];
    
    if (argc < 3)
    {
        printf("usage: bootupd <disk image> <boot sector>\n");
        return (0);
    }
    disk = fopen(*(argv + 1), "r+b");
    boot = fopen(*(argv + 2), "rb");
    fread(buf, 1, 11, disk);
    fread(buf, 1, 11, boot);
    fread(buf + 11, 1, 51, boot);
    fread(buf + 11, 1, 51, disk);
    fread(buf + 62, 1, 512-62, disk);
    fread(buf + 62, 1, 512-62, boot);
    fseek(disk, 0, SEEK_SET);
    fwrite(buf, 1, 512, disk);
    fclose(disk);
    fclose(boot);
    return (0);
}
