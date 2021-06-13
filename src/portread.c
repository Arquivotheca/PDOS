/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  portread - read from a port, normally a serial port              */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <bos.h>

int main(int argc, char **argv)
{
    unsigned int port;
    unsigned int c;
    char *fnm;
    FILE *fq;
    long limit;
    long count;

    if (argc <= 3)
    {
        printf("usage: portread <port number> <filename> <number of bytes>\n");
        printf("reads data from the port and writes it to a file\n");
        return (EXIT_FAILURE);
    }
    port = atoi(*(argv + 1));
    fnm = *(argv + 2);
    limit = atol(*(argv + 3));
    fq = fopen(fnm, "wb");
    if (fq == NULL)
    {
        printf("failed to open %s\n", fnm);
        return (EXIT_FAILURE);
    }
    for (count = 0; count < limit; count++)
    {
        while (1)
        {
            c = BosSerialReadChar(port);
            if ((c & 0x8000U) == 0) break;
        }
        fputc(c, fq);
    }
    fclose(fq);
    return (0);
}
