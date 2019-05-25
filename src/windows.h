/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  windows prototypes                                               */
/*                                                                   */
/*********************************************************************/

#include <stddef.h>

typedef size_t SIZE_T;
typedef unsigned int UINT;
typedef unsigned int DWORD;
typedef unsigned int *LPDWORD;
typedef unsigned int BOOL;
typedef long LONG;
typedef long *PLONG;
typedef void *HANDLE;
typedef void *LPVOID;
typedef const void *LPCVOID;
typedef char *LPSTR;
typedef const char *LPCSTR;
typedef char *LPTSTR;
typedef const char *LPCTSTR;
typedef char **LPTCH;
typedef unsigned int LPSECURITY_ATTRIBUTES;
typedef void *LPSTARTUPINFOA;
typedef void *LPPROCESS_INFORMATION;
typedef void *HGLOBAL;
typedef void *LPOVERLAPPED;

#define WINAPI __stdcall
#define STD_INPUT_HANDLE ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE ((DWORD)-12)
#define INVALID_HANDLE_VALUE ((HANDLE)-1)

/* Access mask. */
#define GENERIC_ALL     0x10000000
#define GENERIC_EXECUTE 0x20000000
#define GENERIC_WRITE   0x40000000
#define GENERIC_READ    0x80000000

#define CREATE_NEW 1
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define TRUNCATE_EXISTING 5

HANDLE WINAPI GetStdHandle(DWORD nStdHandle);

BOOL WINAPI WriteFile(HANDLE hFile,
                      LPCVOID lpBuffer,
                      DWORD nNumberOfBytesToWrite,
                      LPDWORD lpNumberOfBytesWritten,
                      LPOVERLAPPED lpOverlapped);

void WINAPI ExitProcess(UINT uExitCode);

BOOL WINAPI CloseHandle(HANDLE hObject);

#define CreateFile CreateFileA
HANDLE WINAPI CreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile);

#define CreateProcess CreateProcessA
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
    LPPROCESS_INFORMATION lpProcessInformation);

#define DeleteFile DeleteFileA
BOOL WINAPI DeleteFileA(LPCSTR lpFileName);

#define GetCommandLine GetCommandLineA
LPTSTR WINAPI GetCommandLineA(void);

LPTCH WINAPI GetEnvironmentStrings(void);

BOOL WINAPI GetExitCodeProcess(HANDLE hProcess, LPDWORD lpExitCode);

DWORD WINAPI GetLastError(void);

HGLOBAL WINAPI GlobalAlloc(UINT uFlags, SIZE_T dwBytes);

HGLOBAL WINAPI GlobalFree(HGLOBAL hMem);

#define MoveFile MoveFileA
BOOL WINAPI MoveFileA(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName);

BOOL WINAPI ReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped);

DWORD WINAPI SetFilePointer(
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod);

DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
