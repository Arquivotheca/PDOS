/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  udp.h - header for udp.c                                         */
/*                                                                   */
/*********************************************************************/

typedef struct UDPPORTENT {
    unsigned int port;
    /* Stores pointer to function that will be called
     * when datagram with destination port specified above is received.
     * Arguments should be *data, data_len, source_port, *source_address. */
    void (*func)(unsigned char *, unsigned int,
                 unsigned int, unsigned char *);
    struct UDPPORTENT *next;
} UDPPORTENT; /* UDP port table entry. */

/* Whole IP packet should be passed to udpReceive. */
int udpReceive(unsigned char *packet, unsigned int len);
