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

/* Vendor (offset 0x0, 2 bytes) and device (offset 0x2, 2 bytes) codes. */
#define PCI_VENDOR_NO_DEVICE 0xffff

/* Header types (offset 0xe). */
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
