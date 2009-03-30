/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  setjmp.h - setjmp header file.                                   */
/*                                                                   */
/*********************************************************************/

#ifndef __SETJMP_INCLUDED
#define __SETJMP_INCLUDED

typedef struct {
#if defined(__MVS__) || defined(__CMS__)

    int saveptr;   /* pointer to stack savearea */
    int savelng;  /* length of save area */
    int savestk;  /* where to put it */
    int saver13; /* Where to leave it pointing to */
    int saver14; /* and return address */

#elif defined(__WIN32__)
    int ebx;
    int ecx;
    int edx;
    int edi;
    int esi;
    int esp;
    int ebp;
    int retaddr;
#endif
    int retval;
} jmp_buf[1];

#if defined(__WIN32__)
int __setj(void *);
#define setjmp(x) (__setj(x))
#else
int setjmp(jmp_buf env);
#endif
void longjmp(jmp_buf env, int val);

#endif
