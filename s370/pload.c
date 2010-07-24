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

int main(int argc, char **argv)
{
    int dev;

    printf("PDOS should reside on cylinder 1, head 0\n");
    printf("and one day we'll even load it. :-)\n");
    dev = initsys();
    printf("IPL device is %x\n", dev);
    return (0);
}
