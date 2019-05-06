/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  vmm.c - Virtual Memory Management                                */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "vmm.h"
#include "liballoc.h" /* For kmalloc() and kfree(). */

static VMM_NODE *vmmCreateNode(unsigned long addr, unsigned long size);
static void vmmAddNode(VMM *vmm, VMM_NODE *node);
static void vmmRemoveNode(VMM *vmm, VMM_NODE *node);

static void vmmMapPages(VMM *vmm, void *addr, unsigned long num_pages);
static void vmmUnmapPages(VMM *vmm, void *addr, unsigned long num_pages);

/* Helper functions for working with PD and PT as they are volatile. */
static volatile void *memcpy_volatile(volatile void *s1,
                                      volatile void *s2,
                                      size_t n);
static volatile void *memset_volatile(volatile void *s,
                                      int c,
                                      size_t n);

void vmmInit(VMM *vmm, PHYSMEMMGR *physmemmgr)
{
    volatile unsigned long *pd = (void *)RECURSIVE_PD_ADDR;
    volatile unsigned long *new_pd = (void *)MODIFIED_PD_ADDR;

    vmm->physmemmgr = physmemmgr;
    vmm->pd_physaddr = physmemmgrAllocPageFrame(vmm->physmemmgr);

    /* Prepares the new PD for usage using the current one. */
    pd[MODIFIED_PD_INDEX] = (((unsigned long)(vmm->pd_physaddr))
                             | PAGE_RW | PAGE_PRESENT);

    /* Marks all entries as not present. */
    memset_volatile(new_pd, 0, PAGE_SIZE);
    /* Sets up recursive mapping for the new PD. */
    new_pd[RECURSIVE_PD_INDEX] = (((unsigned long)(vmm->pd_physaddr))
                                  | PAGE_RW | PAGE_PRESENT);
    /* +++Remove after moving all kernel data
     * from the first 4 MiB to the kernel space. */
    new_pd[0] = pd[0];

    /* Unmaps the new PD from the current one. */
    pd[MODIFIED_PD_INDEX] = 0;
    /* Flushes the Translation Lookaside Buffer by reloading the CR3. */
    loadPageDirectory(saveCR3());

    vmm->head = NULL;
    vmm->tail = NULL;
}

void vmmTerm(VMM *vmm)
{
    VMM_NODE *node, *next_node;

    if (vmm->head)
    {
        for (node = vmm->head; node; node = next_node)
        {
            next_node = node->next;
            kfree(node);
        }
    }

    physmemmgrFreePageFrame(vmm->physmemmgr, vmm->pd_physaddr);
}

void vmmCopyCurrentMapping(VMM *vmm, void *addr, unsigned long size)
{
    volatile unsigned long *pd = (void *)RECURSIVE_PD_ADDR;
    volatile unsigned long *modified_pd = (void *)MODIFIED_PD_ADDR;
    int start_pd_index = GET_PD_INDEX((unsigned long)addr);
    int end_pd_index = GET_PD_INDEX(((unsigned long)addr) + size);

    /* Maps the new PD into the current one. */
    pd[MODIFIED_PD_INDEX] = (((unsigned long)(vmm->pd_physaddr))
                             | PAGE_RW | PAGE_PRESENT);

    memcpy_volatile(modified_pd + start_pd_index,
                    pd + start_pd_index,
                    (end_pd_index - start_pd_index) * sizeof(unsigned long));

    /* Unmaps the new PD from the current one. */
    pd[MODIFIED_PD_INDEX] = 0;
    /* Flushes the Translation Lookaside Buffer by reloading the CR3. */
    loadPageDirectory(saveCR3());
}

void vmmBootstrap(VMM *vmm, void *addr, unsigned long size_supplied)
{
    VMM_NODE temp_node;
    VMM_NODE *node;

    /* Initializes the temporary node. */
    temp_node.addr = (unsigned long)addr;
    temp_node.size = size_supplied;
    temp_node.prev = NULL;
    temp_node.next = NULL;

    vmmAddNode(vmm, &temp_node);

    node = kmalloc(sizeof(VMM_NODE));
    if (node == NULL)
    {
        printf("(VMM) Bootstrap failed\n");
        printf("System halting\n");
        for (;;);
    }
    /* Replaces the temporary node with the node allocated above. */
    memcpy(node, &temp_node, sizeof(VMM_NODE));
    vmm->head = node;
    vmm->tail = node;
}

