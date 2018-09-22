/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pcnetii.c - driver for PCnet-PCI II                              */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pci.h>
#include "pcnetii.h"
#include "net.h"
#include "nic.h"
#include "eth.h"

unsigned int twoscomplement(unsigned int number)
{
    /* Invert bits and add 1. */
    return ((~number) + 1);
}

unsigned long controller_port;

/* We are using WIO everywhere as there are nearly no registers
 * with usable bits 31 - 16 and WIO is simpler in that case. */

void setRAP(unsigned int number)
{
    outpw(controller_port + WIO_RAP, number & 0xff);
}

unsigned int readCSR(unsigned int number)
{
    setRAP(number);
    return (inpw(controller_port + WIO_RDP));
}

void writeCSR(unsigned int number, unsigned int data)
{
    setRAP(number);
    outpw(controller_port + WIO_RDP, data);
}

unsigned int readBCR(unsigned int number)
{
    setRAP(number);
    return (inpw(controller_port + WIO_BDP));
}

void writeBCR(unsigned int number, unsigned int data)
{
    setRAP(number);
    outpw(controller_port + WIO_BDP, data);
}

void forceWIO()
{
    /* Specification for Am79C970 says that BCR 18 bit 8 - DWIO is NOT reset
     * by S_RESET and specification for Am79C970A says the opposite.
     * Because of this uncertainty, we both reset the controller
     * and manually clear the BCR 18 bit 7 to set WIO. */
    unsigned long data32;
    /* We reset the controller to clear RAP. */
    inpd(controller_port + DWIO_RESET);
    inpw(controller_port + WIO_RESET);
    /* Writing any DWORD to RDP sets DWIO and CSR 0 is safe for writing to. */
    outpd(controller_port + DWIO_RDP, 0);
    /* Now DWIO is set, so we access BCR 18 bit 7 and clear it. */
    outpd(controller_port + DWIO_RAP, 18);
    /* Clears the BCR 18 bit 7 (DWIO). */
    data32 = inpd(controller_port + DWIO_BDP);
    data32 = data32 ^ 0x80; /* XORed with 0x80 to only clear bit 7. */
    outpd(controller_port + DWIO_BDP, data32);
}

void writemac(unsigned char *mac_address)
{
    /* Writes MAC address to APROM (copy of EEPROM)
     * from provided array which should have at least 6 bytes. */
    int i;
    
    for (i = 0; i < MAC_ADDRESS_LEN; i++)
    {
        outp(controller_port + i, mac_address[i]);
    }
}

void readmac(unsigned char *mac_address)
{
    /* Reads MAC address of the controller from APROM (copy of EEPROM)
     * into provided array which should have at least 6 bytes. */
    int i;
    
    for (i = 0; i < 6; i++)
    {
        mac_address[i] = inp(controller_port + i);
    }
}

/* SWSTYLE 3 (PCnet-PCI II Controller) is used for the Descriptors. */
void * initRdes(unsigned char *rdes, size_t bufsize)
{
    /* Allocates receive buffer of size bufsize
     * and initializes RDES with all required entries for operation.
     * Returns pointer to the buffer which must be freed later. */
    void * rbuf;
    unsigned int bcnt;

    rbuf = malloc(bufsize);
    /* Clears the rdes. */
    memset(rdes, '\0', DES_SIZE);
    /* Stores buffer address in rdes. */
    rdes[8] = (unsigned long) rbuf & 0xff;
    rdes[9] = ((unsigned long) rbuf >> 8) & 0xff;
    rdes[10] = ((unsigned long) rbuf >> 16) & 0xff;
    rdes[11] = (unsigned long) rbuf >> 24;
    /* Stores size of the buffer in BCNT (twos complement)
     * and sets the top 4 bits to 1 as they should be like that. */
    bcnt = twoscomplement(bufsize) | 0xf000;
    rdes[4] = bcnt & 0xff;
    rdes[5] = bcnt >> 8;
    /* Sets status bit 7 so the controller owns the buffer. */
    rdes[7] = 0x80;

    return (rbuf);
}

