/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  helper.c - convenience functions                                 */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void upper_str(char *str)
{
    int x;

    for (x = 0; str[x] != '\0'; x++)
    {
        str[x] = toupper((unsigned char)str[x]);
    }
    return;
}

/* Function Converting the Date received in BCD format to int format */
int bcd2int(unsigned int bcd)
{
    return (bcd & 0x0f) + 10 * ((bcd >> 4) & 0x0f);
}
/**/
