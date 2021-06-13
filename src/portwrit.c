/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  portwrit - write to a port, normally a serial port               */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <bos.h>

int main(int argc, char **argv)
{
    unsigned int port;
    int c;
    char *fnm;
    FILE *fp;

    if (argc <= 2)
    {
        printf("usage: portwrit <port number> <filename>\n");
        printf("reads data from the file and writes it to the port\n");
        return (EXIT_FAILURE);
    }
    port = atoi(*(argv + 1));
    fnm = *(argv + 2);
    fp = fopen(fnm, "rb");
    if (fp == NULL)
    {
        printf("failed to open %s\n", fnm);
        return (EXIT_FAILURE);
    }
    while ((c = fgetc(fp)) != EOF)
    {
        BosSerialWriteChar(port, c);
    }
    fclose(fp);
    return (0);
}
