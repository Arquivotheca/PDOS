/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards                             */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  mvsendec - mvs encode and decode                                 */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define CHUNKSZ 40

int main(int argc, char **argv)
{
    int enc = 0;
    int dec = 0;
    FILE *fp;
    FILE *fq;
    char inbuf[CHUNKSZ * 2 + 1];
    char outbuf[CHUNKSZ * 2 + 1];
    size_t count;
    size_t x;
    static char tohex[] = { '0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    static char fromhex[UCHAR_MAX];                            
    
    if (argc < 4)
    {
        printf("usage: mvsendec <enc/dec> <infile> <outfile>\n");
        return (EXIT_FAILURE);
    }
    if ((strcmp(*(argv + 1), "enc") == 0)
        || (strcmp(*(argv + 1), "ENC") == 0))
    {
        enc = 1;
    }
    else if ((strcmp(*(argv + 1), "dec") == 0)
             || (strcmp(*(argv + 1), "DEC") == 0))
    {
        dec = 1;
    }
    else
    {
        printf("need to specify encode or decode\n");
        return (EXIT_FAILURE);
    }

    fromhex['0'] = 0;
    fromhex['1'] = 1;
    fromhex['2'] = 2;
    fromhex['3'] = 3;
    fromhex['4'] = 4;
    fromhex['5'] = 5;
    fromhex['6'] = 6;
    fromhex['7'] = 7;
    fromhex['8'] = 8;
    fromhex['9'] = 9;
    fromhex['A'] = 10;
    fromhex['B'] = 11;
    fromhex['C'] = 12;
    fromhex['D'] = 13;
    fromhex['E'] = 14;
    fromhex['F'] = 15;
    
    if (enc)
    {
        fp = fopen(*(argv + 2), "rb");
        fq = fopen(*(argv + 3), "w");
        if ((fp == NULL) || (fq == NULL))
        {
            printf("file open error\n");
            return (EXIT_FAILURE);
        }
    }
    else if (dec)
    {
        /* don't be tempted to open fp in binary mode knowing
           that it will be 80-character records, as they might
           have transferred the data into a LRECL=200 dataset
           etc instead */
        fp = fopen(*(argv + 2), "r");
        fq = fopen(*(argv + 3), "wb");
        if ((fp == NULL) || (fq == NULL))
        {
            printf("file open error\n");
            return (EXIT_FAILURE);
        }
    }
    
    if (enc)
    {
        while (1)
        {
            count = fread(inbuf, 1, CHUNKSZ, fp);
            for (x = 0; x < count; x++)
            {
                outbuf[x * 2] = tohex[((unsigned char)inbuf[x] & 0xf0) >> 4];
                outbuf[x * 2 + 1] = tohex[(unsigned char)inbuf[x] & 0x0f];
            }
            if (count != 0)
            {
                outbuf[count * 2] = '\n';
                fwrite(outbuf, 1, count * 2 + 1, fq);
            }
            if (count < CHUNKSZ) break;
        }
    }
    else if (dec)
    {
        while (1)
        {        
            count = fread(inbuf, 1, CHUNKSZ * 2 + 1, fp);
            for (x = 0; (x + 1) < count; x += 2)
            {
                outbuf[x / 2] = (fromhex[inbuf[x]] << 4)
                                | fromhex[inbuf[x + 1]];
            }
            fwrite(outbuf, 1, count / 2, fq);
            if (count < (CHUNKSZ * 2 + 1)) break;
        }
    }
    
    fclose(fp);
    fclose(fq);
    return (0);
}
