/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  world - print arguments                                          */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

int main(int argc, char **argv)
{
    int i;

    printf("welcome to my world\n");
    printf("argc = %d\n" , argc);
    for (i = 0; i < argc; i++)
    {
        printf("arg %d is <%s>\n", i, argv[i]);
    }
    printf("looping for a while\n"); /* about 10 seconds */
    for (i = 0; i < 20000000; i++)
    {
        ;
    }
    printf("finished looping\n");
    return (5);
}
