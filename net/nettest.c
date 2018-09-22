/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  nettest.c - test program for PCnet-PCI II                        */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net.h"
#include "nic.h"
#include "eth.h"
#include "arp.h"
#include "crc32.h"

int main(int argc, char **argv)
{
    unsigned char test_ip[IPV4_ADDRESS_LEN];

    test_ip[0] = 192;
    test_ip[1] = 168;
    test_ip[2] = 1;
    test_ip[3] = 1;
    
    netInit();
    
    nicInit();

    arpSendProbe(test_ip);
    nicReceive();
    nicReceive();
    nicReceive();
    nicReceive();
    transPrint();
    
    nicEnd();

    netTerminate();
    
    return (0);
}
