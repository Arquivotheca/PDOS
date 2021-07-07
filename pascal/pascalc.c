/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pascalc.c - runtime support for Free Pascal code                 */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fpc_initializeunits(void)
{
    return;
}

int fpc_get_output(void)
{
    /* note that "files" are normally bigger than just the pointer,
       e.g. they also include the filename, but just a file pointer
       is enough for the moment */
    static FILE *x = NULL;

    if (x == NULL)
    {
        x = stdout;
    }

    return ((int)&x);
}

void fpc_write_text_shortstr(int handle, unsigned char *str, int unknown)
{
    fwrite(str + 1, str[0], 1, *(FILE **)handle);
    return;
}

void fpc_write_text_sint(int handle, int val, int unknown)
{
    fprintf(*(FILE **)handle, "%d", val);
    return;
}

void fpc_iocheck(void)
{
    return;
}

void fpc_writeln_end(int handle)
{
    fprintf(*(FILE **)handle, "\n");
    return;
}

void fpc_do_exit(int x)
{
    exit(x);
}

void fpc_assign_short_string(char *a, char *b)
{
    a += sizeof(FILE *);
    strcpy(a, b);
    return;
}

void fpc_rewrite(void *a)
{
    char *name = (char *)a + sizeof(FILE *) + 1;
    FILE *fq;

    fq = fopen(name, "w");
    *(FILE **)a = fq;
    return;
}
