/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  limits.h - limits header file.                                   */
/*                                                                   */
/*********************************************************************/

#ifndef __LIMITS_INCLUDED
#define __LIMITS_INCLUDED

#define CHAR_BIT 8
#define SCHAR_MIN -128
#define SCHAR_MAX 127
#define UCHAR_MAX 255
#if ('\x80' < 0)
#define CHAR_MIN -128
#define CHAR_MAX 127
#else
#define CHAR_MIN 0
#define CHAR_MAX 255
#endif
#define MB_LEN_MAX 1
#define SHRT_MIN -32768
#define SHRT_MAX 32767
#define USHRT_MAX 65535

#if (defined(__OS2__) || defined(__32BIT__) || defined(__MVS__) \
    || defined(__CMS__) || defined(_WIN32))
#define INT_MIN -2147483648
#define INT_MAX 2147483647
#define UINT_MAX 4294967295U
#endif

#if (defined(__MSDOS__) || defined(__DOS__) || defined(__POWERC))
#define INT_MIN -32768
#define INT_MAX 32767
#define UINT_MAX 65535
#endif

#define LONG_MIN -2147483648
#define LONG_MAX 2147483647
#define ULONG_MAX 4294967295U

#endif
