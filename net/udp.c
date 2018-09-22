/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  udp.c - UDP handling                                             */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "udp.h"
#include "net.h"
#include "ip.h"

/* More information about IPv4 in RFC 791. */

unsigned int udpCalculateChecksum(unsigned char *packet,
                                  unsigned int len,
                                  unsigned char *source_address,
                                  unsigned char *destination_address,
                                  unsigned int ih_protocol,
                                  unsigned int ih_udp_len);

int udpReceive(unsigned char *packet, unsigned int len)
{
    /* ih means IP header. */
    unsigned int ih_header_len;
    unsigned int ih_protocol;
    unsigned char *source_address;
    unsigned char *destination_address;
    unsigned int ih_udp_len; /* UDP length calculated from IP header. */

    unsigned int source_port;
    unsigned int destination_port;
    unsigned int udp_len;
    unsigned int checksum;
    unsigned int calculated_checksum;

    /* IP layer has verified the packet,
     * so we only extract some information
     * out of it. */
    ih_header_len = (packet[0] & 0xf) * 4;
    ih_protocol = packet[9];
    source_address = packet + 12;
    destination_address = packet + 16;
    ih_udp_len = len - ih_header_len;
    packet += ih_header_len; /* Moves the pointer to the UDP header start. */

    /* UDP header layout:
     * Bytes Meaning
     * 0-1   Source port
     * 2-3   Destination port
     * 4-5   UDP length (header + data)
     * 6-7   Checksum (pseudo IP header + UDP header + data) */
    source_port = ((unsigned int) packet[0] << 8) | (unsigned int) packet[1];
    destination_port = ((unsigned int) packet[2] << 8)
                       | (unsigned int) packet[3];
    udp_len = ((unsigned int) packet[4] << 8) | (unsigned int) packet[5];
    checksum = ((unsigned int) packet[6] << 8) | (unsigned int) packet[7];
    calculated_checksum = udpCalculateChecksum(packet, udp_len,
                                               source_address,
                                               destination_address,
                                               ih_protocol, ih_udp_len);
    if (!checksum) printf("(UDP) Checksum disabled on this datagram\n");
    else if (checksum != calculated_checksum)
    {
        printf("(UDP) Invalid checksum: %04x calculated: %04x\n",
               checksum, calculated_checksum);
        return (1);
    }
    printf("(UDP) Source port: %u Destination port: %u\n",
           source_port, destination_port);
    printf("(UDP) Calling port services not yet implemented\n");
    
    return (0);
}

unsigned int udpCalculateChecksum(unsigned char *packet,
                                  unsigned int len,
                                  unsigned char *source_address,
                                  unsigned char *destination_address,
                                  unsigned int ih_protocol,
                                  unsigned int ih_udp_len)
{
    /* Checksum is calculated the same way as IPv4 checksum,
     * but from pseudo IP header, UDP header and data. */
    unsigned long checksum = 0;
    unsigned int i;

    /* Pseudo IP header layout:
     * Bytes Meaning
     * 0-3   IPv4 source address
     * 4-7   IPv4 destination address
     * 8     Zeros
     * 9     Protocol from IPv4 header
     * 10-11 UDP length (IP packet length - IP header length) */ 
    for (i = 0; i < IPV4_ADDRESS_LEN; i += 2)
    {
        checksum += ((unsigned long) source_address[i] << 8)
                    | (unsigned long) source_address[i + 1];
    }
    for (i = 0; i < IPV4_ADDRESS_LEN; i += 2)
    {
        checksum += ((unsigned long) destination_address[i] << 8)
                    | (unsigned long) destination_address[i + 1];
    }
    checksum += ih_protocol;
    checksum += ih_udp_len;

    /* Sum of all 16 bit words (except checksum) in the UDP header and data. */
    for (i = 0; i < len; i += 2)
    {
        if (i == 6) continue; /* Checksum field should be counted as 0. */
        if (i == len - 1)
        {
            /* If the last WORD is not whole, it is padded with zero. */
            checksum += (unsigned long) packet[i] << 8;
            break;
        }
        checksum += ((unsigned long) packet[i] << 8)
                    | (unsigned long) packet[i + 1];
    }
    /* Top 16 bits are added to the bottom 16 bits,
     * one's complement is then created by inverting the bits
     * and then it is shortened to unsigned int. */
    checksum = (~(checksum + (checksum >> 16))) & 0xffff;
    
    return ((unsigned int) checksum);
}
