/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  kernel32 - implementation of Windows based on PDOS/386           */
/*                                                                   */
/*********************************************************************/

#include <windows.h>

#include <pos.h>

typedef unsigned int DWORD;
typedef unsigned int BOOL;
typedef void *HANDLE;
#define WINAPI __stdcall
#define STD_INPUT_HANDLE ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE ((DWORD)-12)

HANDLE WINAPI GetStdHandle(DWORD d)
{
    if (d == -10) return ((HANDLE)0);
    if (d == -11) return ((HANDLE)1);
    if (d == -12) return ((HANDLE)2);
}

BOOL WINAPI WriteFile(HANDLE h, void *buf, DWORD count, DWORD *actual, void *unknown)
{
    size_t written;

    PosWriteFile((int)h, buf, (size_t)count, &written);
    *actual = written;
    return (0);
}


void WINAPI ExitProcess(int rc)
{
    PosTerminate(rc);
    return;
}

BOOL WINAPI CloseHandle(HANDLE h)
{
    int ret;

    ret = PosCloseFile((int)h);
    return (ret == 0);
}