void vmmSupply(VMM *vmm, void *addr, unsigned long size_supplied)
{
    VMM_NODE *node;
    VMM_NODE *front_node = NULL;
    VMM_NODE *back_node = NULL;

    if (vmm->head)
    {
        /* Tries to find virtual memory right before and after the freed block
         * so it can be combined. */
        for (node = (vmm->head); node; node = node->next)
        {
            if ((node->addr) + (node->size) == (unsigned long)addr)
            {
                front_node = node;
                if (back_node) break;
            }
            if ((node->addr) == (((unsigned long)addr) + size_supplied))
            {
                back_node = node;
                if (front_node) break;
            }
        }
        if (front_node)
        {
            /* Free memory found before the freed block,
             * the front node is enlarged. */
            front_node->size += size_supplied;
            if (back_node)
            {
                /* Free memory found after the freed block,
                 * the front node is enlarged more
                 * and the back node is removed. */
                front_node->size += back_node->size;
                vmmRemoveNode(vmm, back_node);
            }
            return;
        }
        if (back_node)
        {
            /* Only free memory after the freed block was found,
             * the back node is enlarged and its address is decreased. */
            back_node->size += size_supplied;
            back_node->addr -= size_supplied;
            return;
        }
    }
    /* There are no nodes that can be combined with the freed block. */
    node = vmmCreateNode((unsigned long)addr, size_supplied);
    if (node == NULL)
    {
        printf("(VMM) Not enough memory for new node\n"
               "with addr %08x and size %08x\n",
               (unsigned long)addr, size_supplied);
        printf("System halting\n");
        for (;;);
    }
    vmmAddNode(vmm, node);
}

void *vmmAlloc(VMM *vmm, void *addr, unsigned long size)
{
    unsigned long num_pages = size / PAGE_SIZE;

    if (size % PAGE_SIZE) num_pages++;

    /* +++Add a way to allocate at specific addresses. */
    if (addr != NULL) return (NULL);

    return (vmmAllocPages(vmm, num_pages));
}

void vmmFree(VMM *vmm, void *addr, unsigned long size)
{
    unsigned long num_pages = size / PAGE_SIZE;

    if (size % PAGE_SIZE) num_pages++;

    vmmFreePages(vmm, addr, num_pages);
}

void *vmmAllocPages(VMM *vmm, unsigned long num_pages)
{
    VMM_NODE *node;
    VMM_NODE *best_node;
    unsigned long size_needed = num_pages * PAGE_SIZE;
    void *allocated_pages;

    if (vmm->head == NULL)
    {
        /* Ran out of virtual address space. */
        return (0);
    }

    /* Tries to find the smallest node large enough for the allocation. */
    best_node = NULL;
    for (node = (vmm->head); node; node = node->next)
    {
        if (node->size >= size_needed)
        {
            if (node->size == size_needed)
            {
                /* The node fits exactly. */
                best_node = node;
                break;
            }
            if (best_node && (best_node->size) > (node->size))
            {
                best_node = node;
                continue;
            }
            if (best_node == NULL) best_node = node;
        }
    }
    if (best_node == NULL) return (0);

    node = best_node;

    allocated_pages = (void *)(node->addr);
    vmmMapPages(vmm, allocated_pages, num_pages);

    node->size -= size_needed;
    if (node->size == 0) vmmRemoveNode(vmm, node);
    else node->addr += size_needed;

    return (allocated_pages);
}

void vmmFreePages(VMM *vmm, void *addr, unsigned long num_pages)
{
    vmmUnmapPages(vmm, addr, num_pages);
    vmmSupply(vmm, addr, num_pages * PAGE_SIZE);
}

static VMM_NODE *vmmCreateNode(unsigned long addr, unsigned long size)
{
    VMM_NODE *node;

    node = kmalloc(sizeof(VMM_NODE));
    if (node == NULL) return (0);

    node->addr = addr;
    node->size = size;
    node->prev = NULL;
    node->next = NULL;

    return (node);
}

static void vmmAddNode(VMM *vmm, VMM_NODE *node)
{
    node->next = NULL;

    if (vmm->tail)
    {
        node->prev = vmm->tail;
        vmm->tail->next = node;
        vmm->tail = node;
    }
    else
    {
        node->prev = NULL;
        vmm->head = node;
        vmm->tail = node;
    }
}

static void vmmRemoveNode(VMM *vmm, VMM_NODE *node)
{
    if (node->prev == NULL)
    {
        vmm->head = node->next;
    }
    else
    {
        node->prev->next = node->next;
    }
    if (node->next == NULL)
    {
        vmm->tail = node->prev;
    }
    else
    {
        node->next->prev = node->prev;
    }
    kfree(node);
}

