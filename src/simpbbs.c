/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  simpbbs - a simple BBS                                           */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <bos.h>

static int port = 0;

/* Note that instead of using Bos functions we should instead be
   using pdcomm or even better PDOS should support opening a COM port */

/* print to both serial port and terminal */
int bbs_printf(const char *format, ...)
{
    va_list arg;
    int ret;
    static char buf[300];
    char *p;

    va_start(arg, format);
    ret = vsprintf(buf, format, arg);
    va_end(arg);

    p = buf;
    while (*p != '\0')
    {
        BosSerialWriteChar(port, *p);
        fputc(*p, stdout);
        p++;
    }
    return (ret);
}

/* input from terminal should come from serial port instead */
/* ideally we could accept from either, but that requires a
   change in ideology to deliberately do timeout/polling */
int bbs_fgetc(FILE *fp)
{
    int c;

    if (fp == stdin)
    {
        while (1)
        {
            c = BosSerialReadChar(port);
            if ((c & 0x8000U) == 0) break;
        }
        if ((c & 0x100U) == 0x100)
        {
            return (c & 0xff);
        }
        return (EOF);
    }
    return (fgetc(fp));
}

int bbs_fputc(int c, FILE *stream)
{
    if (stream == stdout)
    {
        BosSerialWriteChar(port, c);
    }
    fputc(c, stream);
    return (c);
}

#define printf bbs_printf
#define fgetc bbs_fgetc
#define fputc bbs_fputc


/* If ISO weren't a pack of dipshits, and supported "\e", or at least
   (and maybe preferably) some standard defines, I wouldn't need to do
   this. BTW, if your mainframe doesn't support EBCDIC ANSI terminals,
   get a better mainframe vendor. */
#ifdef EBCDIC
#define CHAR_ESC_STR "\x27"
#else
#define CHAR_ESC_STR "\x1b"
#endif

int main(int argc, char **argv)
{
    int ch = 0;

    if (argc <= 1)
    {
        printf("usage: simpbbs <port number>\n");
        printf("e.g. simpbbs 0\n");
        printf("port should already have been initialized with portinit\n");
        return (EXIT_FAILURE);
    }
    port = atoi(*(argv + 1));

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
#ifdef EBCDIC
               "glorious EBCDIC"
#else
               "inglorious ASCII"
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
