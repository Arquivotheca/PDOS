/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards, 3:711/934@fidonet.         */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  assert.h - assert header file.                                   */
/*                                                                   */
/*********************************************************************/

#ifndef __ASSERT_INCLUDED
#define __ASSERT_INCLUDED

int __assert(char *x, char *y, char *z);

#ifdef NDEBUG
#define assert(ignore) ((void)0)
#else
#define assert(x) (x) ? ((void)0) : \
    __assert(#x, __FILE__, __LINE__)
#endif    

#endif