void * initTdes(unsigned char *tdes, size_t bufsize)
{
    /* Allocates transmit buffer of size bufsize
     * and initializes TDES with all required entries for operation.
     * Returns pointer to the buffer which must be freed later. */
    void * tbuf;
    unsigned int bcnt;

    tbuf = malloc(bufsize);
    /* Clears the tdes. */
    memset(tdes, '\0', DES_SIZE);
    /* Stores buffer address in tdes (address can be max. 6 bytes). */
    tdes[8] = (unsigned long) tbuf & 0xff;
    tdes[9] = ((unsigned long) tbuf >> 8) & 0xff;
    tdes[10] = ((unsigned long) tbuf >> 16) & 0xff;
    tdes[11] = (unsigned long) tbuf >> 24;
    /* Stores size of the buffer in BCNT (twos complement)
     * and sets the top 4 bits to 1 as they should be like that. */
    bcnt = twoscomplement(bufsize) | 0xf000;
    tdes[4] = (unsigned char) (bcnt & 0xff);
    tdes[5] = (unsigned char) (bcnt >> 8);
    /* Status bits are not set because we own the transmit buffer. */

    return (tbuf);
}

unsigned char * rbufs[RDES_COUNT]; /* Stores pointers to receive buffers. */
unsigned char * rring; /* Stores receive descriptors. */
unsigned char * tbufs[TDES_COUNT]; /* Stores pointers to transmit buffers. */
unsigned char * tring; /* Stores transmit descriptors. */

void initRings()
{
    /* Sets up the descriptor rings for receive and transmit. */
    int i;
    
    rring = malloc(DES_SIZE * RDES_COUNT);
    tring = malloc(DES_SIZE * TDES_COUNT);
    for (i = 0; i < RDES_COUNT; i++)
    {
        rbufs[i] = initRdes(rring + DES_SIZE * i, ETHFRAMEMAXSIZE);
    }
    for (i = 0; i < TDES_COUNT; i++)
    {
        tbufs[i] = initTdes(tring + DES_SIZE * i, ETHFRAMEMAXSIZE);
    }
}

void freeRings()
{
    /* Frees the buffers to which descriptors in ring point. */
    int i;
    for (i = 0; i < RDES_COUNT; i++)
    {
        free(rbufs[i]);
    }
    for (i = 0; i < TDES_COUNT; i++)
    {
        free(tbufs[i]);
    }
    free(rring);
    free(tring);
}

void initCsrs()
{
    NETINFO *netinfo = getNetinfo();
    /* LADRF is ignored for now, so we write zero to CSR 8 - 11 (inclusive). */
    writeCSR(8, 0x0);
    writeCSR(9, 0x0);
    writeCSR(10, 0x0);
    writeCSR(11, 0x0);
    /* Sets MAC address. */
    writeCSR(12, ((unsigned int) netinfo->mac_address[0] << 8)
                 | netinfo->mac_address[1]);
    writeCSR(13, ((unsigned int) netinfo->mac_address[2] << 8)
                 | netinfo->mac_address[3]);
    writeCSR(14, ((unsigned int) netinfo->mac_address[4] << 8)
                 | netinfo->mac_address[5]);
    /* Overwrites APROM with the provided MAC address,
     * so the original cannot be read. */
    writemac(netinfo->mac_address);
    /* CSR 15 (Mode register) description:
     * Bit Meaning
     * 10  PROM - Promiscuous Mode, all incoming frames received. */
    if (netinfo->promiscuous_mode) writeCSR(15, 0x400);
    else writeCSR(15, 0);
    
    /* Stores address of receive descriptor ring in CSR 24 - 25. */
    writeCSR(24, (unsigned long) rring & 0xffff); /* Lower bits. */
    writeCSR(25, (unsigned long) rring >> 16); /* Upper bits. */
    
    /* Stores address of transmit descriptor ring in CSR 30 - 31. */
    writeCSR(30, (unsigned long) tring & 0xffff); /* Lower bits. */
    writeCSR(31, (unsigned long) tring >> 16); /* Upper bits. */

    /* Sets CSR47 (Polling interval) to 0 as that is the default value. */
    writeCSR(47, 0x0);
    
    /* Sets CSR76 (Receive Descriptor Ring Length) to
     * twoscomplement of RDES_COUNT as the value must be twoscomplement. */
    writeCSR(76, twoscomplement(RDES_COUNT));
    
    /* Sets CSR78 (Transmit Descriptor Ring Length) to
     * twoscomplement of TDES_COUNT as the value must be twoscomplement. */
    writeCSR(78, twoscomplement(TDES_COUNT));

    /* We write 0 to CSR 1 - 2 as we do not use initialization block. */
    /* CSR 1 description:
     * Bits  Meaning
     * 15-0  IADR[15:0] - Lower 16 bits of initialization block address. */
    writeCSR(1, 0);
    /* CSR 2 description:
     * Bits  Meaning
     * 7-0  IADR[23:16] - Upper 8 bits of initialization block address.
     * 15-8 IADR[31:24] - Automatically generated by controller. */
    writeCSR(2, 0);
}

