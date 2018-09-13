/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pci.c - PCI functions                                            */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pci.h"
#include "bos.h" /* Used only for initial PCI check. */

int pciCheck(unsigned char *hwcharacteristics)
{
    int ret;
    /* Dummy variables for BIOS. */
    unsigned char majorversion, minorversion, lastbus;

    /* Only BIOS function in this file. */
    ret = BosPCICheck(hwcharacteristics, &majorversion, &minorversion,
                      &lastbus);
    /* It would be good to have UEFI alternative here as well. */
    if (ret)
    {
        /* BIOS cannot provide us with hwcharacteristics,
         * manual probing should be done. */
        /* It is not yet implemented, so we write what error
         * occured, a message and return 1. */
        printf("BIOS error: %x\n", ret);
        printf("Manual probing is not yet implemented.");
        return (1);
    }

    /* hwcharacteristics description:
     * Bits    Meaning
     * 0       Configuration space access mechanism 1 supported
     * 1       Configuration space access mechanism 2 supported
     * 2 - 3   Reserved
     * 4       Special Cycle generation mechanism 1 supported
     * 5       Special Cycle generation mechanism 2 supported
     * 6 - 7   Reserved */

    return (0);
}    

void pciSetAddress(unsigned int bus, unsigned int slot,
                   unsigned int function, unsigned int offset)
{
    unsigned long address;

    /* Address bits (inclusive):
     * 31      Enable bit (must be 1 for it to work)
     * 30 - 24 Reserved
     * 23 - 16 Bus number
     * 15 - 11 Slot number
     * 10 - 8  Function number (for multifunction devices)
     * 7 - 2   Register number (offset / 4)
     * 1 - 0   Must always be 00 */
    address = 0x80000000 | ((unsigned long) (bus & 0xff) << 16)
              | ((unsigned long) (slot & 0x1f) << 11)
              | ((unsigned long) (function & 0x7) << 8)
              | ((unsigned long) offset & 0xfc);
    /* Full DWORD write to port must be used for PCI to detect new address. */
    outpd(CONFIG_ADDRESS, address);
}

unsigned char pciConfigReadByte(unsigned int bus, unsigned int slot,
                                unsigned int function, unsigned int offset)
{
    pciSetAddress(bus, slot, function, offset);
    /* The PCI registers are little endian,
     * so the last byte of DWORD is read
     * when offset is 0. */
    return (inp(CONFIG_DATA + offset % 4));
}

unsigned int pciConfigReadWord(unsigned int bus, unsigned int slot,
                               unsigned int function, unsigned int offset)
{
    pciSetAddress(bus, slot, function, offset);
    /* The PCI registers are little endian,
     * so the last word of DWORD is read
     * when offset is 0. */
    return (inpw(CONFIG_DATA + offset % 4));
}

unsigned long pciConfigReadDWord(unsigned int bus, unsigned int slot,
                                 unsigned int function, unsigned int offset)
{
    pciSetAddress(bus, slot, function, offset);
    return (inpd(CONFIG_DATA));
}

void pciConfigWriteByte(unsigned int bus, unsigned int slot,
                        unsigned int function, unsigned int offset,
                        unsigned char data)
{
    pciSetAddress(bus, slot, function, offset);
    /* The PCI registers are little endian,
     * so the last byte of DWORD is written
     * when offset is 0. */
    outp(CONFIG_DATA + offset % 4, data);
}

void pciConfigWriteWord(unsigned int bus, unsigned int slot,
                        unsigned int function, unsigned int offset,
                        unsigned int data)
{
    pciSetAddress(bus, slot, function, offset);
    /* The PCI registers are little endian,
     * so the last word of DWORD is written
     * when offset is 0. */
    outpw(CONFIG_DATA + offset % 4, data);
}

void pciConfigWriteDWord(unsigned int bus, unsigned int slot,
                         unsigned int function, unsigned int offset,
                         unsigned long data)
{
    pciSetAddress(bus, slot, function, offset);
    outpd(CONFIG_DATA, data);
}

int pciFindDevice(unsigned int vendor, unsigned int device,
                  unsigned int index,
                  unsigned int *bus, unsigned int *slot)
{
    for (*bus = 0; *bus < MAXBUS; (*bus)++)
    {
        for (*slot = 0; *slot < MAXSLOT; (*slot)++)
        {
            if (pciConfigReadWord(*bus, *slot, 0, 0) == vendor)
            {
                if (pciConfigReadWord(*bus, *slot, 0, 2) == device)
                {
                    if (index == 0) return (0);
                    index--;
                }
            }
        }
    }
    return (1);
}
