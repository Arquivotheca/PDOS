/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards, 3:711/934@fidonet.         */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  setjmp.c - implementation of stuff in setjmp.h                   */
/*                                                                   */
/*********************************************************************/

#include "setjmp.h"

int setjmp(jmp_buf env)
{
    env[0].eax = 0;
    env[0].ebx = 0;
    env[0].ecx = 0;
    env[0].longj = 0;
    /* env[0].sp = sp */
    if (env[0].longj == 1)
    {
        if (env[0].eax == 0)
        {
            env[0].eax = 1;
        }
        return (env[0].eax);
    }
    else
    {
        return (0);
    }
}
        
void longjmp(jmp_buf env, int val)
{
    env[0].longj = 1;
    env[0].eax = val;
    /* load regs */
    return;
}
