/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  arp.c - ARP handling                                             */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arp.h"
#include "net.h"
#include "eth.h"

/* More information about ARP in RFC 826
 * and about IPv4 Address Conflict Detection in RFC 5227 (ARP probes...). */

int arpReceive(unsigned char *packet, unsigned int len)
{
    unsigned int hardware_type;
    unsigned int protocol_type;
    unsigned int hardware_address_len;
    unsigned int protocol_address_len;
    unsigned int operation_code;
    unsigned char mac_address[MAC_ADDRESS_LEN];
    unsigned char ip_address[IPV4_ADDRESS_LEN];
    unsigned char target_ip_address[IPV4_ADDRESS_LEN];
    int merge_flag = 0;

    /* Layout of ARP packet (?-? means not fixed, but their order is fixed):
     * Bytes Meaning
     * 0-1   Hardware type
     * 2-3   Protocol type
     * 4     Hardware address length
     * 5     Protocol address length
     * 6-7   Operation code
     * ?-?   Sender hardware address
     * ?-?   Sender protocol address
     * ?-?   Recipient hardware address
     * ?-?   Recipient protocol address */

    hardware_type = ((unsigned int) packet[0] << 8) | (unsigned int) packet[1];
    protocol_type = ((unsigned int) packet[2] << 8) | (unsigned int) packet[3];
    hardware_address_len = packet[4];
    protocol_address_len = packet[5];
    operation_code = ((unsigned int) packet[6] << 8) | (unsigned int) packet[7];
    packet += 8;

    /* The order of checks and actions is important. */
    if (hardware_type == ETHTYPE || hardware_address_len == MAC_ADDRESS_LEN)
    {
        if (protocol_type == IPV4TYPE
            || protocol_address_len == IPV4_ADDRESS_LEN)
        {
            memcpy(mac_address, packet, MAC_ADDRESS_LEN);
            memcpy(ip_address, packet + MAC_ADDRESS_LEN, IPV4_ADDRESS_LEN);
            packet += MAC_ADDRESS_LEN + IPV4_ADDRESS_LEN;
            /* Updates translation table if we already have the IP address. */
            merge_flag = !transUpdate(ip_address, mac_address);
            
            /* Checks if we are the target of the packet. */
            if (!(memcmp(packet + MAC_ADDRESS_LEN,
                         getNetinfo()->ip_address,
                         IPV4_ADDRESS_LEN)))
            {
                /* If we did not have this address pair in translation table,
                 * it is added now. */
                if (!merge_flag) transAdd(ip_address, mac_address);
                if (operation_code == ARPREQUEST)
                {
                    printf("(ARP) Replying to ARP not yet implemented.\n");
                    /* Reply. */
                }
            }
            return (0);
        }
    }
    printf("(ARP) Unknown ARP packet received\n");
    return (1);   
}

int arpSendProbe(const unsigned char *desired_ip_address)
{
    /* Sends ARP probe to check if provided IPv4 address in in use. */
    unsigned char *packet;
    unsigned int len = ARPDEFAULTLEN;
    unsigned int offset = 0;
    unsigned char eth_destination_ip_address[IPV4_ADDRESS_LEN];

    /* It is a broadcast so ethernet destination IP is 255.255.255.255. */
    memset(eth_destination_ip_address, 0xff, IPV4_ADDRESS_LEN);
    packet = ethCreateFrame(&len,
                            &offset,
                            eth_destination_ip_address,
                            ARPTYPE);
    /* ARP probe:
     * First 8 bytes are same as standard MAC-IPV4 ARP request
     * (opcode ARPREQUEST). */
    packet[offset] = ETHTYPE >> 8;
    packet[offset + 1] = ETHTYPE & 0xff;
    packet[offset + 2] = IPV4TYPE >> 8;
    packet[offset + 3] = IPV4TYPE & 0xff;
    packet[offset + 4] = MAC_ADDRESS_LEN;
    packet[offset + 5] = IPV4_ADDRESS_LEN;
    packet[offset + 6] = ARPREQUEST >> 8;
    packet[offset + 7] = ARPREQUEST & 0xff;
    offset += 8;

    /* Source physical address is ours. */
    memcpy(packet + offset, getNetinfo()->mac_address, MAC_ADDRESS_LEN);
    offset += MAC_ADDRESS_LEN;
    /* Source protocol address and Destination physical address
     * are set to all 0. */
    memset(packet + offset, '\0', IPV4_ADDRESS_LEN + MAC_ADDRESS_LEN);
    offset += IPV4_ADDRESS_LEN + MAC_ADDRESS_LEN;
    /* Destination protocol address is the desired protocol address. */
    memcpy(packet + offset, desired_ip_address, IPV4_ADDRESS_LEN);
    offset += IPV4_ADDRESS_LEN;
    /* Fills the rest of the packet with zeros. */
    memset(packet + offset, '\0', len - offset);

    ethSend(packet, len);

    free(packet);

    return (0);
}