void initPCI(unsigned int bus, unsigned int slot)
{
    /* Reads information from PCI Configuration Space
     * and sets proper PCI configuration. */
    
    /* First base address is the card I/O port (first bit is set),
     * so first two bytes must ANDed out. */
    controller_port = pciConfigReadDWord(bus, slot, 0, 16) & 0xfffffffc;
    /* Configures PCI command register
     * so the controller can be used.
     * Bit 0 - I/O space access enable (IOEN)
     * Bit 2 - Bus master enable (BMEN)
     * Bit 6 - Parity Error Response Enable (PERREN)
     * Bit 8 - System Error Response Enable (SERREN)
     * Other bits read fixed value and cannot be changed. */
    /* IOEN must be enabled so we can interact with the controller
     * after the base address (BAR) will not be changed anymore.
     * BMEN must be enabled so the controller can perform bus master
     * operations. It must be enabled before controller initialization. */
    pciConfigWriteWord(bus, slot, 0, 4, 0x5);
}

void preinitController()
{
    /* After the controller I/O port is found,
     * some registers are configured before initializing the controller. */
    unsigned int data; 
    
    /* Forces WIO so we can safely use 16-bit functions. */
    forceWIO();
    /* Resets the controller to prepare the controller for usage.
     * Reset is done by reading from the reset register. */
    inpw(controller_port + WIO_RESET);

    /* BCR 2 (Miscellaneous Configuration) description:
     * Bit Meaning
     * 1   ASEL - automatically selects 10BASE-T or AUI as needed.
     * 8   APROMWE - enables writes to APROM. */
    data = readBCR(2) | 0x102;
    writeBCR(2, data);

    /* CSR 4 description:
     * Bit Meaning
     * 10  ASTRP_RCV - automatically strips padding and FCS from received.
     * 11  APAD_XMT - automatically pads < 64 bytes transmit.
     * 14  DMAPLUS - DMA Burst Transfer Counter disabled.
     *               Should be set when the controller is used in PCI. */
    /* ASTRP_RCV and APAD_XMT should be done by software
     * so it works on every network interface controller. */
    data = readCSR(4) | 0x4000;
    writeCSR(4, data);

    /* BCR 18 description:
     * Bits  Meaning
     * 0-2   LINBC - does nothing, default 0x1.
     * 5     BWRITE - Burst Write Enable.
                      Should be set when using PCI for maximum performance.
     * 6     BREADE - Burst Read Enable.
                      Should be set when using PCI for maximum performance.
     * 7     DWIO - double word I/O (specification says not reset by S_RESET).
     * 15-12 ROMTMG - Expansion ROM timing. */
    data = readBCR(18) | 0x0060; /* Sets BWRITE and BREADE for performance. */
    writeBCR(18, data);

    /* BCR 20 (alias of CSR 58) (Software Style) description:
     * Bits Meaning
     * 7-0  SWSTYLE - Different software styles, sets SSIZE32 and CSRPCNET.
     * 8    SSIZE32 - 32-bit Software Size, when 0 16-bit structures are used.
     * 9    CSRPCNET - CSR PCnet-ISA configuration (unless working ILACC, set).
     * SWSTYLE SSIZE32 CRSPCNET ENTRIES STYLE_NAME
     * 0       0       1        16-bit  C-LANCE/PCnet-ISA
     * 1       1       0        32-bit  ILACC
     * 2       1       1        32-bit  PCnet-PCI
     * 3       1       1        32-bit  PCnet-PCI II Controller */
    /* PCnet-PCI II Controller should be the best style
     * and there is not anything that would prevent using it
     * on PDOS-16 I think. */
    writeBCR(20, 3); /* Sets style to PCnet-PCI II Controller. */
}

void initController()
{
    /* Initializes ring buffers for receiving and transmitting
     * and then initializes the controller by directly writing
     * to registers (instead of using initialization block). */
    initRings();
    initCsrs();
}

void startController()
{
    /* CSR 0 description:
     * Bit Meaning
     * 0   INIT - starts initialization when set.
     * 1   START - enables sending, receiving and buffer manipulation.
     * 2   STOP - disables DMA (direct memory access).
     * 3   TDMD - Transmit Demand.
     * 4   TDON - Transmit On status. Transmit function enabled.
     * 5   RXON - Receive On status. Receive function enabled.
     * 6   IENA - enables interrupt generation.
     * 7   INTR - Interrupt Flag. One or more interrupt conditions occured.
     * 8   IDON - Initialization Done.
     * 9   TINT - Transmit Interrupt.
     * 10  RINT - Receive Interrupt.
     * 11  MERR - Memory Error.
     * 12  MISS - Missed Frame. Lost incoming frame
                  because no receive descriptor was available.
     * 13  CERR - Collision Error.
     * 14  BABL - Transmitter Time-out Error.
     * 15  ERR  - OR of MERR, MISS, CERR and BABL. */
    writeCSR(0, 0x2);
}

