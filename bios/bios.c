/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  bios - generic BIOS that exports the C library                   */
/*                                                                   */
/*********************************************************************/

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "exeload.h"
#include "osfunc.h"

static int (*genstart)(void (*exposfunc)(void *cbdata, int funccode, void *retptr, char *str), void *cbdata);

static void exposfunc(void *cbdata, int funccode, void *retptr, char *str);

int main(int argc, char **argv)
{
    unsigned char *p;
    unsigned char *entry_point;
    int rc;

    printf("bios starting\n");
    if (argc < 3)
    {
        printf("Usage: bios os.exe disk.img\n");
        return (EXIT_FAILURE);
    }
    p = calloc(1, 1000000);
    if (p == NULL)
    {
        printf("insufficient memory\n");
        return (EXIT_FAILURE);
    }
    if (exeloadDoload(&entry_point, argv[1], &p) != 0)
    {
        printf("failed to load executable\n");
        return (EXIT_FAILURE);
    }
    genstart = (void *)entry_point;
    /* printf("first byte of code is %02X\n", *(unsigned char *)entry_point); */
#if 1
    rc = genstart(exposfunc, NULL);
#else
    rc = 0;
#endif
    printf("return from called program is %d\n", rc);
    printf("bios exiting\n");
    return (0);
}

/* export osfunc() */
/* This is for use by OSes and BIOSes to export their C library
   to spawned processes */
/* As opposed to osfunc() which is what you call when you yourself
   want OS services. In the case of a BIOS, there will be an export,
   and no attempt to request OS services. In the case of an OS,
   there will be both an export and calls to OS (actually, BIOS)
   services. In the case of an application, there will only be
   calls to OS services. Under this model, there could be a cascade
   of 10 operating systems. You could be running MSDOS under OS/2
   under Linux x64. You could also be switching processors along
   the way, e.g. mixing 68000 and 80386 code. */
static void exposfunc(void *cbdata, int funccode, void *retptr, char *str)
{
    /* printf("in callback\n"); */
    if (funccode == OS_PRINTF) printf(str);
    else if (funccode == OS_FOPEN)
    {
        printf("request received to open file %s\n", str);
    }
    else
    {
        printf("unknown request %d\n", funccode);
    }
    return;
}
