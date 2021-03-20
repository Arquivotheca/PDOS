/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  osworld - example operating system that calls the generic BIOS   */
/*                                                                   */
/*********************************************************************/

#include <osfunc.h>

int main(void)
{
#ifdef __MVS__
    /* osfunc(OS_PRINTF, 0, "\x27[2J"); */
#else
    /* osfunc(OS_PRINTF, 0, "\x1b[2J"); */
#endif
    osfunc(OS_PRINTF, 0, "hello, world from osworld\n");
    osfunc(OS_PRINTF, 0, "just checking!\n");
    osfunc(OS_FOPEN, 0, "::"); /* boot disk is designated by :: */
    return (0);
}
