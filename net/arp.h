/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  arp.h - header for arp.c                                         */
/*                                                                   */
/*********************************************************************/

#define ARPDEFAULTLEN 28 /* ARP packet length for MAC and IPv4. */
#define ARPREQUEST 0x0001
#define ARPREPLY   0x0002

int arpReceive(unsigned char *buf, unsigned int len);

int transGetMac(const unsigned char *ip_address, unsigned char *mac_address);
int transUpdate(const unsigned char *ip_address,
                const unsigned char *mac_address);
void transAdd(const unsigned char *ip_address,
              const unsigned char *mac_address);
void transPrint();

void arpInit();
void arpTerminate();

int arpSendProbe(const unsigned char *desired_ip_address); /* +++REMOVE */
