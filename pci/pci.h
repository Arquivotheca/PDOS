/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pci.h - header file for PCI functions                            */
/*                                                                   */
/*********************************************************************/

#define PCI_MAX_BUS 256
#define PCI_MAX_SLOT 32
#define PCI_MAX_FUNCTION 8
#define PCI_CONFIG_ADDRESS 0xcf8
#define PCI_CONFIG_DATA 0xcfc

int pciCheck(unsigned char *hwcharacteristics);

unsigned char pciConfigReadByte(unsigned int bus, unsigned int slot,
                                unsigned int function, unsigned int offset);

unsigned int pciConfigReadWord(unsigned int bus, unsigned int slot,
                               unsigned int function, unsigned int offset);

unsigned long pciConfigReadDWord(unsigned int bus, unsigned int slot,
                                 unsigned int function, unsigned int offset);

void pciConfigWriteByte(unsigned int bus, unsigned int slot,
                        unsigned int function, unsigned int offset,
                        unsigned char data);

void pciConfigWriteWord(unsigned int bus, unsigned int slot,
                        unsigned int function, unsigned int offset,
                        unsigned int data);

void pciConfigWriteDWord(unsigned int bus, unsigned int slot,
                         unsigned int function, unsigned int offset,
                         unsigned long data);

int pciFindDevice(unsigned int vendor, unsigned int device,
                  unsigned int index, unsigned int *bus,
                  unsigned int *slot, unsigned int *function);

int pciFindClassCode(unsigned char class, unsigned char subclass,
                     unsigned char prog_IF, unsigned int index,
                     unsigned int *bus, unsigned int *slot,
                     unsigned int *function);

/* Macros for reading specific registers in PCI Configuration Space. */
#define pciConfigReadVendor(bus, slot, function) \
(pciConfigReadWord((bus), (slot), (function), 0x0))
#define pciConfigReadDevice(bus, slot, function) \
(pciConfigReadWord((bus), (slot), (function), 0x2))
#define pciConfigReadCommand(bus, slot, function) \
(pciConfigReadWord((bus), (slot), (function), 0x4))
#define pciConfigReadStatus(bus, slot, function) \
(pciConfigReadWord((bus), (slot), (function), 0x6))
#define pciConfigReadRevision(bus, slot, function) \
(pciConfigReadByte((bus), (slot), (function), 0x8))
#define pciConfigReadProg_IF(bus, slot, function) \
(pciConfigReadByte((bus), (slot), (function), 0x9))
#define pciConfigReadSubclass(bus, slot, function) \
(pciConfigReadByte((bus), (slot), (function), 0xa))
#define pciConfigReadClass(bus, slot, function) \
(pciConfigReadByte((bus), (slot), (function), 0xb))
#define pciConfigReadCachelineSize(bus, slot, function) \
(pciConfigReadByte((bus), (slot), (function), 0xc))
#define pciConfigReadLatencyTimer(bus, slot, function) \
(pciConfigReadByte((bus), (slot), (function), 0xd))
#define pciConfigReadHeaderType(bus, slot, function) \
(pciConfigReadByte((bus), (slot), (function), 0xe))
#define pciConfigReadBIST(bus, slot, function) \
(pciConfigReadByte((bus), (slot), (function), 0xf))
/* With Header Type 0 there are 6 addresses starting from 0. */
#define pciConfigReadBaseAddress(bus, slot, function, address) \
(pciConfigReadDWord((bus), (slot), (function), 0x10 + 4 * (address)))
#define pciConfigReadCardbusCISPointer(bus, slot, function) \
(pciConfigReadDWord((bus), (slot), (function), 0x28))
/* Subsystem Vendor and Device codes are used to identify subsystem
 * or add-in card.
 * Each company has the same Vendor and Subsystem Vendor code. */
