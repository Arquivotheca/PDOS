/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  setjmp.c - implementation of stuff in setjmp.h                   */
/*                                                                   */
/*********************************************************************/

#include "setjmp.h"

int __longj(void *);

void longjmp(jmp_buf env, int val)
{
    if (val == 0)
    {
        val = 1;
    }
    env[0].retval = val;
    /* load regs */
    __longj(env);
    return;
}
