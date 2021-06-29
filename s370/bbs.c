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
#include <ctype.h>

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
        printf(CHAR_ESC_STR "[2J");
        if (ch == '4')
        {
            FILE *fp;

            fp = fopen("antitwit.txt", "r");
            if (fp != NULL)
            {
                int x;

                while ((x = fgetc(fp)) != EOF)
                {
                    fputc(x, stdout);
                }
                fclose(fp);
            }
        }
        else
        {
            printf("You entered an invalid option! (hex %02X)\n", ch);
            if (isdigit((unsigned char)ch))
            {
                printf("but at least it was a digit!\n");
            }
            else
            {
                printf("it wasn't even a digit!!!\n");
            }
        }
        printf("\n");
        printf("Welcome to the Ten Minute Limit BBS\n");
        printf("Back in action 24 years after a fascist kicked me "
               "off Fidonet\n");
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
        printf("4. Non-twitter feed\n");
        ch = fgetc(stdin);
        if (ch == EOF)
        {
            clearerr(stdin);
        }
    }
    return (0);
}
