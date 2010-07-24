/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pload.c - Load PDOS/370                                          */
/*                                                                   */
/*  When this (main) program is entered, interrupts are disabled,    */
/*  and this program needs to find the PDOS program and load it into */
/*  memory, and transfer control, again with interrupts disabled.    */
/*                                                                   */
/*  The way it does this is that when doing an I/O, it only          */
/*  enables the I/O then and there, and waits on it to complete,     */
/*  at which point it is disabled again.                             */
/*                                                                   */
/*  The operating system itself won't be able to get away with this  */
/*  simplistic logic because it needs to deal with timer interrupts  */
/*  etc.                                                             */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static char buf[40000];

int main(int argc, char **argv)
{
    int dev;

    printf("PDOS should reside on cylinder 1, head 0 of IPL device\n");
    dev = initsys();
    printf("IPL device is %x\n", dev);
    printf("about to read first block of PDOS\n");
    rdblock(dev, 1, 0, 1, buf, sizeof buf);
    printf("got %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3]);
    return (0);
}
