/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  bios - generic BIOS that exports the C library                   */
/*                                                                   */
/*********************************************************************/

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "exeload.h"

static int (*genstart)(void (*cbfunc)(void *cbdata, int funccode, void *retptr, char *str), void *cbdata);

void cbfunc(void *cbdata, int funccode, void *retptr, char *str);

int main(int argc, char **argv)
{
    char *p;
    char *entry_point;
    int rc;

    printf("bios starting\n");
    printf("argv[0] is %s\n", argv[0]);
    printf("argv[1] is %s\n", argv[1]);
    p = calloc(1, 1000000);
    if (p == NULL)
    {
        printf("insufficient memory\n");
        return (EXIT_FAILURE);
    }
    if (exeloadDoload(&entry_point, argv[1], &p) != 0)
    {
        printf("failed to load executable\n");
        return (EXIT_FAILURE);
    }
    genstart = (void *)entry_point;
    printf("first byte of code is %02X\n", *(unsigned char *)entry_point);
#if 1
    rc = genstart(cbfunc, NULL);
#else
    rc = 0;
#endif
    printf("return from called program is %d\n", rc);
    printf("bios exiting\n");
    return (0);
}
    
void cbfunc(void *cbdata, int funccode, void *retptr, char *str)
{
    printf("in callback\n");
    printf(str);
    return;
}