#define pciConfigReadSubsystemVendor(bus, slot, function) \
(pciConfigReadWord((bus), (slot), (function), 0x2c))
#define pciConfigReadSubsystemDevice(bus, slot, function) \
(pciConfigReadWord((bus), (slot), (function), 0x2e))
#define pciConfigReadExpansionROMBaseAddress(bus, slot, function) \
(pciConfigReadDWord((bus), (slot), (function), 0x30))
/* Points to start of linked list of capabilities inside of PCI Configuration
 * space. This structure only exists if Capabilities List bit (bit 4)
 * is set in Status Register. */
#define pciConfigReadCapabilitiesPointer(bus, slot, function) \
(pciConfigReadByte((bus), (slot), (function), 0x34))
#define pciConfigReadInterruptLine(bus, slot, function) \
(pciConfigReadByte((bus), (slot), (function), 0x3c))
#define pciConfigReadInterruptPin(bus, slot, function) \
(pciConfigReadByte((bus), (slot), (function), 0x3d))
#define pciConfigReadMin_Gnt(bus, slot, function) \
(pciConfigReadByte((bus), (slot), (function), 0x3e))
#define pciConfigReadMax_Lat(bus, slot, function) \
(pciConfigReadByte((bus), (slot), (function), 0x3f))

/* Macros for writing specific registers in PCI Configuration Space. */
#define pciConfigWriteCommand(bus, slot, function, data) \
(pciConfigWriteWord((bus), (slot), (function), 0x4, (data)))
#define pciConfigWriteCachelineSize(bus, slot, function, data) \
(pciConfigWriteByte((bus), (slot), (function), 0xc, (data)))
#define pciConfigWriteLatencyTimer(bus, slot, function, data) \
(pciConfigWriteByte((bus), (slot), (function), 0xd, (data)))
#define pciConfigWriteBIST(bus, slot, function, data) \
(pciConfigWriteByte((bus), (slot), (function), 0xf, (data)))
/* With Header Type 0 there are 6 addresses starting from 0. */
#define pciConfigWriteBaseAddress(bus, slot, function, address, data) \
(pciConfigWriteDWord((bus), (slot), (function), 0x10 + 4 * (address), (data)))
#define pciConfigWriteInterruptLine(bus, slot, function, data) \
(pciConfigWriteByte((bus), (slot), (function), 0x3c, (data)))

/* Vendor (offset 0x0, 2 bytes) and device (offset 0x2, 2 bytes) codes. */
#define PCI_VENDOR_NO_DEVICE 0xffff

/* Header types (offset 0xe). PCI to PCI bridges and Cardbus bridges
 * have different register layout with different meanings,
 * so not all of the function macros above are usable for them. */
#define PCI_HEADER_TYPE_SINGLE_FUNCTION 0x00
#define PCI_HEADER_TYPE_PCI_TO_PCI_BRIDGE 0x01
#define PCI_HEADER_TYPE_CARDBUS_BRIGE 0x02
#define PCI_HEADER_TYPE_MULTI_FUNCTION 0x80

/* Class (offset 0xb) and subclass (0xa) codes and Prog IFs (0x9). */
#define PCI_CLASS_UNCLASSIFIED 0x00
#define PCI_CLASS_MASS_STORAGE_CONTROLLER 0x01
    #define PCI_SUBCLASS_SCSI_CONTROLLER 0x00
    #define PCI_SUBCLASS_IDE_CONTROLLER 0x01
    #define PCI_SUBCLASS_FLOPPY_DISK_CONTROLLER 0x02
    #define PCI_SUBCLASS_IPI_BUS_CONTROLLER 0x03
    #define PCI_SUBCLASS_RAID_CONTROLLER 0x04
    #define PCI_SUBCLASS_ATA_CONTROLLER 0x05
    #define PCI_SUBCLASS_SATA_CONTROLLER 0x06
    #define PCI_SUBCLASS_SAS_CONTROLLER 0x07
    #define PCI_SUBCLASS_NONVOLATILE_MEMORY_CONTROLLER 0x08
    #define PCI_SUBCLASS_OTHER_MASS_STORAGE_CONTROLLER 0x80
