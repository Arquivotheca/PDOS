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
#include <string.h>

static char buf[6144]; /* arbitrary buffer size */

int main(int argc, char **argv)
{
    FILE *fp;
    FILE *fq;
    char *in_name;
    char *out_name;
    int c;
    int off = 0;
    char *in = "r";
    char *out = "w";
    unsigned long total = 0;
    char *m1;
    char *m2;
    char *z;
    int i;

    printf("welcome to pdptest\n");
    printf("main function is at %p\n", main);
#if defined(__MVS__) || defined(__CMS__) || defined(__VSE__)
    z = (char *)main;
    printf("first byte of main is %x\n", *z);
    printf("running as amode %d\n", __getam());
#endif

#if 1
    printf("allocating 10 bytes\n");
    m1 = malloc(10);
#else
    printf("allocating 3 GiB\n");
    m1 = malloc(3U*1024*1024*1024);
#endif
    printf("m1 is %p\n", m1);
    if (m1 == NULL) return(EXIT_FAILURE);
    strcpy(m1, "ABCDE");
    printf("allocating 20 bytes\n");
    m2 = malloc(20);
    printf("m2 is %p\n", m2);
    if (m2 == NULL) return (EXIT_FAILURE);
    strcpy(m2, "1234");
    printf("stack is around %p\n", &c);
/*    printf("calling pdptst31\n");
    system("pdptst31");
    printf("done with pdptst31\n"); */
    /* printf("adding 7+5 gives %d\n", __addnum(7,5)); */
    /* __gosup(); */
    /* printf("total memory is %d\n", __getmsz()); */
    /* __goprob(); */
    if ((argc > 1) && (argv[1][0] != '-'))
    {
        printf("printing arguments\n");
        printf("argc = %d\n" , argc);
        for (i = 0; i < argc; i++)
        {
            printf("arg %d is <%s>\n", i, argv[i]);
        }
        return (0);
    }
    if (argc < 3)
    {
        printf("usage: pdptest [-bb/-tt/-tb/-bt] <infile> <outfile>\n");
        printf("default is text to text copy\n");        
        /*printf("not looping now\n");*/
        /*printf("looping now\n");*/
        /*for (;;) ;*/
        return (EXIT_FAILURE);
    }

    if (argc > 3)
    {
        if (argv[1][0] == '-')
        {
            if ((argv[1][1] == 'b') || (argv[1][1] == 'B'))
            {
                in = "rb";
            }
            if ((argv[1][2] == 'b') || (argv[1][2] == 'B'))
            {
                out = "wb";
            }
            off++;
        }
    }
    in_name = *(argv + off + 1);
    if (strcmp(in_name, "-") == 0)
    {
        fp = stdin;
    }
    else
    {
        fp = fopen(in_name, in);
    }
    if (fp == NULL)
    {
        printf("failed to open %s for reading\n", in_name);
        return (EXIT_FAILURE);
    }

    out_name = *(argv + off + 2);
    if (strcmp(out_name, "-") == 0)
    {
        fq = stdout;
    }
    else
    {
        fq = fopen(out_name, out);
    }
    if (fq == NULL)
    {
        printf("failed to open %s for writing\n", out_name);
        return (EXIT_FAILURE);
    }

    printf("copying from file %s, mode %s\n",
           in_name,
           (strlen(in) == 1) ? "text" : "binary");

    printf("to file %s, mode %s\n",
           out_name,
           (strlen(out) == 1) ? "text" : "binary");

    while ((c = fread(buf, 1, sizeof buf, fp)) > 0)
    {
        total += c;
        fwrite(buf, 1, c, fq);
        if (ferror(fq)) break;
    }

    if (ferror(fp) || ferror(fq))
    {
        printf("i/o error\n");
        return (EXIT_FAILURE);
    }
    printf("%lu bytes copied\n", total);
    
    fclose(fq); /* keep last in case it is stdout */

    return (0);
}
