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

/* Recursive mapping is used with the PD as the last entry. */
#define PAGE_TABLES_ADDR  0xFFC00000
#define RECURSIVE_PD_ADDR 0xFFFFF000
/* When creating or modifying new PD the entry before the last one is used. */
#define MODIFIED_PD_ADDR  0xFFFFE000

#define RECURSIVE_PD_INDEX PT_NUM_ENTRIES - 1
#define MODIFIED_PD_INDEX  PT_NUM_ENTRIES - 2

/* Calculates indexes into the PD and PT. */
#define GET_PD_INDEX(addr) ((addr) >> 22)
#define GET_PT_INDEX(addr) (((addr) >> 12) & 0x3FF)

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
    void *pd_physaddr; /* Stores the physical address of the assigned PD. */
    VMM_NODE *head;
    VMM_NODE *tail;
} VMM;

void vmmInit(VMM *vmm, PHYSMEMMGR *physmemmgr);
void vmmTerm(VMM *vmm);

/* Copies entries mapping the virtual memory specified
 * from the current PD to the PD of the provided VMM. */
void vmmCopyCurrentMapping(VMM *vmm, void *addr, unsigned long size);

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

/* Allocates pages of address space at the specified address. */
void *vmmAllocPagesAt(VMM *vmm, unsigned long num_pages, void *addr);

/* Frees pages at the specified address. */
void vmmFreePages(VMM *vmm, void *addr, unsigned long num_pages);

#endif /* VMM_INCLUDED */

