/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  ip.c - IP Layer                                                  */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ip.h"
#include "net.h"
#include "eth.h"

/* More information about IPv4 in RFC 791. */

unsigned int ipCalculateChecksum(unsigned char *packet, unsigned int len);

int ipReceive(unsigned char *packet, unsigned int len)
{
    unsigned int header_len;
    unsigned int checksum;
    unsigned int calculated_checksum;
    unsigned int version;
    unsigned int total_length;
    unsigned int identification;
    unsigned int flags;
    unsigned int fragment_offset;
    unsigned int time_to_live;
    unsigned int protocol;
    unsigned char *source_address;
    unsigned char *destination_address;

    version = (packet[0] >> 4);
    if (version != IPV4VERSION)
    {
        printf("(IP) Unknown IP version: %u\n", version);
        return (1);
    }
    /* Second 4 bits are length of the header in DWORDs. */
    header_len = (packet[0] & 0xf) * 4;
    /* Length of the header must be at least 20 bytes (5 DWORDs). */
    if (header_len < 20 || header_len > len)
    {
        printf("(IP) Invalid length of header: %u\n", header_len);
        return (1);
    }

    /* IPv4 header layout:
     * Bytes Meaning
     * 0     4 bit IP Version and 4 bit Header length
     * 1     Type of service
     * 2-3   Total IP packet length (header + data)
     * 4-5   Identification
     * 6-7   3 bit Flag and 13 bit Fragment offset
     * 8     Time to live (TTL) in hops
     * 9     Protocol (next protocol, for example, UDP or TCP)
     * 10-11 Header checksum
     * 12-15 Source (IPv4) address
     * 16-19 Destination (IPv4) address
     * 20-22 Options
     * 23    Padding (only zeros) */
    checksum = ((unsigned int) packet[10] << 8) | (unsigned int) packet[11];
    calculated_checksum = ipCalculateChecksum(packet, header_len);
    if (checksum != calculated_checksum)
    {
        printf("(IP) Invalid checksum: %04x calculated: %04x\n",
               checksum, calculated_checksum);
        return (1);
    }
    total_length = ((unsigned int) packet[2] << 8) | (unsigned int) packet[3];
    if (total_length > len)
    {
        printf("(IP) Invalid total length: %u\n", total_length);
        return (1);
    }
    identification = ((unsigned int) packet[4] << 8)
                     | (unsigned int) packet[5];
    flags = packet[6] >> 5;
    fragment_offset = ((unsigned int) (packet[6] & 0xef) << 8)
                      | (unsigned int) packet[7];
    time_to_live = packet[8];
    protocol = packet[9];
    source_address = packet + 12;
    destination_address = packet + 16;

    if (fragment_offset)
    {
        printf("(IP) Fragment support not yet implemented\n");
        return (1);
    }

    /* Calls next layer. */
    if (protocol == IP_ICMP_PROTOCOL)
    {
        printf("ICMP not yet implemented\n");
    }
    else if (protocol == IP_TCP_PROTOCOL)
    {
        printf("TCP not yet implemented\n");
    }
    else if (protocol == IP_UDP_PROTOCOL)
    {
        /* Whole IP packet is passed to UDP. */
        udpReceive(packet, total_length);
    }
    else printf("(IP) Unknown protocol: %u\n", protocol);
    
    return (0);
}

unsigned int ipCalculateChecksum(unsigned char *packet, unsigned int len)
{
    unsigned long checksum = 0;
    unsigned int i;

    /* Sum of all 16 bit words (except checksum) in the header. */
    for (i = 0; i < len; i += 2)
    {
        if (i == 10) continue; /* Checksum field should be counted as 0. */
        checksum += ((unsigned long) packet[i] << 8)
                    | (unsigned long) packet[i + 1];
    }
    /* Top 16 bits are added to the bottom 16 bits,
     * one's complement is then created by inverting the bits
     * and then it is shortened to unsigned int. */
    checksum = (~(checksum + (checksum >> 16))) & 0xffff;
    
    return ((unsigned int) checksum);
}

    
