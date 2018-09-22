/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  nic.h - header file for NIC drivers                              */
/*                                                                   */
/*********************************************************************/

int nicInit();
int nicEnd();
int nicReceive();
int nicSend(unsigned char *frame, unsigned int len);
