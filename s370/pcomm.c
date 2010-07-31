/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pcomm - command processor for pdos                               */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

int main(int argc, char **argv)
{
    printf("welcome to pcomm\n");
#if 1
    /* need to get the DCB open exit working first */
    printf("lrecl %d, recfm %x, blksize %d\n", stdout->lrecl,
           stdout->recfm, stdout->blksize);
#endif
    printf("what a relief!\n");
    return (5);
}
