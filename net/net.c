/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  net.c - support program for all network layers                   */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "net.h"
#include "arp.h"

NETINFO netinfo;

void netGenerateMacAddress(unsigned char *mac_address)
{
    /* Generates random MAC address. */
    unsigned int i;
    unsigned int address_part;

    srand(time(NULL));

    for (i = 0; i < MAC_ADDRESS_LEN; i++)
    {
        address_part = rand() % 256;
        if (i == 1)
        {
            /* Second byte of MAC address:
             * Bit Meaning
             * 0   Multicast - must be 0 when used as address for NIC
             * 1   LG - Locally administred address */
            address_part = (address_part | 0x1) ^ 0x1;
        }
        mac_address[i] = address_part;
    }
}    

void netInit()
{
    netGenerateMacAddress(netinfo.mac_address);
    /* IP address is set ones at the beginning
     * because that value should not be used anywhere. */
    memset(netinfo.ip_address, 0xff, IPV4_ADDRESS_LEN);
    netinfo.promiscuous_mode = 0;

    arpInit();
}

void netTerminate()
{
    arpTerminate();
}

NETINFO * getNetinfo()
{
    return (&netinfo);
}
    
