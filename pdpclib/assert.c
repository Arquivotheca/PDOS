/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards, 3:711/934@fidonet.         */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  assert.c - implementation of stuff in assert.h                   */
/*                                                                   */
/*********************************************************************/

#include "assert.h"
#include "stdio.h"
#include "stdlib.h"

int __assert(char *x, char *y, char *z)
{
    fprintf(stderr, "assertion failed for statement %s in"
            "file %s on line %d\n", x, y, z);
    abort();            
    return (0);
}
