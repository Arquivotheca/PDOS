/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  log.h - header for log.c                                         */
/*                                                                   */
/*********************************************************************/

#ifndef LOG_INCLUDED
#define LOG_INCLUDED

/* Log unimplemented flag.
 * If set, unimplemented interrupt calls
 * are logged when logUnimplementedCall() is used. */
extern int logUnimplementedFlag;

/* Log unimplemented call, but only if logUnimplementedFlag=1. */
void logUnimplementedCall(int intNum, int ah, int al);

#endif /* LOG_INCLUDED */