static void vmmMapPages(VMM *vmm, void *addr, unsigned long num_pages)
{
    volatile unsigned long *page_table = (void *)PAGE_TABLES_ADDR;
    /* Last PT in PD is the PD because recursive mapping is used. */
    volatile unsigned long *page_directory = (void *)RECURSIVE_PD_ADDR;
    int pd_index = GET_PD_INDEX((unsigned long)addr);
    int pt_index = GET_PT_INDEX((unsigned long)addr);

    page_table += PT_NUM_ENTRIES * pd_index;
    for (;; pd_index++, pt_index = 0, page_table += PT_NUM_ENTRIES)
    {
        if ((page_directory[pd_index] & PAGE_PRESENT) == 0)
        {
            /* Page table is not present, so a new one must be mapped. */
            unsigned long *new_pt;

            new_pt = physmemmgrAllocPageFrame(vmm->physmemmgr);
            page_directory[pd_index] = (((unsigned long)new_pt)
                                        | PAGE_RW | PAGE_PRESENT);
            /* Zeroes the newly allocated PT
             * so all entries are marked as not present. */
            memset_volatile(page_table, 0, PAGE_SIZE);
        }
        for (;
             (pt_index < PT_NUM_ENTRIES) && (num_pages);
             pt_index++, num_pages--)
        {
            void *new_page_frame;

            new_page_frame = physmemmgrAllocPageFrame(vmm->physmemmgr);
            page_table[pt_index] = (((unsigned long)new_page_frame)
                                    | PAGE_RW | PAGE_PRESENT);
        }
        if (num_pages == 0) return;
    }
}

static void vmmUnmapPages(VMM *vmm, void *addr, unsigned long num_pages)
{
    volatile unsigned long *page_table = (void *)PAGE_TABLES_ADDR;
    /* Last PT in PD is the PD itself because recursive mapping is used. */
    volatile unsigned long *page_directory = (void *)RECURSIVE_PD_ADDR;
    int pd_index = GET_PD_INDEX((unsigned long)addr);
    int pt_index = GET_PT_INDEX((unsigned long)addr);
    /* Used to check whether a PT contains no mapped pages and can be freed. */
    int can_remove_page_table = 1;
    int i;

    page_table += PT_NUM_ENTRIES * pd_index;
    /* Special logic for the first PT from which entries should be freed,
     * as the pages can be freed starting from any index. */
    if (page_directory[pd_index] & PAGE_PRESENT)
    {
        for (i = 0; i < pt_index; i++)
        {
            if (page_table[i])
            {
                /* If the PT entry has anything inside,
                 * the PT should not be freed
                 * as it might contain important information. */
                can_remove_page_table = 0;
                break;
            }
        }
    }
    for (;;
         (pd_index++,
          pt_index = 0,
          page_table += PT_NUM_ENTRIES,
          can_remove_page_table = 1))
    {
        if ((page_directory[pd_index] & PAGE_PRESENT) == 0)
        {
            /* Page table is not present,
             * so special logic is needed. */
            if (num_pages <= PT_NUM_ENTRIES)
            {
                /* It is the last PT checked,
                 * so CR3 is reloaded to flush the TLB
                 * and the function ends. */
                loadPageDirectory(saveCR3());
                return;
            }
            num_pages -= PT_NUM_ENTRIES;
            continue;
        }
        for (;
             (pt_index < PT_NUM_ENTRIES) && (num_pages);
             pt_index++, num_pages--)
        {
            if ((page_table[pt_index] & PAGE_PRESENT) == 0)
            {
                /* Not present pages cannot be returned to physmmemgr,
                 * but if the entries contain anything,
                 * they should be preserved. */
                if (page_table[pt_index]) can_remove_page_table = 0;
                continue;
            }
            physmemmgrFreePageFrame(vmm->physmemmgr,
                                    (void *)(page_table[pt_index]
                                             & PAGE_PHYS_ADDR));
            page_table[pt_index] = 0;
        }
        if (num_pages == 0) break;

        if (can_remove_page_table)
        {
            physmemmgrFreePageFrame(vmm->physmemmgr,
                                    (void *)(page_directory[pd_index]
                                             & PAGE_PHYS_ADDR));
            page_directory[pd_index] = 0;
        }

    }
    /* Special logic for the last PT from which entries should be freed,
     * as there might remain some entries unchecked. */
    for (; pt_index < PT_NUM_ENTRIES; pt_index++)
    {
        if (page_table[pt_index])
        {
            /* If the PT entry has anything inside, the PT should not be freed
             * as it might contain important information. */
            can_remove_page_table = 0;
            break;
        }
    }
    if (can_remove_page_table)
    {
        physmemmgrFreePageFrame(vmm->physmemmgr,
                                (void *)(page_directory[pd_index]
                                         & PAGE_PHYS_ADDR));
        page_directory[pd_index] = 0;
    }
    /* Flushes the Translation Lookaside Buffer by reloading the CR3. */
    loadPageDirectory(saveCR3());
}

static volatile void *memcpy_volatile(volatile void *s1,
                                      volatile void *s2,
                                      size_t n)
{
    volatile unsigned char *f = s2;
    volatile unsigned char *fe;
    volatile unsigned char *t = s1;

    fe = f + n;
    while (f != fe)
    {
        *t++ = *f++;
    }
    return (s1);
}

static volatile void *memset_volatile(volatile void *s,
                                      int c,
                                      size_t n)
{
    size_t x = 0;

    for (x = 0; x < n; x++)
    {
        *((char *)s + x) = (unsigned char)c;
    }
    return (s);
}