/* Prototypes of some translation table functions. */
void transInit();
void transTerminate();

typedef struct TRANSENT {
    unsigned char ip_address[IPV4_ADDRESS_LEN];
    unsigned char mac_address[MAC_ADDRESS_LEN];
    struct TRANSENT *next;
} TRANSENT; /* Address translation table entry. */

/* Translation table functions. */
TRANSENT *starttransent;

void transInit()
{
    /* Creates first translation entry in the table.
     * It is broadcast translation
     * (IP 255.255.255.255 to MAC ff:ff:ff:ff:ff:ff). */
    starttransent = malloc(sizeof(TRANSENT));
    memset(starttransent, 0xff, IPV4_ADDRESS_LEN + MAC_ADDRESS_LEN);
    starttransent->next = 0;
    /*+++This is a vulnerability. One bad ARP and broadcast will be down. */
    return;
}

void transTerminate()
{
    TRANSENT *transent = starttransent;
    TRANSENT *newtransent;
    while (transent)
    {
        newtransent = transent->next;
        free(transent);
        transent = newtransent;
    }
}

int transGetMac(const unsigned char *ip_address, unsigned char *mac_address)
{
    TRANSENT *transent = starttransent;
    
    while (transent)
    {
        if (memcmp(transent->ip_address, ip_address, IPV4_ADDRESS_LEN) == 0)
        {
            memcpy(mac_address, transent->mac_address, MAC_ADDRESS_LEN);
            return (0);
        }
        transent = transent->next;
    }
    return (1);
}

int transUpdate(const unsigned char *ip_address,
                const unsigned char *mac_address)
{
    TRANSENT *transent = starttransent;
    while (transent)
    {
        if (memcmp(transent->ip_address, ip_address, IPV4_ADDRESS_LEN) == 0)
        {
            /* If we already have the IP address in translation table,
             * MAC address is updated. */
            memcpy(transent->mac_address, mac_address, MAC_ADDRESS_LEN);
            return (0);
        }
        transent = transent->next;
    }
    return (1);
}

void transAdd(const unsigned char *ip_address,
              const unsigned char *mac_address)
{
    TRANSENT *transent = starttransent;
    TRANSENT *newtransent;
    while (1)
    {
        if (transent->next == 0)
        {
            newtransent = malloc(sizeof(TRANSENT));
            memcpy(newtransent->ip_address, ip_address, IPV4_ADDRESS_LEN);
            memcpy(newtransent->mac_address, mac_address, MAC_ADDRESS_LEN);
            newtransent->next = 0;
            transent->next = newtransent;
            return;
        }
        transent = transent->next;
    }
}

void transPrint()
{
    TRANSENT *transent = starttransent;
    int i;

    printf("\n-------------\n");
    while (transent)
    {
        for (i = 0; i < IPV4_ADDRESS_LEN; i++)
        {
            if (i < IPV4_ADDRESS_LEN - 1)
                printf("%d.", (transent->ip_address)[i]);
            else
                printf("%d\n", (transent->ip_address)[i]);
        }
        for (i = 0; i < MAC_ADDRESS_LEN; i++)
        {
            if (i < MAC_ADDRESS_LEN - 1)
                printf("%02x-", (transent->mac_address)[i]);
            else
                printf("%02x\n", (transent->mac_address)[i]);
        }
        printf("-------------\n");
        transent = transent->next;
    }
}

void arpInit()
{
    transInit();
}

void arpTerminate()
{
    transTerminate();
}
