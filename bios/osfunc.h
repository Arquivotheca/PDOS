/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  osfunc.h - IDs of supported routines of the BIOS/OS interface    */
/*                                                                   */
/*********************************************************************/

enum {
OS_PRINTF,
OS_FOPEN,
};

void osfunc(int funccode, void *retptr, char *str);
