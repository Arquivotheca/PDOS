/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pdosload.c - Load PDOS/370                                       */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int i;

    printf("hello there\n");
    printf("argc = %d\n" , argc);
    for (i = 0; i < argc; i++)
    {
        printf("arg %d is <%s>\n", i, argv[i]);
    }
    return (0);
}
