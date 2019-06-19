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

HANDLE WINAPI GetStdHandle(DWORD nStdHandle)
{
    if (nStdHandle == -10) return ((HANDLE)0);
    if (nStdHandle == -11) return ((HANDLE)1);
    if (nStdHandle == -12) return ((HANDLE)2);
}

BOOL WINAPI WriteFile(HANDLE hFile,
                      LPCVOID lpBuffer,
                      DWORD nNumberOfBytesToWrite,
                      LPDWORD lpNumberOfBytesWritten,
                      LPOVERLAPPED lpOverlapped)
{
    size_t written;
    int ret;

    ret = PosWriteFile((int)hFile, lpBuffer,
                       (size_t)nNumberOfBytesToWrite, lpNumberOfBytesWritten);
    *lpNumberOfBytesWritten = written;
    /* Positive return code means success. */
    return (!ret);
}


void WINAPI ExitProcess(UINT uExitCode)
{
    PosTerminate(uExitCode);
}

BOOL WINAPI CloseHandle(HANDLE hObject)
{
    int ret;

    ret = PosCloseFile((int)hObject);
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
    int handle;

    if (dwCreationDisposition == CREATE_ALWAYS)
    {
        if (PosCreatFile(lpFileName, 0, &handle))
        {
            return (INVALID_HANDLE_VALUE);
        }
        return ((HANDLE)handle);
    }
    if (dwCreationDisposition == OPEN_EXISTING)
    {
        if (PosOpenFile(lpFileName, 0, &handle))
        {
            return (INVALID_HANDLE_VALUE);
        }
        return ((HANDLE)handle);
    }

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

BOOL WINAPI GetExitCodeProcess(HANDLE hProcess, LPDWORD lpExitCode)
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
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped)
{
    return (!PosReadFile((int)hFile, lpBuffer,
                         nNumberOfBytesToRead, lpNumberOfBytesRead));
}

DWORD WINAPI SetFilePointer(
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod)
{
    return (0);
}

DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
    return (0);
}

void WINAPI GetSystemTime(LPSYSTEMTIME lpSystemTime)
{
    return;
}

/* DllMainCRTStartup() calls this function, but it is not exported. */
BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReserved)
{
    return (TRUE);
}
