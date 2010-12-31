/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  vsemanip - manipulate VSE files                                  */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char buf[1000];

int main(int argc, char **argv)
{
    FILE *fp;
    FILE *fq;
    int arg_upto = 1;
    int inlen = 81;
    int outlen = 80;

    if (argc <= 2)
    {
        printf("usage: vsemanip <in file> <out file>\n");
        printf("by default it will trim 81-character files down to\n");
        printf("80 by stripping a leading control character\n");
        printf("use -i121 to override input length\n");
        printf("e.g. vsemanip -i121 dd:sdi1 dd:sdo1\n");
        return (EXIT_FAILURE);
    }

    if (strncmp(argv[arg_upto], "-i", 2) == 0)
    {
        inlen = atoi(&argv[arg_upto][2]);
        arg_upto++;
    }

    if ((inlen <= 0) || (inlen > sizeof buf))
    {
        printf("invalid length string %s\n", argv[arg_upto - 1]);
        return (EXIT_FAILURE);
    }

    fp = fopen(*(argv + arg_upto), "rb");
    if (fp == NULL)
    {
        printf("can't open %s\n", *(argv + arg_upto));
        return (EXIT_FAILURE);
    }
    arg_upto++;
    
    fq = fopen(*(argv + arg_upto), "wb");
    if (fq == NULL)
    {
        printf("can't open %s\n", *(argv + arg_upto));
        return (EXIT_FAILURE);
    }
    arg_upto++;
    
    while (fread(buf, inlen, 1, fp) == 1)
    {
        fwrite(buf + 1, outlen, 1, fq);
    }

    fclose(fq);
    fclose(fp);
    return (0);
}
