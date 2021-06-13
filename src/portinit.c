/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  portinit - initialize a port, normally a serial port             */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <bos.h>

int main(int argc, char **argv)
{
    unsigned int port;
    unsigned int parm;

    if (argc <= 2)
    {
        printf("usage: portinit <port number> <parm>\n");
        printf("initialize a port, using the parameter\n");
        printf("you probably want 0x03 or 0xE3 for the parm\n");
        printf("(both of those are N81, with different speeds)\n");
        printf("or maybe don't use this function at all and hope that\n");
        printf("the manufacturer has set some sensible default\n");
        return (EXIT_FAILURE);
    }
    port = atoi(*(argv + 1));
    parm = strtol(*(argv + 2), NULL, 0);
    BosSerialInitialize(port, parm);
    return (0);
}
