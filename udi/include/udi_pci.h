/* Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish,
 * use, compile, sell and distribute this work
 * and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions,
 * without complying with any conditions
 * and by any means.
 */

#if !defined (UDI_PCI_VERSION)
 #error "UDI_PCI_VERSION must be defined."
#elif (UDI_PCI_VERSION != 0x101)
 #error "Unsupported UDI_PCI_VERSION."
#endif

/* udi_pio_map regset_idx: */
#define UDI_PCI_CONFIG_SPACE 255
#define UDI_PCI_BAR_0        0
#define UDI_PCI_BAR_1        1
#define UDI_PCI_BAR_2        2
#define UDI_PCI_BAR_3        3
#define UDI_PCI_BAR_4        4
#define UDI_PCI_BAR_5        5