#define PCI_CLASS_NETWORK_CONTROLLER 0x02
    #define PCI_SUBCLASS_ETHERNET_CONTROLLER 0x00
    #define PCI_SUBCLASS_TOKEN_RING_CONTROLLER 0x01
    #define PCI_SUBCLASS_FDDI_CONTROLLER 0x02
    #define PCI_SUBCLASS_ATM_CONTROLLER 0x03
    #define PCI_SUBCLASS_ISDN_CONTROLLER 0x04
    #define PCI_SUBCLASS_WORLDFIP_CONTROLLER 0x05
    #define PCI_SUBCLASS_PICMG_MULTI_COMPUTING 0x06
    #define PCI_SUBCLASS_INFIBAND_CONTROLLER 0x07
    #define PCI_SUBCLASS_FABRIC_CONTROLLER 0x08
    #define PCI_SUBCLASS_OTHER_NETWORK_CONTROLLER 0x80
#define PCI_CLASS_DISPLAY_CONTROLLER 0x03
    #define PCI_SUBCLASS_VGA_COMPATIBLE_CONTROLLER 0x00
    #define PCI_SUBCLASS_XGA_CONTROLLER 0x01
    #define PCI_SUBCLASS_3D_CONTROLLER 0x02 /* Not VGA-compatible. */
    #define PCI_SUBCLASS_OTHER_DISPLAY_CONTROLLER 0x80
#define PCI_CLASS_MULTIMEDIA_CONTROLLER 0x04
    #define PCI_SUBCLASS_VIDEO_DEVICE 0x00
    #define PCI_SUBCLASS_AUDIO_DEVICE 0x01
    #define PCI_SUBCLASS_COMPUTER_TELEPHONY_DEVICE 0x02
    #define PCI_SUBCLASS_OTHER_MULTIMEDIA_DEVICE 0x80
#define PCI_CLASS_MEMORY_CONTROLLER 0x05
    #define PCI_SUBCLASS_RAM 0x00
    #define PCI_SUBCLASS_FLASH 0x01
    #define PCI_SUBCLASS_OTHER_MEMORY_CONTROLLER 0x80
#define PCI_CLASS_BRIDGE_DEVICE 0x06
    #define PCI_SUBCLASS_HOST_BRIDGE 0x00
    #define PCI_SUBCLASS_ISA_BRIDGE 0x01
    #define PCI_SUBCLASS_EISA_BRIDGE 0x02
    #define PCI_SUBCLASS_MCA_BRIDGE 0x03
    #define PCI_SUBCLASS_PCI_TO_PCI_BRIDGE 0x04
    #define PCI_SUBCLASS_PCMCIA_BRIDGE 0x05
    #define PCI_SUBCLASS_NUBUS_BRIDGE 0x06
    #define PCI_SUBCLASS_CARDBUS_BRIDGE 0x07
    #define PCI_SUBCLASS_RACEWAY_BRIDGE 0x08
    #define PCI_SUBCLASS_SEMITRANSPARENT_PCI_TO_PCI_BRIDGE 0x09
    #define PCI_SUBCLASS_INFINIBAND_TO_PCI_BRIDGE 0x0A
    #define PCI_SUBCLASS_OTHER_BRIDGE_DEVICE 0x80
#define PCI_CLASS_SIMPLE_COMMUNICATION_CONTROLLER 0x07
    #define PCI_SUBCLASS_SERIAL_CONTROLLER 0x00
    #define PCI_SUBCLASS_PARALLEL_CONTROLLER 0x01
    #define PCI_SUBCLASS_MULTIPORT_SERIAL_CONTROLLER 0x02
    #define PCI_SUBCLASS_MODEM 0x03
    #define PCI_SUBCLASS_GPIB_CONTROLLER 0x04
    #define PCI_SUBCLASS_SMART_CARD 0x05
    #define PCI_SUBCLASS_OTHER_COMMUNICATION_CONTROLLER 0x80
