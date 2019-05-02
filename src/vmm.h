/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  vmm.h - header file for vmm.c                                    */
/*                                                                   */
/*********************************************************************/

#ifndef VMM_INCLUDED
#define VMM_INCLUDED

#include "physmem.h"

#define PAGE_SIZE 0x1000
#define PT_NUM_ENTRIES 1024
#define PAGE_TABLES_ADDR 0xFFC00000

/* Flags for Page Table/Directory entries. */
#define PAGE_PRESENT 0x1
#define PAGE_RW      0x2
/* Page Table/Directory entry without flags. */
#define PAGE_PHYS_ADDR 0xFFFFF000

typedef struct vmm_node {
    unsigned long addr;
    unsigned long size;
    struct vmm_node *next;
    struct vmm_node *prev;
} VMM_NODE;

typedef struct {
    PHYSMEMMGR *physmemmgr;
    VMM_NODE *head;
    VMM_NODE *tail;
} VMM;

void vmmInit(VMM *vmm, PHYSMEMMGR *physmemmgr);

/* VMM allocates memory for control structures,
 * but allocations require operational kernel VMM.
 * This function is used to start the kernel VMM
 * using temporary structures which are then discarded.
 * Provides address space at the specified address to VMM.
 * Can be called only once for each VMM!
 * It is not recommended to use it
 * for any other VMM than the first kernel VMM. */
void vmmBootstrap(VMM *vmm, void *addr, unsigned long size_supplied);

/* Provides address space at the specified address to VMM. */
void vmmSupply(VMM *vmm, void *addr, unsigned long size_supplied);

/* Allocates virtual memory at the specified address (NULL = any). */
void *vmmAlloc(VMM *vmm, void *addr, unsigned long size);

/* Frees virtual memory. */
void vmmFree(VMM *vmm, void *addr, unsigned long size);

/* Allocates pages of address space at an arbitrary address. */
void *vmmAllocPages(VMM *vmm, unsigned long num_pages);

/* Frees pages at the specified address. */
void vmmFreePages(VMM *vmm, void *addr, unsigned long num_pages);

#endif /* VMM_INCLUDED */

