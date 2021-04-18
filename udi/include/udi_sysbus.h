/* Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish,
 * use, compile, sell and distribute this work
 * and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions,
 * without complying with any conditions
 * and by any means.
 */

#if !defined (UDI_SYSBUS_VERSION)
 #error "UDI_SYSBUS_VERSION must be defined."
#elif (UDI_SYSBUS_VERSION != 0x101)
 #error "Unsupported UDI_SYSBUS_VERSION."
#endif

/* udi_pio_map regset_idx: */
#define UDI_SYSBUS_MEM     0
#define UDI_SYSBUS_IO      1
#define UDI_SYSBUS_MEM_BUS 2
#define UDI_SYSBUS_IO_BUS  3
