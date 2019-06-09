/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  int21.h - header for int21.c                                     */
/*                                                                   */
/*********************************************************************/

#ifndef INT21_INCLUDED
#define INT21_INCLUDED

#ifdef __32BIT__
int int21(unsigned int *regs);
#else
void int21(unsigned int *regptrs,
           unsigned int es,
           unsigned int ds,
           unsigned int di,
           unsigned int si,
           unsigned int dx,
           unsigned int cx,
           unsigned int bx,
           unsigned int cflag,
           unsigned int ax);
#endif

#endif /* INT21_INCLUDED */
