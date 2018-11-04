/* written by Paul Edwards */
/* released to the public domain */
/* pdossupc - support routines for pdos */

#include "stddef.h"

#include <pos.h>
#include <support.h>

int __open(const char *filename, int mode, int *errind)
{
    int handle;

    if (PosOpenFile(filename, 0, &handle)) *errind = 1;
    else *errind = 0;
    return (handle);
}

int __creat(const char *filename, int mode, int *errind)
{
    int handle;

    if (PosCreatFile(filename, 0, &handle)) *errind = 1;
    else *errind = 0;
    return (handle);
}

int __read(int handle, void *buf, size_t len, int *errind)
{
    size_t readbytes;

    if (PosReadFile(handle, buf, len, &readbytes)) *errind = 1;
    else *errind = 0;
    return (readbytes);
}

int __write(int handle, const void *buf, size_t len, int *errind)
{
    size_t writtenbytes;

    if (PosWriteFile(handle, buf, len, &writtenbytes)) *errind = 1;
    else *errind = 0;
    return (writtenbytes);
}

int __seek(int handle, long offset, int whence)
{
    long dummy;
    return (PosMoveFilePointer(handle, offset, whence, &dummy));
}

void __close(int handle)
{
    PosCloseFile(handle);
    return;
}

void __remove(const char *filename)
{
    PosDeleteFile(filename);
    return;
}

void __rename(const char *old, const char *new)
{
    PosRenameFile(old, new);
    return;
}

void __allocmem(size_t size, void **ptr)
{
#ifdef __32BIT__
    *ptr = PosAllocMem(size, LOC32);
#else
    *ptr = PosAllocMem(size, LOC20);
#endif
    return;
}

void __freemem(void *ptr)
{
    PosFreeMem(ptr);
    return;
}

void __exec(char *cmd, void *env)
{
    PosExec(cmd, env);
    return;
}

void __datetime(void *ptr)
{
    int year, month, day, dow;
    int hour, minute, second, hundredths;
    int *iptr = ptr;

    PosGetSystemDate(&year, &month, &day, &dow);
    iptr[0] = year;
    iptr[1] = month;
    iptr[2] = day;
    PosGetSystemTime(&hour, &minute, &second, &hundredths);
    iptr[3] = hour;
    iptr[4] = minute;
    iptr[5] = second;
    iptr[6] = hundredths;
    return;
}

int __pstart(int *i1, int *i2, int *i3, POS_EPARMS *exep)
{
    return (__start(i1, i2, i3, exep));
}

#ifdef PDOS_MAIN_ENTRY
void __callback(void)
{
    __exit(0);
    return;
}
#endif

#ifdef PDOS_MAIN_ENTRY
void __main(int ebp, int retaddr, int i1, int i2, int i3, POS_EPARMS *exep)
{
    __start(&i1, &i2, &i3, exep);
    exep->callback = __callback;
    return;
}
#else
void __main(void)
{
    return;
}
#endif

void __exita(int retcode)
{
#ifndef PDOS_RET_EXIT
     PosTerminate(retcode);
#endif
    return;
}

void __displayc(void)
{
    *__vidptr = 'C';
    return;
}
