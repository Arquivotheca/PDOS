/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  eth.c - Ethernet Layer                                           */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eth.h"
#include "net.h"
#include "nic.h"
#include "crc32.h"

int ethReceive(unsigned char *frame, unsigned int len)
{
    unsigned int protocol_type;
    unsigned int i;

    /* Ethernet header (14 bytes long):
     * Bytes  Meaning
     * 0-5    Destination MAC Address
     * 6-11   Source MAC Address
     * 12-13  Protocol type */
    protocol_type = ((unsigned int) frame[12] << 8) | (unsigned int) frame[13];

    printf("Protocol type: %04x\n", protocol_type);
    for (i = 0; i < len; i++)
    {
        printf("%02x ", (frame[i]));
        if (i > 70)
        {
            printf("\nToo long to display.");
            break;
        }
    }
    printf("\n");

    /* Strips the Ethernet header and FCS from the received frame. */
    frame += ETHHEADERLEN;
    len -= ETHHEADERLEN + ETHFCSLEN;

    if (protocol_type == ARPTYPE)
    {
        arpReceive(frame, len);
    }
    else if (protocol_type == IPV4TYPE)
    {
        ipReceive(frame, len);
    }

    return (0);
}

unsigned char * ethCreateFrame(unsigned int *len,
                               unsigned int *offset,
                               const unsigned char *destination_ip_address,
                               unsigned int protocol_type)
{
    unsigned char *frame;
    unsigned char *destination_mac_address;

    *len += ETHHEADERLEN;
    if (*len < 64) *len = 64;
    /* Allocates space for FCS too,
     * but other layers do not get notified about it,
     * so they do not use it. */
    frame = malloc(*len + ETHFCSLEN);

    if (transGetMac(destination_ip_address, destination_mac_address))
    {
        printf("(ETH) IP address not in translation table\n");
        printf("(ETH) ARP requests are not yet implemented\n");
        return (0);
    }
    memcpy(frame, destination_mac_address, MAC_ADDRESS_LEN);
    memcpy(frame + MAC_ADDRESS_LEN, getNetinfo()->mac_address,
           MAC_ADDRESS_LEN);
    frame[MAC_ADDRESS_LEN * 2] = protocol_type >> 8;
    frame[MAC_ADDRESS_LEN * 2 + 1] = protocol_type & 0xff;
        
    *offset += ETHHEADERLEN;

    return (frame);
}

unsigned long ethCRC32(unsigned char *frame, unsigned int len)
{
    unsigned long crc;
    unsigned int i;

    crc32Init(&crc);
    for (i = 0; i < len; i++)
    {
        crc32Update(&crc, frame[i]);
    }

    return ((~crc) & 0xffffffff);
}

int ethSend(unsigned char *frame, unsigned int len)
{
    unsigned long crc;
    unsigned int i;

    crc = ethCRC32(frame, len - 4);

    /* Be very careful about the byte order!
     * Network is big endian but computers are commonly little endian. */
    frame[len - 1] = (crc >> 24) & 0xff;
    frame[len - 2] = (crc >> 16) & 0xff;
    frame[len - 3] = (crc >> 8) & 0xff;
    frame[len - 4] = crc & 0xff;

    printf("Sending:\n");
    for (i = 0; i < len; i++)
    {
        printf("%02x ", (frame[i]));
        if (i > 70)
        {
            printf("\nToo long to display.");
            break;
        }
    }
    printf("\n");
    
    return (nicSend(frame, len));
}

    
