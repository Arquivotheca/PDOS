/*********************************************************************/
/*                                                                   */
/*  This Program Written By Paul Edwards.                            */
/*  Released to the public domain.                                   */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  This program reads from an input file and writes to an output    */
/*  file.                                                            */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    FILE *fp;
    FILE *fq;
    int c;
    
    if (argc < 3)
    {
        printf("usage: copyfile <infile> <outfile>\n");
        return (EXIT_FAILURE);
    }

    fp = fopen(*(argv + 1), "r");
    if (fp == NULL)
    {
        printf("failed to open %s for reading\n", *(argv + 1));
        return (EXIT_FAILURE);
    }

    fq = fopen(*(argv + 2), "w");
    if (fq == NULL)
    {
        printf("failed to open %s for reading\n", *(argv + 2));
        return (EXIT_FAILURE);
    }

    while ((c = fgetc(fp)) != EOF)
    {
        fputc(c, fq);
    }

    if (ferror(fp) || ferror(fq))
    {
        printf("i/o error\n");
        return (EXIT_FAILURE);
    }
    
    return (0);
}
