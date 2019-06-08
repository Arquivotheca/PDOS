/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  process.h - header for process.c                                 */
/*                                                                   */
/*********************************************************************/

#ifndef PROCESS_INCLUDED
#define PROCESS_INCLUDED

#include "pos.h" /* Some parts of process management are in pos.h. */

#ifdef __32BIT__
#include "vmm.h"
#endif

/* PDOS_PROCESS: data structure with info about a process */
typedef struct _PDOS_PROCESS {
    char magic[4]; /* 'PCB\0' magic, to distinguish PCB (Process Control Block)
                      from MCB (Memory Control Block) */
    char exeName[PDOS_PROCESS_EXENAMESZ]; /* ASCIIZ short name of executable */
    unsigned long pid; /* The process ID.
                        * Under PDOS-32, this is pointer to PCB itself.
                        * Under PDOS-16, this is segment of PSP.
                        * Should match PosGetCurrentProcessId() if this is the
                        * current process.
                        */
    PDOS_PROCSTATUS status;
    void *envBlock; /* Environment block of this process */
#ifdef __32BIT__
    VMM *vmm;
    char *commandLine; /* Points to kernel memory
                        * storing the command line string. */
#endif
    struct _PDOS_PROCESS *parent; /* NULL for root process */
    struct _PDOS_PROCESS *prev; /* Previous process */
    struct _PDOS_PROCESS *next; /* Next process */
} PDOS_PROCESS;

#ifdef __32BIT__
#define PDOS_PROCESS_SIZE sizeof(PDOS_PROCESS)
#define NO_PROCESS NULL
#else
/* What boundary we want the process control block to be a multiple of */
#define PDOS_PROCESS_ALIGN 16

#define PDOS_PROCESS_SIZE \
  ((sizeof(PDOS_PROCESS) % PDOS_PROCESS_ALIGN == 0) ? \
   sizeof(PDOS_PROCESS) : \
   ((sizeof(PDOS_PROCESS) / PDOS_PROCESS_ALIGN + 1) * PDOS_PROCESS_ALIGN))

#define PDOS_PROCESS_SIZE_PARAS ((PDOS_PROCESS_SIZE)/16)
#endif

extern PDOS_PROCESS *curPCB;

void initPCB(PDOS_PROCESS *pcb, unsigned long pid, char *prog, char *envPtr);
void removeFromProcessChain(PDOS_PROCESS *pcb);
void addToProcessChain(PDOS_PROCESS *pcb);
PDOS_PROCESS *findProc(unsigned long pid);
int isProcessPtr(void *ptr);

#endif /* PROCESS_INCLUDED */
