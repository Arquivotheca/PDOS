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
#include "stddef.h"

int __setj(jmp_buf env);
int __longj(void *);

__PDPCLIB_API__ int _setjmp(jmp_buf env)
{
    return (__setj(env));
}

__PDPCLIB_API__ void longjmp(jmp_buf env, int val)
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
