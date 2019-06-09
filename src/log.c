/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  log.c - logging                                                  */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

int logUnimplementedFlag = 0;

void logUnimplementedCall(int intNum, int ah, int al)
{
    if (logUnimplementedFlag)
    {
        printf("UNIMPLEMENTED: INT %02X,AH=%02X,AL=%02X\n", intNum, ah, al);
        return;
    }
}
