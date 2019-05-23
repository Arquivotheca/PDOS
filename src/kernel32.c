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

HANDLE WINAPI GetStdHandle(DWORD d)
{
    if (d == -10) return ((HANDLE)0);
    if (d == -11) return ((HANDLE)1);
    if (d == -12) return ((HANDLE)2);
}

BOOL WINAPI WriteFile(HANDLE h, void *buf, DWORD count, DWORD *actual, void *unknown)
{
    size_t written;
    int ret;

    ret = PosWriteFile((int)h, buf, (size_t)count, &written);
    *actual = written;
    /* Positive return code means success. */
    return (!ret);
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

HANDLE WINAPI CreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    return (INVALID_HANDLE_VALUE);
}

BOOL WINAPI CreateProcessA(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation)
{
    return (0);
}

BOOL WINAPI DeleteFileA(LPCSTR lpFileName)
{
    return (0);
}

LPTSTR WINAPI GetCommandLineA(void)
{
    return ((LPTSTR)PosGetCommandLine());
}

LPTCH WINAPI GetEnvironmentStrings(void)
{
    return ((LPTCH)0);
}

BOOL WINAPI GetExitCodeProcess(HANDLE h, LPDWORD lpExitCode)
{
    return (0);
}

DWORD WINAPI GetLastError(void)
{
    return (0);
}

HGLOBAL WINAPI GlobalAlloc(UINT uFlags, SIZE_T dwBytes)
{
    return (PosAllocMem(dwBytes, POS_LOC32));
}

HGLOBAL WINAPI GlobalFree(HGLOBAL hMem)
{
    PosFreeMem(hMem);
    return (NULL);
}

BOOL WINAPI MoveFileA(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName)
{
    return (PosRenameFile(lpExistingFileName, lpNewFileName) == 0);
}

BOOL WINAPI ReadFile(
    HANDLE h,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped)
{
    return (0);
}

DWORD WINAPI SetFilePointer(
    HANDLE h,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod)
{
    return (0);
}

DWORD WINAPI WaitForSingleObject(HANDLE h, DWORD dwMilliseconds)
{
    return (0);
}
