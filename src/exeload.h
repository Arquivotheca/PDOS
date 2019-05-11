/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  exeload.h - header for exeload.c                                 */
/*                                                                   */
/*********************************************************************/

typedef struct {
    unsigned long entry_point;
    unsigned long sp; /* Stack pointer. */
    unsigned long cs_address;
    unsigned long ds_address;
    /* Variable modifying memory allocation for the executable.
     * Tells to allocates more memory at the end. */
    unsigned long extra_memory_after; /* Including stack. */
    unsigned long stack_size;
} EXELOAD;

int exeloadDoload(EXELOAD *exeload, char *prog);