#define PCI_CLASS_BASE_SYSTEM_PERIPHEAL 0x08
    #define PCI_SUBCLASS_PIC 0x00
    #define PCI_SUBCLASS_DMA_CONTROLLER 0x01
    #define PCI_SUBCLASS_SYSTEM_TIMER 0x02
    #define PCI_SUBCLASS_RTC_CONTROLLER 0x03
    #define PCI_SUBCLASS_PCI_HOTPLUG_CONTROLLER 0x04
    #define PCI_SUBCLASS_SD_HOST_CONTROLLER 0x05
    #define PCI_SUBCLASS_OTHER_SYSTEM_PERIPHEAL 0x80
#define PCI_CLASS_INPUT_DEVICE_CONTROLLER 0x09
    #define PCI_SUBCLASS_KEYBOARD_CONTROLLER 0x00
    #define PCI_SUBCLASS_DIGITIZER_PEN 0x01
    #define PCI_SUBCLASS_MOUSE_CONTROLLER 0x02
    #define PCI_SUBCLASS_SCANNER_CONTROLLER 0x03
    #define PCI_SUBCLASS_GAMEPORT_CONTROLLER 0x04
    #define PCI_SUBCLASS_OTHER_INPUT_CONTROLLER 0x80
#define PCI_CLASS_DOCKING_STATION 0x0A
    #define PCI_SUBCLASS_GENERIC_DOCKING_STATION 0x00
    #define PCI_SUBCLASS_OTHER_DOCKING_STATION 0x01
#define PCI_CLASS_PROCESSOR 0x0B
    #define PCI_SUBCLASS_386 0x00
    #define PCI_SUBCLASS_486 0x01
    #define PCI_SUBCLASS_PENTIUM 0x02
    #define PCI_SUBCLASS_ALPHA 0x10
    #define PCI_SUBCLASS_POWER_PC 0x20
    #define PCI_SUBCLASS_MIPS 0x30
    #define PCI_SUBCLASS_COPROCESSOR 0x40
#define PCI_CLASS_SERIAL_BUS_CONTROLLER 0x0C
    #define PCI_SUBCLASS_IEEE_1394 0x00
    #define PCI_SUBCLASS_ACCESS_BUS 0x01
    #define PCI_SUBCLASS_SSA 0x02
    #define PCI_SUBCLASS_USB 0x03
        #define PCI_PROG_IF_UHCI_CONTROLLER 0x00
        #define PCI_PROG_IF_OHCI_CONTROLLER 0x10
        #define PCI_PROG_IF_EHCI_CONTROLLER 0x20
        #define PCI_PROG_IF_XHCI_CONTROLLER 0x30
        #define PCI_PROG_IF_USB_UNSPECIFIED 0x80
        #define PCI_PROG_IF_USB_DEVICE 0xFE /* Not a host controller. */
    #define PCI_SUBCLASS_FIBRE_CHANNEL 0x04
    #define PCI_SUBCLASS_SMBUS 0x05
    #define PCI_SUBCLASS_INFINIBAND 0x06
    #define PCI_SUBCLASS_IPMI_INTERFACE 0x07
    #define PCI_SUBCLASS_SERCOS_INTERFACE 0x08
    #define PCI_SUBCLASS_CANBUS 0x09
#define PCI_CLASS_WIRELESS_CONTROLLER 0x0D
    #define PCI_SUBCLASS_IRDA_COMPATIBLE_CONTROLLER 0x00
    #define PCI_SUBCLASS_CONSUMER_IR_CONTROLLER 0x01
    #define PCI_SUBCLASS_RF_CONTROLLER 0x10
    #define PCI_SUBCLASS_BLUETOOTH 0x11
    #define PCI_SUBCLASS_BROADBAND 0x12
    #define PCI_SUBCLASS_ETHERNET_CONTROLLER_802_1A 0x20
    #define PCI_SUBCLASS_ETHERNET_CONTROLLER_802_1B 0x21
    #define PCI_SUBCLASS_OTHER_WIRELESS_CONTROLLER 0x80
