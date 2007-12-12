/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  extzip.c - extract an encoded zip from a printout                */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <string.h>

static char buf[800];

int main(void)
{
    int write = 0;
    
    while (fgets(buf, sizeof buf, stdin) != NULL)
    {
        if (memcmp(buf, "\f504B", 5) == 0)
        {
            memmove(buf, buf + 1, strlen(buf));
            write = 1;
        }
        else if (buf[0] == '\f')
        {
            write = 0;
        }
        if (write)
        {
            fputs(buf, stdout);
        }
    }
    return (0);
}
