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

#if defined(__MVS__) || defined(__CMS__)
int __saver(jmp_buf env);
int __loadr(jmp_buf env);
#elif defined(__WIN32__)
int __longj(void *);
#endif

#if !defined(__WIN32__)
int setjmp(jmp_buf env)
{
#if defined(__MVS__) || defined(__CMS__)
    __saver(env);
#endif
    return (0);
}
#endif

void longjmp(jmp_buf env, int val)
{
    if (val == 0)
    {
        val = 1;
    }
    env[0].retval = val;
    /* load regs */
#if defined(__MVS__) || defined(__CMS__)
    __loadr(env);
#elif defined(__WIN32__)
    __longj(env);
#endif
    return;
}