void stopController()
{
    writeCSR(0, 0x4);
}

int nicInit()
{
    unsigned char hwc;
    unsigned int bus;
    unsigned int slot;
    int ret;

    if (ret = pciCheck(&hwc))
    {
        printf("(NIC) pciCheck failed, returned: %d\n", ret);
        return (1);
    }
    if (!(hwc & 0x1))
    {
        printf("(NIC) PCI Configuration space access mechanism 1"
               "not supported\n");
        return (1);
    }
    if (pciFindDevice(VENDORID, DEVICEID, 0, &bus, &slot))
    {
        printf("(NIC) PCnet-PCI II not found\n");
        return (1);
    }

    initPCI(bus, slot);
    preinitController();
    initController();
    startController();
    
    return (0);
}

int nicEnd()
{
    stopController();
    freeRings();
    return (0);
}

/* Stores current positions in ring buffers. */
unsigned int currRdes = 0;
unsigned int currTdes = 0;

/* Debug function. */
void printDes(unsigned char *des)
{
    printf("%02x%02x%02x%02x\n%02x%02x%02x%02x\n\n", des[3], des[2],
           des[1], des[0], des[7], des[6], des[5], des[4]);
}

int nicReceive()
{
    unsigned int offset = currRdes * DES_SIZE;
    unsigned char *buf;
    unsigned int len;
    unsigned int csrdata;

    /* Reads CSR 0 until RINT is set
     * which signalizes that a frame was received.
     * Afterwards the content of CSR 0 is written
     * back to clear RINT. */
    /* It is important to do it this way, because otherwise
     * descriptors with no received data are detected as
     * descriptors with received data. */
    while (!(csrdata = readCSR(0) & 0x400));
    writeCSR(0, csrdata);
    
    while (rring[offset + 7] & 0x80)
    {
        /* Goes through the ring buffer until a received frame is found. */
        currRdes++;
        if (currRdes == RDES_COUNT) currRdes = 0;
        offset = currRdes * DES_SIZE;
    }
    if (rring[offset + 7] & 0x40)
    {
        /* We do not have a way of handling errors yet,
         * so we only write a message. */
        printf("(NIC) Error occured while receiving!\n");
        return (1);
    }

    /* Message length is stored as first 2 bytes of RDES
     * and we have to clear it. */
    len = (unsigned int) rring[offset]
          | ((unsigned int) rring[offset + 1] << 8);
    rring[offset] = 0;
    rring[offset + 1] = 0;

    buf = malloc(len);
    memcpy(buf, rbufs[currRdes], len);

    /* Returns control of RDES back to the controller. */
    rring[offset + 7] = 0x80;

    /* Moves the pointer forward on the ring buffer. */
    currRdes++;
    if (currRdes == RDES_COUNT) currRdes = 0;

    /* Sends the received frame to the next layer. */
    ethReceive(buf, len);

    free(buf);

    return (0);
}

int nicSend(unsigned char *frame, unsigned int len)
{
    unsigned int offset = currRdes * DES_SIZE;
    unsigned int bcnt;

    if (len > ETHFRAMEMAXSIZE)
    {
        printf("(NIC) Maximal ethernet frame length exceeded, size: %u\n",
               len);
        return (1);
    }

    while (tring[offset + 7] & 0x80)
    {
        /* Goes through the ring buffer until a free descriptor is found. */
        currTdes++;
        if (currTdes == TDES_COUNT) currTdes = 0;
        offset = currTdes * DES_SIZE;
    }

    memcpy(tbufs[currTdes], frame, len);
    /* BCNT is twoscomplement of length, but top 4 bits must always be set. */
    bcnt = twoscomplement(len) | 0xf000;
    tring[offset + 4] = (unsigned char) (bcnt & 0xff);
    tring[offset + 5] = (unsigned char) (bcnt >> 8);
    /* Gives control of the descriptor to the controller (0x80)
     * and tells that it is both first and last part of packet (0x3). */
    tring[offset + 7] = 0x83;

    /* Moves the pointer forward on the ring buffer. */
    currTdes++;
    if (currTdes == TDES_COUNT) currTdes = 0;
    
    return (0);
}


    