#define PCI_CLASS_INTELLIGENT_CONTROLLER 0x0E
#define PCI_CLASS_SATELLITE_COMMUNICATION_CONTROLLER 0x0F
    #define PCI_SUBCLASS_TV 0x01
    #define PCI_SUBCLASS_AUDIO 0x02
    #define PCI_SUBCLASS_VOICE 0x03
    #define PCI_SUBCLASS_DATA 0x04
#define PCI_CLASS_ENCRYPTION_CONTROLLER 0x10
    #define PCI_SUBCLASS_NETWORK_AND_COMPUTING_ENCRYPTION 0x00
    #define PCI_SUBCLASS_ENTERTAINMENT_ENCRYPTION 0x10
    #define PCI_SUBCLASS_OTHER_ENCRYPTION 0x80
#define PCI_CLASS_SIGNAL_PROCESSING_CONTROLLER 0x11
    #define PCI_SUBCLASS_DPIO_MODULES 0x00
    #define PCI_SUBCLASS_PERFORMANCE_COUNTERS 0x01
    #define PCI_SUBCLASS_COMMUNICATION_SYNCHRONIZATION 0x02
    #define PCI_SUBCLASS_MANAGEMENT_CARD 0x03
    #define PCI_SUBCLASS_OTHER_SINGAL_PROCESSING_CONTROLLER 0x80
#define PCI_CLASS_PROCESSING_ACCELERATOR 0x12
#define PCI_CLASS_NONESSENTIAL_INSTUMENTATION 0x13
#define PCI_CLASS_COPROCESSOR 0x40
/* Wildcards used only for broader search with pciFindClassCode().
 * Not specified in any documentation. */
#define PCI_CLASS_WILDCARD 0xff
 #define PCI_SUBCLASS_WILDCARD 0xff
  #define PCI_PROG_IF_WILDCARD 0xff

/* Command (offset 0x4, 2 bytes, writable). Reserved bits must be preserved. */
#define PCI_COMMAND_DISABLE_ALL 0x0000
#define PCI_COMMAND_IO_SPACE 0x0001
#define PCI_COMMAND_MEMORY_SPACE 0x0002
#define PCI_COMMAND_BUS_MASTER 0x0004
#define PCI_COMMAND_SPECIAL_CYCLES 0x0008
#define PCI_COMMAND_MEMORY_WRITE_AND_INVALIDATE_ENABLE 0x0010
#define PCI_COMMAND_VGA_PALETTE_SNOOP 0x0020
#define PCI_COMMAND_PARITY_ERROR_RESPONSE 0x0040
/* Bit 7 is reserved and should be hardwired to 0 on newer devices. */
#define PCI_COMMAND_SERR_ENABLE 0x0100
#define PCI_COMMAND_FAST_BACKTOBACK_ENABLE 0x0200
#define PCI_COMMAND_INTERRUPT_DISABLE 0x0400
/* Rest is reserved. */

/* Status (offset 0x6, 2 bytes). Bits cannot be set,
 * but can be reset by writing them. */
/* First three bits are reserved. */
#define PCI_STATUS_INTERRUPT_STATUS 0x0008
#define PCI_STATUS_CAPABILITIES_LIST 0x0010
#define PCI_STATUS_66_MHZ_CAPABLE 0x0020
/* Bit 6 is reserved. */
#define PCI_STATUS_FAST_BACKTOBACK_CAPABLE 0x0080
#define PCI_STATUS_MASTER_DATA_PARITY_ERROR 0x0100
/* Bits 9-10 specify DEVSEL timing, which has 3 allowed timings,
 * 00b - fast, 01b - medium, 10b - slow. 11b is reserved. */
#define PCI_STATUS_DEVSEL_TIMING_MASK 0x0600 /* Extracts the 2 bits. */
#define PCI_STATUS_SIGNALED_TARGET_ABORT 0x0800
#define PCI_STATUS_RECEIVED_TARGET_ABORT 0x1000
#define PCI_STATUS_RECEIVED_MASTER_ABORT 0x2000
#define PCI_STATUS_SIGNALED_SYSTEM_ERROR 0x4000
#define PCI_STATUS_DETECTED_PARITY_ERROR 0x8000

