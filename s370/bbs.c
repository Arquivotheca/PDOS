/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  bbs - a simple BBS                                               */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <string.h>

/* If ISO weren't a pack of dipshits, and supported "\e", or at least
   (and maybe preferably) some standard defines, I wouldn't need to do
   this. BTW, if your mainframe doesn't support EBCDIC ANSI terminals,
   get a better mainframe vendor. */
#ifdef ASCII
#define CHAR_ESC_STR "\x1b"
#else
#define CHAR_ESC_STR "\x27"
#endif

int main(int argc, char **argv)
{
    int ch = 0;

    while (1)
    {
        printf(CHAR_ESC_STR "[2J"
               "You entered an invalid option! (hex %02X)\n\n", ch);
        printf("Welcome to the Ten Minute Limit BBS\n");
        printf("Back in action after a fascist kicked me off Fidonet\n");
        printf("brought to you in "
#ifdef ASCII
               "inglorious ASCII"
#else
               "glorious EBCDIC"
#endif
               "\n");
        printf("enter an option below:\n");
        printf("1. Message area (not yet implemented)\n");
        printf("2. File area (not yet implemented)\n");
        printf("3. The highest quality porn ever produced (coming soon)\n");
        ch = fgetc(stdin);
        if (ch == EOF)
        {
            clearerr(stdin);
        }
    }
    return (0);
}
