/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  eth.h - header for Ethernet Layer                                */
/*                                                                   */
/*********************************************************************/

#define ETHFRAMEMAXSIZE 1518 /* +++Find what is the max. size of a frame. */
#define ETHHEADERLEN 14
#define ETHFCSLEN 4

int ethReceive(unsigned char *frame, unsigned int len);
unsigned char * ethCreateFrame(unsigned int *len,
                               unsigned int *offset,
                               const unsigned char *destination_ip_address,
                               unsigned int protocol_type);
int ethSend(unsigned char *frame, unsigned int len);