/* Base Address Registers (offset 0x10 + 4 * address_number, 4 bytes).
 * Before manipulating with the registers IO space / Memory space access
 * for the device should be disabled in the Command Register. */
/* Base Address in Memory space has bit 0 always 0.
 * Bits 1-2 specify if the address is 32-bit, 16-bit or 64-bit.
 * 64-bit addresses use the next DWord (one higher Base Address Register)
 * for the rest of the address (higher 32 bits).*/
#define PCI_BASE_ADDRESS_MEMORY_SPACE 0x00
#define PCI_BASE_ADDRESS_MEMORY_32BIT_SPACE 0x00
#define PCI_BASE_ADDRESS_MEMORY_16BIT_SPACE 0x02
#define PCI_BASE_ADDRESS_MEMORY_64BIT_SPACE 0x04
#define PCI_BASE_ADDRESS_MEMORY_PREFETCHABLE 0x08
#define PCI_BASE_ADDRESS_MEMORY_ADDRESS 0xfffffff0
/* Base Address in IO space has bit 0 always set and bit 1 always 0.
 * To obtain the usable address, those bits must be ANDed out. */
#define PCI_BASE_ADDRESS_IO_SPACE 0x01
#define PCI_BASE_ADDRESS_IO_ADDRESS 0xfffffffc

/* Capabilities Pointer (offset 0x34, 1 byte) points to linked list
 * of capabilities. The format of each entry is Capability ID (byte),
 * Pointer to next entry and additional registers. */
#define PCI_CAPABILITIES_POINTER_ADDRESS 0xfc /* First 2 bits are reserved. */
#define PCI_CAPABILITIES_POINTER_LAST_ENTRY 0x00
#define pciConfigReadCapability(bus, slot, function, pointer) \
(pciConfigReadByte((bus), (slot), (function), (pointer))
#define pciConfigReadCapabilitiesNextPointer(bus, slot, function, pointer) \
(pciConfigReadByte((bus), (slot), (function), (pointer) + 1)
/* Capabilities codes (offset 0 from the pointer address, 1 byte). */
#define PCI_CAPABILITY_PCI_POWER_MANAGEMENT_INTERFACE 0x01
#define PCI_CAPABILITY_AGP 0x02
#define PCI_CAPABILITY_VPD 0x03
#define PCI_CAPABILITY_SLOT_IDENTIFICATION 0x04
#define PCI_CAPABILITY_MESSAGE_SIGNALED_INTERRUPTS 0x05
#define PCI_CAPABILITY_COMPACTPCI_HOT_SWAP 0x06
#define PCI_CAPABILITY_PCIX 0x07
#define PCI_CAPABILITY_HYPERTRANSPORT 0x08
/* With this Capability code, the byte after the pointer is length field. */
#define PCI_CAPABILITY_VENDOR_SPECIFIC 0x09
#define PCI_CAPABILITY_DEBUG_PORT 0x0a
#define PCI_CAPABILITY_COMPACTPCI_CENTRAL_RESOURCE_CONTROL 0x0b
#define PCI_CAPABILITY_PCI_HOTPLUG 0x0c
#define PCI_CAPABILITY_PCI_BRIDGE_SUBSYSTEM_VENDOR_ID 0x0d
#define PCI_CAPABILITY_AGP_8X 0x0e
#define PCI_CAPABILITY_SECURE_DEVICE 0x0f
#define PCI_CAPABILITY_PCI_EXPRESS 0x10
#define PCI_CAPABILITY_MSIX 0x11

/* Interrupt pin (offset 0x3d, 1 byte). */
#define PCI_INTERRUPT_PIN_NONE 0
#define PCI_INTERRUPT_PIN_INTA 1
#define PCI_INTERRUPT_PIN_INTB 2
#define PCI_INTERRUPT_PIN_INTC 3
#define PCI_INTERRUPT_PIN_INTD 4
