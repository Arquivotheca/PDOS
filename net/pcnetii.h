/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pcnetii.h - header file for pcnetii.c                            */
/*                                                                   */
/*********************************************************************/

#define VENDORID 0x1022
#define DEVICEID 0x2000

#define WIO_RDP   0x10 /* Register Data Pointer, used to access CSR */
#define WIO_RAP   0x12 /* Register Address Pointer */
#define WIO_RESET 0x14 /* Reset Register */
#define WIO_BDP   0x16 /* Bus Data Pointer */

/* Same as above, only DWORD size now. */
#define DWIO_RDP   0x10
#define DWIO_RAP   0x14
#define DWIO_RESET 0x18
#define DWIO_BDP   0x1c

#define RDES_COUNT 16   /* Receive descriptors count. */
#define TDES_COUNT 4    /* Transmit descriptors count. */
#define DES_SIZE   16   /* Size of one descriptor in bytes. */
