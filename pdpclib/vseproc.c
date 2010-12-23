/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  vseproc - generate a compilation proc for VSE                    */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char buf[10000];

int main(int argc, char **argv)
{
    size_t len;
    FILE *fp;
    FILE *fq;
    char *jcl_start;
    char *jcl_end;
    char *fn_start;
    char *fn_end;
    char *p;
    char *q;

    if (argc <= 2)
    {
        printf("usage: vseproc <in file> <out file>\n");
        printf("e.g. vseproc - dd:syspunch\n");
        return (EXIT_FAILURE);
    }

    if (strcmp(*(argv + 1), "-") == 0)
    {
        fp = stdin;
    }
    else
    {
        fp = fopen(*(argv + 1), "r");
        if (fp == NULL)
        {
            printf("can't open %s\n", *(argv + 1));
            return (EXIT_FAILURE);
        }
    }
    
    if (strcmp(*(argv + 2), "-") == 0)
    {
        fq = stdout;
    }
    else
    {
        fq = fopen(*(argv + 2), "w");
        if (fq == NULL)
        {
            printf("can't open %s\n", *(argv + 2));
            return (EXIT_FAILURE);
        }
    }
    
    len = fread(buf, 1, sizeof buf - 1, fp);
    buf[len] = '\0';
    p = strchr(buf, '\n');
    if (p != NULL)
    {
        fn_start = p + 1;
        p = strstr(fn_start, "\n-");
    }
    if (p != NULL)
    {
        fn_end = p + 1;
        p = strchr(fn_end, '\n');
    }
    if (p != NULL)
    {
        jcl_start = p + 1;
        p = strstr(jcl_start, "\n-----");
    }
    if (p != NULL)
    {
        jcl_end = p + 1;
        *jcl_end = '\0';
    }
    if (p != NULL)
    {
        fprintf(fq, " CATALP VSEPROC\n");
        p = fn_start;
        while (p != fn_end)
        {
            q = strchr(p, '\n');
            *q = '\0';
            fprintf(fq, jcl_start, p, p, p, p, p, p, p, p, p, p,
                    p, p, p, p, p, p, p, p, p, p, p, p, p, p, p);
            p = q + 1;
        }
        fprintf(fq, "/+\n");
    }
    fclose(fq);
    return (0);
}
