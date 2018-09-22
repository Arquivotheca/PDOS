/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  net.h - header file for all network layers                       */
/*                                                                   */
/*********************************************************************/

/* Different hardware types. */
#define ETHTYPE 0x1

/* Different protocol types. */
#define IPV4TYPE 0x0800
#define ARPTYPE  0x0806

/* Lengths of different addresses. */
#define MAC_ADDRESS_LEN 6
#define IPV4_ADDRESS_LEN 4

typedef struct {
    unsigned char mac_address[MAC_ADDRESS_LEN];
    unsigned char ip_address[IPV4_ADDRESS_LEN];
    int promiscuous_mode; /* Receive all frames,
                           * ignore recipient written. */
} NETINFO;

void netInit();
void netTerminate();
NETINFO * getNetinfo();
