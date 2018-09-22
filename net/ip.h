/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  ip.h - header for ip.c                                           */
/*                                                                   */
/*********************************************************************/

#define IPV4VERSION 4

#define IP_ICMP_PROTOCOL 1
#define IP_TCP_PROTOCOL 6
#define IP_UDP_PROTOCOL 17

int ipReceive(unsigned char *packet, unsigned int len);
